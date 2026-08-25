//----------------------------------------------------------------------------
// Copyright (C) 2009 , Olivier Girard
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the authors nor the names of its contributors
//       may be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE
//
//----------------------------------------------------------------------------
//
// *File Name: template_periph_16b.v
// 
// *Module Description:
//                       16 bit peripheral template.
//
// *Author(s):
//              - Olivier Girard,    olgirard@gmail.com
//
//----------------------------------------------------------------------------
// $Rev$
// $LastChangedBy$
// $LastChangedDate$
//----------------------------------------------------------------------------

// `include "../openmsp430/msp_memory/ram.v"

module  task_queue_periph (

// OUTPUTs
    per_dout,                       // Peripheral data output

// INPUTs
    mclk,                           // Main system clock
    per_addr,                       // Peripheral address
    per_din,                        // Peripheral data input
    per_en,                         // Peripheral enable (high active)
    per_we,                         // Peripheral write enable (high active)
    puc_rst                         // Main system reset
);

// OUTPUTs
//=========
output       [15:0] per_dout;       // Peripheral data output

// INPUTs
//=========
input               mclk;           // Main system clock
input        [13:0] per_addr;       // Peripheral address
input        [15:0] per_din;        // Peripheral data input
input               per_en;         // Peripheral enable (high active)
input         [1:0] per_we;         // Peripheral write enable (high active)
input               puc_rst;        // Main system reset

/*********************************************************/
/////////////////     Other data      /////////////////////

///// Stucture from periph_template.v
parameter       [15:0] METADATA_BASE_ADDR = 16'h400;
parameter       [13:0] PER_ADDR = METADATA_BASE_ADDR[14:1];                 
parameter              METADATA_SIZE = 4; // in bytes

parameter              DEC_WD      =  3;                 

// Register addresses offset                             
parameter [DEC_WD-1:0] VIOLATE   =  'h0,             //0x400
                       VTID      =  'h1;             //0x402

// Register one-hot decoder utilities                    
parameter              DEC_SZ      =  (1 << DEC_WD);        
parameter [DEC_SZ-1:0] BASE_REG   =  {{DEC_SZ-1{1'b0}}, 1'b1};
                                                         
// Register one-hot decoder                              
parameter [DEC_SZ-1:0] VIOLATE_D  = (BASE_REG << VIOLATE),  
                       VTID_D     = (BASE_REG << VTID);

// Local register selection
wire              reg_sel      =  per_en & (per_addr[13:DEC_WD-1]==METADATA_BASE_ADDR[14:DEC_WD]);

// Register local address
wire [DEC_WD-1:0] reg_addr     =  {1'b0, per_addr[DEC_WD-2:0]};

// Register address decode
wire [DEC_SZ-1:0] reg_dec      = (VIOLATE_D  &  {DEC_SZ{(reg_addr==VIOLATE)}}) |
                                 (VTID_D  &  {DEC_SZ{(reg_addr==VTID)}});
                                 
// Read/Write probes
wire              reg_write =  |per_we & reg_sel;
wire              reg_read  = ~|per_we & reg_sel;

// Read/Write vectors
wire [DEC_SZ-1:0] reg_wr    = reg_dec & {512{reg_write}};
wire [DEC_SZ-1:0] reg_rd    = reg_dec & {512{reg_read}};

// Mem-mapped CFI Violaiton flag
reg  [15:0] violate;

wire        violate_wr  = reg_wr[VIOLATE];
wire [15:0] violate_nxt = per_din;
 
always @ (posedge mclk or posedge puc_rst)
  if (puc_rst)        violate <=  16'h0000;
  else if (violate_wr)  violate <=  violate_nxt; 
wire [15:0] violate_rd     = violate             & {16{reg_rd[VIOLATE]}};

// Mem-mapped violating task ID
//-----------------
reg  [15:0] vtid;

wire       vtid_wr  = reg_wr[VTID];
wire [15:0] vtid_nxt = per_din;

always @ (posedge mclk or posedge puc_rst)
if (puc_rst)        vtid <=  16'h0000;
else if (vtid_wr) vtid <=  vtid_nxt;
wire [15:0] vtid_rd     = vtid             & {16{reg_rd[VTID]}};
/*********************************************************/
//
//
/*********************************************************/
/////////////////       Queue        /////////////////////
parameter       [15:0] QUEUE_BASE_ADDR   = METADATA_BASE_ADDR + METADATA_SIZE; 
parameter              QUEUE_SIZE  =  32;            // 32 bytes 
parameter              QUEUE_ADDR_MSB   = 3;         // Address stored in 16-bit registers, address 32*8 bits using 16-bit registers, need 4 bits -> 3 MSB (start from 0)      

parameter       [13:0] QUEUE_PER_ADDR  = QUEUE_BASE_ADDR[14:1];   

wire   [QUEUE_ADDR_MSB:0] queue_addr_reg = per_addr-QUEUE_PER_ADDR; 
wire                     queue_cen      = per_en & per_addr >= QUEUE_PER_ADDR & per_addr < QUEUE_PER_ADDR+(QUEUE_SIZE*8/16);
wire    [15:0]           queue_dout;
wire    [1:0]            queue_wen      = per_we & {2{per_en}};

queue_ram #(QUEUE_ADDR_MSB, QUEUE_SIZE)
queue (  

    // OUTPUTs
    .ram_dout    (queue_dout),           // Program Memory data output
    // INPUTs
    .ram_addr    (queue_addr_reg),       // Program Memory address
    .ram_cen     (~queue_cen),           // Program Memory chip enable (low active)
    .ram_clk     (mclk),                // Program Memory clock
    .ram_din     (per_din),             // Program Memory data input
    .ram_wen     (~queue_wen)            // Program Memory write enable (low active)
);
wire [15:0]           queue_rd = queue_dout & {16{queue_cen & ~|per_we}};

/*********************************************************/
//
//
/*********************************************************/
// /////////////////       Bounds        /////////////////////
// parameter              TOTAL_TASKS = 4;
// parameter       [15:0] BOUNDS_BASE_ADDR   = 16'h200; //METADATA_BASE_ADDR + METADATA_SIZE + QUEUE_BASE_ADDR;
// parameter              BOUNDS_SIZE       =  TOTAL_TASKS*4;         // Total Bytes --> two bytes per addr, two addr per task --
// parameter              BOUNDS_ADDR_MSB   =  3;                       // Address stored in 16-bit registers, address 32*8 bits using 16-bit registers, need 4 bits -> 3 MSB (start from 0)      

// parameter       [13:0] BOUNDS_PER_ADDR  = BOUNDS_BASE_ADDR[14:1];  
// parameter       [13:0] BOUNDS_PER_MAX  =  BOUNDS_PER_ADDR+(BOUNDS_SIZE*8/16);   

// wire   [QUEUE_ADDR_MSB:0] bounds_addr_reg = per_addr-BOUNDS_PER_ADDR; 
// wire                     bounds_cen      = per_en & per_addr >= BOUNDS_PER_ADDR & per_addr < BOUNDS_PER_MAX;
// wire    [15:0]           bounds_dout;
// wire    [1:0]            bounds_wen      = per_we & {2{per_en}};

// task_bounds_mem #()
// bounds (  

//     // OUTPUTs
//     .ram_dout    (bounds_dout),           // Program Memory data output
//     // INPUTs
//     .ram_addr    (bounds_addr_reg),       // Program Memory address
//     .ram_cen     (~bounds_cen),           // Program Memory chip enable (low active)
//     .ram_clk     (mclk),                // Program Memory clock
//     .ram_din     (per_din),             // Program Memory data input
//     .ram_wen     (~bounds_wen)            // Program Memory write enable (low active)
// );
// wire [15:0]           bounds_rd = bounds_dout & {16{bounds_cen & ~|per_we}};

/*********************************************************/
//
//
/*********************************************************/
/////////////////       Outputs        /////////////////////

//// output mux based on rd signals
assign per_dout = violate_rd | vtid_rd  | queue_rd;// | bounds_rd;

/*********************************************************/

endmodule // template_periph_16b

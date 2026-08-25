//----------------------------------------------------------------------------
// Copyright (C) 2001 Authors
//
// This source file may be used and distributed without restriction provided
// that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// This source file is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
// This source is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this source; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//----------------------------------------------------------------------------
// 
// *File Name: ram.v
// 
// *Module Description:
//                      Scalable RAM model
//
// *Author(s):
//              - Olivier Girard,    olgirard@gmail.com
//
//----------------------------------------------------------------------------
// $Rev$
// $LastChangedBy$
// $LastChangedDate$
//----------------------------------------------------------------------------

`include "openMSP430_defines.v"
module task_bounds_mem (

// OUTPUTs
    ram_dout,                      // RAM data output
    cur_task_id,
    cur_task_min,
    cur_task_max,

// INPUTs
    pc,
    ram_addr,                      // RAM address
    ram_cen,                       // RAM chip enable (low active)
    ram_clk,                       // RAM clock
    ram_din,                       // RAM data input
    ram_wen                        // RAM write enable (low active)
);

// PARAMETERs // overwritten by declaring file
//============
parameter TOTAL_TASKS = 4;
parameter TASK_MSB = $clog2(TOTAL_TASKS);
parameter                      MEM_SIZE   =  4*TOTAL_TASKS;
parameter                      ADDR_MSB   =  3;         
parameter       [15:0] BOUNDS_BASE_ADDR   =  16'h180; 
parameter              BOUNDS_SIZE        =  TOTAL_TASKS*4;         
parameter              BOUNDS_ADDR_MSB    =  3;                       
parameter       [13:0] BOUNDS_PER_ADDR    =  BOUNDS_BASE_ADDR[14:1];  
parameter       [13:0] BOUNDS_PER_MAX     =  BOUNDS_PER_ADDR+(BOUNDS_SIZE*8/16);   

// OUTPUTs
//============
output                [15:0] ram_dout;       // RAM data output
output      [(TASK_MSB-1):0] cur_task_id;
output                [15:0] cur_task_min;       // RAM data output
output                [15:0] cur_task_max;       // RAM data output


// INPUTs
//============
input       [15:0] pc;
input       [13:0] ram_addr;       // RAM address
input              ram_cen;        // RAM chip enable (low active)  
input              ram_clk;        // RAM clock
input       [15:0] ram_din;        // RAM data input
input        [1:0] ram_wen;        // RAM write enable (low active)

// RAM 
//============
// `ifdef PAIR_HW_ONLY
// reg         [15:0] task_bounds;
// `else
(* ram_style = "block" *) reg         [15:0] task_bounds [0:(MEM_SIZE/2)-1]; 
// `endif

reg         [ADDR_MSB:0] ram_addr_reg;
wire        [15:0] mem_val = task_bounds[ram_addr];  

// for now just declare here for testing

wire   [BOUNDS_ADDR_MSB:0] bounds_addr_reg  = ram_addr-BOUNDS_PER_ADDR; 
wire                      bounds_cen       = ram_cen & ram_addr >= BOUNDS_PER_ADDR & ram_addr < BOUNDS_PER_MAX;
wire    [15:0]            bounds_dout;
wire    [1:0]             bounds_wen       = ram_wen & {2{ram_cen}};

initial 
begin
    // `ifdef PAIR_HW_ONLY
    // task_bounds <= 0;
    // `else
    $readmemh("./task_bounds.mem", task_bounds); // todo-- map this to SW-accessible memory (enable tcb updates)
    // `endif
    ram_addr_reg <= 0;
end
  
// always @(posedge ram_clk)
//     begin
//         ram_addr_reg <= bounds_addr_reg;
//         if (bounds_cen & bounds_addr_reg<(MEM_SIZE/2))
//         begin
//             // `ifdef PAIR_HW_ONLY
//             // if      (ram_wen==2'b00) task_bounds        <= ram_din;
//             // else if (ram_wen==2'b01) task_bounds[15:8]  <= ram_din[15:8]; 
//             // else if (ram_wen==2'b10) task_bounds[7:0]   <= ram_din[7:0]; 
//             // `else
//             if      (ram_wen==2'b00) task_bounds[bounds_addr_reg]        <= ram_din;
//             else if (ram_wen==2'b01) task_bounds[bounds_addr_reg][15:8]  <= ram_din[15:8]; 
//             else if (ram_wen==2'b10) task_bounds[bounds_addr_reg][7:0]   <= ram_din[7:0]; 
//             // `endif
//         end
//     end

// `ifdef PAIR_HW_ONLY
// assign ram_dout = task_bounds;
// `else
assign ram_dout = task_bounds[ram_addr_reg];
// `endif

/// check which task is executing
integer i;
reg in_something = 1'b0;
reg [(TASK_MSB-1):0] detected_task = 0;

always @* 
begin
    for (i=0; i<TOTAL_TASKS; i = i + 1)
    begin
        if((pc >= task_bounds[2*i]) && (pc <= task_bounds[2*i+1]))
            detected_task <= i;
    end
end

assign cur_task_id = detected_task;
assign cur_task_min = task_bounds[2*detected_task];
assign cur_task_max = task_bounds[2*detected_task+1];

endmodule 

`ifdef OMSP_NO_INCLUDE
`else
`include "openMSP430_defines.v"
`endif

module acfi (
    clk,
    reset,
    pc,
    data_en,
    data_wr,
    data_addr,
    sp,
    // dma_addr,
    // dma_en,
    // ER_min,
    // ER_max,
    puc,
    irq,
    gie,
    e_state,
    inst_so,
    inst_type,
    mdb_out,
    inst_ad,
    inst_as,
    inst_jmp,
    per_addr,                       // Peripheral address
    per_din,                        // Peripheral data input
    per_en,                         // Peripheral enable (high active)
    per_we,                         // Peripheral write enable (high active)
    //
    per_dout,
    acfi_violation,
    zombify_irq
);
input           clk;
input           reset;
input   [15:0]  pc;
input           data_en;
input           data_wr;
input   [15:0]  data_addr;
input   [15:0]  sp;
// input   [15:0]  dma_addr;
// input           dma_en;
//input   [15:0]  LOG_size;
input           puc;
input           irq;
input           gie;
input   [3:0]   e_state;
input   [7:0]   inst_so;
input   [2:0]   inst_type;
input   [15:0]  mdb_out;
input   [7:0]   inst_ad;
input   [7:0]   inst_as;
input   [7:0]   inst_jmp;
input        [13:0] per_addr;       // Peripheral address
input        [15:0] per_din;        // Peripheral data input
input               per_en;         // Peripheral enable (high active)
input         [1:0] per_we;         // Peripheral write enable (high active)
//
//
output [15:0] per_dout;
output acfi_violation;
output zombify_irq;
///////////////////////////

parameter [15:0] ER_min = `PMEM_OFFSET;
parameter [15:0] ER_size = `PMEM_SIZE-1;
parameter [15:0] ER_max = `PMEM_OFFSET + ER_size;
parameter RTOS_min = 16'ha000;
parameter RTOS_max = 16'hcffe;
parameter TOTAL_TASKS = 16;
parameter TASK_MSB = $clog2(TOTAL_TASKS);
parameter STACK_SIZE = 16;

////GLOBAL-- shared by more than one acfi module
reg [15:0] prev_pc = 0;
always @(posedge clk)
begin
    prev_pc <= pc;  
end
//
wire in_ER = (pc >= ER_min) && (pc <= ER_max);
wire prev_in_ER = (prev_pc >= ER_min) && (prev_pc <= ER_max);
//
wire in_RTOS = (pc >= RTOS_min) && (pc <= RTOS_max);
wire prev_in_RTOS = (prev_pc >= RTOS_min) && (prev_pc <= RTOS_max);
//
wire prev_in_task = (prev_pc >= cur_task_min) && (prev_pc <= cur_task_max);
wire in_task = (pc >= cur_task_min) && (pc <= cur_task_max);
//

//// MODULES


wire [(TOTAL_TASKS-1):0] AR_en;
availability_monitor # (.TOTAL_TASKS (TOTAL_TASKS))
availability_monitor_0 (
    .clk (clk),
    .reset (puc),
    .pc (pc),
    .prev_pc (prev_pc),
    .irq (irq),
    //
    .in_ER (in_ER),
    .in_RTOS (in_RTOS),
    .in_task (in_task),
    .cur_task_id (cur_task_id),
    //
    .cfi_violation (cfi_violation),
    .inter_task_violation (inter_task_violation),
    //
    .AR_en (AR_en),
    .zombify_irq (zombify_irq)
);


// MODELED MEMORY
`ifdef PAIR_HW_ONLY
wire [15:0] top_val = stack_idx;//stack_arr[stack_idx];
reg [15:0] stack_arr;
`else
reg [15:0] stack_arr [0:(STACK_SIZE*TOTAL_TASKS)-1];
wire [15:0] top_val = stack_arr[stack_idx];
`endif

reg [15:0] history_idx [TOTAL_TASKS-1:0];
integer i;
initial
begin   
    `ifdef PAIR_HW_ONLY
    `else
    for (i = 0; i < (STACK_SIZE*TOTAL_TASKS); i = i + 1) begin
        stack_arr[i] = 16'h0000;
    end
    `endif
    for(i=0; i<TOTAL_TASKS; i = i + 1) begin
        history_idx[i] = 0;
    end
end

// Make a shadow stack module, instaniated here, will need to set some of the internal variables
// reg [15:0] top_val = 16'h0;
wire [15:0] hist_idx_out;
wire hist_idx_wen;
wire [15:0] stack_val;

// update stack mem
wire [15:0] stack_idx = history_idx[cur_task_id] + (cur_task_id*STACK_SIZE);
wire stack_wen;
always @(posedge clk)
begin
    if(stack_wen)
        `ifdef PAIR_HW_ONLY
        stack_arr <= stack_val;
        `else
        stack_arr[stack_idx] <= stack_val;    
        `endif
end

// update IDX mem
wire [15:0] hist_idx_in = history_idx[cur_task_id];
always @(posedge clk)
begin
    if(hist_idx_wen)
        history_idx[cur_task_id] = hist_idx_out;
end

wire cfi_violation;
assign cfi_violation = 1'b0;
wire inter_task_violation;
assign inter_task_violation = 1'b0;
/*
 shadow_stack # ()
 shadow_stack_0 (
     .clk (clk),
     .pc (pc),
     .call_flag (call_flag),
     .ret_flag (ret_flag),
     .mdb_out        (mdb_out),
     //
     .in_task (in_task),
     .hist_idx_in (hist_idx_in),
     .top_val (top_val),
     //
     .inter_task_violation (inter_task_violation),
     .violation (cfi_violation),
     //
     .stack_ren (stack_ren),
     .stack_wen (stack_wen),
     .stack_val (stack_val),
     //
     .hist_idx_out (hist_idx_out),
     .hist_idx_wen (hist_idx_wen)
     // .pushed_pc (pushed_pc)
);
*/

 branch_monitor #()
 branch_monitor_0( //Branch Monitor
    
     .clk        (clk),  // Left is module's value, right is cflow's value  
     .pc         (pc),     
     .ER_min     (ER_min),
     .ER_max     (ER_max),
     .acfa_nmi   (1'b0),
     .irq        (irq),
     .gie        (gie),

     .e_state    (e_state),
     .inst_so    (inst_so),
     .inst_type  (inst_type),
     .inst_ad    (inst_ad),
     .inst_as    (inst_as),
     .inst_jmp   (inst_jmp),
    
     .branch_detect (branch_detect),
     .ret_flag (ret_flag),
     .call_flag (call_flag)
);

task_bounds_mem #(.TOTAL_TASKS (TOTAL_TASKS))
task_bounds_mem_0 (
    .pc (pc),
    .ram_addr      (per_addr),      // Peripheral address
    .ram_cen       (~per_en),        // Peripheral enable (high active)
    .ram_clk       (clk),
    .ram_din       (per_din),       // Peripheral data input
    .ram_wen       (~per_we),        // Peripheral write enable (high active)
    //
    .ram_dout      (per_dout),
    .cur_task_id   (cur_task_id),
    .cur_task_min  (cur_task_min),
    .cur_task_max  (cur_task_max)
);

wire [(TASK_MSB-1):0] cur_task_id;
wire [15:0] cur_task_min;
wire [15:0] cur_task_max;

wire invalid_wr;
wire stack_ptr_mem_wen;
wire [15:0] stack_ptr_mem_out;
monitor mointor_0(
    .clk (clk),
    .pc (pc),
    // .call_flag (call_flag),
    // .ret_addr (pushed_pc),
    .sp        (sp),
    //
    .data_addr (data_addr),
    .data_wr (data_wr),
    //
    .in_task (in_task),
    .stack_ptr_mem_in (stack_ptr_mem_in),
    //
    .invalid_wr (invalid_wr),
    .stack_ptr_mem_wen (stack_ptr_mem_wen)
);

// read/write from the sp mem
`ifdef PAIR_HW_ONLY
reg [15:0] task_sps;
`else
reg [15:0] task_sps [0:TOTAL_TASKS-1];
`endif

wire stack_ptr_mem_in = task_sps[cur_task_id];
always @(posedge clk)
begin
    if(stack_ptr_mem_wen)
        `ifdef PAIR_HW_ONLY
        task_sps <= sp;
        `else
        task_sps[cur_task_id] <= sp;
        `endif
        

end

wire ibt_valid;
// ibt_check ibt_check_0(
//     .clk (clk),
//     .rst (puc),
//     .call_flag (call_flag),
//     .index_in (prev_pc),
//     .result_in (pc),
//     .match_found (ibt_valid)
// );

assign acfi_violation = cfi_violation | inter_task_violation | ibt_valid;

endmodule

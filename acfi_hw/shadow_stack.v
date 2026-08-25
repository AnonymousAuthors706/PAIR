`include "openMSP430_defines.v"

module shadow_stack (
    clk,
    // reset,
    pc, 
    // prev_pc,    
    call_flag,
    ret_flag,
    mdb_out,
    //
    in_task,
    hist_idx_in,
    top_val,
    //
    inter_task_violation,
    violation,
    //
    // stack_idx,
    stack_ren,
    stack_wen,
    stack_val,
    hist_idx_out,
    hist_idx_wen
    // pushed_pc
    // condition_1,
    // condition_2,
    // condition_3
);

parameter TOTAL_TASKS = 16;
parameter TASK_MSB = $clog2(TOTAL_TASKS);
parameter STACK_SIZE = 16;
// reg [15:0] stack_arr [0:(STACK_SIZE*TOTAL_TASKS)-1];// modeled as memory


input clk;
// input reset;
input [15:0] pc;
// input [15:0] prev_pc;  
input call_flag;
input ret_flag;
input [15:0] mdb_out;
input [15:0] top_val;
//
// input in_ER;
// input prev_in_ER;
// input in_RTOS;
// input prev_in_RTOS;
input in_task;
// input prev_in_task;
//
input [15:0] hist_idx_in;
// input [(TASK_MSB-1):0] cur_task_id;
// input [15:0] cur_task_min;
// input [15:0] cur_task_max;
//

output inter_task_violation;
// output reg prev_in_task = 1'b0;
// output [15:0] stack_idx;
output stack_ren;
output stack_wen;
output [15:0] stack_val;
output [15:0] hist_idx_out;
output hist_idx_wen;
output violation;
//
// output condition_1;
// output condition_2;
// output condition_3;
//
// output [15:0] pushed_pc;

parameter ER_min = 16'hd000;
parameter ER_max = 16'hfffe;
parameter RTOS_min = 16'ha000;
parameter RTOS_max = 16'hcffe;
//
//
//
wire in_ER = (pc >= ER_min) && (pc <= ER_max);
wire in_valid_task = in_task & in_ER;
// wire prev_in_ER = (prev_pc >= ER_min) && (prev_pc <= ER_max);
//
wire in_RTOS = (pc >= RTOS_min) && (pc <= RTOS_max);
// wire prev_in_RTOS = (prev_pc >= RTOS_min) && (prev_pc <= RTOS_max);
//
reg [15:0] prev_pc = 0;

parameter EXEC = 2'b00;
parameter RTOS_EXEC = 2'b01;
parameter TASK_EXEC = 2'b10;
parameter RESET = 2'b10;

reg [1:0] pc_state = 2'b00;
always @(posedge clk)
begin
    if (pc == 0)
		pc_state <= RESET;
	else if(in_valid_task)
        pc_state <= TASK_EXEC;
    else if(in_ER)
        pc_state <= EXEC;
    else if(in_RTOS)
        pc_state <= RTOS_EXEC;
end

reg inter_violate = 1'b0;
wire prev_in_task = (pc_state == TASK_EXEC);
wire prev_in_ER = (pc_state == EXEC);
wire prev_in_RTOS = (pc_state == RTOS_EXEC);
wire prev_RESET = (pc_state == RESET);

always @(posedge clk)
begin
    if(~prev_in_task & ~prev_in_RTOS & in_valid_task & ~prev_RESET)
        inter_violate <= 1'b1;
    else
        inter_violate <= 1'b0;
end

assign inter_task_violation = inter_violate;

//
// wire task_min_in_ER = (cur_task_min >= ER_min) && (cur_task_min <= ER_max);
// wire task_max_in_ER = (cur_task_max >= ER_min) && (cur_task_max <= ER_max);
// wire valid_task_def = (cur_task_min < cur_task_max) & task_min_in_ER & task_max_in_ER;
// wire prev_in_task = valid_task_def & (prev_pc >= cur_task_min) && (prev_pc <= cur_task_max) && prev_in_ER;
// wire in_task = valid_task_def & (pc >= cur_task_min) && (pc <= cur_task_max);
//


// reg [15:0] po_pc, pu_pc;
reg violation_reg;
// reg [15:0] history_idx [TOTAL_TASKS-1:0];
integer i;

initial
begin
        // for(i=0; i<TOTAL_TASKS; i = i + 1) begin
        //     history_idx[i] = 0;
        // end
        
//         for (i = 0; i < (STACK_SIZE*TOTAL_TASKS); i = i + 1) begin
//             stack_arr[i] = 16'h0000;
//         end
//         po_pc = 0;
//         pu_pc = 0;
//         //
        violation_reg = 0;
end

///*

// This is RIOT-specific
parameter TASK_ENTRY_min = RTOS_min;
parameter TASK_ENTRY_max = TASK_ENTRY_min + 16'h66; 
wire task_switch = (pc == TASK_ENTRY_max);
// //
// //
// // TODO -- make an input
// // wire [15:0] top_val = stack_arr[stck_idx];
// //


// write signal and state var
parameter SS_WEN = 1'b1; //write-enable
parameter SS_WDS = 1'b0; //write-disable
reg s_wen = SS_WDS;

//// push to shadow stack
// reg push_state = 1'b0;
wire push_condition_1 = in_valid_task && call_flag && (s_wen == SS_WDS);
wire push_condition_2 = ~call_flag && (s_wen == SS_WEN); // second stage of call (to self, rtos, other ER)


/// s_wen state transitions
always @(posedge clk)
begin
    if(push_condition_1)
        s_wen <= SS_WEN;
    else if(push_condition_2)
        s_wen <= SS_WDS;
end

reg [15:0] s_val = 16'h0;
always @(posedge clk)
begin
    if(push_condition_1)
        s_val <= mdb_out;
end


//// popp from shadow stack aka ss_ren
reg s_ren = 1'b0;
// return start and came from a source we need to check
wire pop_condition_1 = (in_valid_task | in_RTOS) && ~task_switch && ret_flag && s_ren==1'b0;
// return ended and internal to task (pc and prev pc in task) --> check violation
// wire pop_condition_2 = prev_in_task && in_valid_task && ~ret_flag && s_ren==1'b1;
wire pop_condition_2 = ~ret_flag && s_ren==1'b1;
// return ended and not internal to task --> ignore
wire pop_condition_3 = ~in_valid_task && ~ret_flag && s_ren==1'b1;
//
// reg mismatch = 1'b0;
//

/// pop state machine
always @(posedge clk)
begin
    if(pop_condition_1)
        s_ren <= 1'b1;
    // else if(pop_condition_2)
    //     s_ren <= 1'b0;
    // else if(pop_condition_3)
    else if (~ret_flag && s_ren==1'b1)
        s_ren <= 1'b0;
end

// pop output (violation_reg)
always @(posedge clk)
begin
    if(in_valid_task & ~ret_flag & s_ren)
    begin
        if (top_val != pc)
            violation_reg <= 1'b1;
    end
    else
        violation_reg <= 1'b0;
end
/**/

/// managing history_idx
// wire [15:0] stck_idx = history_idx[cur_task_id] + (cur_task_id*STACK_SIZE);

reg [15:0] hidx_out = 16'h0;
always @(posedge clk)
begin
    if(pop_condition_1 & (hist_idx_in > 16'h0))
        // history_idx[cur_task_id] <= history_idx[cur_task_id] - 1;
    begin
        if(~hidx_wen)
            hidx_out <= hist_idx_in - 1;
    end
    else if(pop_condition_3)
        // history_idx[cur_task_id] <= history_idx[cur_task_id] + 1;
    begin
        if(~hidx_wen)
            hidx_out <= hist_idx_in + 1;
    end
    else if(push_condition_2)
        // history_idx[cur_task_id] <= history_idx[cur_task_id] + 1;
    begin
        if(~hidx_wen)
            hidx_out <= hist_idx_in + 1;
    end
end

reg hidx_wen = 1'b0;
always @(posedge clk)
begin
    if(pop_condition_1)
        // history_idx[cur_task_id] <= history_idx[cur_task_id] - 1;
    begin
        if(~hidx_wen)
            hidx_wen <= 1'b1;
    end
    else if(pop_condition_3)
        // history_idx[cur_task_id] <= history_idx[cur_task_id] + 1;
        // hidx_wen <= 1'b1;
    begin
        if(~hidx_wen)
            hidx_wen <= 1'b1;
    end
    else if(push_condition_2)
        // history_idx[cur_task_id] <= history_idx[cur_task_id] + 1;
        // hidx_wen <= 1'b1;
    begin
        if(~hidx_wen)
            hidx_wen <= 1'b1;
    end
    else
        hidx_wen <= 1'b0;
end


//// outputs

assign violation = violation_reg;
// assign stack_idx = stck_idx;
assign stack_wen = s_wen;
assign stack_ren = s_ren;
assign stack_val = s_val;
assign hist_idx_out = hidx_out;
assign hist_idx_wen = hidx_wen;
// DEBUG
// assign condition_1 = pop_condition_1;
// assign condition_2 = pop_condition_2;
// assign condition_3 = pop_condition_3;
// assign pushed_pc = pu_pc;

endmodule
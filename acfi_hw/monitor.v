`include "openMSP430_defines.v"

module monitor(
	clk,
	pc,
	// ret_addr,
	sp,
	//
	data_addr,
	data_wr,
	//
	in_task,
	// task_start,
	// cur_task_id,
	// cur_task_min,
	// cur_task_max,
	//
	stack_ptr_mem_in,
	//
	invalid_wr,
	stack_ptr_mem_wen
);

input clk;
input [15:0] pc;
// input [15:0] ret_addr;
input [15:0] sp;
//
input         [15:0] data_addr;
input                data_wr;
//
// input in_ER;
//
parameter TOTAL_TASKS = 16;
parameter TASK_MSB = $clog2(TOTAL_TASKS);


// input [(TASK_MSB-1):0] cur_task_id;
// input [15:0] cur_task_min;
// input [15:0] cur_task_max;
input in_task;
// input task_start;
input [15:0] stack_ptr_mem_in;
//
//
//
output invalid_wr;
output stack_ptr_mem_wen;

// instr addr range that relates to registering/scheduling a task
parameter [15:0] SCHEDULE_TASK_MIN = 16'hd100;
parameter [15:0] SCHEDULE_TASK_MAX = 16'hd132;

parameter RTOS_MIN = 16'ha000;
parameter RTOS_MAX = 16'hbffe;
//
parameter TCB_min = 16'hc000;
parameter TCB_max = 16'hcffe;
//
parameter ER_min = 16'hd000;
parameter ER_max = 16'hfffe;

// reg [15:0] task_sps [0:TOTAL_TASKS-1];
// integer i;
reg sp_mem_wen;
reg [15:0] sp_mem_out;
reg [15:0] prev_pc;
reg [1:0] pc_state;
reg [15:0] stack_lower;
reg [15:0] stack_upper;
initial
begin
	sp_mem_wen <= 1'b0;
	sp_mem_out <= 16'h0;
	prev_pc <= 0;
	pc_state <= 2'b00;
	stack_lower <= 16'h0;
	stack_upper <= 16'h0;
// 	for (i=0; i<TOTAL_TASKS; i = i + 1)
// 	begin
// 		task_sps[i] <= 16'h0;
// 	end
end

// wire in_task = (pc >= cur_task_min) & (pc <= cur_task_max);
wire in_ER = (pc >= ER_min) & (pc <= ER_max);
wire in_RTOS = (pc >= RTOS_MIN) & (pc <= RTOS_MAX);
wire in_TCB = (pc >= TCB_min) & (pc <= TCB_max);
wire in_valid_task = in_ER & in_task;

// wire prev_pc_in_task = (prev_pc >= cur_task_min) & (prev_pc <= cur_task_max);
always @(posedge clk)
begin
	prev_pc <= pc;
end

///// track return address & sp from task-granularity
// interface with SS module: 
/// when using a CALL to enter TASK (ss.call_flag and pc in TASK and prev_pc outside) --> save pc+4
// reg [15:0] task_ret_addrs [0:TOTAL_TASKS-1]; 

// wire [15:0] detected_min = (detected_task < TOTAL_TASKS) ? task_bounds[2*detected_task] : 0;
// wire [15:0] detected_max = (detected_task < TOTAL_TASKS) ? task_bounds[2*detected_task+1] : 0;

parameter SHARED_DATA_MIN = 16'h0000;
parameter SHARED_DATA_MAX = 16'h1800; //based on defines file

parameter EXEC = 2'b00;
parameter RTOS_EXEC = 2'b01;
parameter TASK_EXEC = 2'b10;
parameter TCB_EXEC = 2'b11;

always @(posedge clk)
begin
    if(in_valid_task)
        pc_state <= TASK_EXEC;
    else if(in_RTOS)
        pc_state <= RTOS_EXEC;
    else if (in_TCB)
    	pc_state <= TCB_EXEC;
    else if(~in_valid_task & ~in_RTOS & ~in_TCB)
    	pc_state <= EXEC;
end

// reg to keep the upper value of the current task's stack

// always @(posedge clk)
// begin
// 	if(~prev_pc_in_task & in_task & (task_sps[cur_task_id]==0)) // entering task for first time
// 		task_sps[cur_task_id] <= sp;
// 	else if (in_task & (task_sps[cur_task_id]!=0))
// 		stack_lower <= sp;
// end

/// add in the wen/ren logic

// always @(posedge clk)
// begin
// 	if(
// 	  (pc_state == EXEC && in_valid_task) ||
// 	  (pc_state == TCB_EXEC && in_valid_task) ||
// 	  (pc_state == RTOS_EXEC && in_valid_task)
// 	)
// 	begin
// 		sp_mem_wen <= 1'b1;
// 	end
// 	else begin
// 		sp_mem_wen <= 1'b0;
// 	end
// end

// always @(posedge clk)
// begin
// 	if (pc_state != TASK_EXEC && in_valid_task)
// 		sp_mem_out <= 16'h8;
// end

always @(posedge clk)
begin
	if (in_valid_task)
	begin
		stack_upper <= stack_ptr_mem_in;
		stack_lower <= sp;
	end
end

wire data_wr_shared = data_wr & (data_addr >= SHARED_DATA_MIN) & (data_addr < SHARED_DATA_MAX);
wire data_wr_task_stack = data_wr & (data_addr >= stack_lower) & (data_addr < stack_upper);

assign invalid_wr = in_valid_task & data_wr & ~data_wr_shared & ~data_wr_task_stack;

assign stack_ptr_mem_wen = (pc_state != TASK_EXEC && in_valid_task);
// assign stack_ptr_mem_out = sp;

endmodule
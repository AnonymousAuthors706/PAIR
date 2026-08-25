`include "openMSP430_defines.v"

module availability_monitor(
	clk,
	reset,
	pc,
	prev_pc,
	irq,
	in_ER,
	in_RTOS,
	in_task,
	cur_task_id,
	//
	cfi_violation,
	inter_task_violation,
	//
	AR_en,
	zombify_irq,
	AM_violation
);

parameter TOTAL_TASKS = 16;
parameter TASK_MSB = $clog2(TOTAL_TASKS);

input clk;
input reset;
input [15:0] pc;
input [15:0] prev_pc;
input irq;
input in_ER;
input in_RTOS;
input in_task;
input [(TASK_MSB-1):0] cur_task_id;
//
input cfi_violation;
input inter_task_violation;
//
output [(TOTAL_TASKS-1):0] AR_en;
output zombify_irq;
output AM_violation;
//


reg [(TOTAL_TASKS-1):0] AR_bits;
initial
begin
	AR_bits <= {(TOTAL_TASKS){1'b1}};
end

reg [(TASK_MSB-1):0] prev_task_id = 16'h0;
always @(posedge clk)
begin
	if(~in_RTOS)
		prev_task_id <= cur_task_id;
end

//// set AR_en bits

parameter VALID_EXIT = 16'hff00;
always @(posedge clk)
begin
	if (inter_task_violation) //went directly from a "valid" task to "invalid" one, so invalidate the prev task
		AR_bits[prev_task_id] = 1'b0;	
	else if((in_ER | in_RTOS) & cfi_violation & AR_bits[cur_task_id]) // violation came from current task
		AR_bits[cur_task_id] = 1'b0;	
	else if(in_task & ~AR_bits[cur_task_id])
		AR_bits[prev_task_id] = 1'b0;	
	else if (!in_RTOS & !in_ER & (pc == VALID_EXIT)) //pc == valid exit implies not in RTOS or ER, but nuSMV doesn't know that
		AR_bits <= {(TOTAL_TASKS){1'b1}};
end

wire AM_trigger = in_task & ~AR_bits[cur_task_id];

///// set zombify_irq
reg zombify_irq = 1'b0;
wire zombify_irq_acc = zombify_irq & irq;
always @(posedge clk)
begin
	if (reset)
		zombify_irq <= 1'b0;
	else if (cfi_violation | inter_task_violation | AM_trigger)
		zombify_irq <= 1'b1;
	else if (zombify_irq_acc)
		zombify_irq <= 1'b0; // interrupt was accepted, so can clear the flag
end

assign AR_en = AR_bits;
assign AM_violation = AM_trigger;

endmodule
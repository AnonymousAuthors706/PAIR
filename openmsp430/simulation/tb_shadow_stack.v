`timescale 1ns/1ps

module tb_shadow_stack;

  reg clk;
  reg reset;
  reg [15:0] pc;
  reg [15:0] prev_pc;
  reg [15:0] ER_min;
  reg [15:0] ER_max;
  reg call_flag;
  reg ret_flag;
  wire violation;

  // DUT
  shadow_stack uut (
    .clk(clk),
    .reset(reset),
    .pc(pc),
    .prev_pc(prev_pc),
    .ER_min(ER_min),
    .ER_max(ER_max),
    .call_flag(call_flag),
    .ret_flag(ret_flag),
    .violation(violation)
  );

  initial begin
    clk = 0;
    reset = 0;
    forever #5 clk = ~clk;   
  end

  
  always @(posedge clk) begin
    prev_pc <= pc;
  end

  // Stimulus
  initial begin
    // init
    pc        = 16'h0000;
    prev_pc   = 16'h0000;
    ER_min    = 16'h0100;
    ER_max    = 16'h0800;
    call_flag = 0;
    ret_flag  = 0;

    #20;

    // --- Proper CALL/RETURN case ---
    // Enter region
    pc = 16'h0120; #30;
    pc = 16'h0122; #30;

    // Call at 0x0124 (next instr would be 0x0128)
    call_flag = 1;
    pc = 16'h0124; #30;
    call_flag = 0;

    // Inside callee
    pc = 16'h0200; #30;
    pc = 16'h0202; #30;

    // Return correctly to 0x0128
    ret_flag = 1;
    pc = 16'h0204; #30;
    ret_flag = 0;
    pc = 16'h0128; #30;   // <- Expected return address

    // --- Incorrect RETURN case ---
    // Another call at 0x0130 (next instr = 0x0134)
    pc = 16'h0130; #30;
    call_flag = 1;
    #10;
    call_flag = 0;

    // Callee body
    pc = 16'h0300; #30;
    pc = 16'h0302; #30;

    // Wrong return (should be 0x0134 but go to 0x0400 instead)
    ret_flag = 1;
    pc = 16'h0304; #30;
    ret_flag = 0;
    pc = 16'h0400; #30;   // <- Wrong return target
  end

endmodule

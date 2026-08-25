module ibt_check #(
    parameter MEM_SIZE = 16,       // Total memory size
    parameter RESULT_COUNT = 4      // Number of results per table
)(
    input wire clk,
    // input wire rst,
    input wire call_flag,
    input wire [15:0] index_in,
    input wire [15:0] result_in,
    output reg match_found
);

    `ifdef PAIR_HW_ONLY
    reg [15:0] memory [0:1];
    `else
    reg [15:0] memory [0:MEM_SIZE-1];
    `endif

    integer i, j;
    reg [15:0] table_count;
    reg [15:0] current_index;
    reg index_match;
    reg result_match;

    always @(posedge clk) begin
        // if (rst) begin
        //     match_found <= 0;
        // end
        // else
        if (call_flag) begin
            table_count = memory[0];
            index_match = 0;
            result_match = 0;

            for (i = 1; i < MEM_SIZE; i = i + 1 + RESULT_COUNT) begin
                if (!index_match) begin
                    current_index = memory[i];
                    if (current_index == index_in) begin
                        index_match = 1;
                        for (j = 1; j <= RESULT_COUNT; j = j + 1) begin
                            if (memory[i + j] == result_in) begin
                                result_match = 1;
                            end
                        end
                    end
                end
            end

            match_found <= index_match && result_match;
        end
        else 
            match_found <= 0;
    end
endmodule
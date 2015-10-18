module top (input clk, output reg led1, led2, led3);
	reg [23:0] counter = 0;
	always @(posedge clk) begin
		counter <= counter + 1;
		{led1, led2, led3} <= counter[23:21];
	end
endmodule

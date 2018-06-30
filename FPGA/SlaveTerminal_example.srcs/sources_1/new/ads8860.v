//T = 810ns
module ADS8860(
	input wire clk,
	input wire rst,
	input wire adSampleEn,
	output wire[15:0] ADReceiveData,
	output wire SampleFinished,
	output wire CS_AD,
	output wire SCLK_AD,
	input wire MISO_AD	
);
////AD采样的延时
//reg [9:0]adDelayCnt;
//reg adSampleTick;
//always@(posedge clk) begin
//    if(~rst)
//    begin
//        adDelayCnt <= 10'd0;
//        adSampleTick <= 1'b1;
//    end
//    else 
//    begin
//        if(adSampleEn)
//            adDelayCnt <= 10'd80;
//        else if(adDelayCnt != 10'd0) adDelayCnt <= adDelayCnt -10'd1;
//        if(adDelayCnt == 10'd1)    adSampleTick <= 1'b1;
//        else adSampleTick <= 1'b0;
//    end
//end

	//实例化AD //
	my_spi_module  my_spi_module_ad(
 	.clk(clk),
	.rst(rst),
	.data_out_valid(adSampleEn),
	
	.DATA_WITH(5'd15),// = n-1
	.DATA_EDGE_NUM(6'd31),// = n*2-1
	.SPI_CLK_INIT(1'b0),
	.DATA_OUT_EDGE(1'b0),	//下降沿写数据（上升沿读数据）
	
	.CS_DOWN_nclk(16'd4),// = n-2
	.CS_UP_nclk(16'd4),// = n-2
	.SPI_HALF_PERIOD_nclk(16'd1),// = n-1
	
	.SPI_DATA_OUT(32'h0000dddd),//方便测试1101110111011101
	.SPI_DATA_IN(ADReceiveData),
	.SPI_STATE(),

	.SPI_CLK(SCLK_AD),
	.MOSI(),
	.MISO(MISO_AD),
	.CS(CS_AD)
	);
	
	//捕获CS上升沿
	reg[1:0] csad;
	always@(posedge clk) begin
		if(~rst)
			csad <= 2'b11;
		else 
			csad<= {csad[0],CS_AD};
	end
	assign SampleFinished = (csad == 2'b01);

endmodule
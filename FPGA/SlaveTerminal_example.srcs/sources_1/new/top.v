
`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Create Date: 2017/04/30 14:41:14
//Author:GYJ
// Design Name: terminal
// Target Devices: 
// Tool Versions: vivado 2016.4
// Description: 
//
//////////////////////////////////////////////////////////////////////////////////
/*transmit
* 1 
* +------------+---------------+--------------------------+
* | 01(R)/10(W)|  ...(0000)... |       		addr		  |
* +------------+---------------+--------------------------+
*  	 15 14    		  		  8 7						  0

* 2(W/R)
* +-------------------------------------------------------+
* |			           data[31:16]						  |		
* +-------------------------------------------------------+

* 3(W/R)
* +-------------------------------------------------------+
* |			           data[15:0]						  |	
* +-------------------------------------------------------+
*/
`define nWREG 10//0~23 can write by master
`define nRREG 20
module top(
        input wire clk_in,
        input wire crystal_clk,
        //spi slave
        input wire CS,   
        input wire SCK,
        output wire MISO,
        input wire MOSI,
        //clk's
        output wire si5351_scl,
        inout wire si5351_sda,
        //LED
        output reg [7:0] led,
        //fast adda
        input wire [7:0] ad9288_a,
        input wire [7:0] ad9288_b,
        output reg [9:0] dac5652a_a,
        output reg [9:0] dac5652a_b,
		
		//user's

		output wire SCLK_DA,
		output wire CS_DA,
		output wire MOSI_DA,
		
		output wire SCLK_AD,
		output wire CS_AD,
		input wire MISO_AD,
		
		input wire ExtTri,
		
        output wire SquareTick,
        output wire SquareOut

    );
wire clk;
wire clk_200M;
//CLK
 clk_wiz_0 clk_wiz_0_inst
 (
  // Clock out ports
  .clk_100M(clk),
  .clk_200M(clk_200M),
  // Status and control signals
  .resetn(1'b1),
  .locked(),
 // Clock in ports
  .clk_in1(clk_in)
 );
 
/*******************************************************************************************/
//RST
    reg rst;
    initial rst <= 1'b0;
    always@(posedge clk) begin
     if(~rst) rst <= 1'b1;
    end
/*******************************************************************************************/
 //CLK
       Si5351_Init clk_init(
       .clk(crystal_clk),
       .sclx(si5351_scl),
       .sda(si5351_sda)
       );
/*******************************************************************************************/
//BUILD STAGE
        wire[15:0] SEND_BUF,RECEIVE_BUF;
        wire[1:0]SPI_STATE;
        reg[31:0] WREG[0:`nWREG];
        reg[31:0] RREG[`nWREG+1:`nRREG];
        //initial WREG
        initial $readmemh("reg_ini.dat", WREG);
        
        //SPI_SLAVE
        //spi_working     SPI_STATE[0]
        //receive_data     SPI_STATE[1]
        my_spi_slave my_spi_slave_inst(
        .clk(clk),
        .rst(rst),
        .SEND_BUF(SEND_BUF),
        .RECEIVE_BUF(RECEIVE_BUF),
        .spi_state(SPI_STATE),
        .SCK(SCK),
        .CS(CS),
        .MOSI(MOSI),
        .MISO(MISO)
        );    
    
        //STATE: receive_cnt
        //RECEIVE: receive_buf[0:2]
        reg[1:0]receive_cnt = 2'b0;
        reg[15:0] receive_buf[0:2];// 2lsb 1msb 0addr
        always@(posedge clk) begin
            if(~rst) begin
                receive_cnt <= 2'b0;
            end
            else begin
                if(SPI_STATE[1]) begin
                    case(receive_cnt)
                        2'd0:                                     
                            if((RECEIVE_BUF[15:8] == 8'b10000000)||(RECEIVE_BUF[15:8] == 8'b01000000) ) begin
                                if(RECEIVE_BUF[7:0] < `nRREG) begin                            
                                    receive_buf[0]<= RECEIVE_BUF;
                                    receive_cnt <= 2'd1;
                                end            
                            end
                        2'd1: begin
                            receive_buf[1]<= RECEIVE_BUF;
                            receive_cnt <= 2'd2;                
                        end
                        2'd2: begin
                            receive_buf[2]<= RECEIVE_BUF;
                            receive_cnt <= 2'd0;
                        end
                        default: receive_cnt<= 2'b0;
                    endcase
                end
            end
        end    
    
        //STATE CHANGE
        reg[1:0] receive_cnt_l;
        wire STEP_0to1,STEP_1to2,STEP_2to0,READ_EN,WRITE_EN;
        always@(posedge clk) receive_cnt_l <= receive_cnt;
        assign STEP_0to1 = (receive_cnt[0]&(~receive_cnt_l[0]));
        assign STEP_1to2 = (receive_cnt[1]&(~receive_cnt_l[1]));
        assign STEP_2to0 = (receive_cnt_l[1]&(~receive_cnt[1]));
        assign READ_EN   = (receive_buf[0][15:8] == 8'b01000000 );
        assign WRITE_EN  = (receive_buf[0][15:8] == 8'b10000000 );
        
        //READ: SEND DATA
        reg[15:0]send_buf;    
        always@(posedge clk) begin
            if(~rst) begin
            send_buf <= 16'b0;    
            end
            else if(READ_EN) begin
                if(STEP_0to1) begin
					if(receive_buf[0][7:0] <= `nWREG)
						send_buf <= WREG[receive_buf[0][7:0]][31:16];
					else if(receive_buf[0][7:0] <=`nRREG)
						send_buf <= RREG[receive_buf[0][7:0]][31:16];
				end
                if(STEP_1to2) begin
					if(receive_buf[0][7:0] <= `nWREG)
						send_buf <= WREG[receive_buf[0][7:0]][15 :0];
					else if(receive_buf[0][7:0] <=`nRREG)
						send_buf <= RREG[receive_buf[0][7:0]][15 :0];
				end				
            end
        end    
        assign SEND_BUF = send_buf;          
        //begin_read
        //if(receive_buf[0] == ( SPI_STATE[1]&&(receive_cnt == 2'd0)&&(RECEIVE_BUF == 16'h40000|16'd ?REG?)));
        
        //WRITE: UPDATE WREG
        reg[7:0] addr;
        reg reg_valid;
        always@(posedge clk) begin
            if(~rst) begin
                reg_valid <= 1'b0;
                addr <= 8'b0;
            end
            else begin
                if(STEP_2to0 & WRITE_EN & (receive_buf[0][7:0] <= `nWREG)) begin
                    reg_valid <= 1'b1;
                    addr <= receive_buf[0][7:0];
                    WREG[receive_buf[0][7:0]] <= {receive_buf[1],receive_buf[2]};
                end
                else reg_valid <= 1'b0;
            end
        end
 /*******************************************************************************************/
 //USER'S LOGIC
 //一些使能信号：
 //reg_valid 写寄存器有效使能；
 // reg_valid && (addr == 8'd？) 为某寄存器被写的使能；
 // if(receive_buf[0] == ( SPI_STATE[1]&&(receive_cnt == 2'd0)&&(RECEIVE_BUF == 16'h40000|16'd ?REG?))); 为某寄存器被读的使能（下一周期被读走）；
 // (STEP_0to1&&(receive_buf[0] == 16'h40000|16'd ?REG? ));
 
 //寄存器分配：
 //写寄存器： WREG[1] 加法测试；WREG2写DDS频率;WRG3用于采样使能1，或者读取使能2 
 //             WREG4控制采样方式 
 //           RREG[12] 状态；RREG[12] <={26'b0,ReadFifoState,WriteFifoState,stack_empty,stack_full}
 //           RREG[13]数据
 
//测试用
//WREG[0] WREG[1] RREG[11] 
     reg[31:0]sum;
     always@(posedge clk) begin
         if(~rst) begin
             sum <= 32'b0;
         end
         else begin
             if(reg_valid && (addr == 8'd0)) sum <= sum + WREG[0];
             if(reg_valid && (addr == 8'd1)&&(sum >= WREG[1])) sum <= sum - WREG[1];
             RREG[11] <= sum;        
         end
     end
     

wire [1:0] WriteFifoState;
wire [1:0] ReadFifoState;
wire stack_full;
wire stack_empty;
//实例化AD
//WREG4控制采样方式 0:不采样 1：实时采:1K速率 2：等效采样10MHz 3：等效采样200MHz 
 
wire CS_AD_COM;     
wire SampleFinished;
wire [15:0]ADReceiveData;
wire adSampleEn1K,adSampleEn10M,adSampleEn200M;//都是10ns的使能
wire  adSampleEn;
reg [1:0] stateSample;  
assign adSampleEn = (adSampleEn1K|adSampleEn10M|adSampleEn200M) && (stateSample == 2'd2);
       ADS8860 ADS8860_inst(
       .clk(clk),
       .rst(rst),
       .adSampleEn(adSampleEn),
       .ADReceiveData(ADReceiveData),
       .SampleFinished(SampleFinished),
       .CS_AD(CS_AD_COM),
       .SCLK_AD(SCLK_AD),
       .MISO_AD (MISO_AD)
       );  
//控制触发条件                         
    //DAC8811 
//    wire [15:0] DAPreData;
//    assign DAPreData = {ddsOut[9:0] + 10'b10_0000_0000,6'b0};
//    AlwaysWriteDAC8811(
//        .clk(clk),
//        .rst(rst),
//        .DAPreData(DAPreData),
//        .CS_DA(CS_DA),
//        .SCLK_DA(SCLK_DA),	
//        .MOSI_DA(MOSI_DA)
//    );   

wire TriggerEn;//同步触发
//外部触发使能
    reg[1:0] externTriggerEn;
    always@(posedge clk)begin
        if(~rst) externTriggerEn <= 2'b00;
        else begin
            externTriggerEn <= {externTriggerEn[0],ExtTri};
        end
    end
assign TriggerEn = (externTriggerEn == 2'b01);

//采样控制
 wire SampleEn;//写FIFO允许信号
 wire RequireReadFifo;//读FIFO允许信号
 
 assign SampleEn =(reg_valid && (addr == 8'd3))&&(WREG[3] == 32'd1);//写FIFO使能
 assign RequireReadFifo = (reg_valid && (addr == 8'd3))&&(WREG[3] == 32'd2);
 
    //state           
    reg[16:0] cnt1K;
    always@(posedge clk) begin
        if(~rst) stateSample <= 2'd0;
        else begin
            case(stateSample) 
                2'd0: if(SampleEn) stateSample <= 2'd1;
                2'd1: if(TriggerEn) stateSample <= 2'd2;
                2'd2: if(stack_full) stateSample <= 2'd0;
                2'd3: stateSample <= 2'd0;   
            endcase
        end
    end
    //1K; 
    always@(posedge clk) begin
        if(~rst) cnt1K <= 17'd99999;
        else if((WREG[4] == 32'd3)&&(stateSample == 2'd2))//1K : 100000T = 1ms
        begin
            if(cnt1K < 17'd99999) cnt1K <= cnt1K + 17'd1;
             else cnt1K <= 17'd0;  
        end         
        else cnt1K <= 17'd99999;     
    end
    assign adSampleEn1K = (WREG[4] == 32'd3)&&(cnt1K == 17'd99999);
    
    //10M:
    reg[11:0]CNT10MSTAND;//max = 4096
    reg[11:0] cnt10M;  
      
    reg state10M;
    always@(posedge clk) begin
        if(~rst) state10M <= 1'b0;
        else begin
            case(state10M)
                1'd0: begin
                    if(TriggerEn && (stateSample == 2'd2)&&(WREG[4] == 32'd2)) state10M <= 1'd1;//触发来了
                end
                1'd1:begin
                    if(stateSample == 2'd2) begin
                        if(cnt10M == CNT10MSTAND) state10M <= 1'd0;//延时到达，采样EN，下一次触发
                    end 
                end      
            endcase       
        end   
    end
    
    always@(posedge clk) begin
     if(~rst) cnt10M = 12'd0;
     else if((state10M == 1'd1)&&(cnt10M < CNT10MSTAND))
         cnt10M <= cnt10M +12'd1;
     else cnt10M <= 12'd0;
    end
    
    always@(posedge clk) begin
        if(~rst) CNT10MSTAND <= 12'd200;
        else begin
            if((WREG[4] == 32'd2)&&(stateSample == 2'd2)) begin
                if(cnt10M == CNT10MSTAND) CNT10MSTAND <= CNT10MSTAND + 12'd10; //延时递增
            end
            else CNT10MSTAND <= 12'd200;
        end
    end 
    assign adSampleEn10M = (WREG[4] == 32'd2)&&(cnt10M == CNT10MSTAND);//延时到了再出采样信号
    
    
    //200M **********************************************************************************
    wire CS_AD_200M;
    reg[11:0]CNT200MSTAND;//max = 4096
    reg[11:0] cnt200M;  
    reg state200M;
    wire [11:0] CNT200MCSSTAND;
   // reg[1:0] stateCSAD200M;
    reg[1:0] sampleEn200ML;
    always@(posedge clk_200M) begin
        if(~rst) state200M <= 1'b0;
        else begin
            case(state200M)
                1'd0:
                    if(TriggerEn && (stateSample == 2'd2)&&(WREG[4] == 32'd1))begin
                        state200M <= 1'd1;//触发来了
                     end
                1'd1:begin
                    if(stateSample == 2'd2) begin
                        if(cnt200M == CNT200MSTAND) state200M <= 1'd0;//到达延时，下一次触发
                    end 
                    else state200M <= 1'd0;
                end      
            endcase       
        end   
    end
    
    always@(posedge clk_200M) begin //长触发   ?????
        if(TriggerEn && (stateSample == 2'd2)&&(WREG[4] == 32'd1)&& (state200M == 1'd0)) sampleEn200ML <= 2'd01;
        else sampleEn200ML <= (sampleEn200ML<<1);
    end
    
    assign adSampleEn200M = sampleEn200ML[1] | sampleEn200ML[0];
    
    always@(posedge clk_200M) begin //延时计数
     if(~rst) cnt200M = 12'd0;
     else if((state200M == 1'd1)&&(cnt200M < CNT200MSTAND))
         cnt200M <= cnt200M +12'd1;
     else cnt200M <= 12'd0;
    end
   
    always@(posedge clk_200M) begin
        if(~rst) CNT200MSTAND <= 12'd450;
        else begin
            if((WREG[4] == 32'd1)&&(stateSample == 2'd2)) begin
                if(cnt200M == CNT200MSTAND) CNT200MSTAND <= CNT200MSTAND + 12'd1; //延时递增
            end
            else CNT200MSTAND <= 12'd450;
        end
    end 
    assign CNT200MCSSTAND = CNT200MSTAND - 12'd199; //延时最后199T只是防止采样时间不足
    
//    always@(posedge clk_200M) begin
//        if(~rst) stateCSAD200M <= 2'd0;
//        else begin
//            case(stateCSAD200M)
//                2'd0:   if(sampleEn200ML[0]) stateCSAD200M <= 2'd1;
//                2'd1:   if(cnt200M == CNT200MCSSTAND) stateCSAD200M <= 2'd2;
//                2'd2:   if(cnt200M == CNT200MSTAND) stateCSAD200M <= 2'd0;
//                2'd3:   stateCSAD200M <= 2'd0;
//            endcase
//        end  
//    end
//    assign CS_AD_200M = !(stateCSAD200M == 2'd1);
//    assign CS_AD_200M = 1'd1;
//    assign adSampleEn200M = (WREG[4] == 32'd1)&&(cnt200M == CNT200MSTAND);
    assign CS_AD_200M = !(cnt200M < CNT200MCSSTAND);
    assign CS_AD = (WREG[4] == 32'd1)? CS_AD_200M:CS_AD_COM;
    
 //控制读取
    wire [7:0] RegDataOut;
    wire rden;//读FIFO使能
    always@(posedge clk) begin //状态
        RREG[12] <={26'b0,ReadFifoState,stateSample,stack_empty,stack_full};        
    end         
    always@(posedge clk) begin//读
        RREG[13] <= {24'b0,RegDataOut};
    end
    assign rden = (STEP_0to1 && (receive_buf[0] == 16'b0100_0000_0000|16'd13 ));
 //校准方波 100K
    reg[9:0] cntAdjust;
    reg square100K;
//    wire SquareOut;
//    wire SquareTick;
    always@(posedge clk) begin
        if(~rst) begin
            square100K <= 1'b0;
            cntAdjust <= 10'd0;
        end
        else if(cntAdjust < 10'd499)
            cntAdjust <= cntAdjust + 10'd1;
        else begin
             cntAdjust <= 10'd0;
             square100K <= ~square100K;
        end
    end
    assign SquareOut = square100K;
    assign SquareTick = square100K;
//实例化SampleFIFO                  
        MySampleFIFO MySampleFIFO_inst(
        .clk(clk),
        .rst(rst),
        .Sample1timeEndEn(SampleFinished),//要求使能和数据是同相的
        .SampleDataIn(ADReceiveData[15:8]), //一直接着数据输入
   
        .RequireReadFifo(RequireReadFifo),
        .ReadEn(rden),//下一个数据源于上一个End信号，第一个数据无意义，多读一个数据
        .ReadDataOut(RegDataOut),
        
//        .WriteFifoState(WriteFifoState),
        .ReadFifoState(ReadFifoState),
        .stack_full(stack_full),
        .stack_empty(stack_empty)
        );     
          
//高速ADDA 
         reg signed [7:0] data_a,data_b;
        // wire signed [11:0] filter_out;
         always@(posedge clk)begin
             data_a <= ad9288_a;
             data_b <= ad9288_b;
             dac5652a_a[9:0] <= {square100K,9'b0};
             dac5652a_b[9:0] <= ADReceiveData[15:6];
         end
//测试 clk_200M
    reg[31:0] led_cnt;
    always@(posedge clk_200M) begin
        if(~rst) begin
            led_cnt <= 32'd0;
            led <= 8'hFF;
         end
        else begin
            if(led_cnt < 32'd99999999) led_cnt <= led_cnt + 32'd1;
            else begin 
                led_cnt <= 32'd0;
                led <= ~led;
            end
        end    
    end
//    assign led = (led_cnt ==32'd9999999) ? 8'hFF:8'h00;
endmodule

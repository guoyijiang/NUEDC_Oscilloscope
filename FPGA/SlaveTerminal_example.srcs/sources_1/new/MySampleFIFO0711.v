module MySampleFIFO(
  clk,
  rst,
  Sample1timeEndEn,//要求使能和数据是同相的
  SampleDataIn, //一直接着数据输入
  
  RequireReadFifo,
  ReadEn,//下一个数据源于上一个End信号，第一个数据无意义，多读一个数据
  ReadDataOut,

  ReadFifoState,
  stack_full,
  stack_empty
  );
  
  input  clk,rst;
  input wire Sample1timeEndEn;
  input wire [7:0]SampleDataIn;
  
  input wire RequireReadFifo;
  input wire ReadEn;
  output wire [7:0]ReadDataOut;
 
  output wire [1:0]ReadFifoState;
  output wire stack_full ,stack_empty;
  
 //写数据状态机
 // 【0空闲】------允许采样------> 【1预留】------>【2触发记时】----trigger--> 【3连续写数据】
 //	   |										             |					  			|				 
 //	   |										             |					 			计数			 
 //	   |	 							               	   超时								|		         
 //	   |									                 |					 			|
 //	   <----------------------------------------------------------------------------------- V
 //
 
 
 //读数据状态机
 //【0空闲】--FIFO满-->【1读允许】--requireEn--> 【2连续读取】
 //	  |												|
 //	  |												|
 //	  |											   计数
 //	  |												|
 //   <-------------------------------------------- V
 //
  reg[1:0] readFifoState;
  wire [7:0] FifoDataOut;
  reg fifoRst;
  
  assign ReadFifoState = readFifoState;
  always@(posedge clk) begin
    if(~rst) begin
     readFifoState <= 2'b0;
     fifoRst <= 1'b0;
    end
    else begin
      case(readFifoState)
        2'b0://idle
        begin
          fifoRst <= 1'b1;
          if(stack_full)readFifoState <= 2'd1; //改成FIFO满状态标识
        end
        2'd1://fifo已满，可读状态
          if(RequireReadFifo)   readFifoState <= 2'd2;
        2'd2://连续读取
            if(stack_empty) begin
                readFifoState <= 2'd0;
                fifoRst <= 1'b0;//复位						    
            end
		2'd3: readFifoState <= 2'd0;		  
      endcase
    end
  end
 assign  ReadDataOut  = FifoDataOut;
 
FIFO_Buffer #(
  .stack_width(8),
  .stack_height(200),
  .stack_ptr_width(8),
  .AE_level(25),
  .AF_level(175),
  .HF_level(100)
) FIFO_Buffer_inst(
  .Data_out(FifoDataOut),
  .stack_full(stack_full),
  .stack_almost_full(),
  .stack_half_full(),
  .stack_almost_empty(),
  .stack_empty(stack_empty),
  .Data_in(SampleDataIn),
  .write_to_stack(Sample1timeEndEn),
  .read_from_stack(ReadEn),
  .clk(clk),
  .rst(fifoRst)
  );
 
endmodule 

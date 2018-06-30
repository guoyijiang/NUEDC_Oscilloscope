module MySampleFIFO(
  clk,
  rst,
  Sample1timeEndEn,//Ҫ��ʹ�ܺ�������ͬ���
  SampleDataIn, //һֱ������������
  
  RequireReadFifo,
  ReadEn,//��һ������Դ����һ��End�źţ���һ�����������壬���һ������
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
  
 //д����״̬��
 // ��0���С�------�������------> ��1Ԥ����------>��2������ʱ��----trigger--> ��3����д���ݡ�
 //	   |										             |					  			|				 
 //	   |										             |					 			����			 
 //	   |	 							               	   ��ʱ								|		         
 //	   |									                 |					 			|
 //	   <----------------------------------------------------------------------------------- V
 //
 
 
 //������״̬��
 //��0���С�--FIFO��-->��1������--requireEn--> ��2������ȡ��
 //	  |												|
 //	  |												|
 //	  |											   ����
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
          if(stack_full)readFifoState <= 2'd1; //�ĳ�FIFO��״̬��ʶ
        end
        2'd1://fifo�������ɶ�״̬
          if(RequireReadFifo)   readFifoState <= 2'd2;
        2'd2://������ȡ
            if(stack_empty) begin
                readFifoState <= 2'd0;
                fifoRst <= 1'b0;//��λ						    
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

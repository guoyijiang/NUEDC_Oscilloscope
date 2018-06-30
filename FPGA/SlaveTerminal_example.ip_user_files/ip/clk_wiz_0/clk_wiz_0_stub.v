// Copyright 1986-2016 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2016.4 (win64) Build 1756540 Mon Jan 23 19:11:23 MST 2017
// Date        : Thu Jul 27 10:50:35 2017
// Host        : DESKTOP-T0T5B89 running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub
//               d:/Projects/summer2017/WEEK3/week30710/SlaveTerminal_example.srcs/sources_1/ip/clk_wiz_0/clk_wiz_0_stub.v
// Design      : clk_wiz_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7a35tftg256-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
module clk_wiz_0(clk_100M, clk_200M, resetn, locked, clk_in1)
/* synthesis syn_black_box black_box_pad_pin="clk_100M,clk_200M,resetn,locked,clk_in1" */;
  output clk_100M;
  output clk_200M;
  input resetn;
  output locked;
  input clk_in1;
endmodule

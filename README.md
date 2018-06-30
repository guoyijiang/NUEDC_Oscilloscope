## 概述
* 《数字存储示波器》单片机和FPGA端源码，使用实时采样和等效采样法，能精确测量显示10Hz～10MHz的周期信号波形。系统还具有可测2mV小信号、波形存储回放、测频、触发沿选择、校准信号输出等功能。

* Author：Guo Yijiang

* Date：2017.07

* Hardware：

   MCU: STM32F407ZGT6

   FPGA: XIlinx xc7a35tftg256-6

   DAC: dac8811

   ADC：ads8860

## 系统框图

   ![sys](https://raw.githubusercontent.com/guoyijiang/NUEDC_Oscilloscope/master/image/sys.png)

## 流程图
![sys](https://raw.githubusercontent.com/guoyijiang/NUEDC_Oscilloscope/master/image/sys2.png)

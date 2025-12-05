## Line Following Robot
The goal of this project was to build a line-follower robot, this time interfacing  OV7670 cameras with UPduino iCE40 UP5K FPGA board (“the FPGA”) and STM32L432KC microcontroller (“the MCU”). The MCU configures the OV7670 camera over the SCCB protocol. Then the FPGA interfaces with the camera to read in the pixel data and bitmask it to a binary image, and save it over an async FIFO dual frame buffer using the SPRAM, and then send the binary image over to the MCU over SPI. The MCU reads in the full binary image from the FPGA to detect the presence of the line and then uses this information to control the motor driver, corresponding to the camera and the motor on the same side of the robot, and navigate the robot over the line.

## Full Documentation
This project was built by Aabhas Senapati and Diego Weiss, who are Harvey Mudd Class of 2027 Students and designed this project as part of the E155 MicroPs Course in Fall'25.
You can find the full detailed documentation of the design aspects of this robot on the portfolio site: https://diego2317.github.io/E155-project-site/

## Video Demonstration of our robot navigating over a line
https://www.youtube.com/watch?v=vnzX3ZjI89I

## Table of Contents:

[FPGA Design](/fpga)
Code for the FPGA implementation, including the camera pixel capture and bitmasking module, and the SPRAM frame buffer and SPI transfer module.

[MCU Design](/mcu)
Code for the MCU implementation in configuring the camera, reading in the bitmasked image, and detecting the line, and controlling the robot.

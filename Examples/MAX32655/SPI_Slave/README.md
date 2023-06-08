## Description

Two EvKit boards will be used in this demo. One is master, and another one is slave.

This example is running on the slave board and configures the SPI to receive data from the master EvKit board through SPI.

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

(None - this project builds as a standard example)

## Setup

##### Board Selection

##### Required Connections
If using the Standard EV Kit (EvKit_V1):
JH6.4 <--> JH6.4
JH6.5 <--> JH6.5
JH6.6 <--> JH6.6
JH6.7 <--> JH6.7

## Expected Output
**************************** SPI SLAVE RX TEST *************************

1 rx: 0x000A 0x000B
2 rx: 0x000B 0x000C
3 rx: 0x000C 0x000D
4 rx: 0x000D 0x000E
5 rx: 0x000E 0x000F
6 rx: 0x000F 0x0010
7 rx: 0x0010 0x0011
8 rx: 0x0011 0x0012
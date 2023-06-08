## Description

Two EvKit boards will be used in this demo. One is master, another one is slave.

This example is running on the master board and configures the SPI to send data to the slave EvKit board through SPI.

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
**************************** SPI MASTER TEST *************************

send: 0x0001 0x0002
send: 0x0002 0x0003
send: 0x0003 0x0004
send: 0x0004 0x0005
send: 0x0005 0x0006
send: 0x0006 0x0007
send: 0x0007 0x0008
send: 0x0008 0x0009
send: 0x0009 0x000A
send: 0x000A 0x000B
send: 0x000B 0x000C
send: 0x000C 0x000D
send: 0x000D 0x000E
send: 0x000E 0x000F
send: 0x000F 0x0010
send: 0x0010 0x0011
send: 0x0011 0x0012
send: 0x0012 0x0013
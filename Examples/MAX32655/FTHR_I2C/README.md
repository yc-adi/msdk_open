## Description

This example uses I2C to cycle through the 8 colors of the RGB LED connected to the on-board MAX20303 Power Management IC. 

*** NOTE ***: This example is only intended to be run on the MAX32655FTHR.


## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

* NOTE: This project is only compatible with the MAX32655FTHR!

## Required Connections
If using the MAX32655FTHR (FTHR\_Apps\_P1):
-   Connect a USB cable between the PC and the J4 (USB/PWR) connector.
-   Open an terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.

## Expected Behavior
The PMIC RGB LED should change colors once a second, cycling through the 8 colors (off, blue, red, purple, green, light blue, yellow, white).

The Console UART of the device will output these messages:

```
******** Featherboard I2C Demo *********

This demo uses the I2C to change the state of the Power
Management IC's RGB LED.
```

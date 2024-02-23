## Description

This example configures the SPI to send data between the MISO (P0.22) and MOSI (P0.21) pins.  Connect these two pins together.

Multiple word sizes (2 through 16 bits) are demonstrated.

By default, the example performs blocking SPI transactions.  To switch to non-blocking (asynchronous) transactions, reset the `CONTROLLER_SYNC` macro to 0 and set the `CONTROLLER_ASYNC` macro to 1.  To use DMA transactions, set the `CONTROLLER_DMA` macro to 1 instead.

This example uses the Hardware Target Select control scheme (application does not assert the TS pins).

## Software

This project uses the SPI v2 library. More information on the SPI v2 library can be found in the **[MSDK User Guide Developer Notes](https://analog-devices-msdk.github.io/msdk/USERGUIDE/#spi-v2-library)**.

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

Set `MXC_SPI_VERSION=v2` to build the SPI v2 libraries.

## Required Connections

-   Connect a USB cable between the PC and the CN2 (USB/PWR) connector.
-   Connect the 5V power cable at (5V IN).
-   Close jumper (RX - P0.0) and (TX - P0.1) at Headers JP23 (UART 0 EN).
-   Open an terminal application on the PC and connect to the EV kit's console UART at 115200, 8-N-1.
-   Connect P0.21 (MOSI) to P0.22 (MISO).

## Expected Output

The Console UART of the device will output these messages:

```
**************************** SPI CONTROLLER TEST *************************
This example configures the SPI to send data between the MISO (P0.22) and
MOSI (P0.21) pins.  Connect these two pins together.

Multiple word sizes (2 through 16 bits) are demonstrated.

Performing blocking (synchronous) transactions...
--> 2 Bits Transaction Successful
--> 3 Bits Transaction Successful
--> 4 Bits Transaction Successful
--> 5 Bits Transaction Successful
--> 6 Bits Transaction Successful
--> 7 Bits Transaction Successful
--> 8 Bits Transaction Successful
--> 9 Bits Transaction Successful
-->10 Bits Transaction Successful
-->11 Bits Transaction Successful
-->12 Bits Transaction Successful
-->13 Bits Transaction Successful
-->14 Bits Transaction Successful
-->15 Bits Transaction Successful
-->16 Bits Transaction Successful

Example Complete.
```

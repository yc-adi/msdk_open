# Description

Bluetooth version 5.2 controller Zephyr library, accepts HCI commands via ChciTrRecv and responds via ChciTrWrite.

Refer to the [BLE5_zephyr](../../../Libraries/Cordio/docs/Applications/BLE5_zephyr.md) documentation in the Cordio Library.

## Software

- Select the "zephyr" RTOS wsf layer to integrate with Zephyr.
- Optionally build with Zephyr SDK toolchain.
- Disable UART baremetal, enables "CHCI_TR_CUSTOM" for custom transport to Zephyr.

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.

### Required Connections
-   Connect a USB cable between the PC and the (USB/PWR - UART) connector.

### Project-Specific Build Notes
* Setting `TRACE=1` in [**project.mk**](project.mk) initializes the on-board USB-to-UART adapter for
viewing the trace messages and interacting with the application. Port uses settings:
    - Baud            : 115200  
    - Char size       : 8  
    - Parity          : None  
    - Stop bits       : 1  
    - HW Flow Control : No  
    - SW Flow Control : No  

### Building the Zephyr library.

```sh
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr $ make -j8; make lib
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr $ mkdir tmp && cd tmp
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr/tmp $ cp ~/msdk/Libraries/PeriphDrivers/bin/MAX32655/softfp/libPeriphDriver_softfp.a ~/msdk/Libraries/Cordio/bin/max32655_zephyr_T0_softfp/cordio_max32655_zephyr_T0_softfp.a ~/msdk/Libraries/BlePhy/MAX32655/libphy.a ../build/max32655.a .
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr/tmp $ ar -x cordio_max32655_zephyr_T0_softfp.a; ar -x libPeriphDriver_softfp.a; ar -x libphy.a; ar -x max32655.a
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr/tmp $ ~/zephyr-sdk-0.16.1/arm-zephyr-eabi/bin/arm-zephyr-eabi-ar -rcs libblectrl-msdk.a *.o
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr/tmp $ cp libblectrl-msdk.a ~/zephyrproject/modules/hal/adi/zephyr/blobs/lib/max32655/libblectrl.a
~/msdk/Examples/MAX32655/Bluetooth/BLE5_Zephyr/tmp $ sha256sum libblectrl-msdk.a
c97a548db10942a3a794a39ee5a8da66b43bf1f7e703eb83b7f82e36365704d4  libblectrl-msdk.a
```

Copy the sha to the Zephyr ~/zephyrproject/modules/hal/adi/zephyr/module.yml entry.

Then in a separate Zephyr development terminal, run:

```sh
~/zephyrproject/zephyr $ west build -p -b max32655fthr ./samples/bluetooth/peripheral
```

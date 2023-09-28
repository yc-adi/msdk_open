# Description

Bluetooth CGM device demo.

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/)**.

For the OTA function, please refer to the projects of BLE_otac and BLE_otas.

## Required Connections
If using the Standard EV Kit board (EvKit\_V1):
-   Connect a USB cable between the PC and the CN2 (USB/PWR - UART) connector.
-   Close jumpers JP7 (RX_EN) and JP8 (TX_EN).
-   Close jumpers JP5 (LED1 EN) and JP6 (LED2 EN).

### Project-Specific Build Notes
* The Bootloader application needs to be loaded prior to loading this application. This application
will run on top of the Bootloader. The [ota.ld](ota.ld) linker file included with this application must be used
to properly setup the memory sections to coincide with the Bootloader.
* Setting `TRACE=1` in [**project.mk**](project.mk) initializes the on-board USB-to-UART adapter for
viewing the trace messages and interacting with the application. Port uses settings:
    - Baud            : 115200  
    - Char size       : 8  
    - Parity          : None  
    - Stop bits       : 1  
    - HW Flow Control : No  
    - SW Flow Control : No  
* Setting `USE_INTERNAL_FLASH=1` in **project.mk** tells the Bootloader to look for firmware updates in internal flash
* Setting `USE_INTERNAL_FLASH=0` in **project.mk** tells the Bootloader to look for firmware updates in external flash
    - This is the default behavior

### Handlers, Messages, and Events
```
handlerId: 0, TerminalHandler␍␊
handlerId: 1, SchHandler␍␊
handlerId: 2, LlHandler
handlerId: 3, HciHandler␍␊
handlerId: 4, DmHandler␍␊
handlerId: 5, L2cSlaveHandler␍␊
handlerId: 6, AttHandler␍␊
handlerId: 7, SmpHandler␍␊
handlerId: 8, AppHandler␍␊
handlerId: 9, CgmHandler␍␊
handlerId: 10, WdxsHandler␍␊
handlerId: 11, wdxsFileEraseHandler␍␊
handlerId: 12, wdxsFileWriteHandler␍␊

DM events
"CGM got evt="
"AppSlaveSecProcDmMsg evt="
63 DM_PRIV_SET_ADDR_RES_ENABLE_IND
65 DM_CONN_DATA_LEN_CHANGE_IND

"App got evt "
16 APP_CONN_UPDATE_TIMEOUT_IND
```


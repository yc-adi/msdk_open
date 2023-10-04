# BLE_LR_Periph_FreeRTOS

This project is a demo for long range advertising. The coded PHY (s=8) is used. The advertising interval is 40 ms. And in each advertising data, it includes 50-byte user data.  

The project is modified from the BLE_LR_Peripheral project. It works with the BLE_LR_Central project.  

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analog-devices-msdk.github.io/msdk/USERGUIDE/)**.  

### Required Connections

If using the Standard EV Kit board (EvKit\_V1):  
-   Connect a USB cable between the PC and the CN2 (USB/PWR - UART) connector.  
-   Close jumpers JP7 (RX_EN) and JP8 (TX_EN).  
-   Close jumpers JP5 (LED1 EN) and JP6 (LED2 EN).  

### Project-Specific Build Notes
* Setting `TRACE=1` in [**project.mk**](project.mk) initializes the on-board USB-to-UART adapter for  
viewing the trace messages and interacting with the application. Port uses settings:  
    - Baud            : 115200  
    - Char size       : 8  
    - Parity          : None  
    - Stop bits       : 1  
    - HW Flow Control : No  
    - SW Flow Control : No  

### Log
```
===================================  
-=- MAX32665 BLE FreeRTOS (V10.5.1) Demo -=-  
===================================  
SystemCoreClock = 96000000  
terminal: init  
===================================  
BLE long range demo (coded-PHY s=8)  
===================================  
BT_VER: 9  
DmExtStarting scheduler.  
AdvInit  
FitHandlerInit  
MAC Addr: 00:18:80:00:44:B1  
Adv local name: LongRange  
Fit got evt 32  
>>> Reset complete <<<  
dmDevPassEvtToDevPriv: event: 14, param: 2, advHandle: 0  
dmExtAdvHciHandler: event: 55  
dmDevPassEvtToDevPriv: event: 12, param: 71, advHandle: 0  
Fit got evt 71  
>>> Advertising sets started <<<  
Fit got evt 21  
Database hash updated  
02  
03  
Fit got evt 52  
Uptime is 0x0000004E (0 seconds)␊
  
04  
05  
06  
07  
08  
09  
0A  
0B  
0C  
0D  
0E  
0F  
10  
...  
```

;*******************************************************************************
;* Copyright (C) 2018 Maxim Integrated Products, Inc., All Rights Reserved.
;*
;* Permission is hereby granted, free of charge, to any person obtaining a
;* copy of this software and associated documentation files (the "Software"),
;* to deal in the Software without restriction, including without limitation
;* the rights to use, copy, modify, merge, publish, distribute, sublicense,
;* and/or sell copies of the Software, and to permit persons to whom the
;* Software is furnished to do so, subject to the following conditions:
;*
;* The above copyright notice and this permission notice shall be included
;* in all copies or substantial portions of the Software.
;*
;* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
;* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
;* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
;* OTHER DEALINGS IN THE SOFTWARE.
;*
;* Except as contained in this notice, the name of Maxim Integrated
;* Products, Inc. shall not be used except as stated in the Maxim Integrated
;* Products, Inc. Branding Policy.
;*
;* The mere transfer of this software does not imply any licenses
;* of trade secrets, proprietary technology, copyrights, patents,
;* trademarks, maskwork rights, or any other form of intellectual
;* property whatsoever. Maxim Integrated Products, Inc. retains all
;* ownership rights.
;*
;* Description        : MAX32665 device vector table for IAR EWARM toolchain.
;*                      - Sets the initial SP
;*                      - Sets the initial PC == _iar_program_start,
;*                      - Set the vector table entries with the exceptions ISR
;*                        address, all set as PUBWEAK. User may override any ISR
;*                        defined as PUBWEAK.
;*                      - Branches to main in the C library (which eventually
;*                        calls SystemInit() and main()).
;*                      After Reset the Cortex-M4 processor is in Thread mode,
;*                      priority is Privileged, and the Stack is set to Main.
;* $Date: 2019-07-30 14:08:36 -0500 (Tue, 30 Jul 2019) $
;* $Revision: 45136 $
;*
;*******************************************************************************
    MODULE  ?cstartup

    ;; Forward declaration of sections.
	;; I modified this... CSTACKC1
    SECTION CSTACKC1:DATA:NOROOT(3)

    ;;SECTION .intvec:CODE:NOROOT(2)  WILL NOT USE .intvec but will be placed in a Code section
	SECTION .text:CODE:NOROOT(2)

    ;; EXTERN  __iar_program_start
       PUBLIC  __vector_table1
       PUBLIC  __vector_table_modify1
       PUBLIC  __Vectors1
       PUBLIC  __Vectors_End1
       PUBLIC  __Vectors_Size1
       PUBLIC  __isr_vector_core1 

    DATA
__vector_table1
    DCD     sfe(CSTACKC1)
    DCD    Reset_Handler_Core1   ; Reset Handler 
    DCD    NMI_Handler           ; NMI Handler 
    DCD    HardFault_Handler     ; Hard Fault Handler 
    DCD    MemManage_Handler     ; MPU Fault Handler 
    DCD    BusFault_Handler      ; Bus Fault Handler 
    DCD    UsageFault_Handler    ; Usage Fault Handler 
    DCD    0                     ; Reserved 
    DCD    0                     ; Reserved 
    DCD    0                     ; Reserved 
    DCD    0                     ; Reserved 
    DCD    SVC_Handler           ; SVCall Handler 
    DCD    0                     ; Reserved  ; @TODO: Is this the Debug Montior Interrupt? 
    DCD    0                     ; Reserved 
    DCD    PendSV_Handler        ; PendSV Handler 
    DCD    SysTick_Handler       ; SysTick Handler 
__vector_table_modify1
    ; Device-specific Interrupts 
    DCD    PF_IRQHandler                 ; 0x10  0x0040  16: Power Fail 
    DCD    WDT0_IRQHandler               ; 0x11  0x0044  17: Watchdog 0 
    DCD    USB_IRQHandler                ; 0x12  0x0048  18: USB 
    DCD    RTC_IRQHandler                ; 0x13  0x004C  19: RTC 
    DCD    TRNG_IRQHandler               ; 0x14  0x0050  20: True Random Number Generator 
    DCD    TMR0_IRQHandler               ; 0x15  0x0054  21: Timer 0 
    DCD    TMR1_IRQHandler               ; 0x16  0x0058  22: Timer 1 
    DCD    TMR2_IRQHandler               ; 0x17  0x005C  23: Timer 2 
    DCD    TMR3_IRQHandler               ; 0x18  0x0060  24: Timer 3
    DCD    TMR4_IRQHandler               ; 0x19  0x0064  25: Timer 4
    DCD    TMR5_IRQHandler               ; 0x1A  0x0068  26: Timer 5 
    DCD    RSV11_IRQHandler              ; 0x1B  0x006C  27: Reserved 
    DCD    RSV12_IRQHandler              ; 0x1C  0x0070  28: Reserved 
    DCD    I2C0_IRQHandler               ; 0x1D  0x0074  29: I2C0 
    DCD    UART0_IRQHandler              ; 0x1E  0x0078  30: UART 0 
    DCD    UART1_IRQHandler              ; 0x1F  0x007C  31: UART 1 
    DCD    SPI1_IRQHandler               ; 0x20  0x0080  32: SPI1 
    DCD    SPI2_IRQHandler               ; 0x21  0x0084  33: SPI2 
    DCD    RSV18_IRQHandler              ; 0x22  0x0088  34: Reserved 
    DCD    RSV19_IRQHandler              ; 0x23  0x008C  35: Reserved 
    DCD    ADC_IRQHandler                ; 0x24  0x0090  36: ADC 
    DCD    RSV21_IRQHandler              ; 0x25  0x0094  37: Reserved 
    DCD    RSV22_IRQHandler              ; 0x26  0x0098  38: Reserved 
    DCD    FLC0_IRQHandler               ; 0x27  0x009C  39: Flash Controller 
    DCD    GPIO0_IRQHandler              ; 0x28  0x00A0  40: GPIO0 
    DCD    GPIO1_IRQHandler              ; 0x29  0x00A4  41: GPIO2 
    DCD    RSV26_IRQHandler              ; 0x2A  0x00A8  42: GPIO3 
    DCD    TPU_IRQHandler                ; 0x2B  0x00AC  43: Crypto 
    DCD    DMA0_IRQHandler               ; 0x2C  0x00B0  44: DMA0 
    DCD    DMA1_IRQHandler               ; 0x2D  0x00B4  45: DMA1 
    DCD    DMA2_IRQHandler               ; 0x2E  0x00B8  46: DMA2 
    DCD    DMA3_IRQHandler               ; 0x2F  0x00BC  47: DMA3 
    DCD    RSV32_IRQHandler              ; 0x30  0x00C0  48: Reserved 
    DCD    RSV33_IRQHandler              ; 0x31  0x00C4  49: Reserved 
    DCD    UART2_IRQHandler              ; 0x32  0x00C8  50: UART 2 
    DCD    RSV35_IRQHandler              ; 0x33  0x00CC  51: Reserved 
    DCD    I2C1_IRQHandler               ; 0x34  0x00D0  52: I2C1 
    DCD    RSV37_IRQHandler              ; 0x35  0x00D4  53: Reserved 
    DCD    SPIXFC_IRQHandler             ; 0x36  0x00D8  54: SPI execute in place 
    DCD    BTLE_TX_DONE_IRQHandler       ; 0x37  0x00DC  55: BTLE TX Done 
    DCD    BTLE_RX_RCVD_IRQHandler       ; 0x38  0x00E0  56: BTLE RX Recived 
    DCD    BTLE_RX_ENG_DET_IRQHandler    ; 0x39  0x00E4  57: BTLE RX Energy Dectected 
    DCD    BTLE_SFD_DET_IRQHandler       ; 0x3A  0x00E8  58: BTLE SFD Detected 
    DCD    BTLE_SFD_TO_IRQHandler        ; 0x3B  0x00EC  59: BTLE SFD Timeout
    DCD    BTLE_GP_EVENT_IRQHandler      ; 0x3C  0x00F0  60: BTLE Timestamp
    DCD    BTLE_CFO_IRQHandler           ; 0x3D  0x00F4  61: BTLE CFO Done 
    DCD    BTLE_SIG_DET_IRQHandler       ; 0x3E  0x00F8  62: BTLE Signal Detected 
    DCD    BTLE_AGC_EVENT_IRQHandler     ; 0x3F  0x00FC  63: BTLE AGC Event 
    DCD    BTLE_RFFE_SPIM_IRQHandler     ; 0x40  0x0100  64: BTLE RFFE SPIM Done 
    DCD    BTLE_TX_AES_IRQHandler        ; 0x41  0x0104  65: BTLE TX AES Done 
    DCD    BTLE_RX_AES_IRQHandler        ; 0x42  0x0108  66: BTLE RX AES Done 
    DCD    BTLE_INV_APB_ADDR_IRQHandler  ; 0x43  0x010C  67: BTLE Invalid APB Address
    DCD    BTLE_IQ_DATA_VALID_IRQHandler ; 0x44  0x0110  68: BTLE IQ Data Valid 
    DCD    WUT_IRQHandler                ; 0x45  0x0114  69: WUT Wakeup 
    DCD    GPIOWAKE_IRQHandler           ; 0x46  0x0118  70: GPIO Wakeup 
    DCD    RSV55_IRQHandler              ; 0x47  0x011C  71: Reserved 
    DCD    SPI0_IRQHandler               ; 0x48  0x0120  72: SPI AHB 
    DCD    WDT1_IRQHandler               ; 0x49  0x0124  73: Watchdog 1 
    DCD    RSV58_IRQHandler              ; 0x4A  0x0128  74: Reserved 
    DCD    PT_IRQHandler                 ; 0x4B  0x012C  75: Pulse train 
    DCD    SDMA0_IRQHandler              ; 0x4C  0x0130  76: Smart DMA 0 
    DCD    RSV61_IRQHandler              ; 0x4D  0x0134  77: Reserved 
    DCD    I2C2_IRQHandler               ; 0x4E  0x0138  78: I2C 2 
    DCD    RSV63_IRQHandler              ; 0x4F  0x013C  79: Reserved 
    DCD    RSV64_IRQHandler              ; 0x50  0x0140  80: Reserved 
    DCD    RSV65_IRQHandler              ; 0x51  0x0144  81: Reserved 
    DCD    SDHC_IRQHandler               ; 0x52  0x0148  82: SDIO/SDHC 
    DCD    OWM_IRQHandler                ; 0x53  0x014C  83: One Wire Master 
    DCD    DMA4_IRQHandler               ; 0x54  0x0150  84: DMA4 
    DCD    DMA5_IRQHandler               ; 0x55  0x0154  85: DMA5 
    DCD    DMA6_IRQHandler               ; 0x56  0x0158  86: DMA6 
    DCD    DMA7_IRQHandler               ; 0x57  0x015C  87: DMA7 
    DCD    DMA8_IRQHandler               ; 0x58  0x0160  88: DMA8 
    DCD    DMA9_IRQHandler               ; 0x59  0x0164  89: DMA9 
    DCD    DMA10_IRQHandler              ; 0x5A  0x0168  90: DMA10 
    DCD    DMA11_IRQHandler              ; 0x5B  0x016C  91: DMA11 
    DCD    DMA12_IRQHandler              ; 0x5C  0x0170  92: DMA12 
    DCD    DMA13_IRQHandler              ; 0x5D  0x0174  93: DMA13 
    DCD    DMA14_IRQHandler              ; 0x5E  0x0178  94: DMA14 
    DCD    DMA15_IRQHandler              ; 0x5F  0x017C  95: DMA15 
    DCD    USBDMA_IRQHandler             ; 0x60  0x0180  96: USB DMA 
    DCD    WDT2_IRQHandler               ; 0x61  0x0184  97: Watchdog Timer 2 
    DCD    ECC_IRQHandler                ; 0x62  0x0188  98: Error Correction 
    DCD    DVS_IRQHandler                ; 0x63  0x018C  99: DVS Controller 
    DCD    SIMO_IRQHandler               ; 0x64 0x0190  100: SIMO Controller 
    DCD    SCA_IRQHandler                ; 0x65  0x0194  101: SCA  ; @TODO: Is this correct? 
    DCD    AUDIO_IRQHandler              ; 0x66  0x0198  102: Audio subsystem 
    DCD    FLC1_IRQHandler               ; 0x67  0x019C  103: Flash Control 1 
    DCD    RSV88_IRQHandler              ; 0x68  0x01A0  104: UART 3 
    DCD    RSV89_IRQHandler              ; 0x69  0x01A4  105: UART 4 
    DCD    RSV90_IRQHandler              ; 0x6A  0x01A8  106: UART 5 
    DCD    RSV91_IRQHandler              ; 0x6B  0x01AC  107: Camera IF 
    DCD    RSV92_IRQHandler              ; 0x6C  0x01B0  108: I3C  
    DCD    HTMR0_IRQHandler              ; 0x6D  0x01B4  109: HTmr 
    DCD    HTMR1_IRQHandler              ; 0x6E  0x01B8  109: HTmr 

__Vectors_End1
__isr_vector_core1    EQU   __vector_table1
__Vectors1            EQU   __vector_table1
__Vectors_Size1       EQU   __Vectors_End1 - __Vectors1



    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
    THUMB
    PUBWEAK Reset_Handler_Core1
    SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler_Core1
    EXTERN   PreInit_Core1
	EXTERN   SystemInit_Core1
	EXTERN   Core1_Main
	
    LDR     R0, =PreInit_Core1
            BLX     R0

            LDR     R0, =SystemInit_Core1
            BLX     R0

            LDR     R0, =Core1_Main
            BX      R0
__SPIN1
            WFI
            BL __SPIN1



#if 0
    //
    // Enable ECC for System RAM - this must run before the first stack or SRAM operation
    //
    /* Turn on ECC for all banks */
    ldr r0, =0x40006C00                   // 0x40006C00=Addr:MXC_BASE_MCR
    ldr r1, [r0, #0x00000000]             // 0x00000000=Addr:MXC_R_MCR_ECCEN
    orr r1, r1, #0x3F
    str r1, [r0, #0x00000000]             // 0x00000000=Addr:MXC_R_MCR_ECCEN

    /* Zeroize all banks, which ensures ECC bits are written for no errors */
    ldr r0, =0x40000000                   // 0x40000000=Addr:MXC_BASE_GCR
    ldr r1, [r0, #0x0000002C]             // 0x0000002C=Addr:MXC_R_GCR_MEMZCN
    orr r1, r1, #0x3F
    str r1, [r0, #0x0000002C]             // 0x0000002C=Addr:MXC_R_GCR_MEMZCN

    /* wait till complete */
ecc_loop: ldr r1, [r0, #0x0000002C]       // 0x0000002C=Addr:MXC_R_GCR_MEMZCN
         cmp r1, #0
         beq ecc_init_exit
         b ecc_loop;

ecc_init_exit:



; SystemInit will be called as part of the __iar_program_start flow prior to
; the call to main. It needs to be called after static data/variable initialization
; which occurs during the __iar_program_start definition.
; see project file cmain.s for the IAR specific startup flow and call to SystemInit
        ; Set the initial stack pointer
        LDR        R0, =0x10000000
        LDR        R1, [R0]
        MOV        SP, R1
        ; jump into IAR program flow
        LDR        R0, =__iar_program_start
        BX         R0

def_irq_handler MACRO handler_name
        PUBWEAK handler_name
        Section .text:CODE:REORDER:NOROOT(1)
handler_name
        B handler_name
        ENDM
#endif

    EXTERN    NMI_Handler                   ; NMI Handler 
    EXTERN    HardFault_Handler             ; Hard Fault Handler 
    EXTERN    MemManage_Handler             ; MPU Fault Handler 
    EXTERN    BusFault_Handler              ; Bus Fault Handler 
    EXTERN    UsageFault_Handler            ; Usage Fault Handler 
    EXTERN    SVC_Handler                   ; SVCall Handler 
    EXTERN    PendSV_Handler                ; PendSV Handler 
    EXTERN    SysTick_Handler               ; SysTick Handler
    EXTERN    PF_IRQHandler                 ; 0x10  0x0040  16: Power Fail 
    EXTERN    WDT0_IRQHandler               ; 0x11  0x0044  17: Watchdog 0 
    EXTERN    USB_IRQHandler                ; 0x12  0x0048  18: USB 
    EXTERN    RTC_IRQHandler                ; 0x13  0x004C  19: RTC 
    EXTERN    TRNG_IRQHandler               ; 0x14  0x0050  20: True Random Number Generator 
    EXTERN    TMR0_IRQHandler               ; 0x15  0x0054  21: Timer 0 
    EXTERN    TMR1_IRQHandler               ; 0x16  0x0058  22: Timer 1 
    EXTERN    TMR2_IRQHandler               ; 0x17  0x005C  23: Timer 2 
    EXTERN    TMR3_IRQHandler               ; 0x18  0x0060  24: Timer 3
    EXTERN    TMR4_IRQHandler               ; 0x19  0x0064  25: Timer 4
    EXTERN    TMR5_IRQHandler               ; 0x1A  0x0068  26: Timer 5 
    EXTERN    RSV11_IRQHandler              ; 0x1B  0x006C  27: Reserved 
    EXTERN    RSV12_IRQHandler              ; 0x1C  0x0070  28: Reserved 
    EXTERN    I2C0_IRQHandler               ; 0x1D  0x0074  29: I2C0 
    EXTERN    UART0_IRQHandler              ; 0x1E  0x0078  30: UART 0 
    EXTERN    UART1_IRQHandler              ; 0x1F  0x007C  31: UART 1 
    EXTERN    SPI1_IRQHandler               ; 0x20  0x0080  32: SPI1 
    EXTERN    SPI2_IRQHandler               ; 0x21  0x0084  33: SPI2 
    EXTERN    RSV18_IRQHandler              ; 0x22  0x0088  34: Reserved 
    EXTERN    RSV19_IRQHandler              ; 0x23  0x008C  35: Reserved 
    EXTERN    ADC_IRQHandler                ; 0x24  0x0090  36: ADC 
    EXTERN    RSV21_IRQHandler              ; 0x25  0x0094  37: Reserved 
    EXTERN    RSV22_IRQHandler              ; 0x26  0x0098  38: Reserved 
    EXTERN    FLC0_IRQHandler               ; 0x27  0x009C  39: Flash Controller 
    EXTERN    GPIO0_IRQHandler              ; 0x28  0x00A0  40: GPIO0 
    EXTERN    GPIO1_IRQHandler              ; 0x29  0x00A4  41: GPIO2 
    EXTERN    RSV26_IRQHandler              ; 0x2A  0x00A8  42: GPIO3 
    EXTERN    TPU_IRQHandler                ; 0x2B  0x00AC  43: Crypto 
    EXTERN    DMA0_IRQHandler               ; 0x2C  0x00B0  44: DMA0 
    EXTERN    DMA1_IRQHandler               ; 0x2D  0x00B4  45: DMA1 
    EXTERN    DMA2_IRQHandler               ; 0x2E  0x00B8  46: DMA2 
    EXTERN    DMA3_IRQHandler               ; 0x2F  0x00BC  47: DMA3 
    EXTERN    RSV32_IRQHandler              ; 0x30  0x00C0  48: Reserved 
    EXTERN    RSV33_IRQHandler              ; 0x31  0x00C4  49: Reserved 
    EXTERN    UART2_IRQHandler              ; 0x32  0x00C8  50: UART 2 
    EXTERN    RSV35_IRQHandler              ; 0x33  0x00CC  51: Reserved 
    EXTERN    I2C1_IRQHandler               ; 0x34  0x00D0  52: I2C1 
    EXTERN    RSV37_IRQHandler              ; 0x35  0x00D4  53: Reserved 
    EXTERN    SPIXFC_IRQHandler             ; 0x36  0x00D8  54: SPI execute in place 
    EXTERN    BTLE_TX_DONE_IRQHandler       ; 0x37  0x00DC  55: BTLE TX Done 
    EXTERN    BTLE_RX_RCVD_IRQHandler       ; 0x38  0x00E0  56: BTLE RX Recived 
    EXTERN    BTLE_RX_ENG_DET_IRQHandler    ; 0x39  0x00E4  57: BTLE RX Energy Dectected 
    EXTERN    BTLE_SFD_DET_IRQHandler       ; 0x3A  0x00E8  58: BTLE SFD Detected 
    EXTERN    BTLE_SFD_TO_IRQHandler        ; 0x3B  0x00EC  59: BTLE SFD Timeout
    EXTERN    BTLE_GP_EVENT_IRQHandler      ; 0x3C  0x00F0  60: BTLE Timestamp
    EXTERN    BTLE_CFO_IRQHandler           ; 0x3D  0x00F4  61: BTLE CFO Done 
    EXTERN    BTLE_SIG_DET_IRQHandler       ; 0x3E  0x00F8  62: BTLE Signal Detected 
    EXTERN    BTLE_AGC_EVENT_IRQHandler     ; 0x3F  0x00FC  63: BTLE AGC Event 
    EXTERN    BTLE_RFFE_SPIM_IRQHandler     ; 0x40  0x0100  64: BTLE RFFE SPIM Done 
    EXTERN    BTLE_TX_AES_IRQHandler        ; 0x41  0x0104  65: BTLE TX AES Done 
    EXTERN    BTLE_RX_AES_IRQHandler        ; 0x42  0x0108  66: BTLE RX AES Done 
    EXTERN    BTLE_INV_APB_ADDR_IRQHandler  ; 0x43  0x010C  67: BTLE Invalid APB Address
    EXTERN    BTLE_IQ_DATA_VALID_IRQHandler ; 0x44  0x0110  68: BTLE IQ Data Valid 
    EXTERN    WUT_IRQHandler                ; 0x45  0x0114  69: WUT Wakeup 
    EXTERN    GPIOWAKE_IRQHandler           ; 0x46  0x0118  70: GPIO Wakeup 
    EXTERN    RSV55_IRQHandler              ; 0x47  0x011C  71: Reserved 
    EXTERN    SPI0_IRQHandler               ; 0x48  0x0120  72: SPI AHB 
    EXTERN    WDT1_IRQHandler               ; 0x49  0x0124  73: Watchdog 1 
    EXTERN    RSV58_IRQHandler              ; 0x4A  0x0128  74: Reserved 
    EXTERN    PT_IRQHandler                 ; 0x4B  0x012C  75: Pulse train 
    EXTERN    SDMA0_IRQHandler              ; 0x4C  0x0130  76: Smart DMA 0 
    EXTERN    RSV61_IRQHandler              ; 0x4D  0x0134  77: Reserved 
    EXTERN    I2C2_IRQHandler               ; 0x4E  0x0138  78: I2C 2 
    EXTERN    RSV63_IRQHandler              ; 0x4F  0x013C  79: Reserved 
    EXTERN    RSV64_IRQHandler              ; 0x50  0x0140  80: Reserved 
    EXTERN    RSV65_IRQHandler              ; 0x51  0x0144  81: Reserved 
    EXTERN    SDHC_IRQHandler               ; 0x52  0x0148  82: SDIO/SDHC 
    EXTERN    OWM_IRQHandler                ; 0x53  0x014C  83: One Wire Master 
    EXTERN    DMA4_IRQHandler               ; 0x54  0x0150  84: DMA4 
    EXTERN    DMA5_IRQHandler               ; 0x55  0x0154  85: DMA5 
    EXTERN    DMA6_IRQHandler               ; 0x56  0x0158  86: DMA6 
    EXTERN    DMA7_IRQHandler               ; 0x57  0x015C  87: DMA7 
    EXTERN    DMA8_IRQHandler               ; 0x58  0x0160  88: DMA8 
    EXTERN    DMA9_IRQHandler               ; 0x59  0x0164  89: DMA9 
    EXTERN    DMA10_IRQHandler              ; 0x5A  0x0168  90: DMA10 
    EXTERN    DMA11_IRQHandler              ; 0x5B  0x016C  91: DMA11 
    EXTERN    DMA12_IRQHandler              ; 0x5C  0x0170  92: DMA12 
    EXTERN    DMA13_IRQHandler              ; 0x5D  0x0174  93: DMA13 
    EXTERN    DMA14_IRQHandler              ; 0x5E  0x0178  94: DMA14 
    EXTERN    DMA15_IRQHandler              ; 0x5F  0x017C  95: DMA15 
    EXTERN    USBDMA_IRQHandler             ; 0x60  0x0180  96: USB DMA 
    EXTERN    WDT2_IRQHandler               ; 0x61  0x0184  97: Watchdog Timer 2 
    EXTERN    ECC_IRQHandler                ; 0x62  0x0188  98: Error Correction 
    EXTERN    DVS_IRQHandler                ; 0x63  0x018C  99: DVS Controller 
    EXTERN    SIMO_IRQHandler               ; 0x64 0x0190  100: SIMO Controller 
    EXTERN    SCA_IRQHandler                ; 0x65  0x0194  101: RPU  ; @TODO: Is this correct? 
    EXTERN    AUDIO_IRQHandler              ; 0x66  0x0198  102: Audio subsystem 
    EXTERN    FLC1_IRQHandler               ; 0x67  0x019C  103: Flash Control 1 
    EXTERN    RSV88_IRQHandler              ; 0x68  0x01A0  104: UART 3 
    EXTERN    RSV89_IRQHandler              ; 0x69  0x01A4  105: UART 4 
    EXTERN    RSV90_IRQHandler              ; 0x6A  0x01A8  106: UART 5 
    EXTERN    RSV91_IRQHandler              ; 0x6B  0x01AC  107: Camera IF 
    EXTERN    RSV92_IRQHandler              ; 0x6C  0x01B0  108: I3C  
    EXTERN    HTMR0_IRQHandler              ; 0x6D  0x01B4  109: HTmr 
    EXTERN    HTMR1_IRQHandler              ; 0x6E  0x01B8  109: HTmr 
    END

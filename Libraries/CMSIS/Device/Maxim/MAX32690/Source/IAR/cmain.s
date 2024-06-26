/**************************************************
 *
 * Part two of the system initialization code, contains C-level
 * initialization, thumb-2 only variant.
 *
 * Copyright 2006 IAR Systems. All rights reserved.
 *
 * Portions Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 **************************************************/


; --------------------------------------------------
; Module ?cmain, C-level initialization.
;


        SECTION SHT$$PREINIT_ARRAY:CONST:NOROOT(2)
        SECTION SHT$$INIT_ARRAY:CONST:NOROOT(2)

        SECTION .text:CODE:NOROOT(2)

        PUBLIC  __cmain
        ;; Keep ?main for legacy reasons, it is accessed in countless instances of cstartup.s around the world...
        PUBLIC  ?main
        EXTWEAK __iar_data_init3
        EXTWEAK __iar_argc_argv
        EXTWEAK __iar_data_init_done
        EXTERN  __low_level_init
        EXTERN  __call_ctors
        EXTERN  SystemInit
        EXTERN  main
        EXTERN  exit

        THUMB
__cmain:
?main:

; Initialize segments.
; __segment_init and __low_level_init are assumed to use the same
; instruction set and to be reachable by BL from the ICODE segment
; (it is safest to link them in segment ICODE).

        FUNCALL __cmain, __low_level_init
        bl      __low_level_init
        cmp     r0,#0
        beq     ?l1

        FUNCALL __cmain, __iar_data_init3
        bl      __iar_data_init3

?l1:
        REQUIRE ?l3

        SECTION .text:CODE:NOROOT(2)

        PUBLIC  _main
        PUBLIC  _call_main
        THUMB

__iar_init$$done:                       ; Copy initialization is done

?l3:
_call_main:
        BL      __iar_data_init_done

        MOVS    r0,#0                   ; No parameters
        FUNCALL __cmain, SystemInit
        BL      SystemInit

        MOVS    r0,#0                   ;  No parameters

        FUNCALL __cmain, __iar_argc_argv
        BL      __iar_argc_argv         ; Maybe setup command line

        FUNCALL __cmain, main
        BL      main
_main:
        FUNCALL __cmain, exit
        BL      exit

        END

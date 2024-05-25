# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system

# **********************************************************

BOARD=FTHR_Apps_P1

# Enable Cordio library
LIB_CORDIO = 1

# Enable zephyr layer
RTOS = zephyr

# Cordio library options
BLE_HOST = 0
BLE_CONTROLLER = 1

# TRACE option
# Set to 0 to disable
# Set to 2 to enable serial port trace messages
TRACE = 0
WSF_TRACE_ENABLED=0

# Use custom transport for controller
CHCI_TR = CHCI_TR_CUSTOM

# Use minimal buf pool size necessary for Controller-only no-uart build.
WSF_HEAP_SIZE=0x4310

# Optionally use zephyr toolchain.
# TOOL_DIR = /path/to/zephyr-sdk-0.16.1/arm-zephyr-eabi/bin
# ARM_PREFIX = arm-zephyr-eabi

# MFPU=none
# MFLOAT_ABI=soft

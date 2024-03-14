# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system

# **********************************************************

# Enable FreeRTOS library
LIB_FREERTOS = 1

# Un-comment this line to enable tickless mode, standby mode between events
PROJ_CFLAGS += -DUSE_TICKLESS_IDLE=1

# Enable CORDIO library
LIB_CORDIO = 1

# Cordio library options
RTOS = freertos
INIT_PERIPHERAL = 1
INIT_CENTRAL = 0

# TRACE option
# Set to 0 to disable
# Set to 1 to enable serial port trace messages
# Set to 2 to enable verbose messages
TRACE = 1

DEBUG = 1
################################################################################
 # Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
 #
 # Permission is hereby granted, free of charge, to any person obtaining a
 # copy of this software and associated documentation files (the "Software"),
 # to deal in the Software without restriction, including without limitation
 # the rights to use, copy, modify, merge, publish, distribute, sublicense,
 # and/or sell copies of the Software, and to permit persons to whom the
 # Software is furnished to do so, subject to the following conditions:
 #
 # The above copyright notice and this permission notice shall be included
 # in all copies or substantial portions of the Software.
 #
 # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 # IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 # OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 # ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 # OTHER DEALINGS IN THE SOFTWARE.
 #
 # Except as contained in this notice, the name of Maxim Integrated
 # Products, Inc. shall not be used except as stated in the Maxim Integrated
 # Products, Inc. Branding Policy.
 #
 # The mere transfer of this software does not imply any licenses
 # of trade secrets, proprietary technology, copyrights, patents,
 # trademarks, maskwork rights, or any other form of intellectual
 # property whatsoever. Maxim Integrated Products, Inc. retains all
 # ownership rights.
 #
 # $Date: 2019-12-20 13:40:14 -0600 (Fri, 20 Dec 2019) $ 
 # $Revision: 50175 $
 #
 ###############################################################################

# This is the name of the build output file
ifeq "$(PROJECT)" ""
PROJECT=max32665
endif

# Specify the target processor
ifeq "$(TARGET)" ""
TARGET=MAX32665
endif

# Create Target name variables
TARGET_UC:=$(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LC:=$(shell echo $(TARGET) | tr A-Z a-z)

# Select 'GCC' or 'IAR' compiler
COMPILER=GCC

# Specify the board used
ifeq "$(BOARD)" ""
BOARD=EvKit_V1
endif

ifeq "$(MAXIM_PATH)" ""
# MAXIM_PATH is still not defined...
DEPTH := ../../../
MAXIM_PATH := $(abspath $(DEPTH))
$(warning Warning:  MAXIM_PATH is not set!  Set MAXIM_PATH in your environment or in project.mk to clear this warning.)
$(warning Warning:  Attempting to use $(MAXIM_PATH) calculated from relative path)
else
# Sanitize MAXIM_PATH for backslashes
MAXIM_PATH := $(subst \,/,$(MAXIM_PATH))
endif

# Final recalculation of LIBS_DIR/CMSIS_ROOT
LIBS_DIR := $(abspath $(MAXIM_PATH)/Libraries)
CMSIS_ROOT := $(LIBS_DIR)/CMSIS

# Source files for this test (add path to VPATH below)
SRCS  = main.c
SRCS += $(LIBS_DIR)/PeriphDrivers/Source/SEMA/sema_me14.c
SRCS += $(LIBS_DIR)/PeriphDrivers/Source/SYS/mxc_delay.c

ifeq ($(MAKECMDGOALS),sla)
SRCS += sla_header.c
endif
# Where to find source files for this test
VPATH=.
ifeq ($(MAKECMDGOALS),sla)
VPATH += $(SECURE_BOOT_TOOLS_DIR)/sla_header
endif
VPATH += $(LIBS_DIR)/MiscDrivers

# Where to find header files for this project
IPATH += .
IPATH += include
IPATH := $(IPATH)

# Enable assertion checking for development
PROJ_CFLAGS+=-DMXC_ASSERT_ENABLE 

VPATH += $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source
VPATH += $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/GCC

# Use this to enable or disable Core 1
# Enabling Core 1 reserves RAM for its stack
DUAL_CORE=1

# Set MXC_OPTIMIZE to override the default optimization level 
MXC_OPTIMIZE=-O1

# Point this variable to a linker file to override the default file
#LINKERFILE=$(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/GCC/$(TARGET_LC).ld
ifeq ($(MAKECMDGOALS),sla)
LINKERFILE=$(SECURE_BOOT_TOOLS_DIR)/linker/$(TARGET_LC)_sla.ld
endif
################################################################################
# Include external library makefiles here

# Include the BSP
BOARD_DIR=$(LIBS_DIR)/Boards/$(TARGET_UC)/$(BOARD)
include $(BOARD_DIR)/board.mk

# Include the peripheral driver
PERIPH_DRIVER_DIR=$(LIBS_DIR)/PeriphDrivers
include $(PERIPH_DRIVER_DIR)/$(TARGET_LC)_files.mk

IPATH += $(PERIPH_DRIVER_DIR)/Include/$(TARGET_UC)

include $(LIBS_DIR)/libs.mk

################################################################################
# Include the rules for building for this target. All other makefiles should be
# included before this one.
include $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/$(COMPILER)/$(TARGET_LC).mk



sla: all
	arm-none-eabi-objcopy $(BUILD_DIR)/$(PROJECT).elf -R .sig -O binary $(BUILD_DIR)/$(PROJECT).bin
	$(CA_SIGN_BUILD) $(BUILD_DIR)/$(PROJECT).bin $(TEST_KEY)
	arm-none-eabi-objcopy  $(BUILD_DIR)/$(PROJECT).elf --update-section .sig=$(BUILD_DIR)/$(PROJECT).bin.sig
	arm-none-eabi-objcopy $(BUILD_DIR)/$(PROJECT).elf -O binary $(BUILD_DIR)/$(PROJECT).bin.sbin
	$(BUILD_SESSION) $(BUILD_DIR)/$(PROJECT).bin.sbin scp_packets $(TEST_KEY)

libclean: 
	$(MAKE)  -f ${PERIPH_DRIVER_DIR}/periphdriver.mk clean.periph
	
clean: 
#	Extend the functionality of the "clean" recipe here

# The rule to clean out all the build products.
distclean: clean libclean
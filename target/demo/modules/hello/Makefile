# Copyright (c) 2015 Qualcomm Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.
# $ATH_LICENSE_TARGET_MK$
#
default: all

IMAGEDIR := $(SRC_IOE)/image
MODULEDIR := $(SRC_IOE)/module
DEMO_OUT = $(MODULEDIR)/hello.mout

COPTS := -g3 -Os -Wall -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls
DEFINES := \
	-DAR6K \
	-DAR6002 \
	-TARG:unaligned_loads=1 \
	-DIOT_BUILD_FLAG \
	-DAR6002_REV7 \
	-include $(INTERNALDIR)/include/fwconfig_AR6006.h \
	-Wno-return-type \
	-DATHOS \
	-DP2P_ENABLED \
	-DSWAT_WMICONFIG_P2P \
	-DAPP_P2P_SUPPORT \
	-DATH_TARGET \
        -DSWAT_WMICONFIG_MISC_EXT \
        -DSWAT_SSL \
        -DSWAT_I2C \
        -DSWAT_I2S \
        -DSWAT_WMICONFIG_SNTP \
        -DSWAT_OTA \
        -DENABLE_HTTP_CLIENT  \
        -DENABLE_HTTP_SERVER  \
        -DSWAT_DNS_ENABLED \
        -DHTTP_ENABLED \
        -DDNS_ENABLED \
        -DBRIDGE_ENABLED \
        -DCONFIG_HOSTLESS \
        -DSSL_ENABLED \
        -DSWAT_CRYPTO \
        -DSWAT_BENCH_RAW \
        -DENABLE_JSON \
        -DSSDP_ENABLED \
        -DENABLE_MODULE

ifeq ($(AR6002_REV7_VER), 5) # {
DEFINES += -DNUM_OF_VDEVS=2 \
		-DAR6002_REV75 
endif # }

ifeq ($(AR6002_REV7_VER), 6) # {
DEFINES += -DNUM_OF_VDEVS=2 \
		-DAR6002_REV76 
endif # }

ifeq ($(FPGA_FLAG),1) # {
DEFINES += -DFPGA
endif # }

INCLUDES := \
	-I$(FW)/include/AR6002/hw/include \
	-I. \
	-I$(FW)/include/qcom \
	-I$(FW)/include \
	-I$(FW)/include/AR6002/ \
	-I$(FW)/include/AR6K/ \
	-I$(THREADXDIR)/include/ \
	-I$(INTERNALDIR)/include \
	-I$(INTERNALDIR)/include/os \
	-I$(INTERNALDIR)/include/whal

EXTRA_CFLAGS := -DMODULE_NAME=\"$(subst .mout,,$(notdir $(DEMO_OUT)))\"

CFLAGS = $(COPTS) $(DEFINES) $(INCLUDES) $(EXTRA_CFLAGS)

CC = xt-xcc
AR = xt-ar
RM = rm -f
LD = xt-ld

CSRCS := $(wildcard *.c) 
OBJS := $(CSRCS:%.c=%.o)
DEPS := $(CSRCS:%.c=%.d)

MOD_OBJ := mod.info.o
LDFLAGS = -r -T$(IMAGEDIR)/rom.addrs.ld


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),mkdbg)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif


$(MOD_OBJ): $(MODULEDIR)/mod.info.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(DEMO_OUT): $(OBJS) $(MOD_OBJ)
	@mkdir -p $(MODULEDIR)
	$(LD) $(LDFLAGS) $^ -o $@ 

all: $(OBJS) $(DEMO_OUT) 

clean:
	$(foreach d, $(SUBDIRS), make -C $(d) clean;)
	$(RM) *.o
	$(RM) $(DEMO_OUT)


mkdbg:
	@echo pwd=`pwd`
	@echo TARGET = $(TARGET)
	@echo AR6002_REV = $(AR6002_REV)
	@echo ROMDIR = $(ROMDIR)
	@echo DEMO_OUT = $(DEMO_OUT)
	@echo SUBDIRS = $(SUBDIRS)
	@echo CSRCS = $(CSRCS)
	@echo OBJS = $(OBJS)
	@echo DEPS = $(DEPS)
	@echo LIBS = $(LIBS)
	@echo CC = $(CC) at `which $(CC)`
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)

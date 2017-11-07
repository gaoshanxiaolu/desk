# Copyright (c) 2014 Qualcomm Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.
# $ATH_LICENSE_TARGET_MK$
#
default: all

IMAGEDIR := $(SDK_ROOT)/image
DEMO_OUT = $(IMAGEDIR)/RubyANP.out
DEMO_LIB = $(LIBDIR)/libRubyANP.a

COPTS := -g3 -Os -Wall -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -ffunction-sections
ifeq ($(AR6002_REV7_VER), 6) # {
DEFINES := \
	-D__XTENSA \
	-DAR6K \
	-DAR6002 \
	-DAR6002_REV76 \
	-DIOT_BUILD_FLAG \
	-DAR6002_REV7 \
	-include $(INTERNALDIR)/include/fwconfig_AR6006.h \
	-Wno-return-type \
	-DATHOS \
	-DATH_TARGET \
	-DSWAT_SSL \
	-DSWAT_WMICONFIG_SNTP \
	-DSWAT_OTA \
	-DENABLE_HTTP_CLIENT  \
	-DENABLE_HTTP_SERVER  \
	-DSWAT_DNS_ENABLED  \
	-DHTTP_ENABLED \
	-DDNS_ENABLED \
	-DBRIDGE_ENABLED \
	-DCONFIG_HOSTLESS \
	-DSSL_ENABLED \
	-DSWAT_BENCH_RAW \
	-DSSDP_ENABLED \
	-D__time_t_defined \
        -DSDK_50
endif # }

INCLUDES := \
	-I$(FW)/include/AR6002/hw/include \
	-I. \
	-I$(FW)/include/qcom \
	-I$(FW)/include/threadx \
	-I$(FW)/include \
	-I$(FW)/include/AR6002/ \
	-I$(FW)/include/AR6K/ \
	-I$(THREADXDIR)/include/ \
	-I$(INTERNALDIR)/include \
	-I$(INTERNALDIR)/include/os \
	-I$(INTERNALDIR)/include/whal \
	-I../../Hardware \
	-I../../Bluetopia/include \
	-I../../Bluetopia/btpskrnl \
	-I../../Bluetopia/profiles/GATT/include \
	-I../../Bluetopia/profiles_gatt/ANS/include \
	-I../../Bluetopia/profiles_gatt/DIS/include \
	-I../../Bluetopia/profiles_gatt/GAPS/include

CFLAGS = $(COPTS) $(DEFINES) $(INCLUDES) $(WLAN_INCLUDES)

CC = xt-xcc
AR = xt-ar
RM = rm -f

CSRCS := app_start.c RubyANP.c ../../Hardware/HAL.c ../../Bluetopia/hcitrans/HCITRANS.c
OBJS := $(CSRCS:%.c=%.o)
DEPS := $(CSRCS:%.c=%.d)
LIBS := $(LIBDIR)/libRubyANP.a \
	$(LIBDIR)/libadc.a \
	$(LIBDIR)/libramcust_ecc.a \
	$(LIBDIR)/libserport.a \
	$(LIBDIR)/libuartserp.a \
	$(LIBDIR)/libconsole.a\
	$(LIBDIR)/libpatches_iot.a\
	$(LIBDIR)/libpatches.a \
	$(LIBDIR)/libhttppatches.a \
	$(LIBDIR)/libsntppatches.a \
	$(LIBDIR)/libsslpatches.a \
	$(LIBDIR)/libcryptopatches_stub.a \
	$(LIBDIR)/libwhalpatches.a \
	$(LIBDIR)/libssdp.a \
	$(LIBDIR)/libcustpatches.a \
	../../Bluetopia/lib/libBTPSVEND.a \
	../../Bluetopia/lib/libSS1BTPS.a \
	../../Bluetopia/lib/libBTPSKRNL.a \
	../../Bluetopia/profiles/GATT/lib/libSS1BTGAT.a \
	../../Bluetopia/profiles_gatt/ANS/lib/libSS1BTANS.a \
	../../Bluetopia/profiles_gatt/DIS/lib/libSS1BTDIS.a \
	../../Bluetopia/profiles_gatt/GAPS/lib/libSS1BTGAP.a


LDFLAGS = -g -nostdlib -Wl,-EL -Wl,--gc-sections \
	-L$(IMAGEDIR)  \
	-L.\
	-TRubyANP.ld \
	-u app_start \
	-Wl,-static \
	-Wl,--start-group \
	$(LIBS)	\
	-Wl,--end-group\
	$(IMAGEDIR)/rom.addrs.ld \
	-lgcc


ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),mkdbg)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

../../Bluetopia/lib/libBTPSKRNL.a:
	@make -C ../../Bluetopia/btpskrnl -fBTPSKRNL.mak

$(DEMO_LIB): $(OBJS)
	@mkdir -p $(LIBDIR)
	$(AR) ru $@ $^

$(DEMO_OUT): $(LIBS) $(LDS)
	@mkdir -p $(IMAGEDIR)
	$(CC) $(LDFLAGS) -o $@

all: $(OBJS) $(DEMO_LIB) $(DEMO_OUT)

clean:
	$(foreach d, $(SUBDIRS), make -C $(d) clean;)
	$(RM) *.o
	$(RM) $(DEMO_LIB)
	$(RM) $(DEMO_OUT)


mkdbg:
	@echo pwd=`pwd`
	@echo TARGET = $(TARGET)
	@echo AR6002_REV = $(AR6002_REV)
	@echo ROMDIR = $(ROMDIR)
	@echo DEMO_OUT = $(DEMO_OUT)
	@echo DEMO_LIB = $(DEMO_LIB)
	@echo SUBDIRS = $(SUBDIRS)
	@echo CSRCS = $(CSRCS)
	@echo OBJS = $(OBJS)
	@echo DEPS = $(DEPS)
	@echo LIBS = $(LIBS)
	@echo CC = $(CC) at `which $(CC)`
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)

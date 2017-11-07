# Copyright (c) 2014 Qualcomm Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.
# $ATH_LICENSE_TARGET_MK$
#
default: all
IMAGEDIR := $(SRC_IOE)/image
DEMO_OUT = $(IMAGEDIR)/aj_svclite.out
DEMO_LIB = $(LIBDIR)/libaj_svclite.a

COPTS := -g3 -Os -Wall -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -ffunction-sections
DEFINES := \
	-DAR6K \
	-DAR6002 \
	-TARG:unaligned_loads=1 \
	-DIOT_BUILD_FLAG \
	-DAR6002_REV7 \
	-include $(INTERNALDIR)/include/fwconfig_AR6006.h \
	-Wno-return-type \
	-DATHOS \
	-DATH_TARGET \
	-DAJ_CONFIGURE_WIFI_UPON_START \
	-DWIFI_SSID=\"AllJoyn\" \
	-DWIFI_PASSPHRASE=\"ajajajaj\"

ifeq ($(SDK_VERSION_IOE), $(filter $(SDK_VERSION_IOE), REV74 REV75 REV76 )) # {
DEFINES := $(DEFINES) -DAR6002_$(SDK_VERSION_IOE)
endif # SDK_VERSION_IOE 


# set the default value to EMPTY, which implies a non-debug build
AJ_DEBUG_RESTRICT?=EMPTY

ifeq ($(AJ_DEBUG_RESTRICT), EMPTY)
DEFINES += -DNDEBUG
else
# use the supplied value to control how much debug info is available
DEFINES += -DAJ_DEBUG_RESTRICT=$(AJ_DEBUG_RESTRICT)
endif

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
	-I$(INTERNALDIR)/include/whal \
	-I../../../../allseen/core/ajtcl/inc/ \
	-I../../../../allseen/core/ajtcl/external/sha2/ \
	-I../../../aj_qca4004/inc/ 


CFLAGS = $(COPTS) $(DEFINES) $(INCLUDES)

CC = xt-xcc
AR = xt-ar
RM = rm -f

CSRCS := $(wildcard *.c) 
OBJS := $(CSRCS:%.c=%.o)
DEPS := $(CSRCS:%.c=%.d)
LIBS := $(LIBDIR)/libaj_svclite.a \
     $(LIBDIR)/liballjoyn.a \
     $(LIBDIR)/libramcust.a \
     $(LIBDIR)/libadc.a \
     $(LIBDIR)/libserport.a \
     $(LIBDIR)/libuartserp.a \
     $(LIBDIR)/libconsole.a\
     $(LIBDIR)/libpatches_iot.a\
     $(LIBDIR)/libpatches.a \
     $(LIBDIR)/libdnspatches_stub.a \
     $(LIBDIR)/libhttppatches_stub.a \
     $(LIBDIR)/libipv6patches.a \
     $(LIBDIR)/libotapatches.a \
     $(LIBDIR)/libsntppatches_stub.a \
     $(LIBDIR)/libsslpatches_stub.a \
     $(LIBDIR)/libstrrclpatches_stub.a \
     $(LIBDIR)/libwhalpatches.a \
     $(LIBDIR)/libhost_dset_api.a \
     $(LIBDIR)/libqcomcrypto.a \
     $(LIBDIR)/libcryptopatches.a \
     $(LIBDIR)/libcrypto_mgmt.a \
     $(LIBDIR)/libcrypto_aes.a \
     $(LIBDIR)/libcrypto_chacha20_poly1305.a \
     $(LIBDIR)/libcrypto_des.a \
     $(LIBDIR)/libcrypto_hmacsha256.a \
     $(LIBDIR)/libcrypto_hmacsha384.a \
     $(LIBDIR)/libcrypto_hmacsha1.a \
     $(LIBDIR)/libcrypto_srp.a \
     $(LIBDIR)/libssdp.a \
     $(LIBDIR)/libstackghost_stub.a \
     $(LIBDIR)/libcustpatches.a \

LDFLAGS = -g -nostdlib -Wl,-EL -Wl,--gc-sections \
	-L$(IMAGEDIR)  \
	-L.\
	-Taj_svclite.7.6.ld \
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

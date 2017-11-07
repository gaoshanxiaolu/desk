# Copyright (c) 2016 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
default: all

IMAGEDIR := $(SRC_IOE)/image
DEMO_OUT = $(IMAGEDIR)/aws_demo.out
DEMO_LIB = $(LIBDIR)/libaws_sdk_shell.a

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
	-DQCA4010_SUPPORT \
	-DENABLE_AWS \
	-DSSDP_ENABLED \
	-DDIVERSITY_ENABLE


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
	-I$(INTERNALDIR)/include/whal \
	-I$(FW)/demo/sdk_shell \
	-I$(FW)/demo/aws/aws_iot_src/utils \
	-I$(FW)/demo/aws/aws_iot_src/shadow \
	-I$(FW)/demo/aws/sample_apps/shadow_sample \
	-I$(FW)/demo/ezxml


CFLAGS = $(COPTS) $(DEFINES) $(INCLUDES)

CC = xt-xcc
AR = xt-ar
RM = rm -f

CSRCS := $(wildcard ../sdk_shell/*.c) $(wildcard aws_iot_src/shadow/*.c) $(wildcard aws_iot_src/utils/*.c) $(wildcard sample_apps/shadow_sample/*.c)
OBJS := $(CSRCS:%.c=%.o)
DEPS := $(CSRCS:%.c=%.d)
LIBS :=	$(LIBDIR)/libaws_sdk_shell.a \
	$(LIBDIR)/libadc.a \
	$(LIBDIR)/libramcust.a \
	$(LIBDIR)/libserport.a \
	$(LIBDIR)/libuartserp.a \
	$(LIBDIR)/libconsole.a\
	$(LIBDIR)/libpatches_iot.a\
	$(LIBDIR)/libpatches.a \
	$(LIBDIR)/libhost_dset_api.a \
	$(LIBDIR)/libhttppatches.a \
	$(LIBDIR)/libsntppatches.a \
	$(LIBDIR)/libdnspatches.a \
	$(LIBDIR)/libotapatches.a \
	$(LIBDIR)/libstrrclpatches.a \
	$(LIBDIR)/libipv6patches.a \
	$(LIBDIR)/libsslpatches.a \
	$(LIBDIR)/libqcomcrypto.a \
	$(LIBDIR)/libcoap.a \
	$(LIBDIR)/libcryptopatches.a \
	$(LIBDIR)/libcrypto_mgmt.a \
	$(LIBDIR)/libcrypto_aes.a \
	$(LIBDIR)/libcrypto_chacha20_poly1305.a \
	$(LIBDIR)/libcrypto_des.a \
	$(LIBDIR)/libcrypto_srp.a \
	$(LIBDIR)/libcrypto_hmacsha384.a \
	$(LIBDIR)/libcrypto_hmacsha256.a \
	$(LIBDIR)/libcrypto_hmacsha1.a \
	$(LIBDIR)/libwhalpatches.a \
	$(LIBDIR)/libjson.a \
	$(LIBDIR)/lib_ezxml.a \
	$(LIBDIR)/libssdp.a \
	$(LIBDIR)/libbtcoex.a \
	$(LIBDIR)/libcustpatches.a \
	$(LIBDIR)/lib_mqtt_client.a \
	$(LIBDIR)/lib_mqtt_packet.a \
	$(LIBDIR)/lib_mqtt_platform.a \
	$(LIBDIR)/lib_mqtt_wrap.a \
	$(LIBDIR)/libstackghost_stub.a

LDFLAGS = -g -nostdlib -Wl,-EL -Wl,--gc-sections \
	-L$(IMAGEDIR)  \
	-L.\
	-Tsdk_shell.ld \
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
	$(RM) ../sdk_shell/*.o
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

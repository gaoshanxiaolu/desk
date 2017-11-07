# *****************************************************************************
# Copyright 2013-2014, Qualcomm Connected Experiences, Inc.
#
# *****************************************************************************

#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of images to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
ifndef PDIR

ALLSEEN_DIR = ../../allseen

GEN_LIBS = liballjoyn.a

CSRCS = \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_about.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_ardp.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_authentication.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_bufio.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_bus.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_cert.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_connect.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_crc16.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_creds.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_debug.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_disco.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_guid.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_helper.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_init.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_introspect.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_link_timeout.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_msg.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_nvram.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_std.c \
	$(ALLSEEN_DIR)/core/ajtcl/src/aj_util.c \
	$(ALLSEEN_DIR)/core/ajtcl/crypto/ecc/aj_crypto_ecc.c \
	$(ALLSEEN_DIR)/core/ajtcl/malloc/aj_malloc.c \
	../aj_qca4004/src/aj_malloc.c \
	../aj_qca4004/src/aj_net.c \
	../aj_qca4004/src/aj_peer_q.c \
	../aj_qca4004/src/aj_target_crypto.c \
	../aj_qca4004/src/aj_target_nvram.c \
	../aj_qca4004/src/aj_target_util.c \
	../aj_qca4004/src/aj_wifi_ctrl.c \
	../aj_qca4004/src/alljoyn.c

REMOVED_CSRCS = \

endif

# AllJoyn.js
SRCS_JS  = $(CSRCS_JS)
COBJS_JS = $(CSRCS_JS:.c=.o)
SOBJS_JS = $(SSRCS_JS:.S=.o)
OBJS_JS  = $(COBJS_JS) $(SOBJS_JS)
DOBJS_JS = $(DSRCS_JS:.c=.o)

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#
DEFINES += -DALIGNED_ACCESS_REQUIRED

# set the default value to EMPTY, which implies a non-debug build
AJ_DEBUG_RESTRICT?=EMPTY

ifeq ($(AJ_DEBUG_RESTRICT), EMPTY)
DEFINES += -DNDEBUG
else
# use the supplied value to control how much debug info is available
DEFINES += -DAJ_DEBUG_RESTRICT=$(AJ_DEBUG_RESTRICT)
endif

SRCS    = $(CSRCS) $(SSRCS)
COBJS   = $(CSRCS:.c=.o)
SOBJS   = $(SSRCS:.S=.o)
OBJS    = $(COBJS) $(SOBJS)
DOBJS   = $(DSRCS:.c=.o)

CCFLAGS := $(CCFLAGS)

CCFLAGS := $(CCFLAGS) -g3
CCFLAGS := $(CCFLAGS) -Os

ifeq ($(SDK_VERSION_IOE), $(filter $(SDK_VERSION_IOE), REV74 REV75 REV76 )) # {
CCFLAGS := $(CCFLAGS) -DAR6002_$(SDK_VERSION_IOE)
endif # SDK_VERSION_IOE

DEF_FDLIB=_IEEE_
DEFINES +=  -D__fdlib_version=$(DEF_FDLIB)
DEFINES +=  -D_POSIX_SOURCE
DEFINES +=  -D_BSDTYPES_DEFINED

CCFLAGS := $(CCFLAGS) -include fwconfig_AR6006.h -Wno-return-type
CCFLAGS := $(CCFLAGS) -fno-inline-functions -nostdlib -mlongcalls -ffunction-sections
CCFLAGS := $(CCFLAGS) $(DEFINES)

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

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
    -I$(ALLSEEN_DIR)/core/ajtcl/inc/ \
    -I$(ALLSEEN_DIR)/core/ajtcl/malloc/ \
    -I$(ALLSEEN_DIR)/core/ajtcl/external/sha2/ \
    -I../aj_qca4004/inc/

############################################################
# Create a subdirectory for AllJoyn objects
#

.PHONY: all clean clobber install

all: liballjoyn.a

liballjoyn.a: $(OBJS)
	xt-ar $(ARFLAGS) $@ $(OBJS)

clean:
	rm -f $(ALLSEEN_DIR)/core/ajtcl/src/*.o
	rm -f $(ALLSEEN_DIR)/core/ajtcl/malloc/*.o
	rm -f $(ALLSEEN_DIR)/core/ajtcl/external/sha2/*.o
	rm -f $(ALLSEEN_DIR)/core/ajtcl/crypto/ecc/*.o
	rm -f ../aj_qca4004/src/*.o
	rm -f liballjoyn.a

clobber: clean

install:
	rm -f ../../../../lib/liballjoyn.a
	cp liballjoyn.a ../../../../lib/liballjoyn.a

%.o: %.c
	xt-xcc $(CCFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.S
	xt-xcc $(ASFLAGS) $(INCLUDES) -c -o $@ $<

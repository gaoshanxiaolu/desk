# Copyright (c) 2013-2015 Qualcomm Atheros, Inc..
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.
#
#
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

GEN_LIBS = liballjoyn_services.a
CSRCS = $(wildcard ../../../../../../../allseen/services/base_tcl/config/src/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/Common/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/Widgets/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/notification/src/NotificationCommon/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/notification/src/NotificationProducer/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/onboarding/src/*.c)
CSRCS += $(wildcard ../../../../../../../allseen/services/base_tcl/services_common/src/*.c)

endif

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#
DEFINES += -DCONFIG_SERVICE -DONBOARDING_SERVICE -DCONTROLPANEL_SERVICE -DNOTIFICATION_SERVICE_PRODUCER -DEVENTS_AND_ACTIONS
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
ifeq ($(AR6002_REV), 7) # {
CCFLAGS := $(CCFLAGS) -DATH_TARGET -DAR6K -DAR6002 -DAR6002_REV7
ifeq ($(SDK_VERSION_IOE), $(filter $(SDK_VERSION_IOE), REV74 REV75 REV76 )) # {
DEFINES := $(DEFINES) -DAR6002_$(SDK_VERSION_IOE)
endif # SDK_VERSION_IOE
endif # AR6002_REV7 }

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

INCLUDES := $(INCLUDES) \
	-I$(FW)/include/qcom \
	-I$(FW)/include \
	-I$(FW)/include/AR6002/ \
	-I$(FW)/include/AR6K/ \
	-I$(THREADXDIR)/include/ \
	-I$(INTERNALDIR)/include \
	-I$(INTERNALDIR)/include/os \
	-I$(INTERNALDIR)/include/whal \
			-I ../../../../../../../allseen/core/ajtcl/inc \
			-I ../../../../../../../allseen/core/ajtcl/malloc \
			-I ../../../../../../../allseen/core/ajtcl/external/sha2 \
			-I ../../../../../../../alljoyn/aj_qca4004/inc \


INCLUDES := $(INCLUDES) -I . -I $(PDIR)include 

INCLUDES := $(INCLUDES) -I../../../../../../../allseen/services/base_tcl/config/inc
INCLUDES := $(INCLUDES) -I../../../../../../../allseen/services/base_tcl/controlpanel/inc
INCLUDES := $(INCLUDES) -I../../../../../../../allseen/services/base_tcl/notification/inc
INCLUDES := $(INCLUDES) -I../../../../../../../allseen/services/base_tcl/onboarding/inc
INCLUDES := $(INCLUDES) -I../../../../../../../allseen/services/base_tcl/services_common/inc


############################################################
# Create a subdirectory for AllJoyn objects 
#

.PHONY: all clean clobber install

all: liballjoyn_services.a

liballjoyn_services.a: $(OBJS)
	xt-ar $(ARFLAGS) $@ $(OBJS)

clean: 
	rm -f ../../../../../../../allseen/services/base_tcl/config/src/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/Common/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/controlpanel/src/CPSControllee/Widgets/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/notification/src/NotificationCommon/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/notification/src/NotificationProducer/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/onboarding/src/*.o
	rm -f ../../../../../../../allseen/services/base_tcl/services_common/src/*.o
	rm -f liballjoyn_services.a

clobber: clean

install:
	rm -f                    ../../../../../../../../../lib/liballjoyn_services.a
	cp liballjoyn_services.a ../../../../../../../../../lib/liballjoyn_services.a


%.o: %.c
	xt-xcc $(CCFLAGS) $(INCLUDES) -c $< -o $@ 



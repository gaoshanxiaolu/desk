
ifndef CC
   CC = gcc
endif

ifndef AR
   AR = ar
endif

ifndef GLOBCFLAGS
   GLOBCFLAGS = -D__XTENSA -DATH_TARGET -DAR6K -DAR6002 -DAR6002_REV7 -DAR6002_REV76 -Os -Wall -Wpointer-arith -Wundef -Wl,-EL -fno-inline-functions -nostdlib -ffunction-sections -mlongcalls -LNO:aligned_pointers=on -Wno-return-type
endif

INCLDDIRS = -I../include                    \
	    -I$(SDK_ROOT)/include                \
	    -I$(SDK_ROOT)/include/threadx
	    
PROJECT = ../lib/libBTPSKRNL.a

CFLAGS = $(DEFS) $(INCLDDIRS) $(GLOBINCLDDIRS) $(GLOBCFLAGS) -fno-strict-aliasing -DLIBCUST_INDIRECT_ACCESS

LDLIBS = -lpthread

OBJS = BTPSKRNL.o sprintf.o

.PHONY:
all: $(PROJECT)

$(PROJECT): $(OBJS)
	$(AR) r $@ $?

.PHONY: clean veryclean realclean
clean veryclean realclean:
	-rm -f *.o
	-rm -f *~
	-rm -f $(PROJECT)


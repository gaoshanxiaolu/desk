
ifndef CC
   CC = gcc
endif

ifndef AR
   AR = ar
endif

INCLDDIRS = -I../include                     \
	    -I../../../include               \
	    -I../../../profiles/GATT/include

ifndef SYSTEMLIBS
   SYSTEMLIBS = -lpthread -lm
endif

PROJECT = ../lib/libSS1BTRTU.a

CFLAGS = $(DEFS) $(INCLDDIRS) $(GLOBINCLDDIRS) $(GLOBCFLAGS) -fno-strict-aliasing

LDFLAGS = -L../../../lib -L../../../profiles/GATT/lib $(GLOBLDFLAGS)

LDLIBS = -lSS1BTPS -lSS1BTGAT $(SYSTEMLIBS) $(GLOBLDLIBS)

OBJS = RTUS.o

.PHONY:
all: $(PROJECT)

$(PROJECT): $(OBJS)
	$(AR) r $@ $?

.PHONY: clean veryclean realclean
clean veryclean realclean:
	-rm -f *.o
	-rm -f *~
	-rm -f $(PROJECT)


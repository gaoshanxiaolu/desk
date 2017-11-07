
ifndef CC
   CC = gcc
endif

ifndef AR
   AR = ar
endif

ifdef ENABLE_ENVIRONMENT_CONFIG
   ENV_CONFIG=-DENABLE_ENVIRONMENT_CONFIG
endif

INCLDDIRS = -I../include

PROJECT = ../lib/libBTPSVEND.a

CFLAGS = $(DEFS) $(ENV_CONFIG) $(INCLDDIRS) $(GLOBINCLDDIRS) $(GLOBCFLAGS) -fno-strict-aliasing

LDLIBS = -lpthread

OBJS = BTPSVEND.o

.PHONY:
all: $(PROJECT)

$(PROJECT): $(OBJS)
	$(AR) r $@ $?

.PHONY: clean veryclean realclean
clean veryclean realclean:
	-rm -f *.o
	-rm -f *~
	-rm -f $(PROJECT)


#
# GNU Makefile
#

VPATH	= .:../common:../..
DEST	= xasm68k
CC	= vc

CPU ?= 68060
FPU ?= 68060

#CFLAGS = -O0 $(COPT) $(DEBUG)

LKFLAGS = -lmieee

ifeq ($(FPU),68060)
LKFLAGS = -lm040
endif

ifeq ($(FPU),68040)
LKFLAGS = -lm040
endif

ifeq ($(FPU),68881)
LKFLAGS = -lm881
endif

COPT	= -c -c99 -I/r68kasm -maxoptpasses=15 -size

ifeq ($(RELEASE),1)
CFLAGS	:= $(COPT) -O3 -DNDEBUG
else
CFLAGS	= $(COPT) -hunkdebug -g -O0
LKFLAGS := $(LKFLAGS) -hunkdebug -g
endif

ifeq ($(FPU),0)
CFLAGS	:= $(CFLAGS) -cpu=$(CPU)
else
CFLAGS	:= $(CFLAGS) -cpu=$(CPU) -fpu=$(FPU)
endif

OBJS	= amigaobj.o binobj.o fstack.o globlex.o lexer.o main.o object.o options.o parse.o patch.o project.o section.o symbol.o\
	  strings.o amitime.o\
	  locerror.o loclexer.o locopt.o locparse.o locsym.o
AOBJS	=

.PHONY: all clean

all:	$(DEST)

$(DEST): $(OBJS) $(AOBJS)
	vc $(DEBUG) $(LKFLAGS) -lamiga -o $@ $(subst ../,/,$+)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(subst ../,/,$<) -o $@

$(AOBJS): %.o: %.S
	-Delete $@ QUIET
	$(AS) $(ASFLAGS) $(subst ../,/,$<)
	Rename $(patsubst %.S,%.o,$(subst ../,/,$<)) $@

clean:
	-Delete $(OBJS) $(AOBJS) $(DEST)

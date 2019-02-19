SRCS = irq.asm utility.asm
ASMFLAGS = -mcg -iinclude/ -z0

ASMOTOR = ../../../build/cmake/debug/xasm/z80/motorz80
XLIB = ../../../build/cmake/debug/xlib/xlib

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -d$(DEPDIR)/$*.Td

ASSEMBLE = $(ASMOTOR) $(DEPFLAGS) $(ASMFLAGS)
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

mos.lib : $(SRCS:asm=obj)
	$(XLIB) $@ a $+

%.obj : %.asm
%.obj : %.asm $(DEPDIR)/%.d
	$(ASSEMBLE) -o$@ $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

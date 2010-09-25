#
# Makefile to be used with wmake.exe
#

.SILENT
.EXTENSIONS:
.EXTENSIONS:	.lib	.obj	.asm

ASMOPT	= -iinclude\
ASM	= c:\code\asmotor\asm\gameboy\rgbasm.exe $(ASMOPT)

LIB	= c:\code\asmotor\lib\xlib.exe

EXE	= mos.lib

OBJDIR	= objs\
OBJS	= irq.obj	utility.obj	never.obj

.obj:	$(OBJDIR)
.lib:	.
.asm:	.

$(EXE):		$(OBJS)
		echo Making $(EXE)
		cd objs
		$(LIB) $(EXE) a $(OBJS)
		move >NUL $(EXE) ..\$(EXE)
		cd ..

.asm.obj:
		$(ASM) -o$(OBJDIR)$^&.obj $[*.asm

clean:		.SYMBOLIC
		echo Cleaning up
		del >NUL $(OBJDIR)*.obj $(OBJDIR)*.map $(EXE) *.obj *.map
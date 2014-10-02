#
# Makefile to be used with wmake.exe
#

.SILENT
.EXTENSIONS:
.EXTENSIONS:	.gb	.obj	.asm

LNKOPT	= -m..\mapfile.map -z0 -sMain linkfile.lnk
LNK	= \watcom10\binw\wd /tr=rsi c:\code\asmotor\link\xlink.exe $(LNKOPT)
#LNK	= c:\code\asmotor\link\xlink.exe $(LNKOPT)

LIBDIR	= c:\code\asmotor\examples\minios
LIBINC  = $(LIBDIR)\INCLUDE\
LIB     = $(LIBDIR)\MOS.LIB

ASMOPT	= -i$(LIBINC) -z0
ASM	= c:\code\asmotor\asm\gameboy\rgbasm.exe $(ASMOPT)

EXE	= gbfade.gb

OBJDIR	= objs\
OBJS	= main.obj	user.obj	pics.obj

.obj:	$(OBJDIR)
.gb:	.
.asm:	.

$(EXE):		$(OBJS)
		echo Linking $(EXE)
		cd objs
		$(LNK)
		move >NUL main.gb ..\$(EXE)
		cd ..
		c:\code\asmotor\rgbfix\rgbfix -tGBFADE -p -v $(EXE)

.asm.obj:
		$(ASM) -o$(OBJDIR)$^&.obj $[*.asm

clean:		.SYMBOLIC
		echo Cleaning up
		del >NUL $(OBJDIR)*.obj $(OBJDIR)*.map $(EXE) *.obj *.map
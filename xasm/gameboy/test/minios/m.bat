set MYASM=..\..\asm\gameboy\rgbasm.exe -iinclude\ -z0
set MYLIB=..\..\..\lib\xlib.exe

%MYASM% -oobjs\irq.obj irq.asm
%MYASM% -oobjs\utility.obj utility.asm

cd objs
%MYLIB% mos.lib a irq.obj utility.obj
move >NUL mos.lib ..\mos.lib
cd ..

set MYASM=
set MYLIB=
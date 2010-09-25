set MYASM=..\..\asm\gameboy\rgbasm.exe -ic:\code\asmotor\examples\minios\include\
set MYLINK=..\..\link\xlink.exe
set MYFIX=..\..\rgbfix\rgbfix.exe

%MYASM% -oobjs\main.obj main.asm
%MYASM% -oobjs\user.obj user.asm
%MYASM% -oobjs\pics.obj pics.asm

cd objs
..\%MYLINK% -mmap.map -nsym.sym linkfile.lnk
move >NUL main.gb ..\gbfade.gb
cd ..
%MYFIX% -tGBFADE -p -v gbfade.gb

set MYASM=
set MYLINK=
set MYFIX=
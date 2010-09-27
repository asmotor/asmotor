set MYASM=..\..\..\..\Win32Release\motorgb.exe -i..\minios\include\
set MYLINK=..\..\..\..\Win32Release\motorlink.exe
set MYFIX=..\..\..\..\Win32Release\motorgbfix.exe

%MYASM% -omain.obj main.asm
%MYASM% -ouser.obj user.asm
%MYASM% -opics.obj pics.asm

%MYLINK% -mmap.map -tg -ogbfade.gb main.obj user.obj pics.obj ..\minios\mos.lib
%MYFIX% -tGBFADE -p -v gbfade.gb

set MYASM=
set MYLINK=
set MYFIX=
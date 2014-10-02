set MYASM=..\..\..\..\build\Win32\Release\motorgb.exe -i..\minios\include\
set MYLINK=..\..\..\..\build\Win32\Release\motorlink.exe
set MYFIX=..\..\..\..\build\Win32\Release\motorgbfix.exe

%MYASM% -omain.obj main.asm
%MYASM% -ouser.obj user.asm
%MYASM% -opics.obj pics.asm

%MYLINK% -mmap.map -tg -ogbfade.gb main.obj user.obj pics.obj ..\minios\mos.lib
%MYFIX% -tGBFADE -p -v gbfade.gb

set MYASM=
set MYLINK=
set MYFIX=
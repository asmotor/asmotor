set MYASM=..\..\..\..\build\Win32\Release\motorgb.exe -iinclude\ -z0
set MYLIB=..\..\..\..\build\Win32\Release\motorlib.exe

%MYASM% -oirq.obj irq.asm
%MYASM% -outility.obj utility.asm

%MYLIB% mos.lib a irq.obj utility.obj

set MYASM=
set MYLIB=
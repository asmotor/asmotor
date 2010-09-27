set MYASM=..\..\..\..\Win32Release\motorgb.exe -iinclude\ -z0
set MYLIB=..\..\..\..\Win32Release\motorlib.exe

%MYASM% -oirq.obj irq.asm
%MYASM% -outility.obj utility.asm

%MYLIB% mos.lib a irq.obj utility.obj

set MYASM=
set MYLIB=
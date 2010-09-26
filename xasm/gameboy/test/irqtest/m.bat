..\..\..\..\Win32Release\motorgb -oirqtest.obj -i..\minios\include\ irqtest.asm
..\..\..\..\Win32Release\motorlink -ogbirq.gb -tg irqtest.obj
..\..\..\..\Win32Release\motorgbfix -p -v gbirq.gb
..\..\Win32Release\motorgb -oirqtest.obj -i..\minios\include\ irqtest.asm
..\..\..\..\xlink\Win32Release\motorlink -@ irqtest.lk
..\rgbfix\rgbfix -v gbirq.gb
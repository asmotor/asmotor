VDP_RGB		MACRO
		DB	(\3<<4)|(\2<<2)|(\1)
		ENDM

        SECTION "Data",DATA
        VDP_RGB 0,0,0
        VDP_RGB 1,2,3
        
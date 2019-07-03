VDP_RGB		MACRO
		DB	(\3<<4)|((\2)<<2)|(\1)
		ENDM

        SECTION "Test",HOME[$0]
        ld  hl,{ DB "Test",0 }
		ld	hl,{VDP_RGB 1,2,3}

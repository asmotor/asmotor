; --
; -- All the pictures are in seperate sections so we can fit them into an
; -- image. gbi_Show is in the HOME page so no need for lcall
; --
; -- There is no reason why this couldn't be in the USER.Z80 file but I
; -- happen to like very modular projects and small sourcefiles
; --

	INCLUDE	"utility.i"




	SECTION	"Pic1",CODE
Picture1::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\airbrush.gbi"


	SECTION	"Pic2",CODE
Picture2::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\ballerin.gbi"


	SECTION	"Pic3",CODE
Picture3::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\blitz.gbi"


    	SECTION	"Pic4",CODE
Picture4::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\blues.gbi"


	SECTION	"Pic5",CODE
Picture5::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\borninan.gbi"


	SECTION	"Pic6",CODE
Picture6::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\compo-tg.gbi"


	SECTION	"Pic7",CODE
Picture7::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\defeatof.gbi"


	SECTION	"Pic8",CODE
Picture8::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\drill.gbi"


	SECTION	"Pic9",CODE
Picture9::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\frame_id.gbi"


	SECTION	"Pic10",CODE
Picture10::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\frankie.gbi"


	SECTION	"Pic11",CODE
Picture11::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\hidden_f.gbi"


	SECTION	"Pic12",CODE
Picture12::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\inder.gbi"


	SECTION	"Pic13",CODE
Picture13::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\isolatio.gbi"


	SECTION	"Pic14",CODE
Picture14::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\jaggu_he.gbi"


	SECTION	"Pic15",CODE
Picture15::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\joachim.gbi"


	SECTION	"Pic16",CODE
Picture16::	ld	hl,.picture
	call	gbi_Show
	ret
.picture	INCBIN	"pics\\leguana_.gbi"

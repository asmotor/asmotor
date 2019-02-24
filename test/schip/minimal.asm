	SECTION	"Code",CODE[$200]

; ---- minimal chip8 game ---
;OPTION BINARY
;ALIGN OFF  

; V0-2 	scrap
; V3 	player x
; V4 	player y
; V5	enemy x
; V6	enemy y
; VF    collision detection

; keys:
; #2 = up
; #4 = left
; #5 = space
; #6 = right
; #8 = down
; #A = Ctrl
; #B = shift
; #C = Z
; #F = Space


;HIGH		; Uncomment this for Hires Superchip mode (128x64)

GAMEINIT:

  CLS		; clear screen
  CALL INITPLAYER ; init player 
  CALL INITENEMY  ; init enemy 

  CALL DRAWPLAYER ; draw player on startpos
  CALL DRAWENEMY  ; draw enemy on startpos


; ================== game loop ================
GAMELOOP:	
	
  CALL DRAWPLAYER   ; draw xor player (clear last pos)
  CALL HANDLEINPUT  ; move player	
  CALL DRAWPLAYER   ; draw player on new pos

  CALL DRAWENEMY
  CALL MOVEENEMY
  CALL DRAWENEMY  

  ; check collision, init player upon collision
  SNE VF,1
  JP GAMEINIT

  LD V0,1
  LD DT,V0
  CALL WAITDELAY
  

JP GAMELOOP

; ================== functions ================
INITPLAYER:
  LD V3,32
  LD V4,25
  RET

; drawplayer  (uses: v3,v4,I)
DRAWPLAYER:
  LD I,SPR_DUDE	   ;Set I to SPR_DUDE adress
  DRW V3,V4,6
  RET

; handle input (uses v0)
HANDLEINPUT:
  ; check down
  LD V0,8	;Set V0 to Key
  SKP V0	;Check Key (skip next instruction if pressed)
  JP checkup    ;Skip
  ADD V4,1	;y++

checkup:
  LD V0,2	;Set V0 to Key
  SKP V0	;Check Key
  JP checkleft	;skip to next check
  ADD V4,-1	;y--

checkleft:
  LD V0,4	;Set V0 to Key
  SKP V0	;Check Key
  JP checkright	;skip to next check
  ADD V3,-1	;x--

checkright:
  LD V0,6	;Set V0 to Key
  SKP V0	;Check Key
  JP checkdone	;skip to next check
  ADD V3,1	;x++

checkdone:
  RET

INITENEMY:
  LD V5,0
  RND V6,15
  RET

DRAWENEMY:
  LD I,SPR_ENEMY	   ;Set I to SPR_ENEMY adress
  DRW V5,V6,5
  RET

MOVEENEMY:
  ADD V5,1

  ; compare V1>60
  LD V0,V5	
  LD V1,60
  SUB V0,V1
  SNE VF,1	
  CALL INITENEMY
  RET

WAITDELAY:
  LD V0,DT	   ;Set V0 to DelayTimer
  SE V0,0	   ;Check zero
  JP WAITDELAY     
RET  

; ================== graphics ================
SPR_DUDE:
 DB %00111100
 DB %00011000
 DB %11111111
 DB %00011000
 DB %00100100
 DB %11100111

SPR_ENEMY:
 DB %01111110
 DB %11111111
 DB %10011001
 DB %11100111
 DB %00111100

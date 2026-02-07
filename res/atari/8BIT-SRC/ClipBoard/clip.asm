; =================================================================
; AspeQt-2k26 ClipboardDevice (Y:) - BARE METAL
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; ORIGIN: $2000
; =================================================================
;
 		icl "clip_sym.asm"
 		org $4000
 		
 	    .align 256    
 		
IOBuf .ds 128
null  .byte 0
EOF   .byte $FF
    
.proc  Start
	jsr printf
	.byte 'PC Clipboad Y: ',155,0
    jsr OpenIt
nextRead
    bcs exit    
    jsr ReadIt
    lda EOF
    bne nextRead
exit        
    rts   
.endp	
	
	
.proc init
      rts
.endp  	

.proc OpenIt
	jsr SetupDCB_Open
	bpl OK1	
	jsr Printf
	.byte 155,'No server response!',155,0
	sec
	rts
OK1
	jsr Printf
	.byte 155,'Clip Loaded!',155,0
	clc	
	rts
.endp
		

.proc ReadIt
   jsr SetupDCB_Read
   bpl OK2
   jsr Printf
   .byte 155,'No server response!',0
   lda #$00
   sta EOF
   sec
   rts
OK2
   lda IOBuf
   bne NOTEOF
   sta EOF
NOTEOF
   lda #0
   sta IOBuf+128
   jsr Printf
   .byte '%s',0
   .word IOBuf
   clc
   rts	
.endp
  
 
; =================================================================
; SIO SETUP (Matches clip.asm exactly)
; =================================================================
SetupDCB_Open
    lda #$4F        ; Cmd 'O'
    sta DCOMND
    lda #$00        
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$06        ; Timeout $06
    sta DTIMLO
    lda #$00
    sta DUNUSE
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    lda #$59        ; Device 'Y'
    sta DDEVIC
    lda #1
    sta DUNIT
    jsr SIOV
    rts

SetupDCB_Read
    lda #$52        ; Cmd 'R'
    sta DCOMND
    lda #$40        ; Read
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$08        ; Timeout $08
    sta DTIMLO
    lda #$00
    sta DUNUSE
    lda #$80        ; Length 128
    sta DBYTLO
    lda #$00
    sta DBYTHI
    lda #$00        
    sta DAUX1
    sta DAUX2
    lda #$59        ; Device 'Y'
    sta DDEVIC
    lda #1
    sta DUNIT
    jsr SIOV
    rts

    icl 'printf.asm'
     
	 run Start
	
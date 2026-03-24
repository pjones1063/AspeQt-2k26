; =========================================================
; Atari 850 Driver - 
; Assembler: MADS
; =========================================================
    icl "sym.asm"
    
	
    org $3800

start
    lda #$50
    sta DDEVIC
    lda #$01
    sta DUNIT
    lda #$3F
    sta DCOMND
    lda #$40
    sta DSTATS
    lda #$05
    sta DTIMLO
    sta DBUFHI      ; Target Page = $0500
    lda #$00
    sta DBUFLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    lda #$0C
    sta DBYTLO      ; Request exactly 12 bytes
    jsr SIOV

    bpl copy_dcb
    rts

copy_dcb
    ldx #$0B
loop_dcb
    lda $0500,x     ; Read the 12 bytes sent by 850
    sta $0300,x     ; Overwrite the live OS SIO variables
    dex
    bpl loop_dcb

    jsr SIOV        ; Second SIO call uses the injected DCB!
    bmi exit
    
    jsr $0506       ; Execute 850 init routine
    jmp (DOSINI)    ; <--- THE FIX: Warm-start DOS! ($000C)

exit
    rts

    org $02E2
    .word start
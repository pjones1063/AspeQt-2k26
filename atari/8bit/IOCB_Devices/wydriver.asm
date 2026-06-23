; ==================================================================
; AspeQt Triple Device Handler (Y: , W: , A:) 
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler) - Atari Dos 2.x
; MEMORY: Code at $4500
; =================================================================

    icl "sym.asm"

; =================================================================
; RESIDENT CODE
; =================================================================
    org $4500
    
    .align 256 

; --- VARIABLES ---
IOBuf       .ds 256 
BufPtr      .byte 0 
EOF_Flag    .byte 0 
SaveX       .byte 0
CurrentDev  .byte 0
CurrentMode .byte 0  
TransMode   .byte 0  
OldDOSINI   .word 0
DeviceID    .byte 0
TableLo     .byte 0
TableHi     .byte 0

; =================================================================
; JUMP TABLES
; =================================================================

TableY
    .word HandlerOpenY-1
    .word HandlerClose-1
    .word HandlerGet-1
    .word HandlerPut-1
    .word HandlerStat-1
    .word HandlerSpec-1

TableW
    .word HandlerOpenW-1
    .word HandlerClose-1
    .word HandlerGet-1
    .word HandlerPut-1
    .word HandlerStat-1
    .word HandlerSpec-1

TableA
    .word HandlerOpenA-1
    .word HandlerClose-1
    .word HandlerGet-1
    .word HandlerPut-1
    .word HandlerStat-1
    .word HandlerSpec-1

; =================================================================
; DEVICE SPECIFIC OPEN ROUTINES
; =================================================================
HandlerOpenY
    lda #$59
    sta CurrentDev
    jsr SetupDCB_Open
    rts

HandlerOpenW
    lda #$57
    sta CurrentDev
    jsr SetupDCB_Open
    rts

HandlerOpenA
    lda #$41        ; Hardcode Device ID to 'A' ($41)
    sta CurrentDev
    jsr SetupDCB_Open
    rts

; =================================================================
; 2. SHARED HANDLER ROUTINES 
; =================================================================
HandlerClose
    jsr SetupDCB_Open  ; CLOSE is basically an OPEN DCB with Cmd $43
    lda #$43
    sta DCOMND
    jsr SIOV
    lda #1
    ldy #1
    rts

HandlerGet
    lda #1
    ldy #1
    rts

HandlerPut
    stx SaveX
    pha
    jsr SetupDCB_Write
    pla
    sta IOBuf
    jsr SIOV
    ldx SaveX
    lda #1
    ldy #1
    rts

HandlerStat
    lda #1
    ldy #1
    rts

HandlerSpec
    lda #1
    ldy #1
    rts

; =================================================================
; 3. SIO SETUP SUBROUTINES
; =================================================================
SetupDCB_Open
    lda CurrentDev
    sta DDEVIC
    lda #$01
    sta DUNIT
    lda #$4F        ; Cmd 'O'
    sta DCOMND
    lda #$00
    sta DSTATS      ; No data xfer
    lda #$03        ; Fast Timeout
    sta DTIMLO
    
    ; Protect AUX bytes for Y: and W: modes
    lda ICBLL,x
    sta DAUX1
    lda ICBLH,x
    sta DAUX2
    
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$00
    sta DBYTLO
    sta DBYTHI
    rts

SetupDCB_Read
    jsr SetupDCB_Open
    lda #$52        ; Cmd 'R'
    sta DCOMND
    lda #$40        ; Read
    sta DSTATS
    lda #$04        ; Fast Timeout
    sta DTIMLO
    jsr SIOV
    rts

SetupDCB_Write
    jsr SetupDCB_Open
    lda #$57        ; Cmd 'W'
    sta DCOMND
    lda #$80        ; Write
    sta DSTATS
    lda #$0A        ; Write Timeout
    sta DTIMLO
    jsr SIOV
    rts

; =================================================================
; 5. INSTALLER
; =================================================================
OnReset
    lda #<EndHandler
    sta $02E7        
    lda #>EndHandler
    sta $02E8        
    jsr InitHandlersOnly 
    jmp (OldDOSINI)
    
Init
    jsr InitHandlersOnly
    lda #<EndHandler
    sta $02E7
    lda #>EndHandler
    sta $02E8
    
    lda $0C
    sta OldDOSINI
    lda $0D
    sta OldDOSINI+1
    
    lda #<OnReset
    sta $0C
    lda #>OnReset
    sta $0D
    rts

InitHandlersOnly
    ; Install Y ($59)
    lda #$59
    ldx #<TableY
    ldy #>TableY
    jsr InstallOne
    
    ; Install W ($57)
    lda #$57
    ldx #<TableW
    ldy #>TableW
    jsr InstallOne

    ; Install A ($41)
    lda #$41
    ldx #<TableA
    ldy #>TableA
    jsr InstallOne
    
    rts

InstallOne
    sta DeviceID
    stx TableLo
    sty TableHi
    ldx #0
FindSlot
    lda HATABS,x
    beq FoundEmpty
    cmp DeviceID
    beq FoundEmpty
    inx
    inx
    inx
    cpx #33
    bcc FindSlot
    rts
FoundEmpty
    lda DeviceID
    sta HATABS,x
    lda TableLo
    sta HATABS+1,x
    lda TableHi
    sta HATABS+2,x
    rts

EndHandler
    run Init
    
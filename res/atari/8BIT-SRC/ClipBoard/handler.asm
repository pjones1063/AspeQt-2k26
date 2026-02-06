; =================================================================
; AspeQt-2k26 Clipboard Handler (Y:) - GOLDEN MASTER (NULL-TERM)
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; MEMORY: Fixed at $4000
; =================================================================

    icl "clip_sym.asm"
    
    org $4000

; =================================================================
; RESIDENT DATA 
; =================================================================
    .align 256      
IOBuf       .ds 128 
BufPtr      .byte 0 
EOF_Flag    .byte 0 
TestB       .byte 'A'


; =================================================================
; CIO JUMP TABLE
; =================================================================
HandlerTable
    .word HandlerOpen
    .word HandlerClose
    .word HandlerGet
    .word HandlerPut
    .word HandlerStat
    .word HandlerSpec
    
; =================================================================
; 1. OPEN ROUTINE 
; =================================================================
HandlerOpen
    ; Send "OPEN" Command ($4F)
    jsr SetupDCB_Open
    bmi OpenFail    
    
    ; Reset Buffer State and EOF    
    ; We set BufPtr to 128 to force the *first* GET to trigger a Refill.
    lda #128
    sta BufPtr
    lda #0
    sta EOF_Flag
    
    ldy #1          ; Success
    clc             ; Clear Carry (Success)
    rts

OpenFail
    ldy #144        ; Error 144
    sec             ; Set Carry (Error)
    rts

; =================================================================
; 2. CLOSE ROUTINE 
; =================================================================
HandlerClose
    ldy #1          
    clc
    rts

; =================================================================
; 3. GET BYTE ROUTINE
; =================================================================
HandlerGet
     //proof of HandlerGet 
     inc TestB
     lda TestB 
     ldy #$01
     rts

    ; 1. Have we already hit EOF?
    lda EOF_Flag
    beq CheckPtr
    ldy #136        ; Yes, return EOF Error
    sec             
    rts

CheckPtr
    ; 2. Do we need to refill the buffer?
    ldx BufPtr
    cpx #128        ; 128 = Buffer Exhausted
    bne FetchByte   ; If < 128, we have data.

    ; -- REFILL NEEDED --
    jsr RefillBuffer
    
    ; Check SIO Status (Y=1 is Success)
    cpy #1          
    bne RefillFailed 
    
    ; -- REFILL SUCCESS --
    ; Fall through to FetchByte (BufPtr is now 0)

FetchByte
    ldx BufPtr
    lda IOBuf,x     ; Load the character
    
    ; --- NULL TERMINATOR CHECK (CRITICAL FIX) ---
    cmp #0          ; Is it a Null (0x00)?
    beq FoundNull   ; If yes, it's the End of the text.
    ; --------------------------------------------

    inc BufPtr      ; Advance pointer
    ldy #1          ; Success
    clc             ; Clear Carry
    rts

FoundNull
    lda #1
    sta EOF_Flag    ; Mark EOF
    ldy #136        ; Return EOF Error
    sec
    rts

RefillFailed
    sty EOF_Flag    ; Mark Error/EOF
    sec             
    rts

; -- Helper: Refill the Buffer --
RefillBuffer
    jsr SetupDCB_Read       
    bpl RefillOK    
    rts             ; Return with SIO Error (in Y)

RefillOK
    lda #0
    sta BufPtr      ; Reset Pointer to start
    ldy #1          ; Success
    rts

; =================================================================
; UNSUPPORTED / STUBS
; =================================================================
HandlerPut
HandlerStat
HandlerSpec
    ldy #1
    clc
    rts

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

; =================================================================
; INSTALLER 
; =================================================================
Init
    ; 1. Find Empty Slot
    ldx #0
FindSlot
    lda HATABS,x    
    beq FoundEmpty  
    cmp #$59        
    beq FoundEmpty  
    inx
    inx
    inx
    cpx #33         
    bcc FindSlot
    rts

FoundEmpty
    ; 2. Install 'Y'
    lda #$59        
    sta HATABS,x
    
    ; 3. Install Vector
    lda #<HandlerTable
    sta HATABS+1,x
    lda #>HandlerTable
    sta HATABS+2,x

    ; 4. Update MEMLO (Protect $4000-$4200)
    lda #$00
    sta $02E7       ; MEMLO Low
    lda #$42
    sta $02E8       ; MEMLO High
    rts
    
    run Init
    
    
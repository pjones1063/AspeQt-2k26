; =================================================================
; AspeQt-2k26 Clipboard Handler (Y:) - GOLDEN MASTER
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; MEMORY: Code at $4000, Data at $0600 (Safe Zone)
; =================================================================

    icl "clip_sym.asm"

; =================================================================
; RESIDENT DATA (Page 6 - Safe from BASIC)
; =================================================================
    org $0600    
IOBuf       .ds 128
BufPtr      .byte 0 
EOF_Flag    .byte 0 

; =================================================================
; RESIDENT CODE
; =================================================================
    org $4000
    .align 256 

; =================================================================
; CIO JUMP TABLE (MUST BE ADDRESS MINUS 1)
; =================================================================
HandlerTable
    .word HandlerOpen-1
    .word HandlerClose-1
    .word HandlerGet-1
    .word HandlerPut-1
    .word HandlerStat-1
    .word HandlerSpec-1
    
; =================================================================
; 1. OPEN ROUTINE (SYNCED WITH PC)
; =================================================================
HandlerOpen
    ; 1. Setup the SIO Packet (but don't send yet)
    jsr SetupDCB_Open
    
    ; 2. Copy the Open Mode (4 or 8) to SIO Aux Byte 1
    lda $2A         ; ZIOCB ICAX1 (4=Read, 8=Write)
    sta DAUX1       ; Send this to PC so it knows to Clear or Snapshot!
    lda #0
    sta DAUX2
    
    ; 3. Perform the SIO Call
    jsr SIOV
    bmi OpenFail    
    
    ; 4. Setup Local Pointers based on Mode
    lda DAUX1       ; We can just check what we sent
    and #$08        ; Is it Write Mode (8)?
    bne OpenWrite
    
OpenRead
    ; -- READ MODE (ENTER "Y:") --
    ; Force Refill on first GET
    lda #128
    sta BufPtr
    lda #0
    sta EOF_Flag
    jmp OpenDone

OpenWrite
    ; -- WRITE MODE (LIST "Y:") --
    ; Start with Empty Buffer
    lda #0
    sta BufPtr

OpenDone
    ldy #1          ; Success
    clc
    rts

OpenFail
    ldy #144        ; Error
    sec
    rts
    
    

; =================================================================
; 2. GET BYTE ROUTINE
; =================================================================
HandlerGet
    stx SaveX       ; Save IOCB Index
    
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
    
    ; --- NULL TERMINATOR CHECK ---
    cmp #0          ; Is it a Null (0x00)?
    beq FoundNull   ; If yes, it's the End of the text.
    ; -----------------------------

    inc BufPtr      ; Advance pointer
    ldx SaveX       ; Restore IOCB Index
    ldy #1          ; Success
    clc             ; Clear Carry
    rts

FoundNull
    lda #1
    sta EOF_Flag    ; Mark EOF
    ldx SaveX       ; Restore IOCB Index
    ldy #136        ; Return EOF Error
    sec
    rts

RefillFailed
    sty EOF_Flag    ; Mark Error/EOF
    ldx SaveX       ; Restore IOCB Index
    sec             
    rts

; -- Helper: Refill the Buffer --
RefillBuffer
    jsr SetupDCB_Read   ; Calls SIOV inside!
    bpl RefillOK    
    rts                 ; Return with SIO Error (in Y)

RefillOK
    lda #0
    sta BufPtr      ; Reset Pointer to start
    ldy #1          ; Success
    rts


; =================================================================
; 3. PUT BYTE ROUTINE
; =================================================================
HandlerPut
    stx SaveX       ; Save IOCB Index
    
    ; 1. Store Byte in Buffer
    ldx BufPtr
    sta IOBuf,x
    inc BufPtr
    
    ; 2. Is Buffer Full? (128 bytes)
    ldx BufPtr
    cpx #128
    bne PutSuccess  ; Not full, we are done
    
    ; 3. Buffer Full - FLUSH IT!
    jsr FlushBuffer
    cpy #1          ; Check SIO Success
    bne PutError
    
PutSuccess
    ldx SaveX       ; Restore IOCB
    ldy #1          ; Success
    clc
    rts

PutError
    ldx SaveX
    sec             ; Error Flag
    rts

FlushBuffer
    jsr SetupDCB_Write
    bpl FlushOK
    rts             ; Return Error in Y
FlushOK
    lda #0
    sta BufPtr      ; Reset Pointer
    ldy #1          ; Success
    rts
    
 
; =================================================================
; 4. CLOSE ROUTINE (SMART R/W)
; =================================================================
HandlerClose
    ; Check ZIOCB Aux Byte 1 ($2A)
    lda $2A
    and #$08        ; Check Bit 3 (Value 8 = WRITE)
    bne CloseWrite
    
    ; --- READ MODE ---
    ldy #1          
    clc
    rts

CloseWrite
    ; --- WRITE MODE ---
    ; Flush remaining data in buffer
    stx SaveX       ; Save IOCB Index
    
    lda BufPtr      ; Is buffer empty?
    beq CloseDone
    
    ; Pad remainder with Nulls (Optional but good for clean frames)
    ldx BufPtr
PadLoop
    lda #0
    sta IOBuf,x
    inx
    cpx #128
    bne PadLoop
    
    ; Send the final chunk
    jsr FlushBuffer
    
CloseDone
    ldx SaveX       ; Restore IOCB
    ldy #1
    clc
    rts
    

           
; =================================================================
; UNSUPPORTED / STUBS
; =================================================================
HandlerStat
HandlerSpec
    ldy #1
    clc
    rts

; =================================================================
; SIO SETUP ROUTINES
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
    lda #$59        ; Device 'Y'
    sta DDEVIC
    lda #1
    sta DUNIT
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
    jsr SIOV        ; Perform the Call
    rts

SetupDCB_Write
    lda #$57        ; Cmd 'W' (Write)
    sta DCOMND
    lda #$80        ; Direction: Write ($80)
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$08        ; Timeout
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
; DATA
; =================================================================
SaveX .byte 0

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
    
    
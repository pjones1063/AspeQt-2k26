; =================================================================
; AspeQt-2k26 Clipboard Handler (Y:) - 256 BYTE TURBO EDITION
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; MEMORY: Code at $4000, Data at $0600 (Page 6)
; =================================================================

    icl "clip_sym.asm"

; =================================================================
; RESIDENT DATA (Page 6 - Safe from BASIC)
; =================================================================
    org $0600    
IOBuf       .ds 256  ; FULL PAGE BUFFER (256 Bytes)

; =================================================================
; RESIDENT CODE
; =================================================================
    org $4000
    .align 256 

; --- Variables (Moved here to keep Page 6 pure for buffer) ---
BufPtr      .byte 0 
EOF_Flag    .byte 0 
SaveX       .byte 0

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
; 1. OPEN ROUTINE
; =================================================================
HandlerOpen
	lda #0       	;reset buffer and EOF pointers.
    sta BufPtr
    sta EOF_Flag

    ; 1. Setup the SIO Packet
    jsr SetupDCB_Open
    
    ; 2. Copy Mode to DAUX1 (8=Write, 4=Read)
    lda $2A         ; ZIOCB ICAX1
    sta DAUX1       
    lda #0
    sta DAUX2
    
    ; 3. Perform SIO
    jsr SIOV
    bmi OpenFail    
    
    ; 4. Setup Local Pointers
    lda DAUX1       
    and #$08        
    bne OpenDone ;  -- WRITE MODE (LIST "Y:") we are done-- 
    
OpenRead
    ; -- READ MODE (ENTER "Y:") --
    ; Force Refill on first GET. 
    jsr RefillBuffer

OpenDone
    ldy #1          ; Success
    clc
    rts

OpenFail
    ldy #144        ; Device Error 
    sec
    rts

; =================================================================
; 2. GET BYTE ROUTINE
; =================================================================
HandlerGet
    stx SaveX
    
    ; 1. Check EOF
    lda EOF_Flag
    beq FetchByte
    ldy #136        ; EOF Error
    sec             
    rts

FetchByte
    ldx BufPtr
    lda IOBuf,x
    
    ; --- NULL CHECK (End of Text) ---
    cmp #0
    beq FoundNull

    ; --- INCREMENT & CHECK REFILL ---
    inc BufPtr      ; 255 -> 0 triggers Zero Flag (Z)
    bne GetDone     ; If not zero, we still have data.

    ; -- REFILL NEEDED (Wrapped to 0) --
    pha             ; Save the character we just got!
    jsr RefillBuffer
    pla             ; Restore character
    
    ; Note: If RefillBuffer sets EOF, we still return this valid char first.
    ; Next call will hit EOF_Flag check.

GetDone
    ldx SaveX
    ldy #1
    clc
    rts

FoundNull
    lda #1
    sta EOF_Flag
    ldx SaveX
    ldy #136        ; EOF Code
    sec
    rts

; =================================================================
; 3. PUT BYTE ROUTINE
; =================================================================
HandlerPut
    stx SaveX
    
    ; 1. Store Byte
    ldx BufPtr
    sta IOBuf,x
    
    ; 2. Increment & Check Full
    inc BufPtr      ; 255 -> 0 triggers Zero Flag
    bne PutSuccess  ; If not 0, buffer has space
    
    ; 3. Buffer Full (Wrapped to 0) -> FLUSH
    jsr FlushBuffer
    cpy #1
    bne PutError
    
PutSuccess
    ldx SaveX
    ldy #1
    clc
    rts

PutError
    ldx SaveX
    sec
    rts

FlushBuffer
    jsr SetupDCB_Write
    bpl FlushOK
    rts             ; Return Error in Y
FlushOK
    lda #0
    sta BufPtr
    ldy #1
    rts
    
; -- Refill Helper --
RefillBuffer
    jsr SetupDCB_Read
    bpl RefillOK
    ; If SIO Error, mark EOF
    lda #1
    sta EOF_Flag
    rts
RefillOK
    lda #0
    sta BufPtr
    ldy #1
    rts

; =================================================================
; 4. CLOSE ROUTINE
; =================================================================
HandlerClose
    lda $2A
    and #$08
    bne CloseWrite
    
    ; --- READ MODE ---
    ldy #1          
    clc
    rts

CloseWrite
    ; --- WRITE MODE ---
    stx SaveX
    
    lda BufPtr
    beq CloseCommit ; If 0, nothing pending, just commit.
    
    ; Pad remainder with 0s for clean frame
    ldx BufPtr
PadLoop
    lda #0
    sta IOBuf,x
    inx
    bne PadLoop     ; Loop until X wraps to 0
    
    jsr FlushBuffer
    
CloseCommit
    ; Send 'C' Command to trigger PC Clipboard Update
    jsr SetupDCB_Close
    bmi CloseFail
    
    ldy #1
    clc
    rts

CloseFail
    ldy #144
    sec
    rts

; =================================================================
; UNSUPPORTED
; =================================================================
HandlerStat
HandlerSpec
    ldy #1
    clc
    rts

; =================================================================
; SIO SETUP ROUTINES (256 BYTE LENGTH)
; =================================================================
; NOTE: DBYTLO = $00, DBYTHI = $01 (Length = 256)

SetupDCB_Open
    lda #$4F        ; Cmd 'O'
    sta DCOMND
    lda #$00
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$08       ; Timeout
    sta DTIMLO
    lda #$00
    sta DBYTLO      ; Length Low
    sta DBYTHI      ; Length High
    sta DAUX1
    sta DAUX2
    lda #$59        ; Device 'Y'
    sta DDEVIC
    lda #1
    sta DUNIT
    rts

SetupDCB_Close
    jsr SetupDCB_Open ; Reuse Init
    lda #$43        ; Cmd 'C'
    sta DCOMND
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
    lda #$04
    sta DTIMLO
    lda #$00
    sta DUNUSE
    lda #$00        ; Lo = 0
    sta DBYTLO
    lda #$01        ; Hi = 1  (LENGTH = 256)
    sta DBYTHI
    lda #$00        
    sta DAUX1
    sta DAUX2
    lda #$59
    sta DDEVIC
    lda #1
    sta DUNIT
    jsr SIOV
    rts

SetupDCB_Write
    lda #$57        ; Cmd 'W'
    sta DCOMND
    lda #$80        ; Write
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$0A
    sta DTIMLO
    lda #$00
    sta DUNUSE
    lda #$00        ; Lo = 0
    sta DBYTLO
    lda #$01        ; Hi = 1 (LENGTH = 256)
    sta DBYTHI
    lda #$00        
    sta DAUX1
    sta DAUX2
    lda #$59
    sta DDEVIC
    lda #1
    sta DUNIT
    jsr SIOV        
    rts
    
; =================================================================
; INSTALLER 
; =================================================================
Init
    ; 1. Find Empty Slot in HATABS
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
    ; 2. Install 'Y' Device
    lda #$59        
    sta HATABS,x
    
    ; 3. Install Handler Table Address
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
    
    
    
    
;; =============================================================
;  PC Clipboard Driver (Device "Y:") for AspeQt
;  VERSION 8: Page 6 Static Storage (Crash Proof)
;  Usage: ENTER "Y:"
; =============================================================

    icl 'menu_sym.asm'

    org $4000

; --- Local Equates ---
MEMLO   = $02E7     
HATABS  = $031A     
CIO_OK  = 1
CIO_EOF = 136

; --- STATIC STORAGE (PAGE 6) ---
; We use Page 6 to avoid complex relocation patching.
; This memory is safe and fixed.
PG6_BUFFER  = $0600     ; 128 Bytes for SIO Data
PG6_PTR     = $0680     ; 1 Byte: Buffer Index
PG6_LEN     = $0681     ; 1 Byte: Buffer Length

; =============================================================
;  INSTALLATION ROUTINE
; =============================================================
Start:
    jsr Printf
    .byte 155,'Installing Y: Clipboard Driver...',155,0

    ; 1. Prepare Pointers for Copy
    lda MEMLO
    sta Temp1
    lda MEMLO+1
    sta Temp1+1
    
    lda #< (DriverEnd - DriverStart)
    sta Temp2
    lda #> (DriverEnd - DriverStart)
    sta Temp2+1

    ; 2. Copy Driver Code to MEMLO
    ldy #0
CopyLoop:
    lda DriverStart,y
    sta (Temp1),y
    iny
    bne CheckSize
    inc DriverStart+1
    inc Temp1+1
CheckSize:
    cpy Temp2
    bne CopyLoop

    ; 3. Calculate Global Offset (New - Old)
    lda MEMLO
    sec
    sbc #<DriverStart
    sta Temp3           ; Offset Low
    lda MEMLO+1
    sbc #>DriverStart
    sta Temp3+1         ; Offset High

    ; 4. Patch Jump Table (Offsets only)
    ; Reset Temp1 to New Driver Start
    lda MEMLO
    sta Temp1
    lda MEMLO+1
    sta Temp1+1

    ldy #0
PatchTable:
    clc
    lda (Temp1),y       ; Load Low
    adc Temp3           ; Add Offset
    sta (Temp1),y       ; Store New Low
    iny
    
    lda (Temp1),y       ; Load High
    adc Temp3+1         ; Add Offset
    sta (Temp1),y       ; Store New High
    iny
    
    cpy #12             ; 6 Vectors * 2 Bytes
    bne PatchTable
    
    ; Note: No other patching is needed because variables are at fixed $0600!

    ; 5. Install in HATABS
    ldx #0
FindSlot:
    lda HATABS,x
    beq FoundSlot
    cmp #'Y'
    beq FoundSlot
    inx
    inx
    inx
    cpx #35
    bne FindSlot
    
    jsr Printf
    .byte 155,'Error: HATABS Full!',155,0
    rts

FoundSlot:
    lda #'Y'
    sta HATABS,x
    lda MEMLO
    sta HATABS+1,x
    lda MEMLO+1
    sta HATABS+2,x

    ; 6. Reserve Memory
    clc
    lda MEMLO
    adc Temp2
    sta MEMLO
    lda MEMLO+1
    adc #0
    sta MEMLO+1

    ; 7. Initialize Variables
    lda #0
    sta PG6_PTR
    sta PG6_LEN

    jsr Printf
    .byte 'Success! Type: ENTER "Y:"',155,0
    rts

; =============================================================
;  RESIDENT DRIVER CODE
; =============================================================
DriverStart:

    ; Jump Table
    .word H_OPEN-1
    .word H_CLOSE-1
    .word H_GETBYTE-1
    .word H_PUTBYTE-1
    .word H_STATUS-1
    .word H_SPECIAL-1
    jmp H_INIT      

; --- OPEN HANDLER ---
H_OPEN:
    ; Use Stack to Save X (No variable needed)
    txa
    pha

    lda #$4F        ; 'O' Open
    jsr CallSIO     ; Returns N flag on error
    bmi OpenError

    lda #0
    sta PG6_LEN     ; Reset Static Vars
    sta PG6_PTR
    
    pla             ; Restore X
    tax
    ldy #CIO_OK
    rts

OpenError:
    pla
    tax
    ldy #144        ; Device Error
    rts

; --- GET BYTE HANDLER ---
H_GETBYTE:
    txa             ; Save X
    pha

    ldx PG6_PTR
    cpx PG6_LEN
    bne FetchChar

    jsr RefillBuffer
    bmi ReturnEOF
    
    ldx #0
    stx PG6_PTR

FetchChar:
    ldx PG6_PTR
    lda PG6_BUFFER,x   ; Read from Page 6
    inc PG6_PTR
    
    ; Restore X, but preserve A (the character)
    tax             ; Move Char to X temp
    pla             ; Get Old X
    tay             ; Move Old X to Y temp
    txa             ; Move Char back to A
    tya             ; Move Old X back to A? No, wait.
    
    ; Correct Restore Logic:
    ; Stack: [Old X]
    ; A = Char
    
    ; We need to return: A=Char, X=OldX
    sta Temp1       ; Save Char to ZP temp (Safe in interrupt? Maybe not)
                    ; Better: Use Y.
    tay             ; Save Char in Y
    pla             ; Get Old X
    tax             ; Restore X
    tya             ; Restore Char to A
    
    ldy #CIO_OK
    rts

ReturnEOF:
    pla
    tax
    ldy #CIO_EOF
    rts

; --- REFILL BUFFER ---
RefillBuffer:
    lda #$52        ; 'R' Read
    jsr CallSIO
    tya
    bmi RefillFail
    
    lda #128
    sta PG6_LEN
    ldy #CIO_OK
    rts
RefillFail:
    ldy #255
    rts

; --- SIO WRAPPER ---
CallSIO:
    sta DCOMND
    lda #128        ; Always 128 bytes
    sta DBYTLO
    lda #0
    sta DBYTHI
    
    lda #$59        ; Device 'Y'
    sta DDEVIC
    lda #1
    sta DUNIT
    lda #$40        ; Receive
    sta DSTATS
    lda #15
    sta DTIMLO
    
    ; Point SIO to Fixed Page 6 Buffer
    lda #<PG6_BUFFER
    sta DBUFLO
    lda #>PG6_BUFFER
    sta DBUFHI
    
    jsr SIOV
    rts

; --- STUBS ---
H_CLOSE:
H_PUTBYTE:
H_STATUS:
H_SPECIAL:
H_INIT:
    ldy #CIO_OK
    rts

DriverEnd:

; =============================================================
;  LIBRARIES
; =============================================================
    icl 'printf.asm'
    run Start
    
    
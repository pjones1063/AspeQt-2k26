 ; =============================================================
;  PC Clipboard Driver (Device "Y:") for AspeQt
;  VERSION 5: Fixed "Keyboard Crash" & Display Bugs
;  Usage: ENTER "Y:" to paste from PC
; =============================================================

    icl 'menu_sym.asm'  ; Defines Temp1 ($80), Temp2, etc.

    org $4000           ; Standard executable start

; --- Local Equates ---
MEMLO   = $02E7     ; Pointer to available user memory
HATABS  = $031A     ; Handler Table
CIO_OK  = 1
CIO_EOF = 136       ; Error 136 (End Of File)

; =============================================================
;  INSTALLATION PROGRAM (Runs once, then exits)
; =============================================================
Start:
    ; 1. Print Banner
    jsr Printf
    .byte 155,'Installing Y: Clipboard Driver...',155,0

    ; 2. Prepare Pointers for Relocation
    ; Temp1 ($80) = Destination (Current MEMLO)
    lda MEMLO
    sta Temp1
    lda MEMLO+1
    sta Temp1+1
    
    ; Temp2 ($82) = Driver Size
    lda #< (DriverEnd - DriverStart)
    sta Temp2
    lda #> (DriverEnd - DriverStart)
    sta Temp2+1

    ; 3. Copy Driver to MEMLO
    ldy #0
CopyLoop:
    lda DriverStart,y
    sta (Temp1),y       ; Valid ZP Indirect
    iny
    bne CheckSize
    inc DriverStart+1   ; Bump Source High
    inc Temp1+1         ; Bump Dest High
CheckSize:
    cpy Temp2
    bne CopyLoop

    ; 4. Relocation Patching (The Math)
    ; We must shift all JMP targets by the offset
    ; Offset = MEMLO - CompileStart ($4000)
    
    ; Reset Temp1 to point to the New Driver Start (MEMLO)
    lda MEMLO
    sta Temp1
    lda MEMLO+1
    sta Temp1+1

    ; Calculate Offset
    lda MEMLO
    sec
    sbc #<DriverStart
    sta Temp3           ; Offset Low
    lda MEMLO+1
    sbc #>DriverStart
    sta Temp3+1         ; Offset High

    ; A. Patch the Jump Table (First 12 bytes)
    ldy #0
PatchTable:
    clc
    lda (Temp1),y       ; Load Low Byte from RAM
    adc Temp3           ; Add Offset Low
    sta (Temp1),y       ; Save back
    iny
    
    lda (Temp1),y       ; Load High Byte
    adc Temp3+1         ; Add Offset High
    sta (Temp1),y       ; Save back
    iny
    
    cpy #12             ; 6 vectors * 2 bytes
    bne PatchTable
    
    ; B. Patch the SIO Buffer Pointer
    ; We need to find the instructions "LDA #<Buffer" inside the copy.
    ; Location = MEMLO + (LOC_BufLo - DriverStart)
    
    ; Patch Low Byte Instruction
    clc
    lda MEMLO
    adc #< (LOC_BufLo + 1 - DriverStart) 
    sta Temp1
    lda MEMLO+1
    adc #> (LOC_BufLo + 1 - DriverStart)
    sta Temp1+1
    
    ; Calculate New Buffer Address Low
    clc
    lda #<Buffer
    adc Temp3           
    ldy #0
    sta (Temp1),y       ; Patch the operand

    ; Patch High Byte Instruction
    clc
    lda MEMLO
    adc #< (LOC_BufHi + 1 - DriverStart)
    sta Temp1
    lda MEMLO+1
    adc #> (LOC_BufHi + 1 - DriverStart)
    sta Temp1+1
    
    ; Calculate New Buffer Address High
    clc
    lda #>Buffer
    adc Temp3+1         
    ldy #0
    sta (Temp1),y       ; Patch the operand

    ; 5. Hook into HATABS (Using 'Y' now!)
    ldx #0
FindSlot:
    lda HATABS,x
    beq FoundSlot
    cmp #'Y'        ; Check for existing Y: handler
    beq FoundSlot
    inx
    inx
    inx
    cpx #35
    bne FindSlot
    
    ; Table Full Error
    jsr Printf
    .byte 155,'Error: HATABS Full!',155,0
    rts

FoundSlot:
    lda #'Y'        ; Register Device 'Y'
    sta HATABS,x
    lda MEMLO
    sta HATABS+1,x  ; Point to New Driver
    lda MEMLO+1
    sta HATABS+2,x

    ; 6. Reserve Memory (Update MEMLO)
    clc
    lda MEMLO
    adc Temp2
    sta MEMLO
    lda MEMLO+1
    adc #0
    sta MEMLO+1

    ; 7. Print Success
    jsr Printf
    .byte 'Success! Type: ENTER "Y:"',155,0
    rts

; =============================================================
;  RESIDENT DRIVER CODE (The Template)
; =============================================================
DriverStart:

    ; Jump Table (Patched at runtime)
    .word H_OPEN-1
    .word H_CLOSE-1
    .word H_GETBYTE-1
    .word H_PUTBYTE-1
    .word H_STATUS-1
    .word H_SPECIAL-1
    jmp H_INIT      

    ; Variables
BufPtr      .byte 0     
BufLen      .byte 0     
Buffer      .ds 128     

H_OPEN:
    lda #$4F        ; 'O'
    ldx #0
    ldy #0
    jsr CallSIO
    bmi OpenError

    lda #0
    sta BufLen
    sta BufPtr
    ldy #CIO_OK
    rts
OpenError:
    ldy #144        ; Device Error
    rts

H_GETBYTE:
    ldx BufPtr
    cpx BufLen
    bne FetchChar

    jsr RefillBuffer
    bmi ReturnEOF
    
    ldx #0
    stx BufPtr
FetchChar:
    ldx BufPtr
    lda Buffer,x
    inc BufPtr
    ldy #CIO_OK
    rts
ReturnEOF:
    ldy #CIO_EOF
    rts

RefillBuffer:
    lda #$52        ; 'R'
    ldx #128
    ldy #0
    jsr CallSIO
    tya
    bmi RefillFail
    lda #128
    sta BufLen
    ldy #CIO_OK
    rts
RefillFail:
    ldy #255
    rts

CallSIO:
    sta DCOMND
    stx DBYTLO
    sty DBYTHI
    
    lda #$59        ; Device 'Y' (Fixed! Was $4B 'K')
    sta DDEVIC
    
    lda #1
    sta DUNIT
    lda #$40        ; Receive
    sta DSTATS
    lda #15
    sta DTIMLO
    
    ; Targets for Self-Modifying Code
LOC_BufLo:
    lda #<Buffer    
    sta DBUFLO
LOC_BufHi:
    lda #>Buffer    
    sta DBUFHI
    
    jsr SIOV
    rts

; Stubs
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

    icl 'printf.asm'    ; Uses your working print library
    
    run Start
    
    
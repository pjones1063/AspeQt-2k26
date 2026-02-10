; =================================================================
; AspeQt Dual Device Handler - RELOCATABLE V8 (LABEL PATCHING)
; -----------------------------------------------------------------
; FIX: Replaced manual offsets (erroneous) with explicit Labels.
; TARGET: Atari 8-bit (MADS Assembler)
; =================================================================

    icl "sym.asm"

; --- POINTERS ---
DestPtr     = $D4   ; Destination (Target Page)
PatchPtr    = $D6   ; Patching Pointer

; =================================================================
; 1. FIXED VARIABLES (Page 6)
; =================================================================
    org $0600    
    
CurrentDev  .byte 0  
OldDOSINI   .word 0
BufPtr      .byte 0 
EOF_Flag    .byte 0 
SaveX       .byte 0
DeviceID    .byte 0
TableLo     .byte 0
TableHi     .byte 0
TargetPage  .byte 0
SourcePage  .byte 0
PageDiff    .byte 0
JumpDest    .word 0

; =================================================================
; 2. THE INSTALLER (Safe High Memory)
; =================================================================
    org $4000 ; Load high to avoid conflicting with low MEMLO

Installer
    ; --- 1. Calculate Target Page (MEMLO Aligned) ---
    lda MEMLO+1
    sta TargetPage
    lda MEMLO
    cmp #0
    beq SetupCalc
    inc TargetPage

SetupCalc
    ; --- 2. Calculate Shift ---
    lda #>DriverBlob
    sta SourcePage
    lda TargetPage
    sec
    sbc SourcePage
    sta PageDiff

    ; --- 3. Copy Driver ---
    lda #0
    sta DestPtr
    lda TargetPage
    sta DestPtr+1

    ldy #0
CopyLoop1
    lda DriverBlob,y
    sta (DestPtr),y
    iny
    bne CopyLoop1
    inc DestPtr+1
CopyLoop2
    lda DriverBlob+256,y
    sta (DestPtr),y
    iny
    bne CopyLoop2
    dec DestPtr+1 ; Restore Ptr

    ; --- 4. Apply Patches using Labels ---
    ldx #0
RelocLoop
    lda RelocTable,x
    cmp #$FF
    bne DoPatch
    lda RelocTable+1,x
    cmp #$FF
    beq RelocDone

DoPatch
    ; Get Offset from Table
    lda RelocTable,x
    sta PatchPtr
    lda RelocTable+1,x
    sta PatchPtr+1
    
    ; Add DestPtr to find absolute memory address
    lda PatchPtr
    clc
    adc DestPtr
    sta PatchPtr
    lda PatchPtr+1
    adc DestPtr+1
    sta PatchPtr+1

    ; Apply Shift to the byte at [PatchPtr]
    ldy #0
    lda (PatchPtr),y
    clc
    adc PageDiff
    sta (PatchPtr),y
    
    inx
    inx
    jmp RelocLoop

RelocDone
    ; --- 5. Init ---
    lda #<Driver.InitOffset
    sta JumpDest
    lda #>Driver.InitOffset
    clc
    adc PageDiff
    sta JumpDest+1
    jsr CallJumpDest 

    ; --- 6. Reserve Memory ---
    lda #0
    sta MEMLO
    lda TargetPage
    clc
    adc #2
    sta MEMLO+1
    rts

CallJumpDest
    jmp (JumpDest)


; =================================================================
; 3. THE DRIVER BLOB
; =================================================================
    .align 256
DriverBlob
    .local Driver   

; --- JUMP TABLES ---
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

; --- OPEN ---
HandlerOpenY
    lda #$59
    sta CurrentDev
P_Open1 jsr CommonReset     ; [PATCH]
P_Open2 jsr SetupDCB_Open   ; [PATCH]
    lda $2A
    sta DAUX1
    and #$08
    bne DoOpenY
P_Open3 jsr SIOV            ; No patch (SIOV is ROM)
    bmi OpenFail
P_Open4 jsr RefillBuffer    ; [PATCH]
P_Open5 jmp OpenSuccess     ; [PATCH]

DoOpenY
    jsr SIOV
    bmi OpenFail
P_Open6 jmp OpenSuccess     ; [PATCH]

HandlerOpenW
    lda #$57
    sta CurrentDev
P_Open7 jsr CommonReset     ; [PATCH]
P_Open8 jsr SetupDCB_Open   ; [PATCH]
    lda $2A
    sta DAUX1
    
    stx SaveX
    ldx SaveX
    lda $0344,x
    sta DBUFLO
    lda $0345,x
    sta DBUFHI
    
    lda #$80
    sta DSTATS
    lda #$00
    sta DBYTLO
    lda #$01
    sta DBYTHI
    
    jsr SIOV
    bmi OpenFail
    
    lda $2A
    and #$08
    bne OpenSuccess
P_Open9 jsr RefillBuffer    ; [PATCH]

OpenSuccess
    ldx SaveX
    ldy #1
    clc
    rts

OpenFail
    ldx SaveX
    ldy #144
    sec
    rts

CommonReset
    lda #0
    sta BufPtr
    sta EOF_Flag
    rts

; --- SHARED ---
HandlerGet
    stx SaveX
    lda EOF_Flag
    beq FetchByte
    ldy #136
    sec             
    rts

FetchByte
    ldx BufPtr
P_Buf1  lda IOBuf,x         ; [PATCH]
    cmp #0
    beq FoundNull
    inc BufPtr
    bne GetDone
    pha
P_Get1  jsr RefillBuffer    ; [PATCH]
    pla
GetDone
    ldx SaveX
    ldy #1
    clc
    rts

FoundNull
    lda #1
    sta EOF_Flag
    ldx SaveX
    ldy #136
    sec
    rts

HandlerPut
    stx SaveX
    ldx BufPtr
P_Buf2  sta IOBuf,x         ; [PATCH]
    inc BufPtr
    bne PutSuccess
P_Put1  jsr FlushBuffer     ; [PATCH]
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

HandlerClose
    lda $2A
    and #$08
    bne CloseWrite
    ldy #1
    clc
    rts

CloseWrite
    stx SaveX
    lda BufPtr
    beq CloseCommit
    ldx BufPtr
PadLoop
    lda #0
P_Buf3  sta IOBuf,x         ; [PATCH]
    inx
    bne PadLoop
P_Close1 jsr FlushBuffer    ; [PATCH]

CloseCommit
P_Close2 jsr SetupDCB_Close ; [PATCH]
    ldy #1
    clc
    rts

HandlerStat
HandlerSpec
    ldy #1
    clc
    rts

; --- HELPERS ---
RefillBuffer
P_Refill jsr SetupDCB_Read  ; [PATCH]
    bpl RefillOK
    lda #1
    sta EOF_Flag
    rts
RefillOK
    lda #0
    sta BufPtr
    ldy #1
    rts

FlushBuffer
P_Flush jsr SetupDCB_Write  ; [PATCH]
    bpl FlushOK
    rts
FlushOK
    lda #0
    sta BufPtr
    ldy #1
    rts

; --- SIO SETUP ---
SetupDCB_Open
    lda #$4F
    sta DCOMND
    lda #0
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
P_Imm1  lda #>IOBuf     ; [PATCH]
    sta DBUFHI
    lda #$08
    sta DTIMLO
    lda #0
    sta DBYTLO
    sta DAUX1
    sta DAUX2
    sta DUNUSE
    lda #1
    sta DBYTHI
    lda DUNIT
    lda CurrentDev
    sta DDEVIC
    rts

SetupDCB_Close
P_Setup1 jsr SetupDCB_Open   ; [PATCH]
    lda #$43
    sta DCOMND
    jsr SIOV
    rts

SetupDCB_Read
P_Setup2 jsr SetupDCB_Open   ; [PATCH]
    lda #$52
    sta DCOMND
    lda #$40
    sta DSTATS
    lda #$04
    sta DTIMLO
    jsr SIOV
    rts

SetupDCB_Write
P_Setup3 jsr SetupDCB_Open   ; [PATCH]
    lda #$57
    sta DCOMND
    lda #$80
    sta DSTATS
    lda #$0A
    sta DTIMLO
    jsr SIOV
    rts

; --- RESET TRAP ---
OnReset
P_Rst1  jsr InitHandlersOnly ; [PATCH]
    jmp (OldDOSINI)

; --- INIT ---
Init
P_Init1 jsr InitHandlersOnly ; [PATCH]
    
    lda $0C
    sta OldDOSINI
    lda $0D
    sta OldDOSINI+1
    
    lda #<OnReset
    sta $0C
P_Imm2  lda #>OnReset   ; [PATCH]
    sta $0D
    
    rts

InitHandlersOnly
    lda #$59
    ldx #<TableY
P_Imm3  ldy #>TableY    ; [PATCH]
P_Inst1 jsr InstallOne  ; [PATCH]
    
    lda #$57
    ldx #<TableW
P_Imm4  ldy #>TableW    ; [PATCH]
P_Inst2 jsr InstallOne  ; [PATCH]
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

; --- BUFFER ---
    .align 256
IOBuf
    .ds 256         

InitOffset = Init - DriverBlob
    .endl 

; =================================================================
; 4. RELOCATION TABLE (Using Labels!)
; =================================================================
RelocTable
    ; Jump Tables (Patch High Bytes)
    .word (Driver.TableY+1-DriverBlob), (Driver.TableY+3-DriverBlob), (Driver.TableY+5-DriverBlob)
    .word (Driver.TableY+7-DriverBlob), (Driver.TableY+9-DriverBlob), (Driver.TableY+11-DriverBlob)
    .word (Driver.TableW+1-DriverBlob), (Driver.TableW+3-DriverBlob), (Driver.TableW+5-DriverBlob)
    .word (Driver.TableW+7-DriverBlob), (Driver.TableW+9-DriverBlob), (Driver.TableW+11-DriverBlob)

    ; Code Patches (High Byte of Address is at Label+2 for JSR/JMP/STA)
    ; JSR xxxx = 20 LL HH. Label is at 20. HH is at +2.
    
    .word (Driver.P_Open1+2-DriverBlob)
    .word (Driver.P_Open2+2-DriverBlob)
    .word (Driver.P_Open4+2-DriverBlob)
    .word (Driver.P_Open5+2-DriverBlob) ; JMP
    .word (Driver.P_Open6+2-DriverBlob) ; JMP
    .word (Driver.P_Open7+2-DriverBlob)
    .word (Driver.P_Open8+2-DriverBlob)
    .word (Driver.P_Open9+2-DriverBlob)
    
    .word (Driver.P_Get1+2-DriverBlob)
    .word (Driver.P_Put1+2-DriverBlob)
    .word (Driver.P_Close1+2-DriverBlob)
    .word (Driver.P_Close2+2-DriverBlob)
    
    .word (Driver.P_Refill+2-DriverBlob)
    .word (Driver.P_Flush+2-DriverBlob)
    
    .word (Driver.P_Setup1+2-DriverBlob)
    .word (Driver.P_Setup2+2-DriverBlob)
    .word (Driver.P_Setup3+2-DriverBlob)
    
    .word (Driver.P_Rst1+2-DriverBlob)
    .word (Driver.P_Init1+2-DriverBlob)
    .word (Driver.P_Inst1+2-DriverBlob)
    .word (Driver.P_Inst2+2-DriverBlob)

    ; Buffer Access (LDA IOBuf,x = BD LL HH. +2)
    .word (Driver.P_Buf1+2-DriverBlob)
    .word (Driver.P_Buf2+2-DriverBlob)
    .word (Driver.P_Buf3+2-DriverBlob)
    
    ; Immediate Values (LDA #>Label = A9 HH. +1)
    .word (Driver.P_Imm1+1-DriverBlob)
    .word (Driver.P_Imm2+1-DriverBlob)
    .word (Driver.P_Imm3+1-DriverBlob)
    .word (Driver.P_Imm4+1-DriverBlob)

    .word $FFFF

    run Installer
    run Installer
; =================================================================
; AspeQt Dual Device Handler - RELOCATABLE V12 (ALIGNED BUFFER)
; -----------------------------------------------------------------
; FIXES: 
;   1. Forced IOBuf to be PAGE ALIGNED (.align 256).
;      (Unaligned SIO buffers can cause hardware/emulator issues).
;   2. Moved Variables to TOP of Driver (Matches working handler structure).
;   3. Increased Memory Reservation to 4 Pages (1KB) to handle alignment.
; =================================================================

    icl "sym.asm"

; --- POINTERS ---
DestPtr     = $D4   ; Destination (Target Page)
PatchPtr    = $D6   ; Patching Pointer

; =================================================================
; 1. THE INSTALLER
; =================================================================
    org $4000 

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

    ; --- 3. Copy Driver (4 Pages) ---
    ; We copy 4 pages (1024 bytes) to ensure we get the aligned buffer.
    
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
    
    inc DestPtr+1
CopyLoop3
    lda DriverBlob+512,y
    sta (DestPtr),y
    iny
    bne CopyLoop3

    inc DestPtr+1
CopyLoop4
    lda DriverBlob+768,y
    sta (DestPtr),y
    iny
    bne CopyLoop4
    
    ; Restore DestPtr
    lda TargetPage
    sta DestPtr+1

    ; --- 4. Apply Patches ---
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

    ; Apply Shift (PageDiff)
    ldy #0
    lda (PatchPtr),y
    clc
    adc PageDiff
    sta (PatchPtr),y
    
    inx
    inx
    jmp RelocLoop

RelocDone
    ; --- 5. Init Jump ---
    lda #<Driver.InitOffset
    sta JumpDest
    
    lda #>Driver.InitOffset     
    clc
    adc TargetPage              
    sta JumpDest+1
    
    jsr CallJumpDest 

    ; --- 6. Reserve Memory (4 Pages) ---
    lda #0
    sta MEMLO
    lda TargetPage
    clc
    adc #4          ; Reserve 4 Pages (1024 bytes)
    sta MEMLO+1
    
    rts

CallJumpDest
    jmp (JumpDest)

; --- Installer Vars ---
TargetPage  .byte 0
SourcePage  .byte 0
PageDiff    .byte 0
JumpDest    .word 0


; =================================================================
; 2. THE DRIVER BLOB
; =================================================================
    .align 256
DriverBlob
    .local Driver   

; --- VARIABLES (Placed at Top) ---
BufPtr      .byte 0 
EOF_Flag    .byte 0 
SaveX       .byte 0
CurrentDev  .byte 0  
OldDOSINI   .word 0
DeviceID    .byte 0
TableLo     .byte 0
TableHi     .byte 0

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
V_Dev1  sta CurrentDev      ; [PATCH]
    jsr CommonReset
P_Open2 jsr SetupDCB_Open   ; [PATCH]
    lda $2A
    sta DAUX1
    and #$08
    bne DoOpenY
P_Open3 jsr SIOV
    bmi OpenFail
P_Open4 jsr RefillBuffer    ; [PATCH]
P_Open5 jmp OpenSuccess     ; [PATCH]

DoOpenY
    jsr SIOV
    bmi OpenFail
P_Open6 jmp OpenSuccess     ; [PATCH]

HandlerOpenW
    lda #$57
V_Dev2  sta CurrentDev      ; [PATCH]
    jsr CommonReset
P_Open8 jsr SetupDCB_Open   ; [PATCH]
    lda $2A
    sta DAUX1
    
V_Sav1  stx SaveX           ; [PATCH]
V_Sav2  ldx SaveX           ; [PATCH]
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
V_Sav3  ldx SaveX           ; [PATCH]
    ldy #1
    clc
    rts

OpenFail
V_Sav4  ldx SaveX           ; [PATCH]
    ldy #144
    sec
    rts

CommonReset
    lda #0
V_Buf1  sta BufPtr          ; [PATCH]
V_EOF1  sta EOF_Flag        ; [PATCH]
    rts

; --- SHARED ---
HandlerGet
V_Sav5  stx SaveX           ; [PATCH]
V_EOF2  lda EOF_Flag        ; [PATCH]
    beq FetchByte
    ldy #136
    sec             
    rts

FetchByte
V_Buf2  ldx BufPtr          ; [PATCH]
P_Buf1  lda IOBuf,x         ; [PATCH]
    cmp #0
    beq FoundNull
V_Buf3  inc BufPtr          ; [PATCH]
    bne GetDone
    pha
P_Get1  jsr RefillBuffer    ; [PATCH]
    pla
GetDone
V_Sav6  ldx SaveX           ; [PATCH]
    ldy #1
    clc
    rts

FoundNull
    lda #1
V_EOF3  sta EOF_Flag        ; [PATCH]
V_Sav7  ldx SaveX           ; [PATCH]
    ldy #136
    sec
    rts

HandlerPut
V_Sav8  stx SaveX           ; [PATCH]
V_Buf4  ldx BufPtr          ; [PATCH]
P_Buf2  sta IOBuf,x         ; [PATCH]
V_Buf5  inc BufPtr          ; [PATCH]
    bne PutSuccess
P_Put1  jsr FlushBuffer     ; [PATCH]
    cpy #1
    bne PutError
PutSuccess
V_Sav9  ldx SaveX           ; [PATCH]
    ldy #1
    clc
    rts

PutError
V_SavA  ldx SaveX           ; [PATCH]
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
V_SavB  stx SaveX           ; [PATCH]
V_Buf6  lda BufPtr          ; [PATCH]
    beq CloseCommit
V_Buf7  ldx BufPtr          ; [PATCH]
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
V_EOF4  sta EOF_Flag        ; [PATCH]
    rts
RefillOK
    lda #0
V_Buf8  sta BufPtr          ; [PATCH]
    ldy #1
    rts

FlushBuffer
P_Flush jsr SetupDCB_Write  ; [PATCH]
    bpl FlushOK
    rts
FlushOK
    lda #0
V_Buf9  sta BufPtr          ; [PATCH]
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
V_Dev3  lda CurrentDev  ; [PATCH]
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
V_DOS1  jmp (OldDOSINI)      ; [PATCH]

; --- INIT ---
Init
P_Init1 jsr InitHandlersOnly ; [PATCH]
    
    lda $0C
V_DOS2  sta OldDOSINI        ; [PATCH]
    lda $0D
V_DOS3  sta OldDOSINI+1      ; [PATCH]
    
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
V_ID1   sta DeviceID    ; [PATCH]
V_Tab1  stx TableLo     ; [PATCH]
V_Tab2  sty TableHi     ; [PATCH]
    ldx #0
FindSlot
    lda HATABS,x
    beq FoundEmpty
V_ID2   cmp DeviceID    ; [PATCH]
    beq FoundEmpty
    inx
    inx
    inx
    cpx #33
    bcc FindSlot
    rts
FoundEmpty
V_ID3   lda DeviceID    ; [PATCH]
    sta HATABS,x
V_Tab3  lda TableLo     ; [PATCH]
    sta HATABS+1,x
V_Tab4  lda TableHi     ; [PATCH]
    sta HATABS+2,x
    rts


; --- BUFFER (Aligned) ---
    .align 256
IOBuf
    .ds 256         

InitOffset = Init - DriverBlob
    .endl 

; =================================================================
; 4. RELOCATION TABLE
; =================================================================
RelocTable
    ; Jump Tables
    .word (Driver.TableY+1-DriverBlob), (Driver.TableY+3-DriverBlob), (Driver.TableY+5-DriverBlob)
    .word (Driver.TableY+7-DriverBlob), (Driver.TableY+9-DriverBlob), (Driver.TableY+11-DriverBlob)
    .word (Driver.TableW+1-DriverBlob), (Driver.TableW+3-DriverBlob), (Driver.TableW+5-DriverBlob)
    .word (Driver.TableW+7-DriverBlob), (Driver.TableW+9-DriverBlob), (Driver.TableW+11-DriverBlob)

    ; Code Patches
    .word (Driver.P_Open2+2-DriverBlob)
    .word (Driver.P_Open3+2-DriverBlob)
    .word (Driver.P_Open4+2-DriverBlob)
    .word (Driver.P_Open5+2-DriverBlob)
    .word (Driver.P_Open6+2-DriverBlob)
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

    ; Buffer & Immediates
    .word (Driver.P_Buf1+2-DriverBlob)
    .word (Driver.P_Buf2+2-DriverBlob)
    .word (Driver.P_Buf3+2-DriverBlob)
    .word (Driver.P_Imm1+1-DriverBlob)
    .word (Driver.P_Imm2+1-DriverBlob)
    .word (Driver.P_Imm3+1-DriverBlob)
    .word (Driver.P_Imm4+1-DriverBlob)

    ; Variables
    .word (Driver.V_Dev1+2-DriverBlob)
    .word (Driver.V_Dev2+2-DriverBlob)
    .word (Driver.V_Dev3+2-DriverBlob)
    
    .word (Driver.V_Sav1+2-DriverBlob)
    .word (Driver.V_Sav2+2-DriverBlob)
    .word (Driver.V_Sav3+2-DriverBlob)
    .word (Driver.V_Sav4+2-DriverBlob)
    .word (Driver.V_Sav5+2-DriverBlob)
    .word (Driver.V_Sav6+2-DriverBlob)
    .word (Driver.V_Sav7+2-DriverBlob)
    .word (Driver.V_Sav8+2-DriverBlob)
    .word (Driver.V_Sav9+2-DriverBlob)
    .word (Driver.V_SavA+2-DriverBlob)
    .word (Driver.V_SavB+2-DriverBlob)
    
    .word (Driver.V_Buf1+2-DriverBlob)
    .word (Driver.V_Buf2+2-DriverBlob)
    .word (Driver.V_Buf3+2-DriverBlob)
    .word (Driver.V_Buf4+2-DriverBlob)
    .word (Driver.V_Buf5+2-DriverBlob)
    .word (Driver.V_Buf6+2-DriverBlob)
    .word (Driver.V_Buf7+2-DriverBlob)
    .word (Driver.V_Buf8+2-DriverBlob)
    .word (Driver.V_Buf9+2-DriverBlob)
    
    .word (Driver.V_EOF1+2-DriverBlob)
    .word (Driver.V_EOF2+2-DriverBlob)
    .word (Driver.V_EOF3+2-DriverBlob)
    .word (Driver.V_EOF4+2-DriverBlob)
    
    .word (Driver.V_DOS1+2-DriverBlob) 
    .word (Driver.V_DOS2+2-DriverBlob)
    .word (Driver.V_DOS3+2-DriverBlob)
    
    .word (Driver.V_ID1+2-DriverBlob)
    .word (Driver.V_ID2+2-DriverBlob)
    .word (Driver.V_ID3+2-DriverBlob)
    
    .word (Driver.V_Tab1+2-DriverBlob)
    .word (Driver.V_Tab2+2-DriverBlob)
    .word (Driver.V_Tab3+2-DriverBlob)
    .word (Driver.V_Tab4+2-DriverBlob)

    .word $FFFF

    run Installer

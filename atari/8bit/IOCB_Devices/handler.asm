; =================================================================
; AspeQt Dual Device Handler (Y: & W:) - MERGED EDITION
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; MEMORY: Code at $4000, Data at $0600 (Page 6)
; =================================================================

    icl "sym.asm"

; =================================================================
; RESIDENT DATA (Page 6 - Safe from BASIC)
; =================================================================
    org $0600    
IOBuf       .ds 256  ; Shared Buffer (Safe because SIO is blocking)

; =================================================================
; RESIDENT CODE
; =================================================================
    org $4000
    .align 256 

; --- VARIABLES ---
BufPtr      .byte 0 
EOF_Flag    .byte 0 
SaveX       .byte 0
CurrentDev  .byte 0  
OldDOSINI   .word 0
; Temp storage for installer
DeviceID    .byte 0
TableLo     .byte 0
TableHi     .byte 0

; =================================================================
; JUMP TABLES (Two distinct tables!)
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

; =================================================================
; 1. OPEN ROUTINES (The only part that differs significantly)
; =================================================================

; --- OPEN Y: (CLIPBOARD) ---
HandlerOpenY
    lda #$59        ; 'Y'
    sta CurrentDev  ; Remember who we are
    
    jsr CommonReset ; Reset Pointers
    
    ; Setup SIO for Y: (Standard Open, No Data)
    jsr SetupDCB_Open
    
    ; Check Mode (Read/Write)
    lda $2A         ; ICAX1
    sta DAUX1
    and #$08
    bne DoOpenY
    
    ; Read Mode -> Refill Immediately
    jsr SIOV
    bmi OpenFail
    jsr RefillBuffer
    jmp OpenSuccess

DoOpenY
    jsr SIOV
    bmi OpenFail
    jmp OpenSuccess


; --- OPEN W: (WWW Pipe) ---
HandlerOpenW
    lda #$57        ; 'W'
    sta CurrentDev
    
    jsr CommonReset

    ; 1. Base Setup
    jsr SetupDCB_Open
    lda $2A         ; Load ICAX1 (4=Read, 8=Write, 12=Update)
    sta DAUX1
    
    ; 2. N: Specifics - WE MUST SEND THE URL!
    ; The URL pointer is in ICBAL/H ($0344/5)
    stx SaveX
    ldx SaveX
    lda $0344,x     ; ICBAL
    sta DBUFLO
    lda $0345,x     ; ICBAH
    sta DBUFHI
    
    ; 3. Force Write Mode (Sending URL to PC)
    lda #$80        ; Write Direction
    sta DSTATS
    lda #$00
    sta DBYTLO
    lda #$01        ; Length 256
    sta DBYTHI
    
    
    ; 4. Send the Open Command
    jsr SIOV
    bmi OpenFail
    
    ; 5. If we are in READ mode, fill buffer now
    lda $2A         ; ICAX1
    and #$08
    bne OpenSuccess
    jsr RefillBuffer

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

; =================================================================
; 2. SHARED ROUTINES (Get/Put/Close work for BOTH!)
; =================================================================
; Note: These routines use 'CurrentDev' to know which device ID to send

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
    
    ; --- NULL CHECK ---
    cmp #0
    beq FoundNull

    ; --- INCREMENT & CHECK REFILL ---
    inc BufPtr
    bne GetDone

    ; -- REFILL NEEDED --
    pha
    jsr RefillBuffer
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
    
    ; 1. Store Byte
    ldx BufPtr
    sta IOBuf,x
    
    ; 2. Increment & Check Full
    inc BufPtr
    bne PutSuccess
    
    ; 3. Buffer Full -> FLUSH
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
    
    ; Pad Loop
    ldx BufPtr
PadLoop
    lda #0
    sta IOBuf,x
    inx
    bne PadLoop
    
    jsr FlushBuffer

CloseCommit
    ; Send 'C' Command
    jsr SetupDCB_Close
    
    ldy #1
    clc
    rts

; =================================================================
; 3. SHARED HELPERS
; =================================================================
RefillBuffer
    jsr SetupDCB_Read
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
    jsr SetupDCB_Write
    bpl FlushOK
    rts             ; Error in Y
FlushOK
    lda #0
    sta BufPtr
    ldy #1
    rts

HandlerStat
HandlerSpec
    ldy #1
    clc
    rts

; =================================================================
; 4. SIO SETUP (Uses CurrentDev)
; =================================================================
SetupDCB_Open
    lda #$4F        ; Cmd 'O'
    sta DCOMND
    lda #0
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$08        ; Timeout
    sta DTIMLO
    lda #0
    sta DBYTLO
    sta DAUX1
    sta DAUX2
    sta DUNUSE
    lda #1
    sta DBYTHI      ; Length 256
    sta DUNIT
    
    lda CurrentDev  ; <--- MAGIC: Uses $59 or $4E
    sta DDEVIC
    rts

SetupDCB_Close
    jsr SetupDCB_Open
    lda #$43        ; Cmd 'C'
    sta DCOMND
    jsr SIOV
    rts

SetupDCB_Read
    jsr SetupDCB_Open ; Reuse base init
    lda #$52        ; Cmd 'R'
    sta DCOMND
    lda #$40        ; Read
    sta DSTATS
    lda #$04        ; Faster Read Timeout
    sta DTIMLO
    jsr SIOV
    rts

SetupDCB_Write
    jsr SetupDCB_Open ; Reuse base init
    lda #$57        ; Cmd 'W'
    sta DCOMND
    lda #$80        ; Write
    sta DSTATS
    lda #$0A        ; Write Timeout
    sta DTIMLO
    jsr SIOV
    rts

; =================================================================
; 5. THE RESET TRAP (The "Immortal" Logic)
; =================================================================
OnReset
    ; This routine runs AUTOMATICALLY every time System Reset is pressed.
    
    ; 1. Re-Protect Memory (The most important part!)
    lda #<EndHandler
    sta $02E7        ; MEMLO Low
    lda #>EndHandler
    sta $02E8        ; MEMLO High
    
    ; 2. Re-Install Handlers (Just in case OS wiped HATABS)
    jsr InitHandlersOnly 
    
    ; 3. Return control to the real DOS (Chain the hook)
    jmp (OldDOSINI)
    
    
; =================================================================
; INSTALLER 
; =================================================================
Init
    ; 1. Install Handlers initially
    jsr InitHandlersOnly
    
    ; 2. Set MEMLO initially
    lda #<EndHandler
    sta $02E7
    lda #>EndHandler
    sta $02E8
    
    ; 3. HOOK THE RESET VECTOR (DOSINI)
    ; Save the old vector first!
    lda $0C
    sta OldDOSINI
    lda $0D
    sta OldDOSINI+1
    
    ; Point DOSINI to our 'OnReset' routine
    lda #<OnReset
    sta $0C
    lda #>OnReset
    sta $0D
    
    rts

; --- Helper to avoid code duplication ---
InitHandlersOnly
    ; --- Install Y ($59) ---
    lda #$59
    ldx #<TableY
    ldy #>TableY
    jsr InstallOne
    
    ; --- Install W ($57) ---
    lda #$57
    ldx #<TableW
    ldy #>TableW
    jsr InstallOne
    
    ; Fix MEMLO once
    lda #$00
    sta $02E7
    lda #$42
    sta $02E8
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
    beq FoundEmpty ; Overwrite if exists
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
 
    
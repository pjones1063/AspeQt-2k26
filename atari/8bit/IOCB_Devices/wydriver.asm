; ==================================================================
; AspeQt Dual Device Handler (Y: & W:) - PLATINUM FIX (PATCHED)
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler) - Atari Dos 2.x
; MEMORY: Code at $2800, Data at $0600 (Page 6)
; FIXES:  SetupDCB_Open now preserves DAUX1/DAUX2 for ALL Ops
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

; =================================================================
; 1. OPEN ROUTINES
; =================================================================

; --- OPEN Y: (CLIPBOARD) ---
HandlerOpenY
    lda #$59        ; 'Y' (Fixed ID)
    sta CurrentDev
    
    jsr CommonReset
    lda #0          ; Y: is always Text Mode
    sta TransMode   
    
    jsr SetupDCB_Open
    
    ; Check Mode (Read/Write)
    lda $2A         ; ICAX1
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
    ; 1. GET EFFECTIVE UNIT NUMBER
    lda $0341,x     
    bne UnitOK_O    
    lda #1          ; Default to Unit 1
UnitOK_O
    sta DBYTLO      ; Save to Safe Temp
    
    ; 2. CALCULATE REVERSE ID
    lda #$58        
    sec
    sbc DBYTLO      
    sta CurrentDev  
    
    jsr CommonReset

    ; 3. Base Setup
    ; (This now auto-fills DAUX1/DAUX2 from $2A/$2B)
    jsr SetupDCB_Open
    
    ; Save Mode for Handler Logic
    lda $2B         ; Load ICAX2
    sta TransMode   ; SAVE MODE (0=Def, 1=Text, 2=Bin)
    
    ; 4. WE MUST SEND THE URL
    stx SaveX       
    lda $0344,x     ; ICBAL
    sta DBUFLO
    lda $0345,x     ; ICBAH
    sta DBUFHI
    
    ; 5. Force Write Mode for OPEN (Sending URL)
    lda #$80        
    sta DSTATS
    lda #$00
    sta DBYTLO      
    lda #$01        ; Length 256
    sta DBYTHI
    
    ; 6. Send Command
    jsr SIOV
    bmi OpenFail
    
    ; 7. If Read Mode, Refill Now
    lda $2A         
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
; 2. SHARED ROUTINES
; =================================================================

HandlerGet
    stx SaveX
    lda EOF_Flag
    beq FetchByte
    ldy #136        ; EOF Error
    sec             
    rts

FetchByte
    ldx BufPtr
    lda IOBuf,x
    
    ; --- BINARY SAFETY CHECK ---
    pha             ; Save Byte
    lda TransMode
    cmp #2          ; Is it Binary Mode?
    beq IsBinary    ; Yes -> Skip NULL check
    
    pla             ; Restore Byte (Text Mode)
    cmp #0          ; Is it NULL (EOF)?
    beq FoundNull   ; Yes -> Stop
    jmp GotByte

IsBinary
    pla             ; Restore Byte (Binary Mode)
    ; Fall through: 0x00 is valid data here

GotByte
    inc BufPtr
    bne GetDone

    ; Refill if page boundary hit
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
    ldy #136        ; EOF
    sec
    rts

HandlerPut
    stx SaveX
    ldx BufPtr
    sta IOBuf,x
    inc BufPtr
    bne PutSuccess
    
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
    
    ; Pad buffer with 0s
    ldx BufPtr
PadLoop
    lda #0
    sta IOBuf,x
    inx
    bne PadLoop
    jsr FlushBuffer

CloseCommit
    jsr SetupDCB_Close
    ldy #1
    clc
    rts


; =================================================================
; 3. SPECIAL COMMANDS (XIO)
; =================================================================
HandlerStat
    ldy #1
    clc
    rts

HandlerSpec
    stx SaveX       

    ; CHECK COMMAND
    lda $0342,x     
    cmp #$50        ; XIO 80?
    beq DoSpec      
    jmp SpecExit    

DoSpec
    ; --- EXECUTE XIO 80 (Fast Post) ---

    ; PREPARE BUFFER
    ldy #0
    lda #0
ClearLoop
    sta IOBuf,y
    iny
    bne ClearLoop

    ; COPY DATA
    ldx SaveX
    lda $0348,x     ; Length Low
    sta DBYTLO      
    
    lda $0344,x     ; Src Low
    sta SrcRead+1   
    lda $0345,x     ; Src High
    sta SrcRead+2   
    
    ldy #0
CopyLoop
    cpy DBYTLO      
    bcs CopyDone    
    
SrcRead
    lda $FFFF,y     ; Patch
    sta IOBuf,y     
    iny
    bne CopyLoop
CopyDone

    ; CALCULATE ID
    ldx SaveX       
    lda $0341,x     
    bne UnitOK_S    
    lda #1          
UnitOK_S
    sta DBYTLO      
    
    lda #$58        
    sec
    sbc DBYTLO      
    sta CurrentDev  

    ; SETUP SIO
    jsr SetupDCB_Open ; <--- AUTO FILLS DAUX2 FROM $2B
    
    lda DBYTLO      ; Unit
    sta DUNIT 

    lda #$50        ; Command 'P'
    sta DCOMND
    
    lda #$80        ; Write
    sta DSTATS
    
    lda #$3F        ; Timeout
    sta DTIMLO
    
    lda #1          ; Length 256
    sta DBYTHI
    lda #0
    sta DBYTLO
    
    ; SEND
    jsr SIOV
    bmi SpecFail
    
    ldx SaveX
    ldy #1
    clc
    rts

SpecFail
    ldx SaveX
    ldy #144        
    sec
    rts

SpecExit
    ldx SaveX       
    ldy #1          
    clc
    rts
    
; =================================================================
; 4. SIO HELPERS
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
    rts
FlushOK
    lda #0
    sta BufPtr
    ldy #1
    rts

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
    
    ; --- FIX START: LOAD FROM IOCB ZP ---
    ; This ensures DAUX1/2 are correct for Open, Read, Write, and XIO
    lda $2A         ; ICAX1 (Mode)
    sta DAUX1
    lda $2B         ; ICAX2 (Aux2)
    sta DAUX2
    ; --- FIX END ---

    lda #0
    sta DUNUSE
    lda #1
    sta DBYTHI      
    lda #1
    sta DUNIT       
    
    lda CurrentDev  
    sta DDEVIC
    rts

SetupDCB_Close
    jsr SetupDCB_Open
    lda #$43        ; Cmd 'C'
    sta DCOMND
    jsr SIOV
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
    
 
    
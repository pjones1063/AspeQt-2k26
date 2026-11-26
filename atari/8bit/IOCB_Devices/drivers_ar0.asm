; ==================================================================
; THE MASTER LOADER: 850 INTERFACE + Y: & W: HANDLERS
; Assembler: MADS and MyDos 4.53/4
; Features: $6000 Safe Haven, Internalized Buffer, MyDOS Proof
; ==================================================================

    icl "sym.asm"
	
; =================================================================
; 1. RESIDENT CODE & DATA (Moved to $6000 - Above DUP.SYS)
; =================================================================

     org $6000
    .align 256 

; Buffer is now INTERNAL to the protected code block!
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

TableY
    .word HandlerOpenY-1, HandlerClose-1, HandlerGet-1
    .word HandlerPut-1, HandlerStat-1, HandlerSpec-1

TableW
    .word HandlerOpenW-1, HandlerClose-1, HandlerGet-1
    .word HandlerPut-1, HandlerStat-1, HandlerSpec-1 

HandlerOpenY
    lda #$59
    sta CurrentDev
    jsr CommonReset
    lda #0
    sta TransMode   
    jsr SetupDCB_Open
    lda $2A
    and #$08
    bne DoOpenY
    jsr SIOV
    bmi OpenFail
    jsr RefillBuffer
    jmp OpenSuccess
DoOpenY
    jsr SIOV
    bmi OpenFail
    jmp OpenSuccess

HandlerOpenW
    lda $0341,x     
    bne UnitOK_O    
    lda #1
UnitOK_O
    sta DBYTLO      
    lda #$58        
    sec
    sbc DBYTLO      
    sta CurrentDev  
    
    lda $2A            
    sta CurrentMode    

    jsr CommonReset
    jsr SetupDCB_Open
    lda $2B
    sta TransMode   
    stx SaveX       
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
    and #$04
    beq OpenSuccess
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

HandlerGet
    stx SaveX
    lda EOF_Flag
    beq FetchByte
    ldy #136
    sec             
    rts
FetchByte
    ldx BufPtr
    lda IOBuf,x
    pha
    lda TransMode
    cmp #2
    beq IsBinary
    pla
    cmp #0
    beq FoundNull
    jmp GotByte
IsBinary
    pla
GotByte
    inc BufPtr
    bne GetDone
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
    ldx BufPtr
    sta IOBuf,x       ; Drop character into buffer
    
    ; --- THE LINE BUFFER HACK ---
    cmp #$9B          ; Was the character a Return?
    bne SkipFlush     ; No? Skip down to normal block buffering
    
    lda CurrentMode   ; Yes! Were we opened in Mode 12?
    cmp #12
    bne SkipFlush     ; No? Skip down to normal block buffering
    
    ; We have a Return in Mode 12. Force the flush!
    inc BufPtr        ; Move the pointer forward so the Return is included
    jsr FlushBuffer   ; Fire the SIO bus instantly to send the command!
    
    cpy #1            ; Did the flush succeed?
    bne PutCheckErr   ; If network error, bail out immediately
    
    lda #0
    sta EOF_Flag      ; Clear EOF flag from any previous reads
    
    jsr RefillBuffer  ; FETCH THE FRESH SERVER RESPONSE INTO IOBUF!
    
    jmp PutCheckErr   ; Check for SIO errors and exit
    ; ----------------------------
    
SkipFlush
    inc BufPtr        ; Normal block buffering: increment pointer
    bne PutSuccess    ; If it didn't wrap to 0, exit successfully
    jsr FlushBuffer   ; If it hit 256, flush it
    
PutCheckErr
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
    sta IOBuf,x
    inx
    bne PadLoop
    jsr FlushBuffer
CloseCommit
    jsr SetupDCB_Close
    ldy #1
    clc
    rts

HandlerStat
    ldy #1
    clc
    rts


HandlerSpec
    stx SaveX       

    ; 1. PREPARE BUFFER (Clear it with 0s)
    ldy #0
    lda #0
ClearLoop
    sta IOBuf,y
    iny
    bne ClearLoop

    ; 2. COPY DATA (The URL string from the IOCB)
    ldx SaveX
    lda $0348,x     ; Length Low
    sta DBYTLO      ; Safe to use here (SetupDCB hasn't run yet)
    
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

    ; 3. CALCULATE ID AND SAVE TO STACK
    ldx SaveX       
    lda $0341,x     ; Fetch Unit Number from IOCB
    bne UnitOK_S    
    lda #1          ; Default to Unit 1
UnitOK_S
    pha             ; PUSH UNIT TO STACK (Safe storage across subroutine)
    
    ; Calculate Reverse ID ($58 - Unit)
    sta DBYTLO      ; Temp store the Unit number for math
    lda #$58        
    sec
    sbc DBYTLO      ; Subtract the Unit number
    sta CurrentDev  

    ; 4. SETUP BASE SIO (Fills DAUX1/2, Buffers, etc.)
    jsr SetupDCB_Open 
    
    ; 5. OVERWRITE PARAMS FOR XIO COMMAND
    pla             ; PULL UNIT FROM STACK
    sta DUNIT       ; Safely store in SIO Unit register
    
    ldx SaveX
    lda $0342,x     ; Fetch the XIO Command again (ICCMD)
    sta DCOMND      ; OVERWRITE the 'O' left by SetupDCB_Open
    
    lda #$80        ; Write direction (Sending the URL string)
    sta DSTATS
    lda #$3F        ; Long Timeout
    sta DTIMLO
    
    lda #1          ; Length 256
    sta DBYTHI
    lda #0
    sta DBYTLO
    
    ; 6. FIRE SIO
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
    lda #$4F
    sta DCOMND
    lda #0
    sta DSTATS
    lda #<IOBuf
    sta DBUFLO
    lda #>IOBuf
    sta DBUFHI
    lda #$08
    sta DTIMLO
    lda #0
    sta DBYTLO
    lda $2A         
    sta DAUX1
    lda $2B         
    sta DAUX2
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
    lda #$43
    sta DCOMND
    jsr SIOV
    rts

SetupDCB_Read
    jsr SetupDCB_Open
    lda #$52
    sta DCOMND
    lda #$40
    sta DSTATS
    lda #$04
    sta DTIMLO
    jsr SIOV
    rts

SetupDCB_Write
    jsr SetupDCB_Open
    lda #$57
    sta DCOMND
    lda #$80
    sta DSTATS
    lda #$0A
    sta DTIMLO
    jsr SIOV
    rts

OnReset
    jsr CallOldDOSINI   

    jsr InitHandlersOnly 
    
    ; BUMP MEMLO
    lda MEMLO+1
    cmp #>EndHandler
    bcc do_bump         
    bne skip_bump       
    lda MEMLO
    cmp #<EndHandler
    bcs skip_bump       
do_bump
    lda #<EndHandler
    sta MEMLO
    lda #>EndHandler
    sta MEMLO+1
skip_bump
    rts                 

CallOldDOSINI
    jmp (OldDOSINI)     

Init
    lda DOSINI_V
    sta OldDOSINI
    lda DOSINI_V+1
    sta OldDOSINI+1
    
    lda #<OnReset
    sta DOSINI_V
    lda #>OnReset
    sta DOSINI_V+1
    rts

InitHandlersOnly
    lda #$59
    ldx #<TableY
    ldy #>TableY
    jsr InstallOne
    lda #$57
    ldx #<TableW
    ldy #>TableW
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

; =================================================================
; 2. 850 BOOTSTRAP (Moved to $6800)
; =================================================================
    org $6800

Boot850
    lda #$50
    sta DDEVIC
    lda #$01
    sta DUNIT
    lda #$3F
    sta DCOMND
    lda #$40
    sta DSTATS
    lda #$05
    sta DTIMLO
    sta DBUFHI      
    lda #$00
    sta DBUFLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    lda #$0C
    sta DBYTLO      
    jsr SIOV

    bpl copy_dcb
    rts

copy_dcb
    ldx #$0B
loop_dcb
    lda $0500,x     
    sta $0300,x     
    dex
    bpl loop_dcb

    jsr SIOV        
    bmi exit_850
    
    jsr $0506       

exit_850
    rts

; =================================================================
; 3. MASTER CONTROLLER (Moved to $6900)
; =================================================================
    org $6900

MasterStart
    jsr Boot850     
    jsr Init        
    jmp (DOSINI_V)  

; =================================================================
; 4. MYDOS AUTORUN HEADER
; =================================================================
    org $02E2
    .word MasterStart

    
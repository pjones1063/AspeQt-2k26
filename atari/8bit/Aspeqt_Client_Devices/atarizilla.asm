;  ATARIZILLA COMMANDER - MADS ASSEMBLY EDITION
;  Copyright 2026 AspeQt-2k26 Project
;
;  Requires: menu_sym.asm, printf.asm

    icl "sym.asm"

    org $4000

Start
    jmp MainProgram

; --- Data Buffers ---
TBuf        .ds 256
DBuf        .ds 256
NetBuf      .ds 257      ; Dedicated 256-byte SIO Buffer (padded for printf)
MaskBuf     .ds 20       
TBufLen     .byte 0
DBufLen     .byte 0
ErrCode     .byte 0      
LineCount   .byte 0      
RemBufPtr   .byte 0      ; Pointer for the 256-byte SIO buffer

StrWSFTP    .byte 'W:sftp://',0  ; FIXED: Lowercase scheme for QUrl
StrW        .byte 'W:',0
StrD        .byte 'D1:',0    

LocEOF      .byte 0
RemEOF      .byte 0

; ====================================================================
; STRING BUILDER SUBROUTINES
; ====================================================================

.proc ClearTBuf
    lda #0
    sta TBufLen
    sta TBuf            
    rts
.endp

.proc AppendTBufStr
    ldy #0
@   lda (Temp1),y
    beq _done
    ldx TBufLen
    sta TBuf,x
    inc TBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendTBufInput
    ldy #0
@   lda InputBuf,y
    cmp #155        
    beq _done
    ldx TBufLen
    sta TBuf,x
    inc TBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendTBufMask
    ldy #0
@   lda MaskBuf,y
    cmp #155        
    beq _done
    ldx TBufLen
    sta TBuf,x
    inc TBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendTBufChar
    ldx TBufLen
    sta TBuf,x
    inc TBufLen
    rts
.endp

.proc FinishTBuf
    ; FIXED: Terminate string immediately, then pad with nulls
    ldx TBufLen
    lda #155        
    sta TBuf,x
    inx
    lda #0          
_padLoop
    sta TBuf,x
    inx
    bne _padLoop    
    rts
.endp


.proc ClearDBuf
    lda #0
    sta DBufLen
    sta DBuf
    rts
.endp

.proc AppendDBufStr
    ldy #0
@   lda (Temp1),y
    beq _done
    ldx DBufLen
    sta DBuf,x
    inc DBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendDBufInput
    ldy #0
@   lda InputBuf,y
    cmp #155
    beq _done
    ldx DBufLen
    sta DBuf,x
    inc DBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendDBufMask
    ldy #0
@   lda MaskBuf,y
    cmp #155
    beq _done
    ldx DBufLen
    sta DBuf,x
    inc DBufLen
    iny
    bne @-
_done
    rts
.endp

.proc AppendDBufChar
    ldx DBufLen
    sta DBuf,x
    inc DBufLen
    rts
.endp

.proc FinishDBuf
    lda #155        
    jsr AppendDBufChar
    rts
.endp

; ====================================================================
; HARDWARE SIO HELPER ROUTINES
; ====================================================================

.proc SioW
    ; Core parameters shared across all W: commands
    lda #$57            ; DDEVIC: $57 is ATASCII 'W' - Required by AspeQt PipeNetwork
    sta DDEVIC
    lda #$01            ; DUNIT: 1
    sta DUNIT
    lda #$0F            ; DTIMLO: 15 second timeout
    sta DTIMLO
    jsr SIOV
    rts
.endp

.proc GetRemoteLine
    ; Assembles a line in TBuf by streaming 256-byte SIO blocks
    ldx #0
_loop
    lda RemBufPtr
    bne _readByte
    
    lda RemEOF
    bne _eof
    
    ; Fetch next 256-byte block via SIO ($52)
    lda #$52
    sta DCOMND
    lda #$40            ; FIXED: $40 is Read
    sta DSTATS
    lda #<NetBuf
    sta DBUFLO
    lda #>NetBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; FIXED: Explicitly clear accumulator for DAUX safety
    sta DAUX1           
    sta DAUX2
    jsr SioW
    cpy #1
    beq _readByte
    
    ; SIO Error / EOF
    lda #1
    sta RemEOF
    jmp _eof

_readByte
    ldy RemBufPtr
    lda NetBuf,y
    inc RemBufPtr       ; Naturally wraps 255 -> 0, triggering next block fetch
    
    cmp #0
    beq _block_end      ; Padding hit, block ended early

    cmp #155
    beq _line_done

    sta TBuf,x
    inx
    jmp _loop

_block_end
    lda #0
    sta RemBufPtr       ; Force fetch on next loop
    jmp _loop

_line_done
    lda #155
    sta TBuf,x
    rts

_eof
    lda #1
    sta RemEOF
    lda #155
    sta TBuf,x
    rts
.endp

; ====================================================================
; SCREEN RENDERING HELPERS
; ====================================================================

.proc PrintError
    sty ErrCode
    jsr printf
    .byte 155,'Error Code: %b',155,0
    .word ErrCode
    rts
.endp

.proc PrintCharScreen
    sty Temp2           
    tay                 
    ldx #$00
    lda #$0B            
    sta iccom,x
    lda #0
    sta icblen,x
    sta icblen+1,x
    tya                 
    jsr ciov
    ldy Temp2           
    rts
.endp

.proc PrintPadded19
    ldy #0
_loop
    lda (Temp1),y
    beq _pad            
    cmp #155
    beq _pad            
    jsr PrintCharScreen
    iny
    cpy #19
    bne _loop           
    rts
_pad
    lda #' '
    jsr PrintCharScreen
    iny
    cpy #19
    bne _pad            
    rts
.endp

; ====================================================================
; MAIN PROGRAM EXECUTION
; ====================================================================

MainProgram
    jsr printf
    .byte 125,'=== AtariZilla Commander Setup ===',155,0

    jsr ClearTBuf
    lda #<StrWSFTP
    sta Temp1
    lda #>StrWSFTP
    sta Temp1+1
    jsr AppendTBufStr

    jsr printf
    .byte 'Username: ',0
    jsr Input
    jsr AppendTBufInput

    lda #':'
    jsr AppendTBufChar

    jsr printf
    .byte 'Password: ',0
    jsr Input
    jsr AppendTBufInput

    lda #'@'
    jsr AppendTBufChar

    jsr printf
    .byte 'Host (IP:Port): ',0
    jsr Input
    jsr AppendTBufInput

    lda #'/'
    jsr AppendTBufChar

    jsr FinishTBuf

    jsr printf
    .byte 'Connecting to host...',155,0

    ; Execute SIO $29 (Change Directory) to initialize C++ path
    lda #$29
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; FIXED: DAUX1 must be 0 for $29 CD
    sta DAUX1           
    sta DAUX2           ; FIXED: Explicitly zero DAUX2 
    jsr SioW
    cpy #1
    jne _errInit        

MainMenu
    jsr printf
    .byte 125
    .byte 'AtariZilla Commander     Version 7.3',155
    .byte 'Copyright 2026 AspeQt-2k26 Project',155,155
    .byte 'Local (%s)           Remote (W:)',155
    .byte '1. Dual Dir List    A. View Text File',155
    .byte '2. Change Drive     B. Change Dir',155
    .byte '3. Rename File      C. Rename File',155
    .byte '4. Delete File      D. Delete File',155
    .byte '                    E. Download File',155
    .byte '                    F. Upload File',155
    .byte '                    G. Quit to DOS',155,155
    .byte 'Select item or [Return] for menu',155,0
    .word StrD

    jsr Input1
    lda InputBuf
    cmp #155
    jeq MainMenu

    jsr ToUpper

    ; --- LOCAL COMMANDS ---
    cmp #'1'
    jeq DoDualList
    cmp #'2'
    jeq DoChangeDrive
    cmp #'3'
    jeq DoLocalRename
    cmp #'4'
    jeq DoLocalDelete

    ; --- REMOTE COMMANDS ---
    cmp #'A'
    jeq DoViewText
    cmp #'B'
    jeq DoChangeDir
    cmp #'C'
    jeq DoRename
    cmp #'D'
    jeq DoDelete
    cmp #'E'
    jeq DoDownload
    cmp #'F'
    jeq DoUpload
    cmp #'G'
    jeq DoQuit

    jmp MainMenu

WaitKey
    jsr printf
    .byte 155,'Press Return to continue',0
    jsr Input1
    jmp MainMenu

_errInit
    jsr PrintError
    jsr printf
    .byte 155,'Connection Failed!',155
    .byte 'Press Return to retry.',0
    jsr Input1
    jmp Start

; ====================================================================
; THE DUAL-PANE COMMANDER ENGINE
; ====================================================================

DoDualList
    jsr printf
    .byte 125,'File Mask [*.*]: ',0
    jsr Input
    
    lda InputBuf
    cmp #155
    beq _useDefaultMask

    ldy #0
_copyMask
    lda InputBuf,y
    sta MaskBuf,y
    cmp #155
    beq _maskDone
    iny
    cpy #18             
    bne _copyMask
    lda #155
    sta MaskBuf,y
    jmp _maskDone

_useDefaultMask
    lda #'*'
    sta MaskBuf
    lda #'.'
    sta MaskBuf+1
    lda #'*'
    sta MaskBuf+2
    lda #155
    sta MaskBuf+3

_maskDone
    jsr printf
    .byte 155,'Fetching Dual Directories...',155,155,0

    lda #0
    sta LocEOF
    sta RemEOF
    sta LineCount
    sta RemBufPtr

    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    jsr AppendDBufMask
    jsr FinishDBuf

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufMask
    jsr FinishTBuf

    ; 1. Local Open (CIO)
    ldx #$10
    lda #$03            
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #6              
    sta icaux1,x
    lda #0              
    sta icaux2,x
    lda #0              
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _ddlLocErr      

    ; 2. Remote Open (SIO $4F)
    lda #$4F
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending URL payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #6              ; Dir Mode
    sta DAUX1
    lda #1              ; Text Mode
    sta DAUX2
    jsr SioW
    cpy #1
    jne _ddlRemErr      

_ddlLoop
    ; Fetch Local Line
    lda LocEOF
    jne _ddlFetchRem    
    ldx #$10
    lda #$05            
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #255            
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    jpl _ddlFetchRem    
    lda #1
    sta LocEOF
    lda #0
    sta DBuf            

_ddlFetchRem
    ; Fetch Remote Line
    lda RemEOF
    jne _ddlDraw        
    jsr GetRemoteLine
    
_ddlDraw
    lda LocEOF
    and RemEOF
    jne _ddlEnd         

    lda #<DBuf
    sta Temp1
    lda #>DBuf
    sta Temp1+1
    jsr PrintPadded19

    lda #'|'
    jsr PrintCharScreen

    lda #<TBuf
    sta Temp1
    lda #>TBuf
    sta Temp1+1
    jsr PrintPadded19

    lda #155
    jsr PrintCharScreen
    
    inc LineCount
    lda LineCount
    cmp #20             
    bne _ddlNext

    jsr printf
    .byte '- Press Return for more -',0
    jsr Input1          
    
    lda #155            
    jsr PrintCharScreen
    
    lda #0              
    sta LineCount

_ddlNext
    jmp _ddlLoop

_ddlEnd
    ; Local Close (CIO)
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov

    ; Remote Close (SIO $43)
    lda #$43
    sta DCOMND
    lda #$00            ; $00 is fine here, no payload
    sta DSTATS
    lda #0
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    jsr SioW

    jsr printf
    .byte 155,'- End of Directories -',155,0
    jmp WaitKey

_ddlLocErr
    jsr PrintError
    lda #1
    sta LocEOF
    jmp _ddlLoop

_ddlRemErr
    jsr PrintError
    lda #1
    sta RemEOF
    jmp _ddlLoop


; ====================================================================
; LOCAL MENU HANDLERS (D: - REMAIN ON CIO)
; ====================================================================

DoChangeDrive
    jsr printf
    .byte 155,'Enter Local Drive Number (1-8): ',0
    jsr Input1
    lda InputBuf
    cmp #155            
    jeq MainMenu
    cmp #'1'
    jcc _cdrvBad        
    cmp #'9'
    jcs _cdrvBad        

    sta StrD+1          
    
    jsr printf
    .byte 155,'Local Drive Changed!',155,0
    jmp WaitKey
_cdrvBad
    jsr printf
    .byte 155,'Invalid Drive!',155,0
    jmp WaitKey

DoLocalRename
    jsr printf
    .byte 155,'Old Local Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    jsr AppendDBufInput
    lda #','
    jsr AppendDBufChar
    
    jsr printf
    .byte 'New Local Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr AppendDBufInput
    jsr FinishDBuf

    jsr printf
    .byte 155,'Renaming on Local Drive...',155,0

    ldx #$10
    lda #$20            
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov
    jmi _errLocRN       
    jsr printf
    .byte 155,'Local rename successful!',155,0
    jmp WaitKey
_errLocRN
    jsr PrintError
    jmp WaitKey

DoLocalDelete
    jsr printf
    .byte 155,'Local Filename to Delete: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    jsr AppendDBufInput
    jsr FinishDBuf

    jsr printf
    .byte 155,'Deleting from Local Drive...',155,0

    ldx #$10
    lda #$21            
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov
    jmi _errLocDEL      
    jsr printf
    .byte 155,'Local delete successful!',155,0
    jmp WaitKey
_errLocDEL
    jsr PrintError
    jmp WaitKey


; ====================================================================
; REMOTE MENU HANDLERS (W: - CONVERTED TO PURE SIO)
; ====================================================================

DoChangeDir
    jsr printf
    .byte 155,"Enter remote dir ('..' to go up): ",0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    lda #$29            
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; FIXED: Explicitly clear accumulator
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errCD          
    
    jsr printf
    .byte 155,'Directory changed!',155,0
    jmp WaitKey
_errCD
    jsr PrintError
    jmp WaitKey


DoViewText
    jsr printf
    .byte 155,'Remote Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 155,'Downloading text...',155,0

    ; Remote Open (SIO $4F)
    lda #$4F
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending URL payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #4              ; Read
    sta DAUX1
    lda #1              ; Text
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errVT          

_vtLoop
    ; Remote Read (SIO $52)
    lda #$52
    sta DCOMND
    lda #$40            ; FIXED: $40 is Read (Fetching text chunk)
    sta DSTATS
    lda #<NetBuf
    sta DBUFLO
    lda #>NetBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; CLEAR ACCUMULATOR FIX
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    bne _vtEof
    
    ; Null terminate the block strictly for printf
    lda #0
    sta NetBuf+256

    jsr printf
    .byte '%s',155,0
    .word NetBuf
    jmp _vtLoop

_vtEof
    ; Remote Close (SIO $43)
    lda #$43
    sta DCOMND
    lda #$00
    sta DSTATS
    lda #0
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    jsr SioW

    jsr printf
    .byte 155,'- End of File -',155,0
    jmp WaitKey
_errVT
    jsr PrintError
    jmp WaitKey


DoRename
    jsr printf
    .byte 155,'Old Remote Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    lda #','
    jsr AppendTBufChar

    jsr printf
    .byte 'New Remote Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 155,'Renaming on W:...',155,0

    lda #$20            
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending old,new string payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; CLEAR ACCUMULATOR
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errRN          
    
    jsr printf
    .byte 155,'Remote rename successful!',155,0
    jmp WaitKey
_errRN
    jsr PrintError
    jmp WaitKey


DoDelete
    jsr printf
    .byte 155,'Remote Filename to Delete: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 155,'Deleting from W:...',155,0

    lda #$21            
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending filename payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; CLEAR ACCUMULATOR
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errDEL         
    
    jsr printf
    .byte 155,'Remote delete successful!',155,0
    jmp WaitKey
_errDEL
    jsr PrintError
    jmp WaitKey


; ====================================================================
; HIGH-SPEED BLOCK TRANSFER ROUTINES
; ====================================================================

DoDownload
    jsr printf
    .byte 155,'Remote Filename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 'Local Savename (%s): ',0
    .word StrD
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    jsr AppendDBufInput
    jsr FinishDBuf

    jsr printf
    .byte 155,'Downloading (high-speed binary)...',155,0

    ; Remote Open (SIO $4F)
    lda #$4F
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending URL payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #4
    sta DAUX1
    lda #2              ; Binary
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errDL          

    ; Local Open (CIO)
    ldx #$20
    lda #$03
    sta iccom,x
    lda #8
    sta icaux1,x
    lda #0
    sta icaux2,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #0              
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errDLClose1    

_dlLoop
    ; Remote Read (SIO $52)
    lda #$52
    sta DCOMND
    lda #$40            ; FIXED: $40 is Read (Fetching binary chunk)
    sta DSTATS
    lda #<NetBuf
    sta DBUFLO
    lda #>NetBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; CLEAR ACCUMULATOR 
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    bne _dlEof     

    ; Local Write (CIO PUT CHARACTERS)
    ldx #$20
    lda #$0B            
    sta iccom,x
    lda #<NetBuf
    sta icbadr,x
    lda #>NetBuf
    sta icbadr+1,x
    lda #0
    sta icblen,x
    lda #1
    sta icblen+1,x
    jsr ciov
    jmi _dlEnd          
    jmp _dlLoop

_dlEnd
    jsr PrintError
_dlEof
    ; Remote Close (SIO $43)
    lda #$43
    sta DCOMND
    lda #$00
    sta DSTATS
    lda #0
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    jsr SioW

    ; Local Close (CIO)
    ldx #$20
    lda #$0C
    sta iccom,x
    jsr ciov

    jsr printf
    .byte 155,'Download complete!',155,0
    jmp WaitKey

_errDLClose1
    jsr PrintError
    lda #$43
    sta DCOMND
    lda #$00
    sta DSTATS
    lda #0
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    jsr SioW
    jmp WaitKey

_errDL
    jsr PrintError
    jmp WaitKey


DoUpload
    jsr printf
    .byte 155,'Local Filename (%s): ',0
    .word StrD
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    jsr AppendDBufInput
    jsr FinishDBuf

    jsr printf
    .byte 'Remote Savename: ',0
    jsr Input
    lda InputBuf
    cmp #155            
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 155,'Uploading (high-speed binary)...',155,0

    ; Local Open (CIO)
    ldx #$10
    lda #$03
    sta iccom,x
    lda #4
    sta icaux1,x
    lda #0
    sta icaux2,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #0              
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errUL          

    ; Remote Open (SIO $4F)
    lda #$4F
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending URL payload)
    sta DSTATS
    lda #<TBuf
    sta DBUFLO
    lda #>TBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #8              ; Write
    sta DAUX1
    lda #2              ; Binary
    sta DAUX2
    jsr SioW
    cpy #1
    jne _errULClose1    

_ulLoop
    ; Local Read (CIO GET CHARACTERS)
    ldx #$10
    lda #$07            
    sta iccom,x
    lda #<NetBuf
    sta icbadr,x
    lda #>NetBuf
    sta icbadr+1,x
    lda #0              
    sta icblen,x
    lda #1
    sta icblen+1,x
    jsr ciov
    
    tya
    pha                 
    
    ; FIXED: Bulletproof Zero-Padding logic checking MSB first
    lda icblen+1,x      ; Check MSB of bytes read
    bne _sendUL         ; If MSB != 0 (256 bytes read), skip padding and send!
    
    ldy icblen,x        ; Check LSB of bytes read
    beq _ulCheckEOF     ; If MSB is 0 AND LSB is 0, EOF hit, no data to send.

_padLoop
    cpy #0
    beq _sendUL         ; Break loop when Y wraps from 255 -> 0
    lda #0
    sta NetBuf,y
    iny
    bne _padLoop        ; Continue until Y hits 0

_sendUL
    ; Remote Write (SIO $57)
    lda #$57
    sta DCOMND
    lda #$80            ; FIXED: $80 is Write (Sending binary chunk)
    sta DSTATS
    lda #<NetBuf
    sta DBUFLO
    lda #>NetBuf
    sta DBUFHI
    lda #0
    sta DBYTLO
    lda #1
    sta DBYTHI
    lda #0              ; CLEAR ACCUMULATOR
    sta DAUX1
    sta DAUX2
    jsr SioW
    cpy #1
    jne _ulEnd

_ulCheckEOF
    pla
    cmp #136            
    jeq _ulEof
    tax
    jmi _ulEnd
    jmp _ulLoop

_ulEnd
    jsr PrintError
_ulEof
    ; Local Close (CIO)
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov
    
    ; Remote Close (SIO $43)
    lda #$43
    sta DCOMND
    lda #$00
    sta DSTATS
    lda #0
    sta DBYTLO
    sta DBYTHI
    sta DAUX1
    sta DAUX2
    jsr SioW

    jsr printf
    .byte 155,'Upload complete!',155,0
    jmp WaitKey

_errULClose1
    jsr PrintError
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov
    jmp WaitKey

_errUL
    jsr PrintError
    jmp WaitKey


DoQuit
    jsr printf
    .byte 125,'Goodbye.',155,0
    jmp (DOSVEC)        

    icl 'printf.asm' 
    run Start
    
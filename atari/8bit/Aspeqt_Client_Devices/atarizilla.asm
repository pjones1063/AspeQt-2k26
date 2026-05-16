;  ATARIZILLA COMMANDER - MADS ASSEMBLY EDITION
;  Copyright 2026 AspeQt-2k26 Project
;
;  Requires: menu_sym.asm, printf.asm

    icl "menu_sym.asm"

    org $4000

Start
    jmp MainProgram

; --- Data Buffers ---
TBuf        .ds 256
DBuf        .ds 256
TBufLen     .byte 0
DBufLen     .byte 0
ErrCode     .byte 0      ; Required for printf %b to safely display CIO errors

StrWSFTP    .byte 'W:SFTP://',0
StrW        .byte 'W:',0
StrD        .byte 'D1:',0    ; <-- NOW DYNAMIC!

LocEOF      .byte 0
RemEOF      .byte 0

; ====================================================================
; STRING BUILDER SUBROUTINES
; ====================================================================

; --- W: Buffer (Network Drive) Helpers ---
.proc ClearTBuf
    lda #0
    sta TBufLen
    sta TBuf            ; Null terminate immediately
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

.proc AppendTBufChar
    ldx TBufLen
    sta TBuf,x
    inc TBufLen
    rts
.endp

.proc FinishTBuf
    lda #155        
    jsr AppendTBufChar
    rts
.endp

; --- D: Buffer (Local Drive) Helpers ---
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
; SCREEN RENDERING HELPERS
; ====================================================================

.proc PrintError
    ; Safely print CIO status from Y using printf library pointer
    sty ErrCode
    jsr printf
    .byte 155,'I/O Error: %b',155,0
    .word ErrCode
    rts
.endp

.proc PrintCharScreen
    ; Prints a single character in A to the screen (IOCB 0)
    sty Temp2           ; <-- FIX: Preserve Y because CIOV overwrites it!
    tay                 ; Move char to Y for CIO
    ldx #$00
    lda #$0B            ; PUT BYTE
    sta iccom,x
    lda #0
    sta icblen,x
    sta icblen+1,x
    tya                 ; Move char back to A
    jsr ciov
    ldy Temp2           ; <-- FIX: Restore Y so loops don't freeze
    rts
.endp

.proc PrintPadded19
    ; Prints exactly 19 chars from pointer Temp1. Pads with spaces.
    ldy #0
_loop
    lda (Temp1),y
    beq _pad            ; Stop at NULL
    cmp #155
    beq _pad            ; Stop at ATASCII Return
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

    ; Execute XIO 41 (Change Directory) to initialize C++ path
    ldx #$10
    lda #$29
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov

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


; ====================================================================
; THE DUAL-PANE COMMANDER ENGINE
; ====================================================================

DoDualList
    jsr printf
    .byte 125,'Fetching Dual Directories...',155,155,0

    ; 1. Setup Local Path (D*.*)
    jsr ClearDBuf
    lda #<StrD
    sta Temp1
    lda #>StrD
    sta Temp1+1
    jsr AppendDBufStr
    lda #'*'
    jsr AppendDBufChar
    lda #'.'
    jsr AppendDBufChar
    lda #'*'
    jsr AppendDBufChar
    jsr FinishDBuf

    ; 2. Setup Remote Path (W:*)
    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    lda #'*'
    jsr AppendTBufChar
    jsr FinishTBuf

    lda #0
    sta LocEOF
    sta RemEOF

    ; 3. Open D: (IOCB #1)
    ldx #$10
    lda #$03            ; OPEN
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #6              ; Dir Mode
    sta icaux1,x
    lda #0              
    sta icaux2,x
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _ddlLocErr      

    ; 4. Open W: (IOCB #2)
    ldx #$20
    lda #$03            ; OPEN
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #6              ; Dir Mode
    sta icaux1,x
    lda #1              
    sta icaux2,x
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _ddlRemErr      

_ddlLoop
    ; Fetch Local Line
    lda LocEOF
    jne _ddlFetchRem    
    ldx #$10
    lda #$05            ; GET RECORD
    sta iccom,x
    lda #<DBuf
    sta icbadr,x
    lda #>DBuf
    sta icbadr+1,x
    lda #255            ; Safe buffer size for text record
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    jpl _ddlFetchRem    
    lda #1
    sta LocEOF
    lda #0
    sta DBuf            ; Null out buffer on EOF

_ddlFetchRem
    ; Fetch Remote Line
    lda RemEOF
    jne _ddlDraw        
    ldx #$20
    lda #$05            ; GET RECORD
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #255            ; Safe buffer size for text record
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    jpl _ddlDraw        
    lda #1
    sta RemEOF
    lda #0
    sta TBuf            ; Null out buffer on EOF

_ddlDraw
    ; Check if BOTH are empty
    lda LocEOF
    and RemEOF
    jne _ddlEnd         

    ; Draw Left Pane (19 chars)
    lda #<DBuf
    sta Temp1
    lda #>DBuf
    sta Temp1+1
    jsr PrintPadded19

    ; Draw Divider (1 char)
    lda #'|'
    jsr PrintCharScreen

    ; Draw Right Pane (19 chars)
    lda #<TBuf
    sta Temp1
    lda #>TBuf
    sta Temp1+1
    jsr PrintPadded19

    ; Draw Newline
    lda #155
    jsr PrintCharScreen

    jmp _ddlLoop

_ddlEnd
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov

    ldx #$20
    lda #$0C
    sta iccom,x
    jsr ciov

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
; LOCAL MENU HANDLERS (D:)
; ====================================================================

DoChangeDrive
    jsr printf
    .byte 155,'Enter Local Drive Number (1-8): ',0
    jsr Input1
    lda InputBuf
    cmp #155            ; Empty Guard
    jeq MainMenu
    cmp #'1'
    jcc _cdrvBad        
    cmp #'9'
    jcs _cdrvBad        

    sta StrD+1          ; Overwrite the number in the StrD byte array!
    
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
    cmp #155            ; Empty Guard
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
    cmp #155            ; Empty Guard
    jeq MainMenu

    jsr AppendDBufInput
    jsr FinishDBuf

    jsr printf
    .byte 155,'Renaming on Local Drive...',155,0

    ldx #$10
    lda #$20            ; XIO 32
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
    cmp #155            ; Empty Guard
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
    lda #$21            ; XIO 33
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
; REMOTE MENU HANDLERS (W:)
; ====================================================================

DoChangeDir
    jsr printf
    .byte 155,"Enter remote dir ('..' to go up): ",0
    jsr Input
    lda InputBuf
    cmp #155            ; Empty Guard
    jeq MainMenu

    jsr ClearTBuf
    lda #<StrW
    sta Temp1
    lda #>StrW
    sta Temp1+1
    jsr AppendTBufStr
    jsr AppendTBufInput
    jsr FinishTBuf

    ldx #$10
    lda #$29            ; XIO 41
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov
    jmi _errCD          
    
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
    cmp #155            ; Empty Guard
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

    ldx #$10
    lda #$03            ; OPEN
    sta iccom,x
    lda #4              ; READ
    sta icaux1,x
    lda #1              ; TEXT
    sta icaux2,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errVT          

_vtLoop
    ldx #$10
    lda #$05            ; GET RECORD
    sta iccom,x
    lda #<IOBuf
    sta icbadr,x
    lda #>IOBuf
    sta icbadr+1,x
    lda #252            ; <-- FIX: IOBuf is 252 bytes in menu_sym.asm
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    
    tya
    pha                 ; Save CIO Status
    lda icblen,x
    beq _vtCheckEOF     ; Skip print if 0 bytes read
    
    ldy #0
@   lda IOBuf,y
    cmp #155
    beq @+
    iny
    bne @-
@   lda #0
    sta IOBuf,y

    jsr printf
    .byte '%s',155,0
    .word IOBuf

_vtCheckEOF
    pla                 ; Restore Status
    cmp #136
    jeq _vtEof
    tax
    jmi _errVT          ; Error if N flag set and not 136
    jmp _vtLoop

_vtEof
    ldx #$10
    lda #$0C            ; CLOSE
    sta iccom,x
    jsr ciov
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
    cmp #155            ; Empty Guard
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
    cmp #155            ; Empty Guard
    jeq MainMenu

    jsr AppendTBufInput
    jsr FinishTBuf

    jsr printf
    .byte 155,'Renaming on W:...',155,0

    ldx #$10
    lda #$20            ; XIO 32
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov
    jmi _errRN          
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
    cmp #155            ; Empty Guard
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

    ldx #$10
    lda #$21            ; XIO 33
    sta iccom,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              
    sta icaux1,x
    sta icaux2,x
    sta icblen,x        
    sta icblen+1,x
    jsr ciov
    jmi _errDEL         
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
    cmp #155            ; Empty Guard
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
    cmp #155            ; Empty Guard
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

    ldx #$10
    lda #$03
    sta iccom,x
    lda #4
    sta icaux1,x
    lda #2              
    sta icaux2,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errDL          

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
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errDLClose1    

_dlLoop
    ldx #$10
    lda #$07            ; GET CHARACTERS
    sta iccom,x
    lda #<IOBuf
    sta icbadr,x
    lda #>IOBuf
    sta icbadr+1,x
    lda #252            ; <-- FIX: IOBuf in menu_sym is 252 bytes
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    
    tya
    pha                 ; Save CIO status
    lda icblen,x
    bne _dlWrite        ; If lower byte > 0, write bytes
    beq _dlCheckEOF     ; If 0, nothing to write

_dlWrite
    ldx #$20
    lda #$0B            ; PUT CHARACTERS
    sta iccom,x
    lda #<IOBuf
    sta icbadr,x
    lda #>IOBuf
    sta icbadr+1,x
    ; icblen already contains exact bytes read from GET
    jsr ciov
    jmi _dlEnd          ; Write Error

_dlCheckEOF
    pla                 ; Restore status
    cmp #136            
    jeq _dlEof
    tax
    jmi _dlEnd          ; Any other error aborts
    jmp _dlLoop

_dlEnd
    jsr PrintError
_dlEof
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov
    ldx #$20
    lda #$0C
    sta iccom,x
    jsr ciov

    jsr printf
    .byte 155,'Download complete!',155,0
    jmp WaitKey

_errDLClose1
    jsr PrintError
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov
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
    cmp #155            ; Empty Guard
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
    cmp #155            ; Empty Guard
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
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errUL          

    ldx #$20
    lda #$03
    sta iccom,x
    lda #8
    sta icaux1,x
    lda #2              
    sta icaux2,x
    lda #<TBuf
    sta icbadr,x
    lda #>TBuf
    sta icbadr+1,x
    lda #0              ; Length 0 for OPEN
    sta icblen,x
    sta icblen+1,x
    jsr ciov
    jmi _errULClose1    

_ulLoop
    ldx #$10
    lda #$07            ; GET CHARACTERS
    sta iccom,x
    lda #<IOBuf
    sta icbadr,x
    lda #>IOBuf
    sta icbadr+1,x
    lda #252            ; <-- FIX: IOBuf in menu_sym is 252 bytes
    sta icblen,x
    lda #0
    sta icblen+1,x
    jsr ciov
    
    tya
    pha                 ; Save CIO status
    lda icblen,x
    bne _ulWrite
    beq _ulCheckEOF

_ulWrite
    ldx #$20
    lda #$0B            ; PUT CHARACTERS
    sta iccom,x
    lda #<IOBuf
    sta icbadr,x
    lda #>IOBuf
    sta icbadr+1,x
    ; icblen holds bytes read
    jsr ciov
    jmi _ulEnd

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
    ldx #$10
    lda #$0C
    sta iccom,x
    jsr ciov
    ldx #$20
    lda #$0C
    sta iccom,x
    jsr ciov

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

; ====================================================================
; INCLUDES
; ====================================================================
    icl 'printf.asm' 
    run Start
    
;=================================================;
;=          AspeQt Client utility v2.0            ;
;=        Copyright 2010 Ray. N. Ataergin         ;
;=================================================;
; Converted to MADS 6502 Syntax                   ;
; Expanded for AspeQt-2k26 Advanced SIO Commands  ;
; Refactored with Printf & Safe DOS checking      ;
;=================================================;

; Core Macros required by printf.asm and SDX
.macro stax
    sta :1
    stx :1+1
.endm

.macro ldax
    lda #< :1
    ldx #> :1
.endm

; Zero Page Usage (Legacy)
hlpadr      equ   $CB
rtclok      equ   $13
ptr         equ   $D0

; Zero Page Usage (Printf Requirements)
        org $80
Temp1           .ds 2
Temp2           .ds 2
Temp3           .ds 2
Temp4           .ds 2
LeadingZeroFlag .ds 1
ArgIndex        .ds 1
FieldWidth      .ds 1
StringIndex     .ds 1

; Various OS Vectors
dosvec      equ   $0A            
ciov        equ   $E456
siov        equ   $E459

; Device Control Block
ddevic      equ   $0300
dunit       equ   $0301
dcomnd      equ   $0302
dstats      equ   $0303
dbuflo      equ   $0304
dbufhi      equ   $0305
dtimlo      equ   $0306
dbytlo      equ   $0308
dbythi      equ   $0309
daux1       equ   $030A
daux2       equ   $030B

; I/O Control Block for Msg Display
iccom       equ   $0342
icbadr      equ   $0344
icptl       equ   $0346
icpth       equ   $0347
icblen      equ   $0348

; Hardware Registers
portb       equ   $D301

; SpartaDosX Kernel Area (Page 7)
sparta      equ   $700
spver       equ   $701
skernel     equ   $703
sdevic      equ   $761
sdate       equ   $77B
getsymbol   equ   $7EB

; SpartaDosX Vectors
i_settd     equ   $EB0

; SpartaDos User Area
comfnam     equ   33

; SpartaDos Vectors
vsettd      equ   $FFC3
vtdon       equ   $FFC6

        org $4000

;==============================
; Housekeeping & OS Check
;==============================
aStart
            jsr Printf
            .byte 155,'AspeQt CLI, (C)2012-2026 R.A & P.J.',155,0
            
            ; Check for SpartaDOS / RealDOS
            lda sparta            
            cmp #'S'                
            jeq aVers            
            cmp #'R'
            jeq aVers

            ; Not SpartaDOS (or no CLI), show help and exit cleanly
            jmp aNoSw        
                
;==============================
; Retrieve SpartaDos Version
;==============================
aVers
            lda spver            
            ldx #0            
aLoo1
            cmp aSpvI,x                
            jeq aFound
            inx
            cpx #6
            jne aLoo1
            jeq adispV
             
aFound
            lda aSpvo,x
            sta aVerNo
                        
adispV
;================================
; Get the command line switches
;================================            
            ; Explicitly build ZCRNAME jump vector 
            ; COMTAB+4 is LSB, COMTAB+5 is MSB
            ldy #4            
            lda (dosvec),y
            sta zcrname+1
            iny 
            lda (dosvec),y
            sta zcrname+2 
            
aLoo5       
            jsr zcrname
            bne aAna
            lda #1
            cmp aIfSw            
            jne aNoSw                    
            rts                            

;====================================
; Analyze the command line switches
;====================================
aAna
            lda #1
            sta aIfSw                    
            ldy #comfnam+3                
            lda (dosvec),y
            sta aSlSw                    
            cmp aTime                    
            jeq aDT1
            cmp aDisk                    
            jeq aDSK1
            cmp aPrin
            jeq aPR1
            jmp aNoSw
            
;==============================
; No command line switches
;==============================
aNoSw
            jsr aHelp
            jmp aExit                                                            

;====================================
; See if Date/Time options present
;====================================
aDT1
            iny
            ldx #0
            stx aSlSo
aLoo6
            lda (dosvec),y
            cmp aTopt,x
            jne aLoo7 
            lda aTopt,x            
            sta aSlSo                    
            jmp aDt2
            
aLoo7
            inx
            lda #$9B
            cmp aTopt,x
            jne aLoo6
            jeq aNoSw                    

;====================================
; See if Printer options present
;====================================
aPR1
            iny
            ldx #0
            stx aSlSo
aLoo12
            lda (dosvec),y
            cmp aPopt,x
            jne aLoo13 
            lda aPopt,x            
            sta aSlSo
            jmp aPr2
            
aLoo13
            inx
            lda #$9B
            cmp aPopt,x
            jne aLoo12
            jeq aNoSw

;====================================
; See if Disk options present
;====================================
aDsk1
            iny
            ldx #0
            stx aSlSo
aLoo8
            lda (dosvec),y
            cmp aDopt,x
            jne aLoo9 
            lda aDopt,x            
            sta aSlSo
            jmp aDsk2
            
aLoo9
            inx
            lda #$9B
            cmp aDopt,x
            jne aLoo8
            jeq aNoSw                     
            
;==================================            
; Printer Operations Requested
;==================================
aPr2
            jsr Printf
            .byte 155,'Setting Printer Server...',155,0
            
            jsr aSioInit
            ldx #9                  ; Comnd Index 9 = $9B
            lda aComnd,x
            sta dcomnd
            lda #0
            sta dstats
            sta daux1
            
            lda aSlSo
            cmp #'O'
            bne aPrOff
            lda #1
            sta daux2
            jmp aPrGo
aPrOff      lda #0
            sta daux2
aPrGo
            jsr siov
            lda dstats
            cmp #1
            jeq aSucc7
            
            jsr Printf
            .byte 155,'Printer Toggle Failed!',155,0
            rts
aSucc7
            jsr Printf
            .byte 155,'Printer Server Toggled',155,0
            jmp aLoo5

;==================================            
; Date/Time Requested
;==================================
aDt2                     
            jsr Printf
            .byte 155,'Polling server for Date/Time..',155,0
    
;==================================
; Get the Date/Time from the server
;==================================
            jsr aSioInit
            ldx #0
            lda aComnd,x            
            sta dcomnd
            lda #64                    
            sta dstats
            lda #6
            sta dbytlo
            lda #0
            sta dbythi
            sta daux1
            sta daux2
            jsr siov
                    
            lda dstats
            cmp #1
            jeq aSucc1
            
;=================================                                        
; Failed to receive Date/Time
;=================================        
aErr1
            jsr Printf
            .byte 155,'Server offline/did not respond!',155,0
            rts                            

;===================================                        
; Successfuly received the Date/Time
;===================================
aSucc1                
            jsr Printf
            .byte 155,'Date received from server..',155,0
            
;===================================
; Determine which vectors to use
; depending on SD or SDX
;===================================
            lda aVerNo
            cmp #'4'
            jeq aSDX1                                    
            
;============================
; Set Date/Time in COMTAB+13
;============================
aComtab
            ldx #0
            ldy #13
aLoo3
            lda aBuf,x
            sta (dosvec),y
            inx
            iny
            cpx #6
            jne aLoo3
            
aPortb                         ;SD
            lda portb
            pha
            and #254
            sta portb
            clc
            jsr vsettd
            pla
            sta portb                                                
            jcc aOK
            jcs aNOk
aSDX1                         ;SDX
            ldx #0
aLoo4
            lda aBuf,x
            sta sdate,x
            inx
            cpx #6
            jne aLoo4
                        
            ldy #$65
            lda #$10
            sta sdevic
            jsr skernel
            
            cmp #0
            jeq aOK
            
;==============================
; Failed to Set the Date/Time            
;==============================
aNOk
            jsr Printf
            .byte 155,'Failed to set Time/Date!',155,0      
            rts                            
            
;===============================
; Successfuly set the Date/Time
;===============================
aOK
            jsr Printf
            .byte 155,'Time/Date is set.',155,0
            
;===================================
; See if we turn the TD Line ON/OFF
;===================================
            lda aSlSo
            cmp #'S'                        
            jne aNotS
            jmp aLoo5                    

;===================================
; Determine which vectors to use
; depending on SD or SDX
;===================================
aNotS
            lda aVerNo
            cmp #'4'
            jeq aSDX2                                    
            
            lda portb
            pha
            and #254
            sta portb
            clc
            jsr vtdon
            pla
            sta portb
            jcs aErr2
            jmp aLoo5                    

aSDX2
            ldax aitdon                    
            jsr getsymbol                
            jmi aErr2                    

            stax ptr
    
            ldy #1
            lda aSlSo
            cmp #'O'                     
            jeq aCont7
            ldy #0
aCont7            
            jmp (ptr)
            
;================================
; Failed to turn TD line ON/OFF
;================================
aErr2
            jsr Printf
            .byte 155,'Failed to turn TD line ON/OFF!'
            .byte 155,'Check if TDLINE is loaded',155,0                           
            rts                            
                        
;==================================            
; Disk Image Operation Requested
;==================================
aDsk2
            lda aSlSo
;==================================
; Disk Swap
;==================================
            cmp #'S'
            jne aDsk3
            iny
            lda (dosvec),y
            sta aSwD1
            iny
            lda (dosvec),y
            sta aSwD2
            
            jsr Printf
            .byte 155,'Server to swap disks %c-%c',155,0
            .word aSwD1, aSwD2
            
;==================================
; Send disk swap information
;==================================
            jsr aSioInit
            ldx #1
            lda aComnd,x
            sta dcomnd
            lda #0                        
            sta dstats
            lda aSwD1
            sec
            sbc #48
            sta daux1                    
            lda aSwD2                    
            sec
            sbc #48
            sta daux2                    
            jsr siov
                    
            lda dstats
            cmp #1
            jeq aSucc2
            
            jsr Printf
            .byte 155,'Disk swap failed!',155,0
            rts
aSucc2
            jsr Printf
            .byte 155,'Disks swapped',155,0
            jmp aLoo5                    
            
;===============================
; Disk Unmount
;===============================
aDsk3       cmp #'U' 
            jne aDsk4
            iny
            lda (dosvec),y
            sta aUnMD1
            cmp #'*' 
            jeq aUMAll
            
            jsr Printf
            .byte 155,'Server to unmount disk %c',155,0
            .word aUnMD1
            jmp aCont0
aUMAll
            jsr Printf
            .byte 155,'Server to unmount all disks',155,0
aCont0             
            
;==================================
; Send disk unmount information
;==================================
            jsr aSioInit
            ldx #2
            lda aComnd,x
            sta dcomnd
            lda #0                        
            sta dstats
            lda #0
            sta daux1
            lda #'*' 
            cmp aUnMD1
            jeq aCont1            
aSingleD
            lda aUnMD1
aCont1
            sec
            sbc #48
            sta daux2                                        
            jsr siov
                            
            lda dstats
            cmp #1
            jeq aSucc3
aErr4
            jsr Printf
            .byte 155,'Disk unmount failed!',155,0
            rts
aSucc3
            jsr Printf
            .byte 155,'Disk(s) unmounted',155,0
            jmp aLoo5                    
            
;=========================================
; Image Mount / Create a new Image & Mount
;=========================================
aDsk4       cmp #'M'
            jeq aDsk41
            cmp #'N'
            jeq aDsk41
            jmp aDsk5            
aDsk41
            lda #0
            sta aDot
            jsr aSioInit
            
            ldx #0
            iny
aLoo10
            lda (dosvec),y
            cmp #$9B
            jeq aCont2                    
            sta aBuf,x
            cmp #'.'
            jne aCont8
            lda aDot
            clc
            adc #1
            sta aDot
aCont8
            lda aDot
            cmp #2
            jeq aCont6
            lda aBuf,x
            sta aMountF,x 
aCont6
            iny
            inx
            jmp aLoo10
aCont2            
            lda #0
            sta aMountF,x       ; Null terminate for printf
            
            jsr Printf
            .byte 155,'Server to mount: %s',155,0
            .word aMountF
            
;==================================
; Send disk mount information
;==================================            
            lda aSlSo
            cmp #'M'
            jeq aCont4
            ldx #4
            ldy #14
            jmp aCont5
aCont4
            ldx #3
            ldy #12
aCont5
            lda aComnd,x
            sta dcomnd            
            sty dbytlo
            lda #0
            sta dbythi
            lda #128                    
            sta dstats
            lda #0
            sta daux1
            sta daux2
            jsr siov
            
            lda dstats
            cmp #1
            jeq aSucc4

            jsr Printf
            .byte 155,'Image mount failed! Check server logs',155,0
            rts
            
;====================================
; Disk mounted, now get drive number
;====================================
aSucc4
            jsr aSioInit
            
            ldx #11             ; Index 11 = $9D command
            lda aComnd,x
            sta dcomnd
            lda #1
            sta dbytlo
            lda #0              ; Cleanly send DAUX 0
            sta daux1
            sta dbythi
            sta daux2
            lda #64             ; Read Request             
            sta dstats
            jsr siov
                        
            lda dstats
            cmp #1
            jeq aSucc5

            jsr Printf
            .byte 155,'Image mounted, failed to get drive#',155,0
            rts
aSucc5            
            ldx #0
            lda aBuf,x
            clc
            adc #49
            sta aMountN
            sec
            sbc #58
            jmi aCont3
            lda aMountN
            clc
            adc #16
            sta aMountN
aCont3
            jsr Printf
            .byte 155,'Image mounted to drive: %c',155,0
            .word aMountN
            jmp aLoo5                    
            
;====================================
; Toggle Auto-Commit ON/OFF
;====================================
aDsk5
            cmp #'A'
            jeq aDsk51
            jmp aDsk6                    
aDsk51
            iny
            lda (dosvec),y
            sta aTogg1

            jsr Printf
            .byte 155,'Server to toggle Auto-Commit on D%c:',155,0
            .word aTogg1
            
;==================================
; Send Auto-Commit Information
;==================================
            jsr aSioInit
            ldx #5
            lda aComnd,x
            sta dcomnd
            lda #0                        
            sta dstats
            lda aTogg1
            sec
            sbc #48
            sta daux1                    
            lda #0
            sta daux2
            jsr siov
                    
            lda dstats
            cmp #1
            jeq aSucc6
            
            jsr Printf
            .byte 155,'Auto-Commit toggle failed',155,0
            rts
aSucc6
            jsr Printf
            .byte 155,'Auto-Commit toggled',155,0
            jmp aLoo5

;====================================
; Save Disks
;====================================
aDsk6
            cmp #'V'
            jeq aDsk61
            jmp aDsk7
aDsk61
            iny
            lda (dosvec),y
            sta aSavD1
            cmp #'*'
            jeq aSvAll
            
            jsr Printf
            .byte 155,'Server to save disk %c',155,0
            .word aSavD1
            jmp aCont10
aSvAll
            jsr Printf
            .byte 155,'Server to save all disks',155,0
aCont10     
            jsr aSioInit
            ldx #6
            lda aComnd,x
            sta dcomnd
            lda #0
            sta dstats
            sta daux1
            lda #'*'
            cmp aSavD1
            jeq aCont11
            lda aSavD1
aCont11     
            sec
            sbc #48
            sta daux2
            jsr siov
            
            lda dstats
            cmp #1
            jeq aSucc8
            
            jsr Printf
            .byte 155,'Save disk failed!',155,0
            rts
aSucc8      
            jsr Printf
            .byte 155,'Disk(s) saved',155,0
            jmp aLoo5
            
;====================================
; Display Host Path
;====================================
aDsk7
            cmp #'P'
            jeq aDsk71
            jmp aDsk8
aDsk71
            jsr aSioInit
            ldx #7
            lda aComnd,x
            sta dcomnd
            lda #64             ; Read Request
            sta dstats
            lda #$FF
            sta dbytlo
            lda #0
            sta dbythi
            sta daux1
            sta daux2
            jsr siov
            
            lda dstats
            cmp #1
            jeq aSucc9
            
            jsr Printf
            .byte 155,'Failed to get path!',155,0
            rts
aSucc9      
            jsr aTermBuf
            jsr Printf
            .byte 155,'Host Path:',155,'%s',155,0
            .word aBuf
            jmp aLoo5

;====================================
; Display Slot Filename
;====================================
aDsk8
            cmp #'L'
            jeq aDsk81
            jmp aDsk9
aDsk81
            iny
            lda (dosvec),y
            sta aLstD1
            
            jsr aSioInit
            ldx #8
            lda aComnd,x
            sta dcomnd
            lda #64             ; Read Request
            sta dstats
            lda #$20
            sta dbytlo
            lda #0
            sta dbythi
            lda aLstD1
            sec
            sbc #48
            sta daux1
            lda #0
            sta daux2
            jsr siov
            
            lda dstats
            cmp #1
            jeq aSucc10
            
            jsr Printf
            .byte 155,'Failed to get slot info!',155,0
            rts
aSucc10     
            jsr aTermBuf
            jsr Printf
            .byte 155,'%s',155,0
            .word aBuf
            jmp aLoo5
            
;====================================
; Boot Image & Reboot
;====================================
aDsk9
            cmp #'B'
            jeq aDsk91
            jmp aNoSw
aDsk91
            lda #0
            sta aDot
            jsr aSioInit
            
            ldx #0
            iny
aLoo14
            lda (dosvec),y
            cmp #$9B
            jeq aCont12
            sta aBuf,x
            iny
            inx
            jmp aLoo14
aCont12
            jsr Printf
            .byte 155,'Server mounting for boot...',155,0
            
            ldx #10
            lda aComnd,x
            sta dcomnd
            lda #12             ; Filename write buffer
            sta dbytlo
            lda #0
            sta dbythi
            lda #128            ; Write Request
            sta dstats
            lda #1              ; Force mount to D1: for boot
            sta daux1
            lda #0
            sta daux2
            jsr siov
            
            lda dstats
            cmp #1
            jeq aSucc11
            
            jsr Printf
            .byte 155,'Boot mount failed!',155,0
            rts
aSucc11     
            jmp $E477           ; Cold Start (Reboot) Atari
                    

;===================================================================;
;          S   U   B   R   O   U   T   I   N   E   S                ;
;===================================================================;

; Terminate buffer with 0 for Printf %s instead of $9B
aTermBuf
            ldx #0
aTB1        lda aBuf,x
            beq aTB2
            cmp #$9B
            beq aTB2
            inx
            cpx #$FF
            bne aTB1
aTB2        lda #0
            sta aBuf,x
            rts

aHelp
            jsr Printf
            .byte 155,'Usage: ASPECL <cmd> [options]',155,155
            .byte ' TS     Set Time/Date',155
            .byte ' TO/TF  Set Time & TD Line ON/OFF',155
            .byte ' PO/PF  Printer Server ON/OFF',155
            .byte ' DM[f]  Mount Existing Image [f]',155
            .byte ' DN[f.x]Mount New Image [f] type [x]',155,0
            
            jsr Printf
            .byte '        x: 1=SDFD   2=EDFD   3=DDFD',155
            .byte '           4=DSDDFD 5=DDHD   6=QDHD',155
            .byte ' DU[d/*]Unmount disk [d] / All [*]',155
            .byte ' DS[dd] Swap Disks',155
            .byte ' DA[d/*]Auto-commit disk[d] / All[*]',155,0
            
            jsr Printf
            .byte ' DV[d/*]Save disk [d] / All [*]',155
            .byte ' DP     Display Host Path',155
            .byte ' DL[d]  Display Slot Filename',155
            .byte ' DB[f]  Mount [f] & Reboot',155,155,0
            rts
            
aSioInit
            lda aDevic                    
            sta ddevic
            lda aUnit
            sta dunit
            lda #<aBuf
            sta dbuflo
            lda #>aBuf
            sta dbufhi
            
            lda #0
            ldx #0
aLoo11                            
            sta aBuf,x
            inx
            bne aLoo11
            rts
                    
aExit        
            jsr Printf
            .byte 155,155,'Press RETURN to exit',155,0
            ldx #0          ; Use IOCB 0 explicitly
            lda #$05        ; Command 5 (Get Record)
            sta iccom,x
            lda #$01
            sta icblen,x
            lda #0
            sta icblen+1,x
            jsr ciov
            jmp (dosvec)                    

zcrname
            jmp $FFFF

;===============================================================;
; DATA STRUCTURES                                               ;
;===============================================================;

aSpvI   dta c'%23@CD'
aSpvO   dta c'233444'
aIfSw   dta $0                                    
aSlSw   dta $0                                    
aSlSo   dta $0                                    

aTime   dta c'T'                                    
aTopt   dta c'SOF', $9B                            
aDisk   dta c'D'                                    
aDopt   dta c'MNUSAVPLB', $9B                            
aPrin   dta c'P'
aPopt   dta c'OF', $9B

aDevic  dta $46                                    
aUnit   dta $01                                    

; SIO Commands Index: Index 11 added for GetDrvNum ($9D)
; 0=Time($93)  1=Swap($94) 2=Unmnt($95) 3=Mnt($96) 4=New($97)
; 5=A-Cmt($98) 6=Save($99) 7=Path($9C)  8=Slot($92) 9=Prt($9B) 10=Boot($9A) 11=GetDrvNum($9D)
aComnd  dta $93, $94, $95, $96, $97, $98, $99, $9C, $92, $9B, $9A, $9D

aCmd    dta $09                                    
aBlen   dta $FF                                    
aitdon  dta a(I_tdon)
I_tdon  dta c'I_TDON  ', $08                        
        dta a(0)
aBuf    .ds $FF                                         
aDot    .ds $01                                        

; Dynamic Printf String Storage
aVerNo  .ds 1
aSwD1   .ds 1
aSwD2   .ds 1
aUnMD1  .ds 1
aMountF .ds 16
aMountN .ds 1
aTogg1  .ds 1
aSavD1  .ds 1
aLstD1  .ds 1

InputBuf .ds 255

        icl 'printf.asm'

; Run Address
        org $02E0
        dta a(aStart)
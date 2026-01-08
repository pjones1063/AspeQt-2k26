
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; ** 
 
.proc  Menu
	jsr printf
	.byte 'A List Slot       J Auto Commit On',155
	.byte 'B List Disk Image K Auto Commit Off',155
	.byte 'C Create Disk     L Set Date',155,0
    jsr printf	
    .byte 'D Mount Disk      M TD Line On',155
	.byte 'E UnMount Disk    N TD Line Off',155
	.byte 'F Save Disk       O Start Printer',155,0
	jsr printf
	.byte 'G Swap Slot       P Stop Printer',155
	.byte 'H Boot .ATR       Q AspeQT Path',155
	.byte 'I Boot .XEX       R Host Command',155,0
	jsr printf
	.byte '                  S Exit to DOS',155
	.byte '                  T Reboot',155,0
.endp	
	
	
.proc Main	
    jsr printf
	.byte 155,155,'Enter Command or Return for Menu: ',0
	jsr Input1
	bcs Main	
	jsr ToUpper
	
	cmp #'A'			// A List Slots 
    jeq SlotName    
    cmp #'B'			// B List Host Images 
    jeq ListDir
    cmp #'C'			// C Create Disk
    jeq CreateAndMount	
	cmp #'D'			// D Mount Disk 
    jeq Mount
	cmp #'E'			// E UnMount Disk
    jeq UnMount    
    cmp #'F'			// F Save Disk
    jeq Save
    cmp #'G'			// G Swap Slot 
    jeq Swap
    cmp #'H'			// H Boot Disk
    jeq BootATR
    cmp #'I'   			// I Boot XEX/Exe 
    jeq BootXEX   
    cmp #'J' 			// J Auto Commit On   
    jeq CommitOn
    cmp #'K'  			// K Auto Commit Off  
    jeq CommitOff  
    cmp #'L'			// L Set Date
    jeq GetTD
    cmp #'M'			// M TD Line On
    jeq GetTDOn
    cmp #'N'			// N TD Line On
    jeq GetTDOff
    cmp #'O'			// O TD Line On
    jeq SetPrintOn
    cmp #'P'			// P TD Line On
    jeq SetPrintOff	
    cmp #'Q'			// Q Display host path
    jeq GetHostPath	
	cmp #'R'			// R Host Command
    jeq RunHostCommand
	cmp #'S'			// S Exit to Dos
    jeq Exit
	cmp #'T'			// T Cold Reboot
    jeq Reboot

	jmp Start
	
.endp	
	
.proc init
      rts
.endp  	
	
//	
//  get disk in slot
//    
.proc SlotName
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*] : ',0
	
    jsr input1
    jcs main
    jsr toUpper   
    jsr GetDrvWC
    bcc ok1
    lda #$FA 
ok1    
    sta drive
	cmp #$FA
	bne OneSlot
MultiSlot
    ldx #$01
    stx IOLastFile
LOOPa
    txa  
    sta drive
    jsr getSlotFileName
    adb IOLastFile #01
    ldx IOLastFile
    cpx #$0A
    bne LOOPa

    ldx #$1A
    stx IOLastFile
LOOPb
    txa  
    sta drive
    jsr getSlotFileName
    adb IOLastFile #01
    ldx IOLastFile
    cpx #$20
    bne LOOPb
    jmp Main       
OneSlot
    jsr getSlotFileName
    jmp Main
.endp

.proc getSlotFileName
   lda #DCB.GetSL
   jsr SetUpDCB
   mva drive DAUX1
   jsr SIOV
   bpl OKa
   jsr Printf
   .byte 155,'No server response!',0
   sec
   rts
OKa
    adb drive #$30
    jsr Printf
	.byte 155,' %c %s',0
	.word Drive, Path
	clc
	rts	
.endp

//
//
//
.proc GetHostPath
   lda #DCB.GetHostPath
   jsr SetUpDCB
   jsr SIOV
   bpl OKp
   jsr Printf
   .byte 155,'No server response!',0
   sec
   jmp main
OKp
   jsr Printf
   .byte 155,'Remote Folder Path:',155,'%s',0
   .word IOBuf
   clc
   jmp main	
.endp


//
//	Mount disk and boot!
//
.proc BootATR
	ldx #0
	stx ArgFlag
	inx
	stx Drive
	clc
	bcc MountAndBoot
.endp

.proc BootXEX
	ldx #1
	stx ArgFlag
	stx Drive
	clc
.endp

.proc  MountAndBoot
    jsr printf
	.byte 155,'Enter [FILENAME.xxx] : ',0
    jsr input
    jcs main
    cpy #03
    jmi main    
    ldx #0
loopB   
    lda InputBuf,x
	cmp #155
    beq alldoneE
StoreB        
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne loopB
alldoneE 
    lda #0
	sta IOBuf,x   
.endp 
      
.proc DoMountAndBoot
	lda #DCB.MountAndBoot	
	jsr SetUpDCB
	mva Drive   DAUX1
	mva ArgFlag DAUX2
	jsr SIOV
	bpl OKB
	jsr Printf
	.byte 155,'Error mounting image!',155,0
	sec
	jmp main
OKB			; image mounted
	lda Drive  
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 155,155,'Image mounted on slot %c ',0	
	.word DriveID1
	jmp Reboot
.endp
 

//------------------------------------------------------
//
//	Mount disk
//
.proc Mount
	lda #0
	sta CreateFlag
	clc
	jmp doMount
.endp

//
//	Create and mount
//
.proc CreateAndMount
    jsr printf
    .byte 155,'  [1] SSSD  [2] SSED  [3] SSDD'
	.byte 155,'  [4] DSDD  [5] DDHD  [6] QDHD'
	.byte 155,155,'Enter Disk Type : ',0
	jsr input1
    jcs main
    tay
    cmp #'6'+1
	jcs main
    sbc #'0'-1
	jcc main
	jeq main	
    sty CreateFlag 	
.endp


.proc DoMount
    ldy #0
    sty  ArgFlag
noDot1
    jsr printf
	.byte 155,'Enter [FILENAME.ATR]: ',0
    jsr input
    jcs main
    cpy #03
    jmi main    
    ldx #0
loop1    
    cpx #13
    beq noDot1
    lda InputBuf,x
	cmp #155
    beq FlFin1
    cmp #'.'
    bne Store1
    ldy #01
    sty ArgFlag
Store1        
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne Loop1
FlFin1
    ldy ArgFlag
    cpy #0
    beq noDot1
    ldy CreateFlag
    cpy #0
    beq AllDone1
    lda #'.' 
    sta IOBuf,x
    inx
    tya
    sta IOBuf,x
    inx
AllDone1
	lda #0
	sta IOBuf,x       	

    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] : ',0	
    jsr input1
    jcs main
    jsr toUpper   
    jsr GetDrvWC
    jcs main
    sta Drive

    lda #DCB.Mount
	ldy CreateFlag
	cpy #0
	beq goMount
	lda #DCB.CreateAndMount
goMount
	jsr SetUpDCB  
    mva Drive   DAUX1
	jsr SIOV
	bpl OK3
	jsr Printf
	.byte 155,'Error mounting image!',155,0
	sec
	jmp main
OK3		
	lda Drive	
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 'Mounted on drive %c',155,0
	.word DriveID1
	clc
	jmp main
.endp


//
//	Unmount disk
//
.proc UnMount
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*]: ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrvWC
	jcs main	
	sta Drive
	lda #DCB.Unmount
	jsr SetUpDCB
	mva Drive DAUX2
	jsr SIOV
	bpl OK6	
	jsr Printf
	.byte 155,'Error unmounting volume!',155,0
	sec
	jmp main
OK6
	lda Drive
	cmp #-6		 
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 155,'Drive %c unmounted',155,0
	.word DriveID1
	clc
	jmp main
AllDrives
	jsr Printf
	.byte 155,'All drives unmounted',155,0
	clc
	jmp main
.endp

	
//
//	Save disks
//
.proc Save
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*]: ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrvWC
	jcs main	
	sta Drive
	lda #DCB.Save
	jsr SetUpDCB
	mva Drive DAUX2
	jsr SIOV
	bpl OK7	
	jsr Printf
	.byte 155,'Nothing can be saved!',155,0
	sec
	jmp main
OK7
	jsr Printf
	.byte 155,'Disk(s) saved',155,0
	clc	
	jmp main
.endp
		
//
//	Disk Swap
//
.proc Swap
    jsr printf
    .byte 155,'Enter Slot 1  [1-9] [J-O]: ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrv
	jcs main 
	sta Slot
	jsr MakeDriveID	; make some drive IDs for later
	sta DriveID1

    jsr printf
    .byte 155,'Enter Slot 2  [1-9] [J-O]: ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrv
	jcs main
	sta Drive
	jsr MakeDriveID
	sta DriveID2

	lda #DCB.Swap
	jsr SetUpDCB
	mva Slot DAUX1
	mva Drive DAUX2
	jsr SIOV
	bpl OK1
	jsr Printf
	.byte 155,'Error swapping disks!',155,0
	sec
	jmp Main
OK1
	jsr Printf
	.byte 155,'Swapped disk %c with %c',155,0
	.word DriveID1,DriveID2
	clc	
	jmp Main
.endp

//
//  Toggle Auto Commit
//
.proc CommitOn
      LDY #01
      jmp ToggleCommit
.endp      
.proc Commitoff
      LDY #00
.endp      
.proc ToggleCommit
    sty ArgFlag
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*]: ',0
    jsr input1
    jcs main
    jsr toUpper 
	jsr GetDrvWC
	jcs main
	sta Drive
	lda #DCB.AutoToggle
	jsr SetUpDCB
	mva Drive DAUX1 
	mva ArgFlag DAUX2
	jsr SIOV
	bpl OK2
	jsr Printf
	.byte 155,'Error toggling auto-commit!',155,0
	sec
	jmp Main
OK2
	lda Drive
	cmp #-6
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 155,'Auto-commit toggled on drive %c',155,0
	.word DriveID1
	clc
	jmp main
	
AllDrives
	jsr Printf
	.byte 155,'Auto-commit toggled on all drives',155,0
	clc
	jmp main
	.endp


//
//  List pth folder 
//
// 
.proc ListDir
   lda #DCB.GetHostPath
   jsr SetUpDCB
   jsr SIOV
   bpl OKp
   jsr Printf
   .byte 155,'No server response!',0
   sec
   jmp main
OKp
   jsr Printf
   .byte 155,'Remote Folder Path:',155,'%s',155,0
   .word IOBuf
    clc
    lda #$00
    sta IOLastFile + 1
    sta IOLastFile
    jsr printf
	.byte 155,'Enter Filter [*]: ',0
    jsr input
    jcs main
    ldx #0
loop2    
    lda InputBuf,x
	cmp #155
    beq FlFin2
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne Loop2
FlFin2
	lda #0
	sta IOBuf,x   	   
    lda #DCB.SetDR
    jsr SetUpDCB
    mva #$00 DAUX1    
    mva #$00 DAUX2
    jsr SIOV
    bpl list2
    jsr Printf
    .byte 155,'No server response!',155,0
    sec
    jmp Main
list2    
    lda #DCB.GetDR
    jsr SetUpDCB
    mva IOLastFile+1   DAUX1
    mva IOLastFile     DAUX2
    jsr SIOV
    bpl OK2a
    jsr Printf
    .byte 155,'No server response!',155,0
    sec
    jmp Main
OK2a
    lda IOFileOption 
	cmp #$00
    jeq allDone2
    sta SelectB
    jsr Printf
    .byte 155,'%s',155,0
    .word IOBuf
	jsr Printf
	.byte 'Select [A-%c] [Q=quit] [more]: ',0
	.word SelectB
	jsr getkey
	jsr ToUpper
	cmp #'Q'
	jeq allDone2
	cmp #155
	jeq next2
	
    sta ArgFlag
    lda #DCB.PutDR
    jsr SetUpDCB
    mva ArgFlag DAUX1
	mva #0      DAUX2
    jsr SIOV
    bpl OK2b
    jsr Printf
    .byte 155,'No server response!',155,0
    sec
    jmp Main
OK2b
    lda IOBuf
    cmp #'$'
    jeq ListDir   	
    cmp #'@'
    jeq allDone2   	
getSlot
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] : ',0	
    jsr input1
    bcc ok1
	lda #$31	
ok1
    jsr toUpper   
    jsr GetDrvWC
    sta Drive
    jmp DoMountAndBoot

next2       
 	ldx IOLastFile + 1
 	cpx #00
 	jne list2
 	ldx IOLastFile
 	cpx #00 
 	jne list2 	 
allDone2
	clc
	jmp Main	
.endp

.proc SetPrintOn
	ldy #01
	jmp SetPrint
.endp	

.proc SetPrintOff     
    ldy #00
.endp

.proc setPrint    
    sty ArgFlag
	lda #DCB.Print
	jsr SetUpDCB
	mva ArgFlag DAUX2
	jsr SIOV
	bpl OK1
	jsr Printf
	.byte 155,'No server response!',155,0
	sec
	jmp Main
OK1
    lda ArgFlag
    beq OK2
   	jsr Printf
	.byte 155,'Print server on',155,0   
    jmp Main
OK2    
    jsr Printf
	.byte 155,'Print server off',155,0
	jmp Main
.endp
 

.proc RunHostCommand       
   lda #DCB.GetCmd
   jsr SetUpDCB
   jsr SIOV
   bpl OKr1
   jsr Printf
   .byte 155,'No server response!',0
   sec
   jmp main
OKr1
   jsr Printf
   .byte 155,'$ [%s]',0
   .word IOBuf
    clc
    jsr input   
    ldx #0
loopr1    
    lda InputBuf,x
	cmp #155
    beq FlFinr1
    sta IOBuf,x
    inx
    bne Loopr1
FlFinr1
	lda #0
	sta IOBuf,x       	

	lda #DCB.RunCmd
	jsr SetUpDCB
	jsr SIOV
	bpl OKr2
	jsr Printf
	.byte 155,'No server response!',155,0
	sec
	jmp Main
OKr2
   lda #DCB.GetTxt
   jsr SetUpDCB
   jsr SIOV
   bpl OKr3
   jsr Printf
   .byte 155,'No server response!',0
   sec
   jmp main
OKr3
   jsr Printf
   .byte '%s',0
   .word IOBuf
    lda IOFileOption 
	cmp #$00   
    beq alldone
//   jsr Printf
//	.byte 155,'[more]: ',0
//	jsr getkey
     jmp OKr2
alldone    
   	jsr Printf
	.byte 155,155,'Command complete',155,0   
    jmp Main
.endp


//
//	Time/Date Set
//
.proc GetTD
    jsr isSparta
    jcs main
	jsr SetTD	
	jmp Main
.endp

//
//  Time set, enable TD line
//
.proc GetTDOn
    jsr isSparta
    jcs main
	jsr SetTD
	bcs Abort1
	ldy #1
	jsr TDLineOnOff
Abort1
	jmp Main
.endp


//
//	Time set, disable TD line
//
.proc GetTDOff
    jsr isSparta
    jcs main
	jsr SetTD
	bcs Abort2
	ldy #0
	jsr TDLineOnOff
Abort2	
    jmp Main
 .endp 


//
//	Read date and time from the server and set the clock
//
.proc SetTD
	lda #DCB.GetTD
	jsr SetUpDCB
	jsr SIOV
	bpl OK5
	jsr Printf
	.byte 155,'No server response!',155,0
	sec
	rts
	
OK5	; got date and time from server, so attempt to set Sparta clock
	jsr Printf
	.byte 155,'Date received from server',155,0
	lda $701
	cmp #$44
	bcs IsSDX2
	ldx #5
	ldy #$0D+$05
@
	lda IOBuf,x
	sta (comtab),y
	dey
	dex
	bpl @-
	
	lda portb
	pha
	and #$FE
	sta portb
	jsr I_SETTD	; this will fail if the vectors aren't there
	pla
	sta portb
	bcc TDSetOK
	bcs TDSetFailed
	
IsSDX2			; with SDX, use kernel
	ldx #5
@
	lda IOBuf,x
	sta $077B,x
	dex
	bpl @-
	
	ldy #$65
	
	lda #$10
	sta $0761
	jsr $0703
	beq TDSetOK
	
TDSetFailed
	jsr Printf
	.byte 155,'Error setting time/date!',155,0
	sec
	rts
	
TDSetOK
	
	jsr Printf
	.byte 155,'Time/date set %b/%b/20%b',155,0
	.word IOBuf+1
	.word IOBuf 
	.word IOBuf+2
	clc
	rts
.endp



//
//	Enable or disable the TD Line
//	Y = 0: Turn off
//	Y = 1: Turn on
//
.proc TDLineOnOff
	lda $701
	cmp #$44
	bcs IsSDX1
	
	lda portb
	pha
	and #$FE
	sta portb
	jsr I_TDON
	pla
	sta portb
	bcs TDOnFailed
	rts
	
IsSDX1
	sty Temp2
	ldax Symbol	; under SDX, enable TD using kernel
	jsr $07EB	; get symbol address
	bmi TDOnFailed

	stax TDVec+1	; store address
	ldy Temp2
TDVec
	jsr $FFFF	; should really check for errors...
	clc
	rts
	
TDOnFailed
	jsr Printf
	.byte 155,'Error enabling TD Line!',155,0
	sec
	rts
.endp


.proc isSparta 
	lda $0700
	cmp #'S'
	beq ok
	cmp #'R'
	beq ok
	jsr Printf
	.byte 'Not Sparta or Real Dos',155,0
	sec
	rts
ok 
    clc
    rts
.endp		
	
	
//
//	Get drive spec and allow '*'
//
.proc GetDrvWC
	sec
	.byte $24
.endp

//
//	Get drive spec, disallowing '*'
//
.proc GetDrv
	clc
.endp

.proc GetDriveID
	ror Temp1	; wildcard flag
	bit Temp1
	bpl @+
	cmp #'*'
	bne @+
	lda #$FA	; '*' - $30
	clc
	rts
@
	cmp #'9'+1
	bcs NotDigit
	sbc #'0'-1
	bcc Bad
	beq Bad		; disallow 0
	clc
	rts
NotDigit
	cmp #'A'	; handle A-I for 1-9
	bcc Bad
	cmp #'J'
	bcs @+
	sbc #'A'-2	; carry is clear
	clc
	rts
@
	cmp #'O'+1
	bcs Bad
	sbc #'0'-1	; carry is clear, so subtract one less
	bcc Bad
OK
	clc
	rts
Bad
	sec
Abort
	rts
.endp

.proc Exit
	jsr Printf
	.byte 155,'Press a key to quit',155,0
	jsr Input1
	jmp (DOSVEC)
.endp

	
.proc Reboot
	jsr Printf
	.byte 155,'Enter Y to reboot'
	.byte 155,' * You may need to press [Option]: ',0	
	jsr Input1
	cmp #'Y'
	jne main
	jmp $E477
.endp
	
.proc GetKey
	lda $E425
	pha
	lda $E424
	pha
	rts
.endp
	
.proc MakeDriveID
	clc
	adc #'0'	; everything gets $30 added
	rts	
.endp

.proc SetUpDCB
	tay
	ldx DCBIndex,y	; we could multiply by 10 then add nine, but a table is easier
	ldy #9
Loop
	lda DCBTable,x
	sta DComnd,y
	dex
	dey
	bpl Loop
	mva #$46 DDevic	; ddevic and dunit are common to all
	mva #$01 DUnit
	rts
	
.proc Crunch
	jmp $FFFF
.endp
	

DCBIndex
	.byte 9,19,29,39,49,59,69,79,89,99,109,119,129,139,149,159,169,179
.endp

DCBTable

DCBGetCmd
	.byte Cmd.GetCmd	
	.byte $40
	.word IOBuf
	.byte $06,$00
	.word $FF
	.byte $00,$00	
DCBRunCmd
	.byte Cmd.RunCmd
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $20
	.byte $00,$00
DCBTxtCmd
	.byte Cmd.GetTxt 
	.byte $40
	.word IOBuf
	.byte $06,$00
	.word $FF
	.byte $00,$00
DCBSetDR 
	.byte Cmd.SetDR
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $20	
	.byte $00,$00
DCBPutDR
	.byte Cmd.PutDR
	.byte $40
	.word IOBuf
	.byte $08,$00
	.word $FF	
	.byte $00,$00
DCBGetDR
	.byte Cmd.GetDR	  
	.byte $40		   
	.word IOBuf        
	.byte $08,$00	   
	.word $FF	  
	.byte $00,$00		  
DCBGetSL
	.byte Cmd.GetSL	  
	.byte $40		  
	.word Path        
	.byte $06,$00	  
	.word $20		  
	.byte $00,$00	  
DCBGetTD
	.byte Cmd.GetTD	
	.byte $40		
	.word IOBuf		
	.byte $06,$00	
	.word $06		
	.byte $00,$00	
DCBSwap
	.byte Cmd.Swap
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBUnmount
	.byte Cmd.Unmount
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBMount
	.byte Cmd.Mount
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0C
	.byte $00,$00
DCBCreateAndMount
	.byte Cmd.CreateAndMount
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0E
	.byte $00,$00
DCBAutoToggle
	.byte Cmd.AutoToggle
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBGetDrvNum
	.byte Cmd.Mount
	.byte $40
	.word IOBuf
	.byte $06,$00
	.word $01
	.byte $01,$00
DCBSave
	.byte Cmd.Save
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00	
DCBMountAndBoot
	.byte Cmd.MountAndBoot
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0C
	.byte $00,$00	
DCBPrint
	.byte Cmd.Print	
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00	
DCBGetPath
	.byte Cmd.GetHostPath	
	.byte $40
	.word IOBuf
	.byte $08,$00
	.word $FF
	.byte $00,$00	
				
Symbol
	.byte 'I_TDON  ',0

	

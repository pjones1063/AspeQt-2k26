;=================================================;
;=          AspeQt Client utility v1.0            ;
;=        Copyright 2010 Ray. N. Ataergin		  ;
;=================================================;
; This program and its source is released under:  ;
;           GNU GENERAL PUBLIC LICENSE            ;
; Please read the details in the file LICENCE.TXT ;
; included with AspeQT distribution archive       ;
;=================================================;
; This version of the AspeQT Client will only     ;
; work with Spartados versions > 2.5 and with all ;
; versions of SpartaDos X.                        ;
;=================================================;

; Zero Page Usage
hlpadr      =   $CB
rtclok      =   $13
ptr         =   $D0

; Various OS Vectors
dosvec		=	$0A			
ciov		= 	$E456
siov		=	$E459

; Device Control Block
ddevic		=	$0300
dunit		=	$0301
dcomnd		=	$0302
dstats		=	$0303
dbuflo		=	$0304
dbufhi		=	$0305
dtimlo      =   $0306
dbytlo		=	$0308
dbythi		=   $0309
daux1		=	$030A
daux2		=	$030B

; I/O Control Block for Msg Display
iocb0		=	$0340
iccom0		=	$0342
icbal0		=	$0344
icbah0		=	$0345
icbll0		=	$0348
icblh0		=	$0349

; Hardware Registers
portb		=	$D301

; SpartaDosX Kernel Area (Page 7)
sparta		=	$700
spver		=	$701
skernel		=	$703
sdevic		=	$761
sdate		=	$77B
getsymbol 	= 	$7EB

; SpartaDosX Vectors
i_settd     =   $EB0

; SpartaDos User Area
comfnam		= 	33

; SpartaDos Vectors
vsettd		=	$FFC3
vtdon		=	$FFC6



		* =	$3000

;==============================
; Housekeeping
;==============================
aStart
			lda #<aHlp
			sta hlpadr
			lda #>aHlp
			sta hlpadr+1

;==============================
; Display Copyright			
;==============================
			jsr aEol
									
			ldx #<aCpyl		
			ldy #>aCpyl						
			jsr aPrint
			jsr aEol			
			
;==============================
; Check for SpartaDos
;==============================
			lda sparta			
			cmp #83				
			beq aVers			

;==============================
; Not SpartaDos
;==============================
			ldx #<aNotSP			
			ldy #>aNotSP			
			jsr aPrint
			pla
			pla
			jmp	aExit		
				
;==============================
; Retrieve SpartaDos Version
;==============================
aVers
			lda spver			
			ldx #0			
aLoo1
			cmp aSpvI,x				
			beq aFound
			inx
			cpx #6
			bne	aLoo1
			beq	adispV
			 
aFound
			lda aSpvo,x
			sta aVerNo
						
adispV
			ldx #<aSparta		
			ldy #>aSparta		
			jsr aPrint

;================================
; Get the command line switches
;================================			
			ldy	#5
			ldx	#2
aLoo2			
			lda (dosvec),y
			sta zcrname,x
			dey
			dex
			bne aLoo2
aLoo5		jsr zcrname
			bne aAna
			lda #1
			cmp aIfSw			
			bne aNoSw					;No command line switches were specified, exit with help display
			rts							;All requests were fullfilled exit program

;====================================
; Analyze the command line switches
;====================================
aAna
			lda #1
			sta aIfSw					;Set switches found flag
			ldy #comfnam+3				;Get switches from the command line
			lda (dosvec),y
			sta aSlSw					;Store switch for later use
			cmp aTime					;Date/Time?
			beq aDT1
			cmp aDisk					;Disk?
			beq aDSK1
			
;==============================
; No command line switches
;==============================
aNoSw
			jsr aEol
			jsr aHelp
			rts							;Exit Program															

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
			bne aLoo7 
			lda aTopt,x			
			sta aSlSo					;Store selected switch option for later use
			jmp	aDt2
			
aLoo7
			inx
			lda #$9B
			cmp aTopt,x
			bne aLoo6
			beq aNoSw					;Date/Time Options not specified, exit with help display

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
			bne aLoo9 
			lda aDopt,x			
			sta aSlSo
			jmp	aDsk2
			
aLoo9
			inx
			lda #$9B
			cmp aDopt,x
			bne aLoo8
			beq aNoSw 					;Disk Options not specified, exit with help display
			

;==================================			
; Date/Time Requested
;==================================
aDt2
			jsr aEol						
			ldx #<aPollDt			
			ldy #>aPollDt		
			jsr aPrint	
	
;==================================
; Get the Date/Time from the server
;==================================
			jsr aSioInit
			ldx #0
			lda aComnd,x			; Date/Time
			sta dcomnd
			lda #64					; We send the request, we get back the date & time
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
			beq aSucc1
			
;=================================										
; Failed to receive Date/Time
;=================================		
aErr1
			ldx #<aFail			
			ldy #>aFail			
			jsr aPrint			
			rts							; Exit program

;===================================						
; Successfuly received the Date/Time
;===================================
aSucc1				
			ldx #<aDtRcvd			
			ldy #>aDtRcvd				
			jsr aPrint
			
;===================================
; Determine which vectors to use
; depending on SD or SDX
;===================================
			lda aVerNo
			cmp #'4
			beq aSDX1									
			
;============================
; Set Date/Time in COMTAB+13
;============================
aComtab
			ldx	#0
			ldy	#13
aLoo3
			lda aBuf,x
			sta (dosvec),y
			inx
			iny
			cpx #6
			bne aLoo3
			
aPortb 						;SD
			lda	portb
			pha
			and #254
			sta	portb
			clc
			jsr vsettd
			pla
			sta portb												
			bcc aOK
			bcs aNOk
aSDX1 						;SDX
			ldx #0
aLoo4
			lda aBuf,x
			sta sdate,x
			inx
			cpx #6
			bne aLoo4
						
			ldy #$65
			lda #$10
			sta sdevic
			jsr skernel
			
			cmp #0
			beq aOK
			
;==============================
; Failed to Set the Date/Time			
;==============================
aNOk
 			ldx #<aFailT0			
			ldy #>aFailT0				
			jsr aPrint		
			rts							; Exit Program
			
;===============================
; Successfuly set the Date/Time
;===============================
aOk
			ldx #<aSetDt			
			ldy #>aSetDt					
			jsr aPrint
			
;===================================
; See if we turn the TD Line ON/OFF
;===================================
			lda aSlSo
			cmp #'S						; Set Date/Time
			bne	aNotS
			jmp aLoo5					; Get next switch

;===================================
; Determine which vectors to use
; depending on SD or SDX
;===================================
aNotS
			lda aVerNo
			cmp #'4
			beq aSDX2									
			
			lda	portb
			pha
			and #254
			sta	portb
			clc
			jsr vtdon
			pla
			sta portb
			bcs aErr2
			jmp aLoo5					; Get next switch

aSDX2
			lda aitdon					; Get address for SDX Symbol I_TDON (Date/Time ON/OFF)
			ldx aitdon+1
			jsr getsymbol				; returns the address in A and X registers
			bmi aErr2					; Returns NEGATIVE if unsuccessful (does not seem to work for Date/Time)

			sta ptr
			stx ptr+1
	
			ldy #1
			lda aSlSo
			cmp #'O 					; Set Date/Time & TD Line ON
			beq aCont7
			ldy #0
aCont7			
			jmp (ptr)
			
			jmp aLoo5
			
;================================
; Failed to turn TD line ON/OFF
;================================
aErr2
			jsr aEol
			ldx #<aFailT1			
			ldy #>aFailT1			
			jsr aPrint			
			ldx #<aFailT2			
			ldy #>aFailT2			
			jsr aPrint			
			rts							; Exit Program
						
;==================================			
; Disk Image Operation Requested
;==================================
aDsk2
			lda aSlSo
;==================================
; Disk Swap
;==================================
			cmp #'S
			bne aDsk3
			iny
			lda (dosvec),y
			sta aSwD1
			iny
			lda (dosvec),y
			sta aSwD2
			jsr aEol
			ldx #<aSwap			
			ldy #>aSwap		
			jsr aPrint
			
;==================================
; Send disk swap information
;==================================
			jsr aSioInit
			ldx #1
			lda aComnd,x
			sta dcomnd
			lda #0						; No data required from server
			sta dstats
			lda aSwD1
			sec
			sbc #48
			sta daux1					; First drive# to swap
			lda aSwD2					; Send Date/Time
			sec
			sbc #48
			sta daux2					; Second drive# to swap
			jsr siov
					
			lda dstats
			cmp #1
			beq aSucc2
			
			ldx #<aFailD1
			ldy #>aFailD1
			jsr aPrint
			rts
aSucc2
			ldx #<aSwaped
			ldy #>aSwaped
			jsr aPrint
			jmp aLoo5					; Get next switch
			
;===============================
; Disk Unmount
;===============================
aDsk3		cmp #'U 
			bne aDsk4
			iny
			lda (dosvec),y
			sta aUnMD1
			cmp #'* 
			beq aUMAll
			jsr aEol
			ldx #<aUnMnt			
			ldy #>aUnMnt
			jmp aCont0
aUMAll
			jsr aEol
			ldx #<aUnMntA
			ldy #>aUnMntA
aCont0     		
			jsr aPrint
			
;==================================
; Send disk unmount information
;==================================
			jsr aSioInit
			ldx #2
			lda aComnd,x
			sta dcomnd
			lda #0						; No data required from server
			sta dstats
			lda #0
			sta daux1
			lda #'* 
			cmp aUnMD1
			beq acont1			
aSingleD
			lda aUnMD1
aCont1
			sec
			sbc #48
			sta daux2					; Drive# to unmount or * for all drives					
			jsr siov
							
			lda dstats
			cmp #1
			beq aSucc3
aErr4
			ldx #<aFailD2
			ldy #>aFailD2
			jsr aPrint
			rts
aSucc3
			ldx #<aUnmtD
			ldy #>aUnmtD
			jsr aPrint
			jmp aLoo5					; Get next switch
			
;=========================================
; Image Mount / Create a new Image & Mount
;=========================================
aDsk4		cmp #'M
			beq aDsk41
			cmp #'N
			beq aDsk41
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
			beq aCont2					; An EOL stops filename crunch
			sta aBuf,x
			cmp #'.
			bne aCont8
			lda aDot
			clc
			adc #1
			sta aDot
aCont8
			lda aDot
			cmp #2
			beq aCont6
			lda aBuf,x
			sta aMountF,x 
aCont6
			iny
			inx
			jmp aLoo10
aCont2			
			ldx #<aMount			
			ldy #>aMount	
			jsr aPrint
			
;==================================
; Send disk mount information
;==================================			
			lda aSlSo
			cmp #'M
			beq aCont4
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
			lda #128					; We send the filename
			sta dstats
			lda #0
			sta daux1
			sta daux2
			jsr siov
			
			lda dstats
			cmp #1
			beq aSucc4

			ldx #<aFailD3
			ldy #>aFailD3
			jsr aPrint
			rts
;====================================
; Disk mounted, now get drive number
;====================================
aSucc4
			jsr aSioInit
			
			ldx #3
			lda aComnd,x
			sta dcomnd
			lda #1
			sta dbytlo
			sta daux1
			lda #0
			sta dbythi
			sta daux2
			lda #64					; We get the drive number
			sta dstats
			jsr siov
						
			lda dstats
			cmp #1
			beq aSucc5

			ldx #<aFailD4
			ldy #>aFailD4
			jsr aPrint
			rts
aSucc5			
			ldx #0
			lda aBuf,x
			clc
			adc #49
			sta aMountN
			sec
			sbc #58
			bmi  aCont3
			lda aMountN
			clc
			adc #16
			sta aMountN
aCont3
			ldx #<aMountD
			ldy #>aMountD
			jsr aPrint
			jmp aLoo5					; Get next switch
			
;====================================
; Toggle Auto-Commit ON/OFF
;====================================
aDsk5
			cmp #'A
			beq aDsk51
			jmp aNoSw					; No valid command line switches specified
aDsk51
			iny
			lda (dosvec),y
			sta aTogg1

			jsr aEol
			ldx #<aToggle			
			ldy #>aToggle		
			jsr aPrint
			
;==================================
; Send Auto-Commit Information
;==================================
			jsr aSioInit
			ldx #5
			lda aComnd,x
			sta dcomnd
			lda #0						; No data required or sent
			sta dstats
			lda aTogg1
			sec
			sbc #48
			sta daux1					; Drive# or all drives (*)
			lda #0
			sta daux2
			jsr siov
					
			lda dstats
			cmp #1
			beq aSucc6
			
			ldx #<aFailD5
			ldy #>aFailD5
			jsr aPrint
			rts
aSucc6
			ldx #<aToggld
			ldy #>aToggld
			jsr aPrint
			jmp aLoo5					; Get next switch
					
;===================================================================;
;          S   U   B   R   O   U   T   I   N   E   S                ;
;===================================================================;

;===========						
; Print EOL
;===========
aEol
			ldx #<aCr
			ldy #>aCr
			jsr aPrint
            rts
			
;=================================
; Display all available help text 
;=================================
aHelp			
			ldy #$FF
aLo99		
			iny
			lda (hlpadr),y
			sta aHlpBuf,y
			cmp #$9B
			bne aLo99
			iny
			tya
			clc
			adc hlpadr
			sta hlpadr
			bcc aHlp1
			lda #1
			clc
			adc hlpadr+1
			sta hlpadr+1
aHlp1		
			ldx #<aHlpBuf
			ldy #>aHlpBuf
			jsr aPrint
			ldy #0
			lda (hlpadr),y			
 			cmp #$FF
 			beq aHlp2			
			bne aHelp
aHlp2
			rts
			
;===============================			
; Print a line on the screen
;===============================
aPrint
			txa
			ldx #$00		
			sta	icbal0,x
			tya			
			sta icbah0,x
			lda aCmd
			sta iccom0,x					
			lda aBlen				
			sta icbll0,x
			jsr ciov
			rts
			
;===============================
; Set-up DCB (SIO)
;===============================
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
aLoo11							; Clear data buffer		
			sta aBuf,x
			inx
			bne aLoo11
			rts
					
;================================
; Exit for non-sparta DOS
;================================	
aExit		
			ldx #<aExi		
			ldy #>aExi				
			jsr aPrint
			lda #$05
			sta iccom0,x
			lda #$01
			sta icbll0,x
			jsr ciov
			jmp (dosvec)					
			rts
zcrname
			jmp $FFFF

;===============================================================;
; DATA STRUCTURES                                               ;
;===============================================================;
aCpyl	.byte "AspeQt CLient, (C) 2012 Ray Ataergin", $9B
aNotSP	.byte "Only SpartaDOS v2.5 and up is supported!", $9B
aSparta	.byte "SpartaDos v"
aVerNo	.byte "?.xx"
aRunin  .byte " detected", $9B
aPollDt	.byte "Polling server for Date/Time..", $9B
aDtRcvd	.byte "Date received from server..", $9B
aSetDt	.byte "Time/Date is set.", $9B
aFail	.byte "Server offline/did not respond/NAKed!", $9B
aFailT0	.byte "Failed to set Time/Date!", $9B
aFailT1	.byte "Failed to turn TD line ON/OFF!", $9B
aFailT2 .byte "Check if TDLINE is loaded", $9B
aSwap   .byte "Server to swap disks x-x", $9B
aSwD1	=	* - 4
aSwD2   =   * - 2
aSwaped .byte "Disks swapped", $9B
aUnMntA .byte "Server to unmount all disks", $9B
aUnMnt  .byte "Server to unmount disk x", $9B
aUnMD1  =   * - 2
aUnmtD  .byte "Disk(s) unmounted", $9B
aMount	.byte "Server to mount:             ", $9B
aMountF =   * - 13
aMountD .byte "Image mounted to drive: x", $9B
aMountN =   * - 2
aToggle .byte "Server to toggle Auto-Commit on D :", $9B
aTogg1  =   * - 3
aToggld .byte "Auto-Commit toggled", $9B
aFailD1 .byte "Disk swap failed!", $9B
aFailD2 .byte "Disc unmount failed!", $9B
aFailD3 .byte "Image mount failed! Check server logs", $9B
aFailD4 .byte "Image mounted, failed to get drive#", $9B
aFailD5 .byte "Auto-Commit toggle failed", $9B
aExi	.byte "Press RETURN to exit", $9B
aHlp  	.byte "Usage: ASPECL TS|TO|TF|DM|DN|DU|DS|DA", $9B, $9B
		.byte " TS: Set Time/Date", $9B
		.byte " TO: Set Time/Date, TD Line ON", $9B
  		.byte " TF: Set Time/Date, TD Line OFF", $9B
  		.byte " DM[fname.ext]: Existing Image Mount", $9B
  		.byte " DN[fname.ext.x]: New Image Mount", $9B
  		.byte "  x: 1-SDFD, 2-EDFD, 3-DDFD, 4-DSDDFD", $9B
  		.byte "     5-DDHD, 6-QDHD", $9B
  		.byte " DU[d/*]: Unmount disk/all disks", $9B
		.byte " DS[dd]:  Swap Disks", $9B
		.byte " DA[d/*]: Auto-commit disk/all disks", $9B, $FF
aSpvI	.byte "%23@CD"
aSpvO   .byte "233444"
aIfSw   .byte $0									;Command Line switches present? >> 0:NO, 1:YES
aSlSw	.byte $0									;Selected switch
aSlSo	.byte $0									;Selected Switch Option
aTime	.byte "T"									;Command Line switch (T: Time/Date)
aTopt	.byte "SOF", $9B							;Time/Date switch options
aDisk	.byte "D"									;Command Line switch (D: Disk)
aDopt	.byte "MNUSA", $9B							;Disk switch options
aDevic	.byte $46									;Device: 46 (AspeCL Client)
aUnit	.byte $01									;Unit: 01
													;=============================================
aComnd	.byte $93, $94, $95, $96, $97, $98			; AspeCL Command Structure
													;=============================================
													;   $93 (147): Date/Time
													;		dbuf <-- Date/Time
													;   $94 (148): Disk Swap
													; 		aux1 --> Drive# 1
													;		aux2 --> Drive# 2
													;   $95 (149): Disk Unmount
													;		aux1 --> Drive# or * (all drives)
													;   $96 (150): Image Mount
													; 		aux1 = 0 ---> Mount Image File
													;		dBuf --> File Name
													;		aux1 = 1 ---> Get mounted drive number
													;       dBuf <-- Drive Number
													;   $97 (151): New Image Mount
													; 		aux1 = 0 ---> Mount Image File
													;		dBuf --> File Name + Image Size
													;		aux1 = 1 ---> Get mounted drive number
													;       dBuf <-- Drive Number
													;   $98 (152): Toggle Auto Save ON/OFF
													; 		aux1 --> Drive#
													;=============================================

aCmd	.byte $09									;Default Command for E: (Put Record)
aBlen	.byte $FF									;Default Buffer Length for E:
aCr		.byte $9B									;Carriage Return
aitdon  .word I_tdon
I_tdon  .byte "I_TDON  ", $08						;Symbol structure for I_TDON
        .word 0
aHlpBuf .ds $25										;CIO buffer for displaying all help text
aBuf	.ds $FF							 			;Storage for server data
aDot    .ds $01										;Number of dots in the filename

		
*			=	$02E0								;Run Address
		.word	aStart


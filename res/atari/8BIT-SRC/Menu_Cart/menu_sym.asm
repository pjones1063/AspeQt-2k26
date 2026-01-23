;
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
;

DDEVIC	equ $0300
DUNIT	equ $0301
DCOMND	equ $0302
DSTATS	equ $0303
DBUFLO	equ $0304
DBUFHI	equ $0305
DTIMLO	equ $0306
DUNUSE	equ $0307
DBYTLO	equ $0308
DBYTHI	equ $0309
DAUX1	equ $030A
DAUX2	equ $030B
NOCKSM	equ $003C

iccom	equ $0342
icbadr	equ $0344
icptl	equ $0346
icpth	equ $0347
icblen	equ $0348
icaux1	equ $034A
icaux2	equ $034B

ciov	equ $E456
portb	equ $D301
dday    equ $077B
dmth    equ $077C
dyer    equ $077D
dhrs    equ $077E
dmin    equ $077F
dsec    equ $0780
SIOV	equ $E459
I_SETTD	equ $FFC3
I_TDON	equ $FFC6
I_GETTD equ $FFC0

DOSVEC	equ $0A
DOSINI	equ $0C
comfnam	equ $21
comtab	equ $0A


	.enum Cmd
GetCmd	        = $86	
RunCmd	        = $87
GetTxt	        = $88
SetDR           = $89 
PutDR           = $90
GetDR           = $91	
GetSL           = $92
GetTD	        = $93
Swap	     	= $94
Unmount		    = $95
Mount		    = $96
CreateAndMount	= $97
AutoToggle	    = $98
Save            = $99
MountAndBoot    = $9A   
Print           = $9B
GetHostPath     = $9C
	.ende

	.enum DCB
GetCmd
RunCmd
GetTxt
SetDR
PutDR	
GetDR	
GetSL
GetTD
Swap
Unmount
Mount
CreateAndMount
AutoToggle
GetDrvNum
Save
MountAndBoot
Print
GetHostPath
	.ende


	org $80
Temp1			.ds 2
Temp2			.ds 2
Temp3			.ds 2
Temp4			.ds 2
LeadingZeroFlag	.ds 1
ArgIndex		.ds 1
FieldWidth		.ds 1
StringIndex		.ds 1
SelectB			.ds 1 	
ArgFlag			.ds 1
CreateFlag		.ds 1
Slot			.ds 1
Drive			.ds 1
DriveID1		.ds 1
DriveID2		.ds 1

	org  $3600
Path 			.ds 22
Command         .ds 33
Filename		.ds 16		
IOBuf			.ds 252
IOFileOption 	.ds 1	
IOLastFile		.ds 2			
InputBuf 		.ds 255
	
.macro ldyx
	ldy #< :1
	ldx #> :1
.endm

.macro styx
	sty :1
	stx :1+1
.endm

.macro ldax
	lda #< :1
	ldx #> :1
.endm

.macro stax
 	sta :1
 	stx :1+1
.endm

.macro lday
	lda #< :1
	ldy #> :1
.endm

.macro stay
 	sta :1
 	sty :1+1
.endm

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
HATABS  equ $031A   ; Handler Table Address
MEMLO   equ $02E7   ; Low Memory Pointer (Low Byte)

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

 
 

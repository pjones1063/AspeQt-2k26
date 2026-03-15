;       .TITLE      'DISK UTILITY PROGRAMS (DUP)  VER 2.9  11/18/80'
;*************************************************************************
;               THIS IS FINAL VERSION OF DUP  ---- 2.0S ----
;*************************************************************************
;                   FILENAME = DOS2.DUP29Q ON TANDEM
;-------------------------------------------------------------------------
;CHANGED FOR SYSTEM RESET:            DUPFLG                               
;ADDED INTERRUPT ROUTINES FROM SIO:   KB
;ADDED SAVE/RESTORE OF DOSINI VECTOR: KB
;-------------------------------------------------------------------------
;SCANNED BY:                          MR.ATARI   JAN-2004 
;PRETTY PRINTED & FIXED ALL COMMENTS: UNIXcoffee 16-JUN-2011 
;-------------------------------------------------------------------------
;     .PAGE 
;
;=========================================================================
;  ****  EQUATES  ****
;=========================================================================
CIO =   $E456
DKHND = $E453
SETVBV = $E45C
SYSVBV = $E45F
XITVBV = $E462
CIOINV = $E46E
MEMTOP = $02E5
BRKKEY = $11
DOSVEC = $0A
DOSINI = $0C    ;DOS INIT VECTOR
WARMST = 8
LMARGN = $52
RMARGN = $53
CARTST = $BFFA
INTRVEC = $020A ;INTERRUPT VECTOR LOC FOR SIO PATCH
MEMLO = $02E7
SHFLOK = $02BE
INITAD = $02E2
RUNAD = $02E0
ICHIDZ = $20
ICDNOZ = $21
ICBALZ = $24
ICBAHZ = $25
ICIDNO = $2E
MAXDEV = $21
HATABS = $031A
USRDOS = $1700
FMS =   $0700
FMINIT = FMS+$E0
DOS =   FMS+$0E40
WRMSTR = $E474  ;WARM START VECTOR
BSIOR = $0772   ;ENTRY POINT TO FMS DISK HANDLER USED BY DUP DISK
CDTMV3 = $021C  ;ADDRESS OF SYSTEM TIMER # 3
CDTMF3 = $022A  ;ADDRESS OF SYS TIMER # 3 TIME OUT FLAG
;
CR  =   $9B
CUP =   $1C
CDN =   $1D
CLF =   $1E
CRT =   $1F
DLL =   $9C
CLSCR = $7D
EOF =   $88     ;ENDFILE RETURN CODE FROM CIO
;
;
OPEN =  $03
CLOSE = $0C
PUTCHR = $0B
GETCHR = $07
GETREC = $05
PUTREC = $09
RENAME = $20
DELETE = $21
FORMAT = $FE
LOCK =  $23
UNLOCK = $24
STAREQ = $53    ;STATUS COMMAND TO DISK CONTROLER
;
IOCB1 = $10
;
DVSTAT = $02EA  ;ADDRESS OF STATUS INFO STORED BY OS
;.........................................................................
;
DCB =   $0300
DUNIT = DCB+1
DCOMND = DCB+2
DSTATS = DCB+3
DBUFLO = DCB+4
DBUFHI = DCB+5
DSLO =  DCB+$0A
DSHI =  DCB+$0B
;.........................................................................
IOCB =  $0340
ICHID = IOCB+0
ICDNO = IOCB+1
ICCOM = IOCB+2
ICSTA = IOCB+3
ICBAL = IOCB+4
ICBAH = IOCB+5
ICBLL = IOCB+8
ICBLH = IOCB+9
ICAX1 = IOCB+10
ICAX2 = IOCB+11
;.........................................................................
SYSED = $00
OWRIT = $08
ORDWRT = $0C
;.........................................................................
;     .PAGE 
;=========================================================================
;  ****  ZERO  PAGE  VARIABLES  **** 
;=========================================================================
    virtual at $18
JMPTBL   .ds 2 
RAMLO    .ds 2 
     endv
BUFADR = RAMLO  ;SAVE AREA FOR BUFFER ADDRESS USED BY USER
;     .PAGE 
;=========================================================================
;  ****  INIT CODE FOR DUP  ****
;=========================================================================
;       INITIALIZATION CODE FOR DUP - CALLS FMS INIT CODE.
;       CALLED ON WARM START AND COLD START.
;-------------------------------------------------------------------------
    org DOS
    LDA #0
    STA OPT
    LDA # ; <MNDUPL
    STA DOSVEC
    LDA # ; <MNDUPH
    STA DOSVEC+1
    LDA # ; <ISRSIR ;SET UP INTERRUPT VECTORS FOR SIO PA
    STA INTRVEC ;INSTEAD OF USING THE SERIAL INPUT READY
    LDA # ; >ISRSIR ;SERVICE ROUTINE AND THE SERIAL OUTPUT
    STA INTRVEC+1 ;INTERRUPT SERVICE ROUTINE IN THE OS ROM
    LDA # ; <ISRODN ;USE THE VERSIONS IN RAM FOLLOWING THE
    STA INTRVEC+2 ;RESIDENT PORTION OF DUP.
    LDA # ; >ISRODN
    STA INTRVEC+3
    JSR FMINIT
    LDA WARMST ;  ;ON COLDSTART, LOAD AUTORUN.SYS
    BNE CKMDOS ;  ;WARMSTART CHECK IF DUP WAS RUNNING
    LDA # ; <AFL
    STA ICBAL+$10
    LDA # ; <AFH
    STA ICBAH+$10
    JSR INITX ;  ;CLEAR DUPFLG SHOW DUP NOT IN MEMORY.
    LDA #$C0
    JSR STLOAD ;  ;LOAD, INIT AND RUN THE AUTORUN FILE
    JMP CLOSX ;  ;MAKE SURE IOCB #1 IS CLOSED & RETURN
;
CKMDOS LDA DUPFLG ;SEE IF DUP WAS IN MEMORY
    BEQ INITX ;  ;=ZERO THEN WASN'T
;.........................................................................
    LDA MEMFLG ;  ;SEE IF USER AREA WRITTEN TO MEM.SAV
    BEQ CLDSET ;  ;=ZERO THEN WASN'T
    JSR LDMEM1 ;  ;ELSE GET USER MEMORY BACK IN
;.........................................................................
    JSR RELDIN ;  ;RELOAD SAVED DOSINI VECTOR
    JSR INITX ;  ;CLEAR DUP IN MEMORY FLAG
    JSR WRMSTR ;  ;REDO WARMSTART
;.........................................................................
INITX LDA #0 ;  ;SAY DUP NOT IN MEMORY
    STA DUPFLG ;  ;CLEAR FLAG
    RTS 
;.........................................................................
CLDSET STA WARMST ;NO VALID USER MEMORY
    BEQ INITX ;  ;SET TO COLD START
;     .PAGE 
;=========================================================================
;  ****  LOADER ROUTINE  *** 
;=========================================================================
;       LOADS FROM THE FILE (MUST BE LOAD FORMAT)
;       INTO MEMORY. RETURNS:
;         X=0 LOAD OK
;         X=1 OPEN ERRORS Y=CIO CODE
;         X=2 READ ERRORS Y=CIO CODE
;         X=3 BAD LOAD FILE
; ON ENTRY, IOCB 1 POINTS TO FILENAME.
;-------------------------------------------------------------------------
DUPFLG .BYTE 0  ;FLAG -IF DUP IN MEMORY NOT ZERO
OPT .BYTE 0     ;HOLDS VALUE OF OPTION GIVEN BY USER
LOADFG .BYTE 0  ;FLAG = $80 IF MEMORY FILE DOESN'T HAVE
HDBUF    .ds 4 
HDBUFH = HDBUF/256
HDBUFL =  <HDBUF
SFLOAD LDA #$80
STLOAD STA LOADFG
LOAD LDA # ; <RTS
    STA RUNAD
    LDA # ; >RTS
    STA RUNAD+1 ;MAKE RUN AT EOF DEFAULT TO RTS
    LDX #$10
    LDA #OPEN
    STA ICCOM,X
    LDA #4 ;  ;OPEN TYPE=INPUT
    STA ICAX1,X
    JSR CIO ;  ;TRY TO OPEN FILE
    BPL RDLF ;  ;CONT IF OK
    LDA #1 ;  ;OPEN ERRORS
    BNE CLFX ;  ;CLOSE AND EXIT
RDLF LDX #$10
    LDA # ; <DBUFL
    STA ICBAL,X
    LDA # ; <DBUFH
    STA ICBAH,X
    LDA #2
    STA ICBLL,X
    LDA #0
    STA ICBLH,X
    STA MEMLDD ;  ;CLEAR MEM.SAV LOADED FLAG
    LDA #GETCHR
    STA ICCOM,X
    JSR CIO
    BMI ERST ;  ;IF ERRS
    LDA #$FF
    CMP DBUF ;  ;CHECK FOR VALID LOAD FILE
    BNE LNLF
    CMP DBUF+1
    BNE LNLF ;  ;BRANCH IF NOT A LOAD FILE
RDDRC LDX #$10
    LDA # ; <HDBUFL
    STA ICBAL,X
    LDA # ; <HDBUFH
    STA ICBAH,X
    LDA #4
RDDRC1 STA ICBLL,X
    LDA #0
    STA ICBLH,X
    JSR CIO ;  ;NO ERROR CHECK SO CAN CATCH EOF
    BPL STOK ;  ;IF NO ERROR
    CPY #$88 ;  ;SEE IF EOF
    BNE ERST ;  ;IF SOME ERROR STATUS
;=========================================================================
;EOF SO DONE, EXIT
;=========================================================================
    JSR CLOSX ;  ;CLOSE IOCB'S 1 AND 2
    BIT OPT
    BMI DRUN ;  ;BRANCH IF NO RUN OPTION
    JSR JMPRUN ;  ;JUMP THROUGH RUN VECTOR
DRUN LDA #0 ;  ;OK STATUS
    BIT LOADFG ;  ;WAS MEMORY SWAPPED?
    STA LOADFG
    BMI CLFX ;  ;BRANCH IF MEMORY WASN'T SWAPPED
    JSR MEMSVQ ;  ;DOES MEMORY SAVE FILE EXIST?
    BMI DRUN1 ;  ;BRANCH IF NOT
    PLA 
    PLA 
    JMP GOOD ;  ;WRITE MEMORY AND RELOAD DUP
;=========================================================================
;       SEE IF DUP WRITTEN OVER. IF IS RELOAD & TELL USER NEED MEM.SAV TO
;       LOAD THIS FILE.
;=========================================================================
DRUN1 LDA DUPFLG ;SEE IF DUP CLOBBERED
    BNE DRUN2 ;  ;NO, THEN RETURN
    LDA # ; <NMSFL ;ELSE TELL USER NEED MEM.SAV
    LDX # ; <NMSFH
    JSR PRNTMSG ;PRINT MSG
    JMP RRDUP ;  ;RELOAD & RUN DUP
;=========================================================================
;       RETURN TO CALLING ROUTINE
;=========================================================================
DRUN2 LDA #0 ;  ;NO DUP ERR MSG ON EOF
CLFX TAX 
RTS ; RTS 
;=========================================================================
;       ERROR RETURNS
;=========================================================================
LNLF JSR CLOSX
    LDA #3 ;  ;BAD LOAD FILE
    BNE CLFX
ERST TYA 
    PHA 
    JSR CLOSX
    PLA 
    TAY 
    BNE CLFX
;=========================================================================
;       CONTINUE WITH LOAD - CHECK LOAD ADDRESS FOR HEADER
;       HEADER IF HAVE CONCATENATED LOAD FILES
;=========================================================================
STOK LDX #$10
    LDA HDBUF ;  ;MOVE PARAMS TO IOCB
    STA ICBAL,X
    PHA 
    LDA HDBUF+1
    STA ICBAH,X
    TAY 
    PLA 
    INY ;  ;WAS ADDRESS FF?
    BNE ADOK ;  ;BRANCH IF NOT
    TAY 
    INY ;  ;OTHER BYTE FF?
    BNE ADOK ;  ;BRANCH IF NOT
;=========================================================================
;       HAVE A HEADER & START ADDRESS - GET END ADDRESS FOR TEXT & DO AG
;=========================================================================
    LDA HDBUF+2
    STA HDBUF
    LDA HDBUF+3
    STA HDBUF+1 ;MOVE LOAD ADDRESS
    LDA # ; <HDBUF+2
    STA ICBAL,X
    LDA # ; >HDBUF+2
    STA ICBAH,X ;SO LOAD ADDRESS DOESN'T GET WIPED OUT B
    LDA #2
    JMP RDDRC1
;=========================================================================
;       GET LENGTH OF TEXT. THEN DETERMINE IF IN DUP
;=========================================================================
ADOK LDA HDBUF+2
    SEC 
    SBC HDBUF
    STA ICBLL,X
    LDA HDBUF+3
    SBC HDBUF+1
    STA ICBLH,X
    LDA HDBUF+1
    JSR AWDQ ;  ;IS BEGINNING ADDRESS WITHIN DUP?
    BCS AWD ;  ;BRANCH IF SO
    LDA HDBUF+3
    JSR AWDQ ;  ;IS ENDING ADDRESS WITHIN DUP?
    BCS AWD ;  ;BRANCH IF SO
;=========================================================================
;       SINCE TEXT IN DUP, LOAD MEM.SAV IF NECCESARY
;=========================================================================
ANWD LDA MEMLDD
    BMI AWD ;  ;BRANCH IF MEM.SAV ALREADY LOADED
    LDA #$80
    ORA LOADFG
    STA LOADFG ;  ;SET MEM.SAV DOESN'T HAVE TO BE LOADED F
AWD INC ICBLL,X
    BNE *+5
    INC ICBLH,X
    BIT LOADFG ;  ;DOES MEMORY HAVE TO BE LOADED?
    BMI DLM ;  ;BRANCH IF NOT
    LDA MEMLDD ;  ;WAS MEM.SAV ALREADY LOADED?
    BMI DLM ;  ;BRANCH IF SO
    DEC MEMLDD
    JSR LDMEM ;  ;LOAD MEM.SAVE FILE (IF IT EXISTS)
    LDA #0 ;  ;SHOW USER AREA NOT DUP IN MEMORY
    STA DUPFLG
    JSR RELDIN ;  ;RESTORE DOSINI VECTOR FROM SAVED LOC
;=========================================================================
;       SET NO INIT ADDR DEFAULT THEN READ IN TEXT & ATTEMPT INIT
;=========================================================================
DLM LDX #$10
    LDA # ; <RTS
    STA INITAD
    LDA # ; >RTS
    STA INITAD+1 ;INIT DEFAULTS TO AN RTS
    JSR CIO ;  ;READ DATA DIRECTLY TO MEMORY
    BPL DLM1
    JMP ERST ;  ;IF ERRORS
DLM1 BIT OPT
    BMI DINIT ;  ;BRANCH IF NOGO OPTION
    JSR JMPINT ;  ;DO INIT
DINIT JMP RDDRC ;GET NEXT SECTION OF LOAD FILE
;=========================================================================
;       SUBROUTINE TO DETERMINE IF ADDRESS IS WITHIN DUP ADDRESS SPACE.
;               ENTRY - HI BYTE OF ADDRESS IN REG. A
;               RETURNS - CARRY SET : WITHIN DUP
;                         CARRY CLR : NOT WITHIN DUP
;=========================================================================
AWDQ CMP # ; <NDOSH
    BCC AWDQR ;  ;BRANCH IF HI BYTE LT DUP START
    CMP # ; <NMDUPH+1
    ROL
    EOR #1
    LSR ;  ;COMPLEMENT CARRY
AWDQR RTS 
;.........................................................................
;
JMPINT JMP (INITAD)
JMPRUN JMP (RUNAD)
;.........................................................................
;
MEMLDD .BYTE 0
AF  .BYTE "D1:AUTORUN.SYS",CR
AFH =   AF/256
AFL =    <AF
NMSF .BYTE "NEED MEM.SAV TO LOAD THIS FILE.",CR
NMSFH = NMSF/256
NMSFL =  <NMSF
;     .PAGE 
;=========================================================================
;  ****  CREATE  MEM.SAV  FILE  ****
;=========================================================================
;ROUTINE WRITTEN BY MICHAEL EKBERG,APRIL 21,1980
;-------------------------------------------------------------------------
;THIS ROUTINE CREATES A FILE ON DISK OF DATA FROM MEMORY
;CREATE FILE CALLED 'D1:MEM.SAV',SET Y=1
;
;ABLE TO CREATE FILE THEN SET REG.Y=ERROR RETURNED FROM CIO
;THE RAM TO BE OCCUPIED BY DUP IS STORED BY THIS ROUTINE INTO 'MEMORY.SAV'
;-------------------------------------------------------------------------
NAME .BYTE "D1:MEM.SAV",CR
NAMEH = NAME/256
NAMEL =  <NAME
MWRITE JSR CLOSX ;CLOSE IOCB AND OPEN IT TO WRITE
    LDA #OWRIT ;  ;
    STA ICAX1,X ;
    JSR OREST ;  ;OPEN FOR WRITE
    BMI ERRWR ;  ;IF ERROR THEN JMP AND RET
;
;=========================================================================
;WRITE MEMORY BLOCK
;=========================================================================
    LDA #PUTCHR
    STA ICCOM,X
    LDA # ; <NDOSL ;STORE START OF BLOCK FOR CIO
    STA ICBAL,X
    LDA # ; <NDOSH ;START ADDR (HIGH)
    STA ICBAH,X
    LDA # ; <MLENL+1 ;LENGTH OF BLOCK
    STA ICBLL,X
    LDA # ; <MLENH ;LENGTH(HIGH)
    STA ICBLH,X
    JSR CIO ;  ;WRITE DATA BLOCK
    BMI ERRWR ;  ;IF WRITE ERROR THEN JMP
    JSR CLOSX
    BMI ERRWR
    LDY #0
RET RTS 
;-------------------------------------------------------------------------
OREST LDA # ; <OPEN
    STA ICCOM,X
    LDA # ; <NAMEL ;ROUTINE TO COMPLETE OPEN OF 'D1:MEMORY.
    STA ICBAL,X ;CALLING SUB SUPPLIES 'READ' OR 'WRITE'
    LDA # ; <NAMEH ;IN ICAX1
    STA ICBAH,X
    JMP CIO
;-------------------------------------------------------------------------
ERRWR STY TEMP+1 ;TEMP STORE FOR Y FLAG
    JSR CLOSX ;  ;CLOSE #$20
    LDA # ; <DELETE ;DELETE PART OF MENSAV
    STA ICCOM,X
    JSR OREST
TEMP LDY #0 ;  ;RESTORE FLAG
    RTS ;  ;RETURN TO MAIN CALLER
;     .PAGE 
;=========================================================================
;  ****  ENTRY  POINT  ON  'DOS'  CALL  ****
;=========================================================================
INISAV .WORD 0 ;DOSINI VECTOR SAVE LOC
MEMFLG .BYTE 0
MNDUP LDX #0
    STX MEMFLG
    STX LOADFG
    DEX 
    STX WARMST
    JSR INITIO
;.........................................................................
    JSR MEMSVQ ;  ;FIND OUT IF FILE D1:MEM.SAV EXISTS
    BPL GOOD ;  ;BRANCH IF MEM.SAV FILE EXITS
    LDA #0
    STA WARMST ;  ;CLEAR WARM START FLAG
    BEQ FINAL
;.........................................................................
;
GOOD JSR MWRITE ;WRITE USER AREA TO MEM.SAV
    BMI ERROR
    DEC MEMFLG ;  ;SHOW MEMORY WRITTEN
    BMI FINAL
;.........................................................................
ERROR
    LDA # ; <ERRMES ;PRINT ERROR OCCURED MSG
    LDX # ; >ERRMES
    JSR PRNTMSG ;GOTO MSG PRINTER
;
    LDA # ; <ERR  ;PRINT QUERY TO RUN DOS
    LDX # ; >ERR
    JSR PRNTMSG ;GOTO MSG PRINTER
;
;                               ;WAIT FOR Y TO RUN DOS
;.........................................................................
    LDA #GETREC
    STA ICCOM
    LDA # ; <STAKL
    STA ICBAL
    LDA # ; <STAKH
    STA ICBAH
    LDA #2
    STA ICBLL
    LDA #0
    STA ICBLH
    JSR CIO
    LDA STAK ;  ;SEE IF Y TYPED
    CMP #'Y'
    BNE RTCART ;  ;BRANCH IF NOT
    LDA #0
    STA WARMST
;-------------------------------------------------------------------------
FINAL LDX #$20
    LDA #CLOSE
    STA ICCOM,X ;SET UP CLOSE COMMAND
    JSR CIO ;  ;PERFORM CLOSE COMMAND
;-------------------------------------------------------------------------
RRDUP LDA DOSINI ;SAVE DOS INIT VECTOR
    STA INISAV
    LDA DOSINI+1
    STA INISAV+1
;.........................................................................
    LDA # ; <DOS  ;SET UP DUP INIT ADDR AS
    STA DOSINI ;  ;DOS INIT VECTOR
    LDA # ; >DOS
    STA DOSINI+1
;-------------------------------------------------------------------------
RRDUP1 LDA # ; <DUPSYS
    LDX #$10
    STA ICBAL,X
    LDA # ; >DUPSYS
    STA ICBAH,X
    LDY #0
    STY OPT ;  ;ASSURE NO /N OPTION IN EFFECT
    DEY ;  ;SHOW THAT DUP IS IN MEMORY
    STY DUPFLG
    JSR SFLOAD ;  ;LOAD DUP.SYS AND RUN IT
RTCART RTS 
EC  .BYTE "E:",CR
ECH =   EC/256
ECL =    <EC
MNDUPH = MNDUP/256
MNDUPL =  <MNDUP
DUPSYS .BYTE "D1:DUP.SYS",CR
;.........................................................................
ERRMES .BYTE "ERROR-SAVING USER MEMORY ON DISK",CR
ERR .BYTE "TYPE Y TO STILL RUN DOS",CR
;=========================================================================
;  ****  SUBROUTINES  FOR  RESIDENT  DUP  **** 
;=========================================================================
;       ROUTINE TESTS IF MEM.SAV IS PRESENT ON THE DISK.
;       RETURNS - MINUS IF MEM.SAV IS NOT THERE
;                 PLUS  IF MEM.SAV IS THERE
;-------------------------------------------------------------------------
MEMSVQ JSR CLOS20 ;CLOSE IOCB # 2
    LDA #OPEN
    STA ICCOM,X
    LDA # ; <NAMEL
    STA ICBAL,X
    LDA # ; <NAMEH
    STA ICBAH,X
    LDA #ORDWRT
    STA ICAX1,X ;TRY TO OPEN D1:MEM.SAV FOR READ/WRITE
    JSR CIO
    PHP ;  ;SAVE STATUS
    JSR CLOS20 ;  ;CLOSE MEM.SAV
    PLP ;  ;RESTORE STATUS
    RTS 
;
;=========================================================================
;       SAVE FILE SUBROUTINE - WRITE FILE BODY, INIT, & RUN VECTORS
;=========================================================================
WDR1 LDA #0 ;  ;THIS IMMEDIATE VALUE MODIFIED
    BEQ WDR2 ;  ;BRANCH IF MEMORY FILE DOESN'T HAVE TO B
    JSR LDMEM
WDR2 LDX #$10
    JSR CIO ;  ;DO SAVE - WRITE BODY TO DISK
INITQ LDA #0 ;  ;THIS IMMEDIATE VALUE CHANGED DURING SAV
    BEQ RUNQ ;  ;SET TO FF WHEN AN INIT VECTOR IS PRESENT
    INC INITQ+1
    LDA INITAD
    STA VECTR ;  ;IF INIT VECTOR FOR FILE SAVE IT
    LDA INITAD+1
    STA VECTR+1
    LDA # ; <INITAD
    TAX 
    STA LDST
    LDA # ; >INITAD
    JSR WRVEC
RUNQ LDA #0 ;  ;THIS IMMEDIATE VALUE MODIFIED
    BEQ NORNAD ;  ;SET TO FF WHEN A RUN VECTOR IS PRESENT
    INC RUNQ+1
    LDA RUNAD
    STA VECTR ;  ;IF RUN VECTOR FOR FILE SAVE IT
    LDA RUNAD+1
    STA VECTR+1
    LDA # ; <RUNAD
    TAX 
    STA LDST
    LDA # ; >RUNAD
    JSR WRVEC
NORNAD JSR CLOSX ;CLOSE IOCBS 1 &2
    LDA MEMFLG
    AND WDR1+1
    BEQ DRRDUP
    INC WDR1+1 ;  ;RESET MEM.NEEDS TO BE LOADED FLAG
    JMP RRDUP1 ;  ;RELOAD & RUN DUP
DRRDUP JMP DOSOS ;RUN THE SWAPPED IN DUP
;-------------------------------------------------------------------------
;
;
WRVEC STA LDST+1
    INX 
    STX LDND
    STA LDND+1
    LDX #$10
    LDA # ; <LDST
    STA ICBAL,X
    LDA # ; >LDST
    STA ICBAH,X
    LDA #6
    STA ICBLL,X
    LDA #0
    STA ICBLH,X
    JMP CIO ;  ;WRITE INIT OR RUN ADDRESS
;
;=========================================================================
;       JUMP TO CARTRIDGE
;=========================================================================
CLMJMP JSR LDMEM
    LDA #0 ;  ;SHOW DUP NO LONGER IN MEMORY
    STA DUPFLG
    JSR RELDIN ;  ;RESTORE DOS INIT VECTOR SAVED
    JMP (CARTST) ;JUMP TO CARTRIDGE
;
;=========================================================================
;       LOAD MEM.SAV (IF IT EXISTS) BEFORE RUN AT ADDRESS
;=========================================================================
LMTR JSR LDMEM ;  ;LOAD MEM.SAVE IF IT EXISTS
    LDA #0 ;  ;SHOW THAT DUP NO LONGER IN MEMORY
    STA DUPFLG
    JSR RELDIN ;  ;RESTORE DOS INIT VECTOR SAVED
    JMP (RAMLO) ;RUN AT ADDRESS
;=========================================================================
;       RESTORE DOSINI VECTOR FROM SAVED LOCATION
;=========================================================================
RELDIN LDA INISAV
    STA DOSINI
    LDA INISAV+1
    STA DOSINI+1
    RTS 
;
;=========================================================================
;       SUBROUTINE - LDMEM
;       LOAD MEM.SAV IF IT EXISTS
;=========================================================================
LDMEM LDA MEMFLG
    BNE LDMEM1 ;  ;BRANCH IF MEMORY WAS SAVED
    RTS 
LDMEM1 JSR MEMSVQ
    BPL LDMEM2 ;  ;BRANCH IF MEM.SAV FILE DOES EXIST
    LDA #0 ;  ;TELL CART PGM AREA CLOBBERED
    STA WARMST
    BEQ CLOS2 ;  ;GO CLOSE AND GOTO CART
;-------------------------------------------------------------------------
LDMEM2 LDA #OPEN
    STA ICCOM,X
    JSR CIO ;  ;REOPEN MEM.SAV
    LDA #GETCHR
    STA ICCOM,X
    LDA # ; <MLENL+1
    STA ICBLL,X
    LDA # ; <MLENH
    STA ICBLH,X
    LDA # ; <NDOSL
    STA ICBAL,X
    LDA # ; <NDOSH
    STA ICBAH,X
    JSR CIO
CLOS2 LDA #CLOSE
    STA ICCOM,X
    JMP CIO ;  ;CLOSE MEM.SAV
;=========================================================================
;       CLOSE ALL IOCBS & RE-OPEN ZERO AS SCREEN EDITOR
;=========================================================================
INITIO JSR CIOINV ;THIS ROUTINE CLOSES ALL IOCB'S
;                               ;THEN REOPENS THE SCREEN EDITOR
    LDX #0
    LDA #OPEN
    STA ICCOM,X
    LDA # ; <ECL
    STA ICBAL,X
    LDA # ; <ECH
    STA ICBAH,X
    LDA #ORDWRT
    STA ICAX1,X
    JSR CIO
;.........................................................................
    LDX #0 ;  ;DELAY UNTIL DMA (SCREEN) IS RESTORED
    STX CDTMV3 ;  ;CLEAR TIMER NUMBER 3
    STX CDTMV3+1
    LDY #1 ;  ;WAIT FOR ONE VBLANK
    LDA #3 ;  ;USE TIMER # 3
    STA CDTMF3 ;  ;SET TIMER DONE FLAG TO NOT DONE
    JSR SETVBV ;  ;SYSTEM CALL TO SET TIMER
WAITIM LDA CDTMF3 ;WAIT UNTIL TIMER IS DONE
    BNE WAITIM
;.........................................................................
    RTS 
;=========================================================================
;  CLOSX - CLOSE IOCBS 10,20
;=========================================================================
CLOSX LDA #CLOSE
    LDX #$10
    STA ICCOM,X
    JSR CIO
;=========================================================================
;       ENTRY TO CLOSE IOCB # 2 ONLY
;=========================================================================
CLOS20 LDX #$20
    LDA #CLOSE
    STA ICCOM,X
    JMP CIO

;    .INCLUDE #D:DUPSYS2.M65
;    .INCLUDE #D:DUPSYS3.M65
;    .INCLUDE #D:DUPSYS4.M65
;=========================================================================
;       SUBROUTINE - PRNTMSG
;=========================================================================
;       PUTS A CHARACTER STRING TERMINATED BY A CARRIAGE RETURN CHAR TO
;       SCREEN EDITOR.
;               ENTRY - REG A : LOW BYTE MSG ADDRESS
;                       REG X : HI BYTE MSG ADDRESS
;=========================================================================
;       PUT PARAMS IN IOCB - USE IOCB 0 FOR SCREEN EDITOR
;=========================================================================
PRNTMSG STA ICBAL ;SET MSG ADDR IN IOCB BUFF ADDR
    STX ICBAH
;=========================================================================
;       SET UP REST OF IOCB
;=========================================================================
    LDA #$80 ;  ;SET IN BUFFER LENGTH
    STA ICBLL ;  ;ASSUME 128 BYTES MAX
    LDX #0 ;  ;USE REG X TO SET IN IOCB INDEX FOR CIO
    STX ICBLH
    LDA #PUTREC ;PUT MSG
    STA ICCOM
;=========================================================================
;       TEST IF DUP IS RESIDENT - IF IS THEN USE INDIRECT CIO ROUTINE TO
;       FOR BREAK KEY ABORT
;=========================================================================
    LDA DUPFLG ;  ;=ZERO IF NON-RESIDENT DUP NOT IN MEMORY
    BNE INMEM ;  ;IN MEMORY THEN USE INDIRECT CIO CALL
;.........................................................................
    JMP CIO ;  ;ELSE GO DIRECT TO CIO & RETURN
;-------------------------------------------------------------------------
INMEM JMP CIO1 ;  ;USE CIO CALL WITH TEST FOR BREAK KEY ABORT
;-------------------------------------------------------------------------
;
SAVH .BYTE $FF,$FF
SAVHH = SAVH/256
SAVHL =  <SAVH
LDST     .ds 2 
LDSTH = LDST/256
LDSTL =  <LDST
LDND     .ds 2 
VECTR    .ds 2 
;     .PAGE 
;=========================================================================
;  ****  SIO  INTERRUPT  SERVICE  ROUTINES  ****
;=========================================================================
;       EQUATES FOR INTERRUPT ROUTINES MOVED FROM SIO ZERO PAGE
;
;       
;-------------------------------------------------------------------------
BUFRLO = $32    ;POINTER TO BYTE TO SEND OR RECEIVE
BUFRHI = $33
BFENLO = $34    ;POINTER TO BYTE AFTER END OF BUFFER
BFENHI = $35
CHKSUM = $31    ;LOC TO STORE DATA FRAME CHECKSUM
CHKSNT = $3B    ;CHECKSUM SENT FLAG- =FF SENT
NOCKSM = $3C    ;FLAG NO CHECK SUM TO BE RECEIVED-NOT ZERO
STATUS = $30    ;HOLD FOR STATUS TO BE PUT IN DCB
BUFRFL = $38    ;FLAG-IF FF RECEIVE BUFFER IS FULL
RECVDN = $39    ;FLAG RECEIVE NOT DONE. USED BY WAIT LOOP
POKMSK = $10    ;POKEY INTERRUPT MASK SHADOW FOR IRQEN
;=========================================================================
;       HARDWARE REGISTERS USED IN SIO INTERRUPT ROUTINES
;=========================================================================
SKRES = $D20A   ;SERIAL PORT STATUS RESET ON POKEY
SEROUT = $D20D  ;SERIAL OUTPUT REGISTER
SERIN = SEROUT  ;SERIAL PORT INPUT REG ON POKEY
IRQEN = $D20E   ;IRQ INTERRUPT ENABLE ON POKEY
SKSTAT = $D20F  ;SERIAL PORT STATUS REG ON POKEY
;=========================================================================
;       ERROR CODES RETURNED BY SIO
;=========================================================================
FRMERR = $8C    ;FRAMING ERROR ON INPUT
OVRRUN = $8E    ;DATA FRAME OVER RUN-BIT D5 IN SKSTAT
CHKERR = $8F    ;DATA FRAME CHECKSUM ERROR
;     .PAGE 
;=========================================================================
;  ****  INTERRUPT  SERVICE  ROUTINE  TO  OUTPUT  DATA  NEEDED  ****
;=========================================================================
;       KEITH BALL 6/10/80
;-------------------------------------------------------------------------
;       IT UPDATES THE BYTE TO PUT ON SERIAL I/O BUS POINTER
;       UNTIL END OF BUFFER.  AFTER EACH UPDATE OF THE PTR ADDS THE
;       VALUE OF THE BYTE TO THE CHECKSUM.  OUTPUTS THE CHECKSUM WHEN
;       PTR EQUALS THE END OF BUFFER PTR (POINTS TO BYTE AFTER BUFFER).
;       RETURNS TO THIS ROUTINE AFTER CHECKSUM PASSED AND RESETS POKEY
;       INTERRUPT REG TO HAVE THE TRANSMIT DONE ROUTINE CALLED TO END
;       WAIT LOOP (SEE SIO LISTING).
;-------------------------------------------------------------------------
;
ISRODN TYA ;  ;SAVE Y REG ON STACK
    PHA 
;.........................................................................
    INC BUFRLO
    BNE NOWRP0 ;  ;INCREMENT PTR TO NEXT BYTE
    INC BUFRHI ;  ;TO SEND
;=========================================================================
;       PATCH TO ROUTINE        CHANGED CHECK
;=========================================================================
NOWRP0 LDA BUFRLO ;CHECK IF PTR IS WITHIN BUFFER
    CMP BFENLO ;  ;DO A DOUBLE PRECISION SUBTRACT
    LDA BUFRHI
    SBC BFENHI
    BCC NOTEND ;  ;BRANCH IF (BUFR) < (BFEN)-MORE TO SEND
;.........................................................................
    LDA CHKSNT ;  ;TEST IF CHECKSUM ALREADY SENT
    BNE RELONE ;  ;BRANCH IF ALREADY SENT
;=========================================================================
;       SEND CHECKSUM AND SET FLAG
;=========================================================================
    LDA CHKSUM
    STA SEROUT ;  ;PUT CHECKSUM IN SERIAL OUT REG
    DEC CHKSNT ;  ;SET FLAG TO FF HEX
    BNE CHKDON ;  ;RETURN
;=========================================================================
;       AFTER CHECKSUM SENT AND CAUSE NEXT INTERRUPT THEN CHANGE POKEY
;       MASK TO ENABLE TRANSMIT DONE INTERRUPT AND TERMINATE WAIT LOOP.
;=========================================================================
RELONE LDA POKMSK ;GET POKEY MASK
    ORA #$08 ;  ;OR IN ENABLE
    STA POKMSK
    STA IRQEN ;  ;ENABLE THE INTERRUPTS
;=========================================================================
;       RESTORE REGS AND RETURN
;=========================================================================
CHKDON PLA 
    TAY ;  ;RESTOR Y REG
    PLA ;  ;RESTORE A REG SAVED IN OS IRQ INTERRUPT
    RTI 
;=========================================================================
;       MORE TO SEND.  SEND NEXT BYTE POINTED AT BY BUFR.
;=========================================================================
NOTEND LDY #0
    LDA (BUFRLO),Y ;GET NEXT BYTE
    STA SEROUT ;  ;PUT IN SERIAL OUT REG
;.........................................................................
    CLC 
    ADC CHKSUM ;  ;ADD BYTE TO CHECKSUM
    ADC #0
    STA CHKSUM
;-------------------------------------------------------------------------
    JMP CHKDON ;  ;GO RETURN AND WAIT FOR NEXT BYTE
;-------------------------------------------------------------------------
;        END OF OUT SERVICE ROUTINE **************************************
;     .PAGE 
;=========================================================================
;  ****  SERIAL  INPUT  READY  INTERRUPT  SERVICE  ROUTINE  ****
;=========================================================================
;       KEITH BALL    6/11/80
;-------------------------------------------------------------------------
;       AFTER SERIAL RECEIVE IS ENABLED ROUTINE IS USED TO COLLECT
;       BYTES FROM THE SERIAL INPUT REG AND PUT THEM IN BUFFER. 
;       WILL STOP WHEN BUFFER IS FULL.  IF A CHECKSUM IS EXPECTED
;       ROUTINE WILL MARK BUFFER FULL AND CONTINUE. WHEN CHECKSUM
;       RECEIVED IT WILL CHECK IF = TO CHECKSUM IT WAS MAKING.
;       WILL STORE ERRORS FOUND IN STATUS LOCATION.
;       
;       THE IRQ INTERRUPT HANDLER IN THE OS PUSHES THE USER'S A REGISTER 
;       ONTO THE STACK BEFORE CALLING THIS ROUTINE.
;       
;-------------------------------------------------------------------------
ISRSIR TYA ;  ;SAVE Y REG ON STACK
    PHA 
;=========================================================================
;       GET STATUS FROM POKEY THEN RESET IT.
;=========================================================================
    LDA SKSTAT
    STA SKRES ;  ;IGNORES VALUE- JUST STROBED
;=========================================================================
;       CHECK FOR ERRORS
;=========================================================================
    BMI NTFRAM ;  ;BIT 8 SET IF NO FRAMING ERROR
    LDY #FRMERR
    STY STATUS ;  ;SET FRAME ERROR STATUS
;-------------------------------------------------------------------------
NTFRAM AND #$20 ;IF BIT 5 CLEAR THEN FRAME OVER RUN
    BNE NTOVRN ;  ;BRANCH IF NO OVER RUN
    LDY #OVRRUN
    STY STATUS ;  ;ELSE SET OVERRUN ERROR STATUS
;=========================================================================
;       CHECK IF BUFFER FULL AND THIS IS A CHECKSUM.  IF IT IS, THEN CHECK
;       IF DATA SENT WAS VALID.
;=========================================================================
NTOVRN LDA BUFRFL ;TEST FOR BUFFER FULL (NOT ZERO)
    BEQ NOTYET ;  ;IF ZERO THEN NOT YET, THIS IS DATA.
    LDA SERIN ;  ;ELSE THIS IS CHECKSUM
    CMP CHKSUM ;  ;ARE THEY EQUAL?
    BEQ SRETRN ;  ;YES,THEN RETURN
    LDY #CHKERR ;ELSE SET CHECK SUM ERROR STATUS
    STY STATUS
;=========================================================================
;       SET RECEIVE DONE TO END WAIT LOOP
;=========================================================================
SRETRN LDA #$FF ;DONE VALUE
    STA RECVDN
;=========================================================================
;       RESTORE REGS AND RETURN
;=========================================================================
SUSUAL PLA 
    TAY ;  ;RESTORE Y REG
    PLA ;  ;RESTORE A REG
    RTI 
;=========================================================================
;       IF BYTE IS DATA, THEN GET HERE.  PUT BYTE IN BUFFER AND CHECK IF
;       AT END OF BUFFER.
;=========================================================================
NOTYET LDA SERIN ;GET DATA BYTE
    LDY #0
    STA (BUFRLO),Y ;STORE IT IN THE BUFFER
;.........................................................................
    CLC 
    ADC CHKSUM ;  ;ADD DATA BYTE TO CHECKSUM
    ADC #0
    STA CHKSUM
;.........................................................................
    INC BUFRLO ;  ;INCREMENT POINTER TO LOCATION
    BNE NTWRP1 ;  ;FOR NEXT BYTE INPUT
    INC BUFRHI
;=========================================================================
;       THE PATCH CHANGED THE TEST FOR END OF BUFFER
;=========================================================================
NTWRP1 LDA BUFRLO ;DO DOUBLE PRECISION SUBTRACT
    CMP BFENLO
    LDA BUFRHI
    SBC BFENHI ;  ;CARRY CLEAR IF BORROW
    BCC SUSUAL ;  ;BRANCH IF (BUFR) < (BFEN)-WITHIN BUFFER
;=========================================================================
;       DONE WITH DATA.  SEE IF CHECKSUM TO BE SENT
;=========================================================================
    LDA NOCKSM ;  ;IF = ZERO THEN A CHECKSUM
    BEQ GOON ;  ;WILL FOLLOW THE DATA
;.........................................................................
    LDA #0 ;  ;ELSE NO CHECKSUM TO FOLLOW
    STA NOCKSM ;  ;CLEAR NO CHECKSUM FLAG
    BEQ SRETRN ;  ;RETURN AFTER SET RECEIVE DONE FLAG
;=========================================================================
;       SET BUFFER FULL AND THEN GO GET CHECKSUM
;=========================================================================
GOON DEC BUFRFL ;SET BUFFER FULL FLAG TO FF
    BNE SUSUAL ;  ;GO RETURN
;.........................................................................
;       END OF RECEIVE SERIAL INPUT INTERRUPT ROUTINE*********************
MDEND = *
MDENDH = MDEND/256
MDENDL =  <MDEND
    org $070C
    .BYTE MDENDL,MDENDH ;SET END ADDRESS IN FMS PAST RESIDENT DUP
    ;           BUFFERS DON'T CLOBBER IT.
STAK =  $0100
STAKH = STAK/256
STAKL =  <STAK
;=========================================================================
;  ****  BEGINNING  OF  NON-RESIDENT  PORTION  OF  DUP  **** 
;=========================================================================
NDOS =  MDEND+$0C00 ;END OF THE SYSTEM BUFFERS AND M
NDOSH = NDOS/256
NDOSL =  <NDOS
    org NDOS
PAR      .ds 40 ; PARAMETER AREA
PARH =  PAR/256
PARL =   <PAR
LINE     .ds 80 ; TYPEIN LINE BUFFER
LBUFH = LINE/256
LBUFL =  <LINE
DBUF     .ds $0100 ; DATA BUFFER FOR COPY
DB1 =   DBUF+$80
DB3 =   DBUF-3
DBUFH = DBUF/256
DBUFL =  <DBUF
DB1H =  DB1/256
DB1L =   <DB1
DB3H =  DB3/256
DB3L =   <DB3
DBLL =  0
DBLH =  1       ;DATA BUFFER LENGTH=$100
EDBLL = $FA     ;DATA BUFFER LENGTH USED IN USEPGM
EDBLH = 0       ;MUST BE A MULTIPLE OF 125, SECTOR DATA L
MENUSZ   .ds 1 
PER      .ds 1 
UNNO     .ds 1 
RCNT     .ds 1 
SSTAT    .ds 1 
SWDP     .ds 5 
CSRC     .ds 1 
CDES     .ds 1 
SAVX     .ds 1 
PTR      .ds 1 
IPTR     .ds 1 
CTR      .ds 1 
T1       .ds 2 
BUFLEN = T1     ;SAVE AREA FOR BUFFER LENGTH, USED IN USE
STVEC    .ds 2 ;A TEMP OF SOME KIND
MLT125 = STVEC  ;TEMP STORE FOR MULTIPLE OF 125, USEPGM
SECSIZ   .ds 2 ;USED TO STORE SECTOR SIZE IN BYTES FOR DUP DISK
EOFFLG   .ds 1 ;ENDFILE FLAG FOR SOURCE IN DUPFIL
FTRF     .ds 1 ;FIRST TIME READ FLAG USED IN DUPFIL
TWODRV = FTRF   ;FLAG TO SHOW IF 1 OR 2 DRIVES. USED IN DUPDISK
DTH =   *
DTHH =  DTH/256
DTHL =   <DTH
EDN .BYTE "E:",CR
EDH =   EDN/256
EDL =    <EDN
;     .PAGE 
;=========================================================================
;  ****  DOS  MENU  ****
;=========================================================================
DMENU .BYTE CLSCR
    .BYTE "DISK OPERATING SYSTEM II VERSION 2.0S",CR
    .BYTE "NEW VERSION BY ANGE ",CR,CR
    .BYTE "A. DISK DIRECTORY I. FORMAT DISK",CR
    .BYTE "B. RUN CARTRIDGE  J. DUPLICATE DISK",CR
    .BYTE "C. COPY FILE      K. BINARY SAVE",CR
    .BYTE "D. DELETE FILE(S) L. BINARY LOAD",CR
    .BYTE "E. RENAME FILE    M. RUN AT ADDRESS",CR
    .BYTE "F. LOCK FILE      N. CREATE MEM.SAV",CR
    .BYTE "G. UNLOCK FILE    O. DUPLICATE FILE",CR
    .BYTE "H. WRITE DOS FILES",CR
    .BYTE CDN,CDN,CDN,CDN,CDN
DMEND = *
DULEN = DMEND-DMENU
DULENH = DULEN/256
DULENL =  <DULEN
DMENUH = DMENU/256
DMENUL =  <DMENU
;
DUJPT .WORD DIRLST,STCAR,CPYFIL,DELFIL,RENFIL,LKFIL,ULFIL
    .WORD WBOOT,FMTDSK,DUPDSK,SAVFIL,LDFIL,BRUN,MEMSAV
    .WORD DUPFIL
DUJPTH = DUJPT/256
DUJPTL =  <DUJPT
DUNUM = 15      ;NUMBER OF FUNCTIONS
;     .PAGE 
;=========================================================================
;  ****  DISK  OPERATING  SYS  MONITOR  ****
;=========================================================================
DOSOS LDX #$FF
DOSOSH = DOSOS/256
DOSOSL =  <DOSOS
    CLD ;  ;MAKE SURE DECIMAL MODE OFF
    STX BRKKEY
    INX 
    STX LOADFG
    LDA #2
    STA LMARGN
    LDA #39
    STA RMARGN ;  ;SET MARGINS
    LDA POKMSK ;  ;ENABLE BREAK INTERRRUPTS
    ORA #$80
    STA POKMSK
    STA IRQEN
    JSR INITIO ;  ;CLOSE FILES
;=========================================================================
;       DISK UTILITY MONITOR
;=========================================================================
DU1 LDA #DUNUM
    STA MENUSZ ;  ;SET MENU SIZE.
    LDA # ; <DUJPTL
    STA JMPTBL
    LDA # ; <DUJPTH
    STA JMPTBL+1 ;SET UP JUMP TABLE ADDRESS
;------------------------------------------------------------------------- 
; FALL THRU TO MENU SELECT
;
;=========================================================================
;       MENU SELECT MONITOR -- VECTORS TO ROUTINE SELECTED FROM MENU.
;=========================================================================
SHMEN LDA # ; <DMENUL ;GET MENU ADDRESS
    STA ICBAL
    LDA # ; <DMENUH
    STA ICBAH
    LDA # ; <DULENL ;GET MENU LENGTH
    STA ICBLL
    LDA # ; <DULENH
    STA ICBLH
    JSR DSPMSG ;  ;SHOW MENU
;.........................................................................
;SELECT ITEM FROM MENU
;     .PAGE 
;=========================================================================
;  ****  FUNCTIONS COME HERE WHEN THEY ARE DONE  **** 
MENUSL LDX #$FF ;RESET STACK AT THIS POINT
    TXS 
    INX 
    STX WCFLAG ;  ;CLEAR WILD-CARD FLAG
    LDA # ; <SITL ;SELECT ITEM MESSAGE
    LDX # ; <SITH
    JSR PRNTMSG
    LDA #$40 ;  ;MAKE SURE UPPER CASE
    STA SHFLOK
    JSR CHRGET ;  ;GO GET KEYBOARD CHAR.
;.........................................................................
    CMP #CR ;  ;IF CR REDISPLAY MENU
    BEQ SHMEN
;.........................................................................
    SEC 
    SBC #'A' ;  ;CONVERT ASCII CHAR. TO BINARY # & SUBTRACT
    BMI RANGE ;  ;IF ASCII CHAR NOT A #, GO READ AGAIN
    CMP MENUSZ ;  ;IS THE # ENTERED > MENU SIZE?
    BPL RANGE ;  ;IF YES, GO READ AGAIN.
    ASL
    TAY ;  ;SET INDEX TO (MENU # - 1) * 2
    LDA (JMPTBL),Y
    INY 
    STA RAMLO ;  ;GET STRING POINTER
    LDA (JMPTBL),Y
    STA RAMLO+1
    LDY #1 ;  ;LOAD STRING POINTER INTO REGISTERS
    LDA (RAMLO),Y ;FOR DSPLIN
    TAX 
    DEY 
    LDA (RAMLO),Y
    JSR DSPLIN ;  ;PRINT MODULES INITIAL STRING
    JSR SCROL ;  ;SCROLL INPUT WINDOW
    LDA RAMLO ;  ;INC BY 2 TO POINT PAST STRING POINTER
    CLC 
    ADC #2
    STA RAMLO
    LDA RAMLO+1
    ADC #0 ; CARRY
    STA RAMLO+1 ;PUT HI BYTE.
    JMP (RAMLO) ;JUMP TO ROUTINE SELECTED BY MENU.
RANGE LDA # ; <NSIL
    LDX # ; <NSIH
    JSR DSPLIN ;  ;NO SUCH ITEM MESSAGE
    JMP MENUSL
NSI .BYTE "NO SUCH ITEM",CR
SIT .BYTE "SELECT ITEM OR ",$D2,$C5,$D4,$D5,$D2,$CE
    .BYTE " FOR MENU",CR
NSIH =  NSI/256
NSIL =   <NSI
SITH =  SIT/256
SITL =   <SIT
MNSL =  MENUSL
MNSLH = MNSL/256
MNSLL =  <MNSL
;     .PAGE 
;=========================================================================
;  ****  DIRECTORY  LISTING  ROUTINE  ****
;=========================================================================
DIRLST .WORD DLMG
    JSR GETIC1
    JSR USEBUF ;  ;INIT BUFADR & BUFLEN
    LDX PTR
    LDA #CR
    STA PAR-1,X ;ASSURE GOOD TERM
    LDA PAR-2,X ;LAST CHAR OF SEARCH SPEC
    CMP #':' ;  ;IF COLON, ADD *.*
    BNE GLF
    LDA #'*'
    STA PAR-1,X
    STA PAR+1,X
    LDA #'.'
    STA PAR,X
    LDA #CR
    STA PAR+2,X
    INX 
    INX 
    INX 
    STX PTR
GLF STX SAVX
    LDX #$20
    JSR PIOCB
    JSR GETFIL
    JSR PERX
    LDA #6 ;  ;READ DIR INFO
    LDX #$10
    STA ICAX1,X
    LDA #OPEN ;  ;OPEN
    STA ICCOM,X
    STX CSRC ;  ;COPY SOURCE=DIRECTORY INFO
    CPX #$10
    BNE *+3
    JSR CIOCL
    LDA PTR
    SEC 
    SBC SAVX
    CMP #3 ;  ;IF ONLY 3 CHARS, IS 'D:'CR, USE DEFAULT
    BEQ DLST1
DLST0 JMP PDES ;  ;GO INTO COPY
DLST1 LDX SAVX
    LDA PAR,X
    CMP #'D'
    BNE DLST0
    JMP PDES1 ;  ;GO INTO COPY WITH DES='E:'
;       .PAGE 
DLMG  .BYTE "DIRECTORY--SEARCH SPEC,LIST FILE?",CR
;=========================================================================
;  ****  DELETE  FILE  ROUTINE  ****
;=========================================================================
DELFIL .WORD DEMG
      JSR GETIC1
      JSR PERX ;  ;EXIT IF PARAM ERRORS
;-------------------------------------------------------------------------
    JSR CHKVER ;  ;BE SURE THAT IT IS VER. 2 DISKETTE
;=========================================================================
;       CONTINUE WITH DELETE - ALLOW ONLY FOR DISK DEVICE ID
;=========================================================================
    LDA PAR ;  ;GET DEVICE
    CMP #'D' ;  ;ONLY ALLOW DELETE FOR D:
    BEQ DF1
    LDA # ; <NDFL
    LDX # ; <NDFH
    JSR DSPLIN
    JMP MENUSL
NDF .BYTE "NOT A DISK FILE",CR
NDFH =  NDF/256
NDFL =   <NDF
DF1 LDX #$10
    LDA OPT
    CMP #'N' ;  ;IF OPTION=N, NO QUERY
    BNE DWQ ;  ;NO, DELETE WITH QUERY
    LDA #DELETE
    STA ICCOM,X
    JSR CIOCL
    JMP MENUSL
DWQ LDA # ; <TYQL
    LDX # ; <TYQH
    JSR DSPLIN ;  ;SAY TYPE Y TO DELETE...
    LDA #0
    STA IPTR ;  ;HOW MANY FILES TO SKIP, NONE AT FIRST
    LDX #$20 ;  ;SET UP DELETE IOCB
    LDA #DELETE
    STA ICCOM,X
    LDA # ; <DB3L
    STA ICBAL,X
    LDA # ; <DB3H
    STA ICBAH,X
    LDA #'D'
    STA DBUF-3
    LDA #':'
    STA DBUF-1
    LDA PAR+1 ;  ;DEVICE NUMBER OR : FROM OP INPUT
    CMP #':'
    BNE *+4
    LDA #'1'
    STA DBUF-2 ;  ;KLUDGE KLUDGE KLUDGE
IDRD LDX #$10
    LDA #OPEN
    STA ICCOM,X
    LDA #6
    STA ICAX1,X ;DIR READ OPEN
    LDA #PARL
    STA ICBAL,X
    LDA #PARH
    STA ICBAH,X
    JSR CIOCL
    LDA # ; <DBUFL
    STA ICBAL,X
    LDA # ; <DBUFH
    STA ICBAH,X
    LDA #GETREC
    STA ICCOM,X
    LDA #0
    STA PTR ;  ;HOW MANY FILES WE HAVE SKIPPED
;READ FILENAME FROM DIR, QUERY AND DELETE
RDFN LDX #$10
    LDA #0
    STA ICBLL,X
    LDA #1
    STA ICBLH,X
    JSR CIOCL ;  ;READ A LINE FROM DIRECTORY
    LDA DBUF+1 ;  ;IF FILE LINE, THIS IS BLANK
    CMP #' ' ; '
    BNE DELX ;  ;THIS IS FREE BLOCKS LINE
    INC PTR ;  ;COUNT THIS FILE
    LDA PTR ;  ;HAVE WE SKIPPED ENUF YET
    CMP IPTR
    BMI RDFN ;  ;BR IF NO
    LDX #0 ;  ;PUT PTR
    LDY #2 ;  ;GET PTR
;MASSAGE DELETE FILE NAMES
MDN1 LDA DBUF,Y
    CMP #' '  ; ';END OF FILENAME
    BEQ MDN2
    STA DBUF,X
    INX 
    INY 
    CPX #8
    BMI MDN1
;FILENAME IS MOVED, PUT .EXT
MDN2 LDA #'.'
    STA DBUF,X
    INX 
    LDY #10 ;  ;WHERE EXT IS
MDN3 LDA DBUF,Y
    STA DBUF,X
    INY 
    INX 
    CPY #13
    BMI MDN3
    STX SAVX ;  ;PUT CR HERE LATER
    LDA #'?' ;  ;FOR QUERY
    STA DBUF,X
    INX 
    LDA #CR
    STA DBUF,X
    LDA # ; <DB3L
    LDX # ; <DB3H
    JSR DSPLIN ;  ;GO ASK ABOUT THIS FILE
    JSR CHRGET
    CMP #'Y'
    BNE RDFN ;  ;GO DO NEXT FILENAME
    LDA PTR ;  ;NUMBER FILES WE HAVE GONE THRU SO FAR
    STA IPTR ;  ;IS NEW NUMBER TO SKIP.
    LDX SAVX
    LDA #CR
    STA DBUF,X
    LDX #$20 ;  ;DELETE IOCB
    JSR CIOCL
    JSR CLOS1
    JMP IDRD ;  ;CLOSE AND REOPEN DIR READ FILE
DELX JSR CLOS1 ;  ;CLOSE DIR READ FILE
    JMP MENUSL
CLOS1 LDX #$10
    LDA #CLOSE
    STA ICCOM,X
    JMP CIOCL ;  ;DO CLOSE AND RETURN
TYQ .BYTE "TYPE ",$22,"Y",$22," TO DELETE...",CR
TYQH =  TYQ/256
TYQL =   <TYQ
DEMG .BYTE "DELETE FILE SPEC",CR
;LIST
;     .PAGE 
;=========================================================================
;  ****  COPY FILE ROUTINE  ****
;=========================================================================
CPMG .BYTE "COPY--FROM, TO?",CR
OE  .BYTE "OPTION NOT ALLOWED",CR
OEH =   OE/256
OEL =    <OE
;.........................................................................
;
;
;
;
WCFLAG   .ds 1 
WCSKP1   .ds 1 
WCSKP2   .ds 1 
WCBUFL = 20
WCBUF    .ds WCBUFL 
WCOPYM .BYTE "  COPYING---"
WCBUF2 .BYTE "DN:"
         .ds WCBUFL-3 
CPYFIL .WORD CPMG ;COPY FILE PROMPT
    JSR GETIC1 ;  ;GET SOURCE DEVICE, ETC.
    LDA PTR
    STA SAVX
    LDA PAR ;  ;GET 1ST CHAR. OF DEVICE
    CMP #'D' ;  ;TEST IF IT IS THE DISK
    BNE JMPNWC ;  ;BRANCH IF NOT THE DISK (THEN USE OLD CODE)
    LDX #0 ;  ;LOOK AT SOURCE FILE SPEC.
    JSR LOOKWC ;  ;LOOK FOR WILDCARDS IN FILE SPEC.
    BEQ CPYFL1 ;  ;BRANCH IF WILDCARDS USED IN DISK SPEC.
JMPNWC JMP NOTWC ;USE OLD CODE
CPYFL1 LDA #$80
;.........................................................................
;
WCINIT STA WCFLAG ;SET 'WILDCARD' MODE  (COPY-FILE OR DUPLICATE)
    LDA #0
    STA WCSKP1
;-------------------------------------------------------------------------
WCOPYL LDA #0
    STA WCSKP2
    LDX #$10 ;  ;OPEN DIRECTORY
    LDA #6
    STA ICAX1,X
    LDA #OPEN
    STA ICCOM,X
    LDA # ; <PAR
    STA ICBAL,X
    LDA # ; >PAR
    STA ICBAH,X
    JSR CIOCL
;
;-------------------------------------------------------------------------
WCOPYR LDA #GETREC ;READ DIRECTORY
    STA ICCOM,X
    LDA #WCBUFL
    STA ICBLL,X
    LDA #0
    STA ICBLH,X
    LDA # ; <WCBUF
    STA ICBAL,X
    LDA # ; >WCBUF
    STA ICBAH,X
    JSR CIOCL
;.........................................................................
    LDA WCBUF ;  ;IF 1ST CHAR. OF DIRECTORY READ IS AN # -
    CMP #'0'
    BCC WCGOT
    CMP #':'
    BCS WCGOT
;.........................................................................
    LDA #CLOSE ;  ;ALL DONE -- NORMAL EXIT OF WILDCARDED CMD
    STA ICCOM,X
    JSR CIOCL
    JMP MENUSL
;
;-------------------------------------------------------------------------
WCGOT LDA WCSKP1 ;IF ALREADY COPIED OR SKIPPED THIS FILE
    CMP WCSKP2
    BEQ SKIP1
;.........................................................................
    INC WCSKP2
    BNE WCOPYR
;-------------------------------------------------------------------------
SKIP1 INC WCSKP1
;
    LDA #CLOSE ;  ;CLOSE DIRECTORY READ FILE
    STA ICCOM,X
    JSR CIOCL
;
;-------------------------------------------------------------------------
    LDY #2 ;  ;DON'T COPY .SYS FILES
SYSLOP LDA WCBUF+10,Y
    CMP DOTSYS,Y
    BNE NOSYS
    DEY 
    BPL SYSLOP
    BMI WCOPYL
;-------------------------------------------------------------------------
DOTSYS .BYTE "SYS"
;-------------------------------------------------------------------------
NOSYS LDY #'1' ;  ;CALC SOURCE DRIVE NUMBER
    LDA PAR+1
    CMP #':'
    BEQ WCGOT1
    TAY 
WCGOT1 STY WCBUF2+1
;
;.........................................................................
    LDX #2 ;  ;COMPRESS SPACES, ADD ':', ADD 'CR'
    LDY #3
;-------------------------------------------------------------------------
COMPR1 LDA WCBUF,X
    CMP #$20
    BEQ COMPR2
    STA WCBUF2,Y
    INY 
;-------------------------------------------------------------------------
COMPR2 INX 
    CPX #10
    BNE COMPR1
;.........................................................................
    LDA WCBUF,X
    CMP #$20
    BEQ COMPR5
    LDA #'.'
    STA WCBUF2,Y
    INY 
COMPR3 LDA WCBUF,X
    CMP #$20
    BEQ COMPR4
    STA WCBUF2,Y
    INY 
COMPR4 INX 
    CPX #13
    BNE COMPR3
;-------------------------------------------------------------------------
COMPR5 LDA #CR
    STA WCBUF2,Y
;.........................................................................
;
    LDA # ; <WCOPYM ;PRINT 'COPYING---DEV:FILENAME.EXT' MESSAGE
    LDX # ; >WCOPYM
    JSR DSPLIN
;.........................................................................
    BIT WCFLAG
    BVC WCOPY ;  ;BRANCH TO MIDDLE OF DUPLICATE FILE ROUTINE
;
    LDX #$10 ;  ;SET UP BUFFER ADDRESS TO POINT TO WILDCARD
    LDA # ; <WCBUF2
    STA ICBAL,X
    LDA # ; >WCBUF2
    STA ICBAH,X
    JMP WCDUPS
;-------------------------------------------------------------------------
WCOPY JSR USEPGM ;SET BUFFER SIZES
    LDX #$10 ;  ;OPEN COPY SORCE FILE
    LDA #OPEN
    STA ICCOM,X
    LDA #4
    STA ICAX1,X
    LDA # ; <WCBUF2
    STA ICBAL,X
    LDA # ; >WCBUF2
    STA ICBAH,X
    STX CSRC
    JSR CIOCL
;.........................................................................
    LDX #$20
    JSR PIOCB ;  ;GET COPY DESTINATION FILE
    LDA PTR ;  ;SAVE  PTR,IPTR BECAUSE MIGHT REPEAT GETTING
MES
    PHA 
    LDA IPTR
    PHA 
    JSR GETFIL ;  ;GET 2ND FILE NAME TO PAR
    PLA ;  ;RECOVER  IPTR,PTR
    STA IPTR
    PLA 
    STA PTR
    LDX SAVX
    LDA PAR,X
    CMP #'D'
    BEQ WCOPY0
    JMP PDES ;  ;JUMP TO OLD COPY-FILE CODE IF NOT A DISK
;-------------------------------------------------------------------------
WCOPY0 LDY #'1' ;  ;CALCULATE DESTINATION DRIVE #
    LDA PAR+1,X
    CMP #':'
    BEQ WCOPY1
;-------------------------------------------------------------------------
    TAY 
WCOPY1 CPY WCBUF2+1
    BNE WCOPY2
    JSR CLOSX ;  ;CAN NOT COPY TO SAME DRIVE NUMBER -- ERROR
;.........................................................................
    JMP ODMS
;
;-------------------------------------------------------------------------
WCOPY2 LDX #$20
    STY WCBUF2+1 ;CHANGE FILESPEC TO DESTINATION
    LDA # ; <WCBUF2
    STA ICBAL,X
    LDA # ; >WCBUF2
    STA ICBAH,X
    JMP OPDES1 ;  ;CONTINUE INTO OLD COPY-FILE CODE
;
;-------------------------------------------------------------------------
NOTWC = *
    LDX #$20 ; IOCB 3
    JSR PIOCB
    JSR GETFIL ;  ;GET SECOND FILENAME
;=========================================================================
;       MAKE SURE DESTINATION IS NOT DOS.SYS
;=========================================================================
    LDX SAVX ;  ;ENTRY-INDEX TO DEST FILE SPEC
    JSR TSTDOS ;  ;WON'T RETURN IF IS DOS.SYS
;.........................................................................
    LDX SAVX
    JSR LOOKWC
    BNE NWCIND ;  ;BRANCH IF NO WILDCARDS IN DESTINATION
    LDA # ; <NWAL
    LDX # ; <NWAH
    JSR DSPLIN
    JMP MENUSL
NWA .BYTE "WILD CARDS NOT ALLOWED IN DESTINATION",CR
NWAH =  NWA/256
NWAL =   <NWA
NWCIND = *
    JSR PERX ;  ;IF PARAM ERRS, EXIT
    JSR USEPGM ;  ;ASK USER IF CAN USE PGM AREA OR DATA BUF
PSRC =  *
    LDA PAR ;  ;GET 1ST LETR OF PARAM
    CMP #'K'
    BEQ ODMS ;  ;K: GETS 'OPTION DOES NOT MAKE SENSE' FOR NOW
    CMP #'C'
    BEQ ODMS ;  ;C: GETS 'OPTION DOES NOT MAKE SENSE' FOR NOW
    CMP #'E' ;  ;E: AS SOURCE IS SPECIAL
    BNE OPSRC ;  ;IF NO THEN OPEN SOURCE FILE
    LDX #0
    STX CSRC
    JMP PDES
OPSRC CMP #'S'
    BEQ ODMS ;  ;S: AS SOURCE GETS 'OPTION DOES NOT MAKE SENSE' FOR NOW
;=========================================================================
;       OPEN SOURCE FILE
;=========================================================================
    LDX #$10
    LDA #OPEN
    STA ICCOM,X
    LDA #4 ;  ;OPEN IN
    STA ICAX1,X
    STX CSRC
    CPX #$10
    BNE *+33
    JSR CIOCL ;  ;OPEN SOURCE FILE HERE
;=========================================================================
;       READY FOR OPEN OF DESTINATION
;=========================================================================
PDES LDX SAVX
    LDA PAR,X
;.........................................................................
    CMP #'K' ;  ;IS DEST KEYBOARD?
    BEQ ODMS ;  ;YES, THEN CAN'T DO IT
;.........................................................................
    CMP #'E' ;  ;CHECK FOR SPECIAL CASE
    BNE OPDES ;  ;IF NOT
PDES1 LDA #0 ;  ;SPECIAL CASE - DONT OPEN, USE EXISTING IOCB
    STA CDES
    JMP DOCPY
ODMS LDA #OEL
    LDX #OEH ;  ;SAY OPTION NOT ALLOWED
    JSR DSPLIN
    JSR CLOSX ;  ;CLOSE IOCB 1 & 2
    JMP MENUSL
;-------------------------------------------------------------------------
OPDES CMP #'C'
    BEQ ODMS ;  ;C: GETS 'OPTION DOESNOT MAKE SENSE' FOR NOW
    LDX OPT ;  ;GET 2ND FILE OPTION
;.........................................................................
    CPX #'A' ;  ;APPEND TO DISK FILE
    BNE OPDES1
    CMP #'D'
    BNE ODMS
    LDA #9
    BNE OPDES3
OPDES1 LDA #8
OPDES3 LDX #$20
    STA ICAX1,X ; OPEN TYPE OUT
    LDA #OPEN
    STA ICCOM,X ; OPEN
    STX CDES
    JSR CIOCL
    LDA #0
    STA ICAX2,X
;COPY FROM CSRC TO CDES
DOCPY LDA #GETCHR
GC1 LDX CSRC
    LDY CDES
    STA ICCOM,X
    LDA #PUTCHR
    STA ICCOM,Y
    LDA BUFADR ;  ;ADDRESS OF BUFFER - EITHER
    STA ICBAL,X ;PGM AREA (MEMLO) OR DATA BUFFER (DBUF)
    STA ICBAL,Y
    LDA BUFADR+1 ;BUFADR IN LSB,MSB ORDER
    STA ICBAH,X
    STA ICBAH,Y
CLOOP LDX CSRC
    LDA BUFLEN ;  ;LENGTH OF BUFFER ADDRESSED
    STA ICBLL,X ;BY BUFADR
    LDA BUFLEN+1 ;BOTH BUFADR & BUFLEN ARE ASSIGNED
    STA ICBLH,X ;IN SUBROUTINE USEPGM
    JSR CIO ;  ;READ FROM INPUT
    STY SSTAT
    LDX CDES
    LDY CSRC
    LDA ICBLL,Y
    STA ICBLL,X
    LDA ICBLH,Y
    STA ICBLH,X
    ORA ICBLL,Y ;IF SOURCE FILE LENGTH = 0
    BEQ CKRS ;  ;DONT DO WRITE
    JSR CIOCL ;  ;WRITE, ABORT IF ERROR
CKRS LDA SSTAT ;  ;GET READ OPERATION STATUS BACK
    BPL CLOOP ;  ;IF OK, GO READ SOME MORE
    CMP #$88 ;  ;EOF STATUS
    BEQ *+5
    JMP CIOER ;  ;IF NOT, ABORT
CLOC LDX CSRC
    BEQ DU4 ;  ;IF E:, DONT CLOSE
;=========================================================================
;CLOSE SOURCE FILE
;=========================================================================
    LDA #CLOSE
    STA ICCOM,X
    JSR CIO
DU4 LDX CDES
    BEQ DU3 ;  ;IF DES=E:
    LDA #CLOSE
    STA ICCOM,X
    JSR CIO
DU3 LDX CDES
    BNE DU6
    LDA # ; <DDSK+1
    LDX # ; >DDSK+1
    JSR PRNTMSG ;PRINT A CR BEFORE SELECT OR WILDCARD PRO
DU6 =   *
;.........................................................................
    BIT WCFLAG
    BPL DU5
    JMP WCOPYL ;  ;BRANCH BACK TO WILD CARD LOOP
DU5 JMP MENUSL
;     .PAGE 
;=========================================================================
;  ****  RENAME  FILE  ROUTINE  **** 
;=========================================================================
;       RENAME SETS UP IOCB #1 WITH THE OLD FILE NAME AND THE BUFFER ADD
;       POINTS TO THE NEW FILE NAME.  THE NEW FILE SPECIFICATION CANNOT
;       A DEVICE ID.  THE DEVICE ID IS THE SAME AS SPECIFIED FOR THE OLD
;               EG D2:ABC.S2,QQQ.R3     THIS RENAMES ABC.S2 ON DRIVE #2
;                                       QQQ.R3
;-------------------------------------------------------------------------
RENFIL .WORD RNMG
    JSR GETIC1 ;  ;GET OLD FILE SPEC & PUT ADDR IN IOCB
    JSR GETNAME ;GET NEW FILE NAME
    JSR PERX ;  ;EXIT IF PARAMETER ERRORS
;.........................................................................
    JSR CHKVER ;  ;MAKE SURE VER 2 DISKETTE
;=========================================================================
;       CONTINUE WITH RENAME
;=========================================================================
    LDA #RENAME
    LDX #$10
    STA ICCOM,X
    JSR CIOCL
    JMP MENUSL
RNMG .BYTE "RENAME - GIVE OLD NAME, NEW",CR
;=========================================================================
;*******************   SUBROUTINE   *******************
;
;       MAKE SURE THIS IS A VERSION 2 FORMAT DISK
;=========================================================================
CHKVER LDY #1 ;  ;ASSUME DRIVE 1- GET DRIVE #
    LDA PAR+1 ;  ;TEST CHAR 2 OF FILE SPEC FOR SEMICOLON
    CMP #':' ;  ;IF IS, USING DEFAULT DRIVE (1)
    BEQ DRV1 ;  ;IT IS, SO SAVE DRIVE #
    AND #$0F ;  ;ELSE CHAR 2 IS ASCII REP OF DRIVE #
    TAY ;  ;CONVERT TO BINARY & SAVE IT
DRV1 STY UNNO ;  ;SAVE DRIVE #
;.........................................................................
    JMP TSTVER2 ;TEST FOR VERSION 2 DISK- WON'T RETURN IF
;.........................................................................
;     .PAGE 
;=========================================================================
;  ****  FORMAT  DISK  ROUTINE  ****
;=========================================================================
FMTDSK .WORD WHD
    JSR GETLIN
    JSR GETDN
    CLC 
    ADC #'0'
    STA DDSK
    STA CDSK
    JSR PERX
    LDA # ; <VFML ;QUERY TO VERIFY DRIVE NUMBER
    LDX # ; <VFMH
    JSR DSPLIN
    JSR CHRGET
    CMP #'Y' ;  ;SEE IF OK
    BNE FMX
    LDA # ; <FDPL
    LDX #$10
    STA ICBAL,X
    LDA # ; <FDPH
    STA ICBAH,X
    LDA #FORMAT
    STA ICCOM,X
    JSR CIOCL ;  ;CALL CIO TO DO FORMAT
FMX JMP MENUSL ;  ;EXIT.
WHD .BYTE "WHICH DRIVE TO FORMAT?",CR
VFM .BYTE "TYPE ",$22,"Y",$22," TO FORMAT DISK "
DDSK     .ds 1 
    .BYTE CR
FDP .BYTE "D"
CDSK     .ds 1 
    .BYTE ":",CR
WHDH =  WHD/256
WHDL =   <WHD
VFMH =  VFM/256
VFML =   <VFM
FDPH =  FDP/256
FDPL =   <FDP
;     .PAGE 
;=========================================================================
;  ****  START  CARTRIDGE  ROUTINE  ****
;=========================================================================
SYVBL = SYSVBV
SYVBLH = SYVBL/256
SYVBLL =  <SYVBL
XTVBL = XITVBV
XTVBLH = XTVBL/256
XTVBLL =  <XTVBL
STCAR .WORD SCMG ; NO MSG, PRINT A <CR>
ROMTST = $BFFD
    LDY ROMTST ;  ;TEST IF RAM OR OTHER
    LDA #$AA ;  ;PATTERN #1
    STA ROMTST
    CMP ROMTST
    BNE NOTRAM ;  ;BRANCH IF NOT RAM
    LDA #$55 ;  ;PATTERN #2
    STA ROMTST
    CMP ROMTST
    BNE NOTRAM ;  ;BRANCH IF NOT RAM
;-------------------------------------------------------------------------
    STY ROMTST ;  ;THERE IS VALID RAM - SAY NO CART
NOCART LDA # ; <NCAL
    LDX # ; <NCAH ;SAY NO CART
    JSR DSPLIN
    JMP MENUSL
;=========================================================================
;       CHECK IF ROM OR EMPTY ADDRESS SPACE
;=========================================================================
NOTRAM LDA $BFFC ;KNOWN ROM ZERO BYTE
    BNE NOCART ;  ;BRANCH IF EMPTY ADDRESS SPACE
;-------------------------------------------------------------------------
    TAX ;  ;SINCE EMPTY ADDRESS SPACE GIVES A RANDOM
CKCART LDA ROMTST ;VALUE, TEST THE SAME LOCATION MANY TIMES
    BEQ NOCART ;  ;BRANCH IF NO CARTRIDGE
    CMP ROMTST
    BNE NOCART ;  ;BRANCH IF NO CARTRIDGE
    INX 
    BNE CKCART ;  ;LOOP BACK
;
;=========================================================================
;       RESET VERTICAL BLANK VECTORS BEFORE ENTERING CART
;=========================================================================
    JSR INITIO
    LDA #6 ;  ;SET VVBLKI
    LDX # ; <SYVBLH ;HI BYTE
    LDY # ; <SYVBLL
    JSR SETVBV
    LDA #7 ;  ;SET VVBLKD
    LDX # ; <XTVBLH
    LDY # ; <XTVBLL
    JSR SETVBV
    JMP CLMJMP
;     .PAGE 
NCA .BYTE "NO CARTRIDGE"
SCMG .BYTE CR
NCAH =  NCA/256
NCAL =   <NCA
;
;=========================================================================
;       *******  RUN AT ADDRESS *******   
;=========================================================================
;
;
BRUN .WORD BRMG
    JSR GETLIN
    JSR GETNO
    JSR PERX
    STA RAMLO
    STX RAMLO+1
    LDA CTR
    CMP #4
    BEQ MOUT1 ;  ;RETURN TO MENU IF NO RUN ADDRESS GIVEN
    JSR INITIO ;  ;CLOSE ALL IOCB'S, THEN REOPEN S/E
    JMP LMTR ;  ;LOAD MEM.SAV & JUMP TO ADDRESS
;
;-------------------------------------------------------------------------
BRMG .BYTE "RUN FROM WHAT ADDRESS?",CR
;=========================================================================
;  ****  CREATE  MEM.SAV  FILE  ON  DISK  **** 
;=========================================================================
MEMS .BYTE "TYPE ",$22,"Y",$22," TO CREATE MEM.SAV",CR
MEMSAV .WORD MEMS
    JSR CHRGET ;  ; GET CHAR (CR)
    CMP #'Y'
    BNE MOUT ;  ;BRANCH IF USER'S ANSWER NOT A Y
    JSR MEMSVQ ;  ;TRY TO OPEN MEM.SAV
    BMI MCONT ;  ; IF FILE DOESN'T EXIST THEN JUMP
    LDA # ; <MEMSGL ; ELSE 'MEMORY.SAVE' AREADY EXIST
    LDX # ; <MEMSGH ;
    JSR DSPLIN ;  ;DISPLAY THIS FACT
MOUT JSR CLOSX ;  ;EXIT AFTER CLOSING IOCB1
MOUT1 JMP MENUSL ;
;=========================================================================
; WRITE MEMORY.SAVE TO DISK
;=========================================================================
MCONT JSR MWRITE ; WRITE FILE
    BPL MOUT
MERR JMP CIOER1 ; DISPLAY ERROR
;-------------------------------------------------------------------------
MEMSG .BYTE "MEM.SAV FILE ALREADY EXISTS",CR
MEMSGH = MEMSG/256
MEMSGL =  <MEMSG
;     .PAGE 
;=========================================================================
;  ****  WRITE  DOS  &  DUP  **** 
;=========================================================================
WBOOT .WORD DOSDRV ; ADDRESS OF DRIVE # PROMPT
;=========================================================================
;       RETREIVE DRIVE NUMBER FROM USER.
;=========================================================================
    JSR GETLIN ;  ;GET INPUT
    JSR GETDN ;  ;GET DRIVE AS NUMBER, VERIFY IT
    JSR PERX ;  ;EXIT IF ERROR
    STA UNNO ;  ;SAVE IT FOR TSTVER2
    ORA #'0' ;  ;TURN BACK TO ASCII REP
    STA DS+1 ;  ;STORE IN DOS.SYS FILE SPEC
    STA QWMG+31 ;& IN PROMPT
;.........................................................................
    JSR TSTVER2 ;TEST IF VERSION 2 DISK - IF ISN'T WON'T
;=========================================================================
;       ASK USER IF CAN WRITE DOS & DUP TO SPECIFIED DRIVE
;=========================================================================
    LDA # ; <QWMGL ;PRINT PROMPT
    LDX # ; <QWMGH
    JSR DSPLIN
    JSR CHRGET
    CMP #'Y'
    BNE WBX ;  ;EXIT UNLESS Y
;=========================================================================
;       TELL USER WRITING DOS FILES AND WRITE DOS.SYS FIRST- JUST OPEN IT
;=========================================================================
    LDA # ; <WBMGL
    LDX # ; <WBMGH
    JSR DSPLIN
;.........................................................................
    LDA #OPEN
    LDX #$10 ;  ;OPEN DOS.SYS ON IOCB #1
    STA ICCOM,X ;WILL CAUSE FMS TO REWRITE BOOT SECTOR
    LDA # ; <DSL  ;& A COPY OF DOS.SYS
    STA ICBAL,X
    LDA # ; <DSH
    STA ICBAH,X
    LDA #8
    STA ICAX1,X
    JSR CIOCL ;  ;DO OPEN, IF ERROR GOTO MENU
;.........................................................................
    LDX #$10
    LDA #CLOSE
    STA ICCOM,X
    JSR CIOCL ;  ;DONE CLOSE IT.
;=========================================================================
;       WRITE DUP.SYS - SWAP AREA FILE
;=========================================================================
    LDX #11 ;  ;MOVE 11 CHARS
MDUPBL LDA DUPSYS-1,X
    STA PAR-1,X ;MOVE FILE NAME TO PARAMETER LIST
    DEX 
    BNE MDUPBL
    LDA DS+1 ;  ;GET DRIVE NUMBER
    STA PAR+1 ;  ;PUT IT IN DUP.SYS FILE SPEC
;.........................................................................
    STX PTR
    LDX #$10
    JSR PIOCB ;  ;PUT FILE NAME POINTER IN IOCB
    LDA # ; <DTHL
    STA LDST
    LDA # ; <DTHH
    STA LDST+1
    LDA # ; <NMDUP
    STA LDND
    LDA # ; <LENL
    STA WDRL+1
    LDA # ; <LENH
    STA WDRH+1
    LDA # ; >NMDUP
    STA LDND+1
    PHA ;  ;NO /A
    LDA # ; <DOSOS
    STA RUNAD
    LDA # ; >DOSOS
    STA RUNAD+1 ;SET DUP.SYS RUN ADDRESS
    DEC RUNQ+1 ;  ;SET RUN FLAG
    JMP NRUNAD ;  ;WRITE DUP.SYS
WBX JMP MENUSL
DOSDRV .BYTE "DRIVE TO WRITE DOS FILES TO?",CR
WBMG .BYTE "WRITING NEW DOS FILES",CR
WBMGH = WBMG/256
WBMGL =  <WBMG
;     .PAGE 
QWMG .BYTE "TYPE ",$22,"Y",$22," TO WRITE DOS TO DRIVE 2.",CR
QWMGH = QWMG/256
QWMGL =  <QWMG
DS  .BYTE "D2:DOS.SYS",CR
DSH =   DS/256
DSL =    <DS
WVD .BYTE "ERROR - NOT VERSION 2 FORMAT.",CR
WVDH =  WVD/256
WVDL =   <WVD
;     .PAGE 
;=========================================================================
;  **** TEST FOR VERSION 2 FORMAT - SUBROUTINE ****
;=========================================================================
;       SUBROUTINE - TSTVER2
;-------------------------------------------------------------------------
;       
;       READS THE DISK'S VTOC AND CHECKS IF VERSION BYTE IS SET AS 2.
;                                                                    
;               ENTRY     - DRIVE # STORED IN UNNO                   
;               EXIT      - RETURNS ONLY IF IS A VERSION 2 DISK      
;                           ELSE DOES AN ERROR EXIT BACK TO MENU     
;               CALLS     - DRVSTAT AND RVTOC                        
;               CALLED BY - DELFIL, RENFIL, WBOOT.                   
;
;=========================================================================
;       GET DRIVE TYPE SO KNOW CORRECT SECTOR SIZE - NEEDED FOR RVTOC
;=========================================================================
TSTVER2 = *
    LDA #0 ;  ;GET DRIVE TYPE IN SECSIZ
    STA SECSIZ ;  ;ASSUME 256 - NEEDED BY RVTOC
    LDA UNNO ;  ;GET DRIVE #
    JSR DRVSTAT ;FIND OUT TYPE - CARRY FLAG
    BCS OKTYP ;  ;BRANCH IF 256 TYPE
    LDA #$80 ;  ;ELSE SET AS 128 BYTE DEVICE
    STA SECSIZ
;=========================================================================
;       READ THE VTOC & CHECK IF VERSION 2
;=========================================================================
OKTYP JSR RVTOC ;READ IN VTOC TO DBUF
    LDA DBUF ;  ;1ST BYTE IS VERSION #
    CMP #2 ;  ;IS IT VERSION 2?
    BEQ SMVRS ;  ;YES, SAME VERSION - RETURN
;=========================================================================
;       NOT A VERSION 2 DISK - PRINT MSG & GOTO MENU
;=========================================================================
    LDA # ; <WVDL ;ELSE, NOT SAME VERSION
    LDX # ; <WVDH ;PRINT INCOMPATIBLE MSG
    JSR DSPLIN
    JMP MENUSL ;  ;GOTO MENU
;=========================================================================
;       DISK IS VERSION TWO SO RETURN
;=========================================================================
SMVRS RTS ;  ;RETURN
;     .PAGE 
;=========================================================================
;  ****  LOAD  USER  FILE  FUNCTION  ****
;=========================================================================
LDFIL .WORD LFMG
    JSR GETIC1
    LDA #0
    LDX OPT
    STA OPT
    CPX #'N' ;  ;IS OPTION N FOR DON'T LOAD AND GO?
    BNE NOTN ;  ;BRANCH IF NOT
    DEC OPT
NOTN JSR PERX
    JSR LOAD
    CPX #0 ;  ;PROCESS LOAD SUBR RESPONSE
    BEQ LDFX ;  ;BRANCH IF LOAD WAS OK
    CPX #3
    BEQ NLF ;  ;IF BAD LOAD FILE
    TYA ;  ;OTHERWISE WE GOT A CIO ERROR
    JMP CIOER ;  ;GO SAY WHAT IT IS
NLF LDA # ; <BLFL
    LDX # ; <BLFH
    JSR DSPLIN ;  ;BAD LOAD FILE MSG
    JSR CLOSX ;  ;CLOSE THE FILE
LDFX JMP MENUSL ;EXIT
BLF .BYTE "BAD LOAD FILE",CR
BLFH =  BLF/256
BLFL =   <BLF
LFMG .BYTE "LOAD FROM WHAT FILE?",CR
;=========================================================================
;  ****  LOCK  &  UNLOCK  FILE  COMMANDS  **** 
;=========================================================================
LKFIL .WORD LKMG ;DO LOCK
    JSR GETIC1
    JSR PERX
    LDA #LOCK
    LDX #$10
    STA ICCOM,X
    JSR CIOCL
    JMP MENUSL
LKMG .BYTE "WHAT FILE TO LOCK?",CR
;-------------------------------------------------------------------------
ULFIL .WORD ULMG ;DO UNLOCK
    JSR GETIC1
    JSR PERX
    LDA #UNLOCK
    LDX #$10
    STA ICCOM,X
    JSR CIOCL
    JMP MENUSL
ULMG .BYTE "WHAT FILE TO UNLOCK?",CR
;DUPLICATE DISK ROUTINE
DDMG .BYTE "DUP DISK-SOURCE,DEST DRIVES?",CR
OK  .BYTE "TYPE ",$22,"Y",$22," IF OK TO USE PROGRAM AREA",CR
OKH =    >OK
OKL =    <OK
CMSI .BYTE "CAUTION: A ",$22,"Y",$22," INVALIDATES MEM.SAV.",CR
CMSIH =  >CMSI
CMSIL =  <CMSI
RVTOC
    LDA #1
    STA DSHI
    LDA #$68
    STA DSLO
    LDA # ; <DBUFH
    STA DBUFHI
    LDA # ; <DBUFL
    STA DBUFLO
    JSR RSEC1
    LDA #0
    STA PTR
    LDA DBUF+$0A
    STA CSRC
    LDA #8
    STA IPTR
    LDA #0
    STA DSHI
    LDA #1
    STA DSLO
    RTS 
DUPDSK .WORD DDMG
    LDA #0
    STA TWODRV
    JSR GETLIN
    JSR GETDN
    STA UNNO
    JSR GETDN
    STA CDES
    JSR PERX
    LDA #$80
    STA SECSIZ
    LDA #0
    STA SECSIZ+1
    LDA UNNO
    JSR DRVSTAT
    BCC ONE28
    LDX #0
    STX SECSIZ
    INX 
    STX SECSIZ+1
ONE28
    LDA CDES
    JSR DRVSTAT
    BCC IS128
    BIT SECSIZ
    BPL SAME
INCOMP
    LDA # ; <NCDRL
    LDX # ; <NCDRH
    JSR DSPLIN
    JMP MENUSL
IS128
    BIT SECSIZ
    BPL INCOMP
SAME
    LDA UNNO
    CMP CDES
    BEQ SDD
    LDX # ; <IBDH
    LDA # ; <IBDL
    JSR DSPLIN
    JSR CHRGET
    DEC TWODRV
    BMI DODKDP
IBD .BYTE "INSERT BOTH DISKS, TYPE RETURN",CR
IBDH =   >IBD
IBDL =   <IBD
NCDR .BYTE "ERROR - DRIVES INCOMPATIBLE.",CR
NCDRH =  >NCDR
NCDRL =  <NCDR
SDD
    LDA # ; <ISDL
    LDX # ; <ISDH
    JSR DSPLIN
    JSR CHRGET
DODKDP
    LDA # ; <NMDUPL
    STA STVEC
    LDA # ; <NMDUPH
    STA STVEC+1
    LDA MEMTOP
    SEC 
    SBC SECSIZ
    STA T1
    LDA MEMTOP+1
    SBC SECSIZ+1
    STA T1+1
    LDA T1
    CMP STVEC
    LDA T1+1
    SBC STVEC+1
    BCS ENUF
NORM
    LDA #NRML
    LDX #NRMH
    JSR DSPLIN
    JMP MENUSL
ENUF
    JSR CKMEM
    LDA #0
    STA OPT
    JSR RVTOC
    LDA DSLO
    STA SWDP
    LDA DSHI
    STA SWDP+1
    LDA PTR
    STA SWDP+2
    LDA IPTR
    STA SWDP+3
    LDA CSRC
    STA SWDP+4
    JMP LRS1
;
DORD
    LDA #0
    STA OPT
    BIT TWODRV
    BMI LRS1
    LDA # ; <ISDL
    LDX # ; <ISDH
XBLK
    JSR DSPLIN
    JSR CHRGET
LRS1
    JSR DOSWDP
LRS
    JSR AAM
    BMI ASPT
    BIT OPT
    BMI DOW
    JSR RSEC1
    JMP IOD
DOW
    JSR DKWRT
IOD
    LDA DBUFLO
    CLC 
    ADC SECSIZ
    STA DBUFLO
    LDA DBUFHI
    ADC SECSIZ+1
    STA DBUFHI
ASPT
    JSR ASP
    BEQ STDD1
    LDA T1
    CMP DBUFLO
    LDA T1+1
    SBC DBUFHI
    BCS LRS
;
STDD
    LDA OPT
    BMI DORD
STDD2
    DEC OPT
    BIT TWODRV
    BMI LRS1
    LDA # ; <IDDL
    LDX # ; <IDDH
    JMP XBLK
STDD1
    LDA OPT
    BPL STDD2
    JMP MENUSL
;
DOSWDP
    LDY #4
SWLOP
    LDA SWATL,Y
    STA RAMLO
    LDA SWATH,Y
    STA RAMLO+1
    LDX #0
    LDA (RAMLO,X)
    PHA 
    LDA SWDP,Y
    STA (RAMLO,X)
    PLA 
    STA SWDP,Y
    DEY 
    BPL SWLOP
    LDA STVEC
    STA DBUFLO
    LDA STVEC+1
    STA DBUFHI
    RTS 
;
DSLOH =  >DSLO
DSLOL =  <DSLO
DSHIH =  >DSHI
DSHIL =  <DSHI
PTRH =   >PTR
PTRL =   <PTR
IPTRH =  >IPTR
IPTRL =  <IPTR
CSRCH =  >CSRC
CSRCL =  <CSRC
;
SWATL .BYTE DSLOL,DSHIL,PTRL,IPTRL,CSRCL
SWATH .BYTE DSLOH,DSHIH,PTRH,IPTRH,CSRCH
NRM .BYTE "NOT ENOUGH ROOM",CR
ISD .BYTE "INSERT SOURCE DISK,TYPE RETURN",CR
IDD .BYTE "INSERT DESTINATION DISK,TYPE RETURN",CR
NRMH =   >NRM
NRML =   <NRM
ISDH =   >ISD
ISDL =   <ISD
IDDH =   >IDD
IDDL =   <IDD
AAM
    ASL CSRC
    DEC IPTR
    BNE CBIT
    INC PTR
    LDX PTR
    LDA DBUF+$0A,X
    STA CSRC
    LDA #8
    STA IPTR
CBIT
    LDA CSRC
    RTS 
;
ASP
    LDA DSLO
    CMP #208
    BNE NXS
    LDA DSHI
    CMP #2
    BEQ ASPX
NXS
    INC DSLO
    BNE ASPX
    INC DSHI
ASPX
    RTS 
;
RSEC1
    LDA UNNO
    STA DUNIT
    CLC 
    PHP 
    JMP CLDKH
;
DKWRT
    LDA CDES
    STA DUNIT
    SEC 
    PHP 
CLDKH
    LDA #2
    STA RCNT
CLD1
    LDX #1
    BIT SECSIZ
    BMI NOT256
    INX 
NOT256
    PLP 
    PHP 
    JSR BSIOR
    BPL DRTS
    DEC RCNT
    BPL CLD1
    JMP CIOER1
DRTS
    PLP 
    RTS 
;
CKMEM
    LDA WARMST
    BEQ CPTR1
    LDA # ; <OKL
    LDX # ; <OKH
    JSR DSPLIN
    LDA # ; <CMSIL
    LDX # ; <CMSIH
    JSR DSPLIN
    JSR CHRGET
    CMP #'Y'
    BNE DDXT
    LDA #0
    STA WARMST
    STA MEMFLG
CPTR1
    RTS 
DDXT
    PLA 
    PLA 
    JMP MENUSL
;
DRVSTAT
    STA DUNIT
    LDA #STAREQ
    STA DCOMND
    LDA #2
    STA RCNT
DOSTAT
    JSR DKHND
    BPL CHKTYP
    DEC RCNT
    BPL DOSTAT
    JMP CIOER1
CHKTYP
    CLC 
    LDA DVSTAT
    AND #$20
    BEQ RETSTAT
    SEC 
RETSTAT
    RTS 
;
DPFM .BYTE "NAME OF FILE TO MOVE?",CR
;
DUPFIL .WORD DPFM
    JSR GETIC1
    JSR PERX
    LDA PAR
    CMP #'D'
    BEQ ISDISK
    JMP ODMS
ISDISK
    JSR USEPGM
    LDX # ; <ISDH
    LDA # ; <ISDL
    JSR DSPLIN
    JSR GETLIN
    JSR PERX
    JSR LOOKWC
    BNE NOWC
    LDA #$40
    JMP WCINIT
NOWC
    LDX #0
    JSR TSTDOS
WCDUPS
    LDX #$10
    LDA #OPEN
    STA ICCOM,X
    LDA #4
    STA ICAX1,X
    JSR CIOCL
    LDA #0
    STA EOFFLG
    STA FTRF
DODUP
    LDX #$10
    LDA BUFADR
    STA ICBAL,X
    LDA BUFADR+1
    STA ICBAH,X
    LDA BUFLEN
    STA ICBLL,X
    LDA BUFLEN+1
    STA ICBLH,X
    LDA #GETCHR
    STA ICCOM,X
    JSR CIO
    BPL INSDES
    CPY #EOF
    BEQ SETFLG
    JMP CIOER1
SETFLG
    DEC EOFFLG
INSDES
    LDX # ; <IDDH
    LDA # ; <IDDL
    JSR DSPLIN
    JSR GETLIN
    BIT PER
    BPL DODEST
    JMP CLSSRC
DODEST
    LDX #$20
    LDY #9
    LDA FTRF
    BNE OPNDES
    LDY #8
    INC FTRF
OPNDES
    TYA 
    STA ICAX1,X
    LDA #OPEN
    STA ICCOM,X
    LDA #PARL
    LDY #PARH
    BIT WCFLAG
    BVC SKIPWC
    LDA # ; <WCBUF2
    LDY # ; >WCBUF2
SKIPWC
    STA ICBAL,X
    TYA 
    STA ICBAH,X
    JSR CIOCL
    LDY #$10
    LDX #$20
    LDA #0
    CMP ICBLL,Y
    BNE DOWRIT
    CMP ICBLH,Y
    BEQ CLSDES
DOWRIT
    LDA #PUTCHR
    STA ICCOM,X
    LDA BUFADR
    STA ICBAL,X
    LDA BUFADR+1
    STA ICBAH,X
    LDA ICBLL,Y
    STA ICBLL,X
    LDA ICBLH,Y
    STA ICBLH,X
    JSR CIOCL
;
CLSDES
    LDA #CLOSE
    STA ICCOM,X
    JSR CIOCL
    LDA EOFFLG
    BNE CLSSRC
    LDX # ; <ISDH
    LDA # ; <ISDL
    JSR DSPLIN
    JSR GETLIN
    BIT PER
    BMI CLSSRC
    JMP DODUP
CLSSRC
    LDX #$10
    LDA #CLOSE
    STA ICCOM,X
    JSR CIO
    BIT WCFLAG
    BVC DUPFEX
    LDX # ; <ISDH
    LDA # ; <ISDL
    JSR DSPLIN
    JSR GETLIN
    JSR PERX
    JMP WCOPYL
DUPFEX = *
    JMP MENUSL
USEPGM
    LDA WARMST
    BEQ USEDB4
    LDA # ; <OKL
    LDX # ; <OKH
    JSR DSPLIN
    LDA # ; <CMSIL
    LDX # ; <CMSIH
    JSR DSPLIN
    JSR CHRGET
    CMP #'Y'
    BNE USEBUF
;
USEDB4
    LDA #0
    STA WARMST
    STA MEMFLG
    LDA # ; <NMDUPL
    STA BUFADR
    LDA # ; <NMDUPH
    STA BUFADR+1
    LDA MEMTOP
    SEC 
    SBC # ; <NMDUPL
    STA BUFLEN
    LDA MEMTOP+1
    SBC # ; <NMDUPH
    STA BUFLEN+1
    LDA #0
    STA MLT125
    STA MLT125+1
FINDGM
    LDA #125
    CLC 
    ADC MLT125
    STA MLT125
    LDA #0
    ADC MLT125+1
    STA MLT125+1
    LDA BUFLEN+1
    CMP MLT125+1
    BCC GETMLT
    BNE FINDGM
    LDA BUFLEN
    CMP MLT125
    BCS FINDGM
GETMLT
    LDA MLT125+1
    BNE REPLAC
    LDA #125
    CMP MLT125
    BCC REPLAC
    RTS 
REPLAC
    LDA MLT125
    SEC 
    SBC #125
    STA BUFLEN
    LDA MLT125+1
    SBC #0
    STA BUFLEN+1
    RTS 
;
USEBUF
    LDA # ; <DBUFL
    STA BUFADR
    LDA # ; <DBUFH
    STA BUFADR+1
    LDA #EDBLL
    STA BUFLEN
    LDA #EDBLH
    STA BUFLEN+1
    RTS 
;
LOOKWC
    LDA PAR,X
    INX 
    CMP #'*'
    BEQ LOOKW2
    CMP #'?'
    BEQ LOOKW2
    CMP #CR
    BEQ LOOKW1
    CMP #','
    BNE LOOKWC
LOOKW1
    INX 
LOOKW2
    RTS 
; SUBROUTINE TESTDOS
TSTDOS
    INX 
    LDA PAR,X
    CMP #':'
    BEQ GOTCOL
    INX 
GOTCOL
    INX 
    LDY #0
NXTCHAR
    LDA DS+3,Y
    CMP PAR,X
    BNE NOTSAM
    INY 
    INX 
    CPY #7
    BNE NXTCHAR
    LDA # ; <DCDSL
    LDX # ; <DCDSH
    JSR DSPLIN
    JMP MENUSL
NOTSAM
    RTS 
DCDS .BYTE "DESTINATION CANT BE DOS.SYS",CR
DCDSH =  >DCDS
DCDSL =  <DCDS
;
; SAVE FILE ROUTINE
;
SAVFIL .WORD SFMG
    LDA #0
    STA $18A0
    STA $18BE
    JSR GETIC1
    LDA OPT
    PHA 
    LDX PTR
    LDA #CR
    STA PAR-1,X
    JSR GETNO
    STA LDST
    STX LDST+1
    CPX # ; <NDSH
    BCS DSLMFG
    DEC WDR1+1
DSLMFG
    JSR GETNO
    STA LDND
    STX LDND+1
    SEC 
    SBC LDST
    STA WDRL+1
    TXA 
    SBC LDST+1
    BPL ADDOK
    JMP MENUSL
ADDOK
    STA WDRH+1
    CPY #CR
    BEQ NRUNAD
    JSR GETNO
    STA INITAD
    STX INITAD+1
    ORA INITAD+1
    BEQ NINTAD
    DEC $18A0
NINTAD
    CPY #CR
    BEQ NRUNAD
    JSR GETNO
    JSR PERX
    STA RUNAD
    STX RUNAD+1
    ORA RUNAD+1
    BEQ NRUNAD
    DEC $18BE
NRUNAD
    LDA #0
    STA OPT
    PLA 
    CMP #'A'
  BNE *+5
  DEC OPT
  LDX #$10
  LDA #OPEN
  STA ICCOM,X
  BIT OPT
  BMI *+6
  LDA #8
  BNE *+4
  LDA #9
  STA ICAX1,X
  JSR CIOCL
  LDA #PUTCHR
  STA ICCOM,X
  LDA # ; <SAVHL
  STA ICBAL,X
  LDA # ; <SAVHH
  STA ICBAH,X
  LDA #6
  STA ICBLL,X
  LDA #0
  STA ICBLH,X
  BIT OPT
  BPL WHEAD
  LDA #4
  STA ICBLL,X
  LDA # ; <LDSTL
  STA ICBAL,X
  LDA # ; <LDSTH
  STA ICBAH,X
WHEAD
  JSR CIOCL
;
;WRITE DATA RECORD
;
WDR
  LDX #$10
WDRL
  LDA #0
  STA ICBLL,X
WDRH
  LDA #0
  STA ICBLH,X
  INC ICBLL,X
  BNE *+5
  INC ICBLH,X
  LDA LDST
  STA ICBAL,X
  LDA LDST+1
  STA ICBAH,X
WEX
  JMP WDR1
SFMG .BYTE "SAVE-GIVE FILE,START,END(,INIT,RUN)",CR
;
;MISC.  SUBROUTINES
;
GETLIN
  LDA #CR
  LDX #79
  STA LINE,X
  DEX 
  BPL *-4
  LDA #0
  STA PTR
  STA IPTR
  STA PER
  JSR CIOGET
  JSR SCROL
  RTS 
;
CIOGET
  LDA #GETREC
  STA ICCOM
  LDA #LBUFL
  STA ICBAL
  LDA #LBUFH
  STA ICBAH
  LDA #80
  STA ICBLL
  LDA #0
  STA ICBLH
  LDX #0
  JSR CIO
  CPY #$80
  BNE *+5
  DEC PER
  RTS 
;
CHRGET
  LDA #0
  STA PER
CHRG1
  JSR CIOGET
  LDA ICBLL
  STA RCNT
  JSR SCROL
  LDA PER
  BPL CHRG2
  JSR CLOSX
  JMP MENUSL
CHRG2
  LDA RCNT
  CMP #3
  BMI CHRG3
  LDA # ; <OLL
  LDX # ; <OLH
  JSR DSPLIN
  JMP CHRG1
CHRG3
  LDA LINE
  RTS 
;
OL .BYTE "PLEASE TYPE 1 LETTER",CR
OLH =  >OL
OLL =  <OL
;
PERX
  BIT PER
  BMI PERX1
  RTS 
PERX1
  PLA 
  PLA 
  JMP MENUSL
;
GETIC1
  JSR GETLIN
GETIC2
  LDX #$10
  JSR PIOCB
  JMP GETFIL
;
GETNAME
  LDA #8
  STA CTR
  LDY PTR
  LDX IPTR
  JMP CFTE
;
GETFIL
  LDY PTR
  LDX IPTR
  LDA #11
  STA CTR
  LDA LINE,X
  CMP #','
  BEQ ADDC
  CMP #CR
  BEQ ADDC
  LDA LINE+1,X
  CMP #','
  BEQ GT1
  CMP #CR
  BEQ GT1
  LDA #':'
  CMP LINE+2,X
  BEQ CFTE
  CMP LINE+1,X
  BNE GT1
  DEC CTR
  LDA LINE,X
  CMP #'A'
  BPL CFTE
GT2
  LDA #'D'
  STA PAR,Y
  INY 
  BPL CFTE
GT1
  DEC CTR
  DEC CTR
  CMP LINE,X
  BEQ GT2
  DEC CTR
ADDC
  LDA #'D'
  STA PAR,Y
  INY 
  LDA #':'
  STA PAR,Y
  INY 
CFTE
  LDA #0
  STA OPT
CFTE1
  LDA LINE,X
  STA PAR,Y
  INX 
  INY 
  CMP #CR
  BEQ EOC
  CMP #','
  BEQ EOC
  CMP #'/'
  BEQ POPT
  CMP #'.'
  BNE CFTE2
  LDA #4
  STA CTR
CFTE2
  DEC CTR
  BPL CFTE1
  LDA # ; <NTLL
  LDX # ; <NTLH
  JSR DSPLIN
  DEC PER
STE
  LDA LINE,X
  INX 
  CMP #','
  BEQ EOC
  CMP #CR
  BNE STE
EOC
  STX IPTR
  STY PTR
  RTS 
POPT
  LDA LINE,X
  STA OPT
  INX 
  LDA LINE,X
  STA PAR-1,Y
  INX 
  BPL EOC
NTL .BYTE "NAME TOO LONG",CR
NTLH =  >NTL
NTLL =  <NTL
;
DSPMSG
  LDA #PUTCHR
  STA ICCOM
  LDX #0
CIO1
  JSR CIO
  CPY #$80
  BNE *+5
  JMP MENUSL
  RTS 
;
DSPLIN
  JSR PRNTMSG
  JMP SCROL
;
SCROL
  LDA #0
  TAX 
  STA ICBLH,X
  LDA #10
  STA ICBLL,X
  LDA # ; <ZAPH
  STA ICBAH,X
  LDA # ; <ZAPL
  STA ICBAL,X
  JMP DSPMSG
;
ZAP .BYTE CUP,CUP,CUP,CUP,CUP
  .BYTE DLL,CDN,CDN,CDN,CDN
ZAPH =  >ZAP
ZAPL =  <ZAP
;
PIOCB
  LDA #PARL
  CLC 
  ADC PTR
  STA ICBAL,X
  LDA #PARH
  ADC #0
  STA ICBAH,X
  RTS 
;
CIOCL
  JSR CIO
  TYA 
  BMI *+3
  RTS 
CIOER1
  TYA 
CIOER
  SEC 
  SBC #100
  LDX #'0'-1
CTNS
  INX 
  SEC 
  SBC #10
  BPL CTNS
  CLC 
  ADC #10+'0'
  STA EUN
  STX ETN
  LDX # ; <CIEH
  LDA # ; <CIEL
CIEX
  JSR DSPLIN
  JSR CLOSX
  JMP MENUSL
CIE
  .BYTE "ERROR-   1"
ETN
  .BYTE 0
EUN
  .BYTE 0
  .BYTE CR
;
CIEH =  >CIE
CIEL =  <CIE
;
GETNO
  LDA #4
  STA CTR
  LDA #0
  STA T1
  STA T1+1
GHB
  LDX IPTR
  LDA LINE,X
  INC IPTR
  CMP #CR
  BEQ GND
  CMP #','
  BEQ GND
  JSR HEXCON
  BMI ERRX
  LDY #3
SHT1
  CLC 
  ROL T1+1
  ROL T1
  DEY 
  BPL SHT1
  ORA T1+1
  STA T1+1
  DEC CTR
  BPL GHB
  LDA # ; <TMDL
  LDX # ; <TMDH
ERRX1
  JSR DSPLIN
  DEC PER
  RTS 
GND
  TAY 
  LDA T1+1
  LDX T1
  RTS 
ERRX
  LDA # ; <IHPL
  LDX # ; <IHPH
  BNE ERRX1
TMD .BYTE "TOO MANY DIGITS",CR
;
TMDH =  >TMD
TMDL =  <TMD
;
IHP .BYTE "INVALID HEXADECIMAL PARAMETER",CR
IHPH =  >IHP
IHPL =  <IHP
;
HEXCON
  SEC 
  SBC #'0'
  BMI ERRX2
  CMP #10
  BMI OKX
  SEC 
  SBC #7  ; ASCII 'A' minus '0' minus 10
  CMP #10
  BMI ERRX2
  CMP #$10
  BMI OKX
ERRX2
  LDA #$FF
OKX
  CMP #0
  RTS 
;
GETDN
  BIT PER
  BMI GDR
  LDX IPTR
GETD
  LDA LINE,X
  INX 
  CMP #'D'
  BEQ GETD
  SEC 
  SBC #'0'
  BEQ BDS
  BMI BDS
  CMP #5
  BPL BDS
  PHA 
GD1
  LDA LINE,X
  INX 
  CMP #','
  BEQ GDX
  CMP #CR
  BNE GD1
GDX
  STX IPTR
  PLA 
GDR
  RTS 
BDS
  DEC PER
  LDA # ; <NDSL
  LDX # ; <NDSH
  JMP DSPLIN
;
NDS .BYTE "NEED D1 THRU D4",CR
NMDUP .BYTE 0
LEN = NMDUP-EDN
LENH =  >LEN
LENL =  <LEN
MLEN = NMDUP-NDOS
MLENH =  >MLEN
MLENL =  <MLEN
NDSH =  >NDS
NDSL =  <NDS
NMDUPH =  >NMDUP
NMDUPL =  <NMDUP
    org $02E0
  .WORD DOSOS

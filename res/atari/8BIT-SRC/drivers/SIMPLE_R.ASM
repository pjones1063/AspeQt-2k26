  org $4000       ; Load at 16384

; --- EQUATES ---
HATABS  = $031A     ; Handler Table
CIOV    = $E456     ; CIO Vector
SIOV    = $E459     ; SIO Vector
RTCLOK  = $14       ; Real Time Clock
COLOR4  = $02C8     ; Shadow Register for Background Color (STICKS!)

; SIO DCB
DDEVIC  = $0300
DUNIT   = $0301
DCOMND  = $0302
DSTATS  = $0303
DBUFLO  = $0304
DBUFHI  = $0305
DTIMLO  = $0306
DRESLO  = $0308
DRESHI  = $0309
DAUX1   = $030A
DAUX2   = $030B

START:
    ; --- INSTALLATION ROUTINE ---
    ldx #0
FindSlot:
    lda HATABS,x
    cmp #'R'
    beq FoundSlot
    cmp #0
    beq FoundSlot
    inx
    inx
    inx
    cpx #36
    bne FindSlot
    rts

FoundSlot:
    lda #'R'
    sta HATABS,x
    
    lda #<HANDLER_TABLE  
    sta HATABS+1,x  
    lda #>HANDLER_TABLE  
    sta HATABS+2,x  
    
    ; --- PRINT MESSAGE ---
    ldx #0
    lda #11
    sta $0342,x
    lda #<MSG
    sta $0344,x
    lda #>MSG
    sta $0345,x
    lda #<MSG_LEN
    sta $0348,x
    lda #>MSG_LEN
    sta $0349,x
    jsr CIOV

    ; --- DELAY LOOP ---
    lda RTCLOK
    clc
    adc #60
WaitLoop:
    cmp RTCLOK
    bne WaitLoop

    rts

; --- HANDLER TABLE ---
; FIX: Removed "-1". CIO uses JMP Indirect, so we need exact addresses.
HANDLER_TABLE:
    dta <H_OPEN,  >H_OPEN
    dta <H_CLOSE, >H_CLOSE
    dta <H_GET,   >H_GET
    dta <H_PUT,   >H_PUT
    dta <H_STAT,  >H_STAT
    dta <H_SPEC,  >H_SPEC
    jmp H_INIT      
    nop             

; --- CIO FUNCTIONS ---

; -----------------------------------------------------
; OPEN: TURN SCREEN RED
; -----------------------------------------------------
H_OPEN:
    lda #$24        ; RED
    sta COLOR4      ; Write to SHADOW (Persists!)
    ldy #1          ; Success
    rts

; -----------------------------------------------------
; CLOSE: TURN SCREEN WHITE
; -----------------------------------------------------
H_CLOSE:
    lda #$0F        ; WHITE
    sta COLOR4
    ldy #1
    rts

; -----------------------------------------------------
; PUT BYTE: TURN SCREEN GREEN
; -----------------------------------------------------
H_PUT:
    pha             
    sta IO_BUFFER   

    lda #$C4        ; GREEN
    sta COLOR4

    ; Setup SIO
    lda #$50        ; Device R1
    sta DDEVIC
    lda #1          
    sta DUNIT
    lda #$57        ; Write
    sta DCOMND
    lda #$80        ; Send
    sta DSTATS
    
    lda #<IO_BUFFER 
    sta DBUFLO
    lda #>IO_BUFFER 
    sta DBUFHI
    
    lda #$80        ; Timeout (~2s)
    sta DTIMLO
    
    lda #1          ; Len 1
    sta DRESLO
    lda #0
    sta DRESHI

    sta DAUX2       ; Aux2 = 0
    lda #1          ; Aux1 = 1
    sta DAUX1
    
    jsr SIOV        
    
    pla             
    ldy #1          
    rts

; -----------------------------------------------------
; GET BYTE
; -----------------------------------------------------
H_GET:
    lda #$50        
    sta DDEVIC
    lda #1          
    sta DUNIT
    lda #$52        ; Read
    sta DCOMND
    lda #$40        ; Receive
    sta DSTATS
    
    lda #<IO_BUFFER 
    sta DBUFLO
    lda #>IO_BUFFER 
    sta DBUFHI
    
    lda #$80        
    sta DTIMLO
    
    lda #1          
    sta DRESLO
    lda #0
    sta DRESHI

    sta DAUX2       
    lda #1          
    sta DAUX1
    
    jsr SIOV        
    bmi SioError    
    
    lda IO_BUFFER   
    ldy #1          
    rts

SioError:
    ldy #$88        ; Device Error
    rts

; -----------------------------------------------------
; STATUS
; -----------------------------------------------------
H_STAT:
    lda #$50
    sta DDEVIC
    lda #1
    sta DUNIT
    lda #$53
    sta DCOMND
    lda #$40
    sta DSTATS
    
    lda #<STAT_BUF
    sta DBUFLO
    lda #>STAT_BUF
    sta DBUFHI
    
    lda #$20
    sta DTIMLO

    lda #4
    sta DRESLO
    lda #0
    sta DRESHI

    sta DAUX1
    sta DAUX2
    
    jsr SIOV
    
    ldx #0
CopyStat:
    lda STAT_BUF,x
    sta $2EA,x      ; DVSTAT
    inx
    cpx #4
    bne CopyStat
    
    ldy #1
    rts

H_SPEC:
H_INIT:
    ldy #1
    rts

; --- DATA ---
MSG:        dta c'AspeQt Driver Loaded (Rainbow V2)', $9B
MSG_LEN     = *-MSG

IO_BUFFER:  dta 0   
STAT_BUF:   dta 0,0,0,0 

END_ASM:

; --- RUN VECTOR ---
    org $02E2       
    dta <START, >START
    
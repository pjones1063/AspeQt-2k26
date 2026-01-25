      org $3000       ; Load address (Safe area)

; --- EQUATES ---
HATABS  = $031A     ; Handler Table
CIOV    = $E456     ; CIO Vector
SIOV    = $E459     ; SIO Vector

; SIO Device Control Block (DCB)
DDEVIC  = $0300     ; Device ID
DUNIT   = $0301     ; Unit Number
DCOMND  = $0302     ; Command
DSTATS  = $0303     ; Status (Direction)
DBUFLO  = $0304     ; Buffer Low
DBUFHI  = $0305     ; Buffer High
DTIMLO  = $0306     ; Timeout Low
DRESLO  = $0308     ; Byte Count Low
DRESHI  = $0309     ; Byte Count High
DAUX1   = $030A     ; Aux 1
DAUX2   = $030B     ; Aux 2

; --- HEADER FOR DOS ---
    dta $FF,$FF     ; Header
    dta <START, >START
    dta <END_ASM, >END_ASM

START:
    ; --- INSTALLATION ROUTINE ---
    ; We scan HATABS for a free slot ($00) or an existing 'R' entry
    ldx #0
FindSlot:
    lda HATABS,x    ; Load entry ID
    cmp #'R'        ; Is it already 'R'?
    beq FoundSlot   ; Yes, overwrite it
    cmp #0          ; Is it empty?
    beq FoundSlot   ; Yes, take it
    inx
    inx
    inx
    cpx #36         ; End of table?
    bne FindSlot
    
    ; Table full? Just exit (or print error, but we keep it simple)
    rts

FoundSlot:
    lda #'R'
    sta HATABS,x    ; Set ID
    
    lda <HANDLER_TABLE
    sta HATABS+1,x  ; Set Low Byte of our table
    lda >HANDLER_TABLE
    sta HATABS+2,x  ; Set High Byte
    
    ; Print a message using CIO (ID 0 - Editor)
    ldx #0          ; IOCB 0
    lda #11         ; PUT CHARS
    sta $0342,x     ; ICCOM
    lda <MSG
    sta $0344,x     ; ICBAL
    lda >MSG
    sta $0345,x     ; ICBAH
    lda <MSG_LEN
    sta $0348,x     ; ICBLL
    lda >MSG_LEN
    sta $0349,x     ; ICBLH
    jsr CIOV
    rts

; --- HANDLER TABLE (Standard CIO Interface) ---
HANDLER_TABLE:
    dta <H_OPEN-1,  >H_OPEN-1
    dta <H_CLOSE-1, >H_CLOSE-1
    dta <H_GET-1,   >H_GET-1
    dta <H_PUT-1,   >H_PUT-1
    dta <H_STAT-1,  >H_STAT-1
    dta <H_SPEC-1,  >H_SPEC-1
    jmp H_INIT      ; Initialization (not used here)
    nop             ; Padding

; --- CIO FUNCTIONS ---

; -----------------------------------------------------
; OPEN: Just return Success
; -----------------------------------------------------
H_OPEN:
    ldy #1          ; Success Code
    rts

; -----------------------------------------------------
; CLOSE: Just return Success
; -----------------------------------------------------
H_CLOSE:
    ldy #1
    rts

; -----------------------------------------------------
; PUT BYTE: Send Accumulator to SIO Device $50
; -----------------------------------------------------
H_PUT:
    pha             ; Save the character
    sta IO_BUFFER   ; Store it in our buffer
    
    ; Setup SIO for Write (Cmd 'W' $57)
    lda #$50        ; Device ID (R:)
    sta DDEVIC
    lda #1          ; Unit 1
    sta DUNIT
    lda #$57        ; Command 'W' (Write)
    sta DCOMND
    lda #$80        ; Status (Write / Computer -> Device)
    sta DSTATS
    
    lda <IO_BUFFER
    sta DBUFLO
    lda >IO_BUFFER
    sta DBUFHI
    
    lda #7          ; Timeout (approx 7 seconds, usually shorter)
    sta DTIMLO
    
    lda #1          ; Length = 1 Byte
    sta DRESLO
    lda #0
    sta DRESHI
    
    jsr SIOV        ; Call SIO!
    
    pla             ; Restore character (CIO expects this)
    ldy #1          ; Success
    rts

; -----------------------------------------------------
; GET BYTE: Read into Accumulator from SIO Device $50
; -----------------------------------------------------
H_GET:
    ; Setup SIO for Read (Cmd 'R' $52)
    lda #$50        ; Device ID
    sta DDEVIC
    lda #1          ; Unit 1
    sta DUNIT
    lda #$52        ; Command 'R' (Read)
    sta DCOMND
    lda #$40        ; Status (Read / Device -> Computer)
    sta DSTATS
    
    lda <IO_BUFFER
    sta DBUFLO
    lda >IO_BUFFER
    sta DBUFHI
    
    lda #15         ; Timeout
    sta DTIMLO
    
    lda #1          ; Length = 1 Byte
    sta DRESLO
    lda #0
    sta DRESHI
    
    jsr SIOV        ; Call SIO!
    bmi SioError    ; If Negative flag set, SIO failed
    
    lda IO_BUFFER   ; Load the byte we got
    ldy #1          ; Success
    rts

SioError:
    ldy #$88        ; EOF or Device Error
    rts

; -----------------------------------------------------
; STATUS: Check if data is waiting (Cmd 'S' $53)
; -----------------------------------------------------
H_STAT:
    lda #$50
    sta DDEVIC
    lda #1
    sta DUNIT
    lda #$53        ; Command 'S' (Status)
    sta DCOMND
    lda #$40        ; Read
    sta DSTATS
    
    lda <STAT_BUF
    sta DBUFLO
    lda >STAT_BUF
    sta DBUFHI
    
    lda #4          ; Length = 4 Bytes
    sta DRESLO
    lda #0
    sta DRESHI
    
    jsr SIOV
    
    ; CIO expects status bytes in locations $2EA-$2ED
    ldx #0
CopyStat:
    lda STAT_BUF,x
    sta $2EA,x      ; DVSTAT
    inx
    cpx #4
    bne CopyStat
    
    ldy #1
    rts

; -----------------------------------------------------
; SPECIAL / INIT: Stubs
; -----------------------------------------------------
H_SPEC:
H_INIT:
    ldy #1
    rts

; --- DATA STORAGE ---
MSG:        dta c'AspeQt Driver Loaded', $9B
MSG_LEN     = *-MSG

IO_BUFFER:  dta 0   ; 1-byte buffer for Get/Put
STAT_BUF:   dta 0,0,0,0 ; 4-byte buffer for Status

END_ASM:
; =================================================================
; AspeQt-2k26 ClipboardDevice (Y:) - BARE METAL
; -----------------------------------------------------------------
; TARGET: Atari 8-bit (MADS Assembler)
; ORIGIN: $2000
; =================================================================

    org $2000

; -----------------------------------------------------------------
; SYSTEM EQUATES
; -----------------------------------------------------------------
SIOV    = $E459     ; SIO Vector
CIOV    = $E456     ; CIO Vector

; -----------------------------------------------------------------
; MAIN PROGRAM
; -----------------------------------------------------------------
start:
    ; =============================================================
    ; STEP 1: PRINT WELCOME MESSAGE
    ; Uses IOCB #0 (Editor 'E:')
    ; =============================================================
    ldx #0              ; IOCB Index 0
    lda #$0B            ; COMMAND: Put Characters ($0B)
    sta $0342,x         ; ICCOM

    lda #>msg_welcome    ; BUFFER ADDRESS (Low)
    sta $0344,x         ; ICBAL
    lda #<msg_welcome   ; BUFFER ADDRESS (High)
    sta $0345,x         ; ICBAH

    lda #22             ; LENGTH (Low) - Hardcoded length of welcome
    sta $0348,x         ; ICBLL
    lda #0             ; LENGTH (High)
    sta $0349,x         ; ICBLH

    jsr CIOV            ; Call CIO

    ; =============================================================
    ; STEP 2: CLEAR RECIEVE BUFFER
    ; Fill with spaces ($20) to ensure clean output
    ; =============================================================
    ldx #0
    lda #$20
clear_loop:
    sta buffer,x
    inx
    bne clear_loop

    ; =============================================================
    ; STEP 3: SETUP SIO FOR DEVICE 'Y'
    ; Reference: Mapping the Atari, Page 111
    ; =============================================================
    lda #$59            ; DDEVIC: Device 'Y'
    sta $0300
    
    lda #1              ; DUNIT: Unit 1
    sta $0301
    
    lda #'R'            ; DCOMND: Read Command
    sta $0302
    
    lda #$40            ; DSTATS: Direction Read (From Computer -> Atari)
    sta $0303
    
    lda <buffer         ; DBUFLO: Buffer Address Low
    sta $0304
    
    lda >buffer         ; DBUFHI: Buffer Address High
    sta $0305
    
    lda #15             ; DTIMLO: Timeout (~10 seconds)
    sta $0306
    
    lda <256            ; DBYTLO: Byte Count Low (256)
    sta $0308
    
    lda >256            ; DBYTHI: Byte Count High
    sta $0309
    
    lda #0              ; DAUX1/2: Zero
    sta $030A
    sta $030B

    ; =============================================================
    ; STEP 4: CALL SIO
    ; =============================================================
    jsr SIOV
    bmi sio_error       ; If Negative Flag is set, SIO failed

    ; =============================================================
    ; STEP 5: PRINT RECEIVED BUFFER
    ; =============================================================
    ; Print Header First
    ldx #0
    lda #$0B
    sta $0342,x
    lda <msg_success
    sta $0344,x
    lda >msg_success
    sta $0345,x
    lda <11
    sta $0348,x
    lda >11
    sta $0349,x
    jsr CIOV

    ; Print The Data Buffer
    ldx #0
    lda #$0B
    sta $0342,x
    lda <buffer
    sta $0344,x
    lda >buffer
    sta $0345,x
    lda <256            ; Print full 256 bytes
    sta $0348,x
    lda >256
    sta $0349,x
    jsr CIOV
    
    jmp loop_forever

sio_error:
    ; =============================================================
    ; ERROR HANDLER
    ; =============================================================
    ldx #0
    lda #$0B
    sta $0342,x
    lda <msg_error
    sta $0344,x
    lda >msg_error
    sta $0345,x
    lda <11
    sta $0348,x
    lda >11
    sta $0349,x
    jsr CIOV

loop_forever:
    jmp loop_forever

; =================================================================
; DATA SEGMENT
; =================================================================

; ATASCII Strings (EOL is $9B)
msg_welcome: .byte "Y: CLIPBOARD READER", $9B, 0     ; Length 20 + 2
msg_success: .byte "RECEIVED:", $9B, 0               ; Length 11
msg_error:   .byte "SIO ERROR!", $9B, 0              ; Length 11

    .align 256
buffer:      .ds 256

    run start
    
    
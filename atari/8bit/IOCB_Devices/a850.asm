; =========================================================
; ATARI 850 INTERFACE HANDLER BOOTSTRAP
; Assembler: MADS
; =========================================================

; --- Atari System Equates ---
DDEVIC   equ $0300      ; Device ID
DUNIT    equ $0301      ; Unit Number
DCOMND   equ $0302      ; Command Byte
DSTATS   equ $0303      ; Status (Read/Write)
DBUFLO   equ $0304      ; Buffer Pointer Low
DBUFHI   equ $0305      ; Buffer Pointer High
DTIMLO   equ $0306      ; Timeout
DBYTLO   equ $0308      ; Byte Count Low
DBYTHI   equ $0309      ; Byte Count High
SIOV     equ $E459      ; Serial I/O Vector

; --- Origin (Standard Atari DOS Header created by MADS) ---
    org $3800

start
; --- 1. Set up the Device Control Block (DCB) ---
    lda #$50        ; 'P' for Printer/850 Interface
    sta DDEVIC
    lda #$01        ; Unit 1
    sta DUNIT
    lda #$3F        ; Command '?' (Download)
    sta DCOMND
    lda #$40        ; Device -> Computer (Read)
    sta DSTATS
    lda #$00        
    sta DBUFLO
    lda #$09        
    sta DBUFHI      ; Destination: $0900
    lda #$00        
    sta DBYTLO
    lda #$0A        
    sta DBYTHI      ; Length: $0A00 (2560 bytes)
    lda #$0C        ; 12 second timeout
    sta DTIMLO

; --- 2. Request First Handler Block (PRN) ---
    jsr SIOV        ; Call SIO
    bpl copy_prn    ; If positive (Success), proceed
    rts             ; Error: Return to DOS

; --- 3. Copy PRN Handler to Safekeeping ---
; The 850 interface downloads multiple handlers to the same 
; $0900 buffer. We must move the first one to $0A00.
copy_prn
    ldx #$00
loop_move
    lda $0900,x
    sta $0A00,x
    dex
    bne loop_move   ; Loop 256 times

; --- 4. Request Second Handler Block (RS-232) ---
    jsr SIOV        ; Call SIO again for the next block
    bmi exit        ; If negative (Error), exit
    jmp ($0A0C)     ; Success! Jump to the PRN init vector

exit
    rts

; =========================================================
; INIT VECTOR
; =========================================================
; This block tells DOS to run the code at 'start' 
; immediately after loading.
    org $02E2
    .word start

; =========================================================
; RUN VECTOR (Optional)
; =========================================================
; This ensures that if DOS doesn't use the INIT vector, 
; the program starts when loading is finished.
    org $02E0
    .word start

; =================================================================
; ASPEQT-2K26 UNIFIED SOFTCART LOADER V15
; Features: Auto-detects 8KB vs 16KB .CAR files
; Required: Input file must be pre-patched for $D301 compatibility
; =================================================================

    org $2000           

Start
    cld
    ; 1. READ HEADER (Byte 11) TO DETECT TYPE
    lda Payload+11      
    cmp #$02            ; Type 2 = Standard 16KB Cartridge
    beq setup_16k

setup_8k
    ; Config for 8KB ($A000 - $BFFF)
    lda #$A0            
    sta target_hi       ; Store high byte for copy loop
    sta $6A             ; Set RAMTOP to $A0
    sta $02E4           ; Set RAMSIZ to $A0
    ldx #$20            ; 32 pages = 8KB
    stx page_count
    jmp init_env

setup_16k
    ; Config for 16KB ($8000 - $BFFF)
    lda #$80            
    sta target_hi       
    sta $6A             ; Set RAMTOP to $80
    sta $02E4           ; Set RAMSIZ to $80
    ldx #$40            ; 64 pages = 16KB
    stx page_count

init_env
    jsr $EF9C           ; Call OS Graphics 0 to move screen below cart

    ; 2. EXPOSE RAM
    sei                 
    lda $D301           
    ora #$02            ; Disable BASIC, reveal RAM
    sta $D301           

    ; 3. DATA COPY (Skipping 16-byte .CAR header)
    lda #<(Payload+16)  
    sta src+1
    lda #>(Payload+16)
    sta src+2
    
    lda #$00
    sta dst+1
    lda target_hi: #$00 ; Value set in setup_8k/16k
    sta dst+2
    
    ldx page_count: #$00; Value set in setup_8k/16k
    ldy #$00

copy_loop
src lda $FFFF,y         
dst sta $A000,y         ; High byte overwritten by 'target_hi'
    iny
    bne copy_loop
    inc src+2           
    inc dst+2           
    dex
    bne copy_loop

    ; 4. SET SYSTEM FLAGS FOR CART
    lda #$02
    sta $03F8           ; Flag: BASIC disabled
    lda #$00
    sta $BFFC           ; Flag: Cartridge present
    sta $08             ; Force Cold Start environment

    ; 5. EXECUTE
    cli                 
    
    ; Check if Init Vector ($BFFE) is populated
    lda $BFFF           
    beq skip_init       ; Skip if high byte is 0
    cmp #$FF
    beq skip_init       ; Skip if high byte is $FF (empty)

    jsr do_init      

skip_init
    ldx #$FF
    txs                 ; Reset Stack
    jmp ($BFFA)         ; Jump to Cart Run Vector

do_init
    jmp ($BFFE)

; =================================================================
; PAYLOAD INJECTION
; =================================================================
Payload
    ; Change this filename to test different patched .car files
    ins 'test.car' 

    run Start
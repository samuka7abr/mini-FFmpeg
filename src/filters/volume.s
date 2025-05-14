section .text
global apply_volume_asm

apply_volume_asm:
    pxor   xmm2, xmm2

.loop:
    cmp    rsi, 8
    jb     .tail

    movdqu xmm1, [rdi]
    movdqu xmm3, xmm1
    pmovsxwd xmm1, xmm1
    psrldq xmm3, 8
    pmovsxwd xmm3, xmm3

    cvtdq2ps xmm1, xmm1
    cvtdq2ps xmm3, xmm3

    mulps    xmm1, xmm0
    mulps    xmm3, xmm0

    cvttps2dq xmm1, xmm1
    cvttps2dq xmm3, xmm3

    packssdw xmm1, xmm3
    movdqu    [rdi], xmm1

    add     rdi, 16
    sub     rsi, 8
    jmp     .loop

.tail:
    test   rsi, rsi
    jz     .done

    mov    ecx, esi

.scalar:
    movsx  eax, word [rdi]
    cvtsi2ss xmm1, eax
    mulss    xmm1, xmm0
    cvtss2si eax, xmm1

    cmp    eax, 32767
    jle    .L1
    mov    eax, 32767
    jmp    .L2

.L1:
    cmp    eax, -32768
    jge    .L2
    mov    eax, -32768

.L2:
    mov    [rdi], ax
    add    rdi, 2
    loop   .scalar

.done:
    ret

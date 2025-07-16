$sys_enter:
    jmp $sys_enter
.l1:
    jmp .l1
    jmp .l2
.l2:
    mov r0, r0
    jmp .l2
.l3:
    call .l3
    call f
f:
    ret
    call f

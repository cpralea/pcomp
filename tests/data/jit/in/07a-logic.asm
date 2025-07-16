    mov r1, 0x11
    mov r2, 0x13
    and r1, r2
    and r1, 0xef

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r1, 0x11
    mov r2, 0x02
    or r1, r2
    or r1, 0x20

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r1, 0x22
    mov r2, 0x30
    xor r1, r2
    xor r1, 0x03

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r1, 0xfffffffffffffff0
    not r1

    push r1
    mov r1, 1
    push r1
    call $sys_enter

    mov r0, 0
    push r0
    call $sys_enter

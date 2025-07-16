$sys_enter:
    jmp $sys_enter
    load r1, [r2 + 1]
    store [r2 - 2], r1

# note: label address computation test; cannot actually be executed



# jump to current
.l1:
    jmp .l1

# jump to next
    jmp .l2
.l2:
    mov r0, r0

# jump to previous
    jmp .l2



# call current
.l3:
    call .l3

# call next
    call f
f:
    ret

# call previous
    call f

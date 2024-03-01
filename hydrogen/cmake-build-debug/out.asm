global _start
_start:
    mov rax, 1
    push rax
    ;; /let
    mov rax, 2
    push rax
    ;; /let
    mov rax, 1
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax, 2
    push rax
    pop rax
    mov [rsp + 8], rax
    push QWORD [rsp + 8]
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
    jmp label1
label0:
    ;; / begin elif
    mov rax, 1
    push rax
    pop rax
    test rax, rax
    jz label2
    mov rax, 3
    push rax
    pop rax
    mov [rsp + 8], rax
    push QWORD [rsp + 8]
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
    jmp label1
label2:
    ;; / begin else
    mov rax, 69
    push rax
    mov rax, 60
    pop rdi
    syscall
    add rsp, 0
    ;; / end else
    ;; / end elif
label1:
    ;; /if
    mov rax, 60
    mov rdi, 0
    syscall
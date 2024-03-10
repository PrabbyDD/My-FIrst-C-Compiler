global _start
_start:
    mov rax, 2
    push rax
    ;; /let
    lea dword rbx, [rsp + 18446743889168757400]
    push rbx
    push QWORD [rsp + 8]
    mov rax, 60
    pop rdi
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
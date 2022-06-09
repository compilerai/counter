; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=-popcnt | FileCheck %s --check-prefix=X86-NOPOPCNT
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=-popcnt | FileCheck %s --check-prefix=X64-NOPOPCNT
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+popcnt | FileCheck %s --check-prefix=X86-POPCNT
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+popcnt | FileCheck %s --check-prefix=X64-POPCNT

define i32 @parity_32(i32 %x) {
; X86-NOPOPCNT-LABEL: parity_32:
; X86-NOPOPCNT:       # %bb.0:
; X86-NOPOPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    movl %eax, %ecx
; X86-NOPOPCNT-NEXT:    shrl $16, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %eax
; X86-NOPOPCNT-NEXT:    xorb %ch, %cl
; X86-NOPOPCNT-NEXT:    setnp %al
; X86-NOPOPCNT-NEXT:    retl
;
; X64-NOPOPCNT-LABEL: parity_32:
; X64-NOPOPCNT:       # %bb.0:
; X64-NOPOPCNT-NEXT:    movl %edi, %ecx
; X64-NOPOPCNT-NEXT:    shrl $16, %ecx
; X64-NOPOPCNT-NEXT:    xorl %edi, %ecx
; X64-NOPOPCNT-NEXT:    movl %ecx, %edx
; X64-NOPOPCNT-NEXT:    shrl $8, %edx
; X64-NOPOPCNT-NEXT:    xorl %eax, %eax
; X64-NOPOPCNT-NEXT:    xorb %cl, %dl
; X64-NOPOPCNT-NEXT:    setnp %al
; X64-NOPOPCNT-NEXT:    retq
;
; X86-POPCNT-LABEL: parity_32:
; X86-POPCNT:       # %bb.0:
; X86-POPCNT-NEXT:    popcntl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    andl $1, %eax
; X86-POPCNT-NEXT:    retl
;
; X64-POPCNT-LABEL: parity_32:
; X64-POPCNT:       # %bb.0:
; X64-POPCNT-NEXT:    popcntl %edi, %eax
; X64-POPCNT-NEXT:    andl $1, %eax
; X64-POPCNT-NEXT:    retq
  %1 = tail call i32 @llvm.ctpop.i32(i32 %x)
  %2 = and i32 %1, 1
  ret i32 %2
}

define i64 @parity_64(i64 %x) {
; X86-NOPOPCNT-LABEL: parity_64:
; X86-NOPOPCNT:       # %bb.0:
; X86-NOPOPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    xorl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    movl %eax, %ecx
; X86-NOPOPCNT-NEXT:    shrl $16, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %eax
; X86-NOPOPCNT-NEXT:    xorb %ch, %cl
; X86-NOPOPCNT-NEXT:    setnp %al
; X86-NOPOPCNT-NEXT:    xorl %edx, %edx
; X86-NOPOPCNT-NEXT:    retl
;
; X64-NOPOPCNT-LABEL: parity_64:
; X64-NOPOPCNT:       # %bb.0:
; X64-NOPOPCNT-NEXT:    movq %rdi, %rax
; X64-NOPOPCNT-NEXT:    shrq $32, %rax
; X64-NOPOPCNT-NEXT:    xorl %edi, %eax
; X64-NOPOPCNT-NEXT:    movl %eax, %ecx
; X64-NOPOPCNT-NEXT:    shrl $16, %ecx
; X64-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X64-NOPOPCNT-NEXT:    movl %ecx, %edx
; X64-NOPOPCNT-NEXT:    shrl $8, %edx
; X64-NOPOPCNT-NEXT:    xorl %eax, %eax
; X64-NOPOPCNT-NEXT:    xorb %cl, %dl
; X64-NOPOPCNT-NEXT:    setnp %al
; X64-NOPOPCNT-NEXT:    retq
;
; X86-POPCNT-LABEL: parity_64:
; X86-POPCNT:       # %bb.0:
; X86-POPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    xorl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    popcntl %eax, %eax
; X86-POPCNT-NEXT:    andl $1, %eax
; X86-POPCNT-NEXT:    xorl %edx, %edx
; X86-POPCNT-NEXT:    retl
;
; X64-POPCNT-LABEL: parity_64:
; X64-POPCNT:       # %bb.0:
; X64-POPCNT-NEXT:    popcntq %rdi, %rax
; X64-POPCNT-NEXT:    andl $1, %eax
; X64-POPCNT-NEXT:    retq
  %1 = tail call i64 @llvm.ctpop.i64(i64 %x)
  %2 = and i64 %1, 1
  ret i64 %2
}

define i32 @parity_64_trunc(i64 %x) {
; X86-NOPOPCNT-LABEL: parity_64_trunc:
; X86-NOPOPCNT:       # %bb.0:
; X86-NOPOPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    xorl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    movl %eax, %ecx
; X86-NOPOPCNT-NEXT:    shrl $16, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %eax
; X86-NOPOPCNT-NEXT:    xorb %ch, %cl
; X86-NOPOPCNT-NEXT:    setnp %al
; X86-NOPOPCNT-NEXT:    retl
;
; X64-NOPOPCNT-LABEL: parity_64_trunc:
; X64-NOPOPCNT:       # %bb.0:
; X64-NOPOPCNT-NEXT:    movq %rdi, %rax
; X64-NOPOPCNT-NEXT:    shrq $32, %rax
; X64-NOPOPCNT-NEXT:    xorl %edi, %eax
; X64-NOPOPCNT-NEXT:    movl %eax, %ecx
; X64-NOPOPCNT-NEXT:    shrl $16, %ecx
; X64-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X64-NOPOPCNT-NEXT:    movl %ecx, %edx
; X64-NOPOPCNT-NEXT:    shrl $8, %edx
; X64-NOPOPCNT-NEXT:    xorl %eax, %eax
; X64-NOPOPCNT-NEXT:    xorb %cl, %dl
; X64-NOPOPCNT-NEXT:    setnp %al
; X64-NOPOPCNT-NEXT:    retq
;
; X86-POPCNT-LABEL: parity_64_trunc:
; X86-POPCNT:       # %bb.0:
; X86-POPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    xorl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    popcntl %eax, %eax
; X86-POPCNT-NEXT:    andl $1, %eax
; X86-POPCNT-NEXT:    retl
;
; X64-POPCNT-LABEL: parity_64_trunc:
; X64-POPCNT:       # %bb.0:
; X64-POPCNT-NEXT:    popcntq %rdi, %rax
; X64-POPCNT-NEXT:    andl $1, %eax
; X64-POPCNT-NEXT:    # kill: def $eax killed $eax killed $rax
; X64-POPCNT-NEXT:    retq
  %1 = tail call i64 @llvm.ctpop.i64(i64 %x)
  %2 = trunc i64 %1 to i32
  %3 = and i32 %2, 1
  ret i32 %3
}

define i8 @parity_32_trunc(i32 %x) {
; X86-NOPOPCNT-LABEL: parity_32_trunc:
; X86-NOPOPCNT:       # %bb.0:
; X86-NOPOPCNT-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NOPOPCNT-NEXT:    movl %eax, %ecx
; X86-NOPOPCNT-NEXT:    shrl $16, %ecx
; X86-NOPOPCNT-NEXT:    xorl %eax, %ecx
; X86-NOPOPCNT-NEXT:    xorb %ch, %cl
; X86-NOPOPCNT-NEXT:    setnp %al
; X86-NOPOPCNT-NEXT:    retl
;
; X64-NOPOPCNT-LABEL: parity_32_trunc:
; X64-NOPOPCNT:       # %bb.0:
; X64-NOPOPCNT-NEXT:    movl %edi, %eax
; X64-NOPOPCNT-NEXT:    shrl $16, %eax
; X64-NOPOPCNT-NEXT:    xorl %edi, %eax
; X64-NOPOPCNT-NEXT:    movl %eax, %ecx
; X64-NOPOPCNT-NEXT:    shrl $8, %ecx
; X64-NOPOPCNT-NEXT:    xorb %al, %cl
; X64-NOPOPCNT-NEXT:    setnp %al
; X64-NOPOPCNT-NEXT:    retq
;
; X86-POPCNT-LABEL: parity_32_trunc:
; X86-POPCNT:       # %bb.0:
; X86-POPCNT-NEXT:    popcntl {{[0-9]+}}(%esp), %eax
; X86-POPCNT-NEXT:    andb $1, %al
; X86-POPCNT-NEXT:    # kill: def $al killed $al killed $eax
; X86-POPCNT-NEXT:    retl
;
; X64-POPCNT-LABEL: parity_32_trunc:
; X64-POPCNT:       # %bb.0:
; X64-POPCNT-NEXT:    popcntl %edi, %eax
; X64-POPCNT-NEXT:    andb $1, %al
; X64-POPCNT-NEXT:    # kill: def $al killed $al killed $eax
; X64-POPCNT-NEXT:    retq
  %1 = tail call i32 @llvm.ctpop.i32(i32 %x)
  %2 = trunc i32 %1 to i8
  %3 = and i8 %2, 1
  ret i8 %3
}

declare i32 @llvm.ctpop.i32(i32 %x)
declare i64 @llvm.ctpop.i64(i64 %x)
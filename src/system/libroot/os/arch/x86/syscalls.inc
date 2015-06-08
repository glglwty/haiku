/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
 * syscall interface works as such:
 * eax has syscall #
 * esp + 4 points to the syscall parameters
 * branch to the syscall vector in the commpage
 */

#include <asm_defs.h>
#include <commpage_defs.h>
.data
.global libroot_in_kern
libroot_in_kern:
	.long 0

.global libroot_use_direct_funccall
libroot_use_direct_funccall:
	.long 0

.global libroot_syscall_addr_table
libroot_syscall_addr_table:
	.space 2000

libroot_syscall_space:
	.space 100

//A syscall function FOLLOWS cdecl
.text
#define _SYSCALL(name, n, nargs)							\
	.align 8;												\
	FUNCTION(name):											\
		movl $libroot_in_kern, %eax;						\
		movl (%eax), %eax;									\
		testl %eax, %eax;									\
		jne	name##use_kern_syscall;							\
		movl	$n, %eax;									\
		movl	__gCommPageAddress, %edx;					\
		addl	4 * COMMPAGE_ENTRY_X86_SYSCALL(%edx), %edx;	\
		jmp 	*%edx;										\
	name##use_kern_syscall:									\
		movl $libroot_use_direct_funccall, %eax;			\
		movl (%eax), %eax;									\
		testl %eax, %eax;									\
		jne name##direct_call;								\
		movl $libroot_syscall_space, %eax;					\
		leal 4(%esp), %edx;									\
		movl $nargs, %ecx;									\
	name##arg_copy_loop:									\
		testl %ecx, %ecx;									\
		je name##arg_copy_loop_out;							\
		subl $1, %ecx;										\
		pushl %ecx;											\
		movl (%edx), %ecx;									\
		movl %ecx, (%eax);									\
		addl $4, %edx;										\
		addl $4, %eax;										\
		popl %ecx;											\
		jmp name##arg_copy_loop;							\
	name##arg_copy_loop_out:								\
		push $16;											\
		push $libroot_syscall_space;						\
		subl $4, (%esp);									\
		movl $n, %eax;										\
		int $99;											\
		add $8, %esp;										\
		ret;												\
	name##direct_call:										\
		movl %esp, %eax;									\
		pushl %fs; 											\
		pushl %gs;   										\
		movl $88, %edx;										\
		movw %dx, %gs;										\
		movl %esp, %ecx;									\
		subl $4*nargs, %esp;								\
		addl $4*nargs, %eax;								\
	name##direct_call_forward_arg_loop:						\
		cmpl %ecx, %esp;									\
		je name##direct_call_foward_arg_loop_out;			\
		subl $4, %ecx;										\
		movl (%eax), %edx;									\
		movl %edx, (%ecx);									\
		subl $4, %eax;										\
		jmp name##direct_call_forward_arg_loop;				\
	name##direct_call_foward_arg_loop_out:					\
		movl $libroot_syscall_addr_table, %eax;				\
		movl 4*n(%eax), %eax;								\
		call *%eax;											\
		addl $4*nargs, %esp;									\
		popl %gs; 											\
		popl %fs; 											\
		ret;													\
	FUNCTION_END(name)


#define SYSCALL0(name, n) _SYSCALL(name, n, 0)
#define SYSCALL1(name, n) _SYSCALL(name, n, 1)
#define SYSCALL2(name, n) _SYSCALL(name, n, 2)
#define SYSCALL3(name, n) _SYSCALL(name, n, 3)
#define SYSCALL4(name, n) _SYSCALL(name, n, 4)
#define SYSCALL5(name, n) _SYSCALL(name, n, 5)
#define SYSCALL6(name, n) _SYSCALL(name, n, 6)
#define SYSCALL7(name, n) _SYSCALL(name, n, 7)
#define SYSCALL8(name, n) _SYSCALL(name, n, 8)
#define SYSCALL9(name, n) _SYSCALL(name, n, 9)
#define SYSCALL10(name, n) _SYSCALL(name, n, 10)
#define SYSCALL11(name, n) _SYSCALL(name, n, 11)
#define SYSCALL12(name, n) _SYSCALL(name, n, 12)
#define SYSCALL13(name, n) _SYSCALL(name, n, 13)
#define SYSCALL14(name, n) _SYSCALL(name, n, 14)
#define SYSCALL15(name, n) _SYSCALL(name, n, 15)
#define SYSCALL16(name, n) _SYSCALL(name, n, 16)
#define SYSCALL17(name, n) _SYSCALL(name, n, 17)
#define SYSCALL18(name, n) _SYSCALL(name, n, 18)
#define SYSCALL19(name, n) _SYSCALL(name, n, 19)
#define SYSCALL20(name, n) _SYSCALL(name, n, 20)


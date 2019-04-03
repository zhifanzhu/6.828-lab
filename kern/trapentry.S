/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################
/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(vector0, T_DIVIDE)
TRAPHANDLER_NOEC(vector1, T_DEBUG)
TRAPHANDLER_NOEC(vector2, T_NMI)
TRAPHANDLER_NOEC(vector3, T_BRKPT)
TRAPHANDLER_NOEC(vector4, T_OFLOW)
TRAPHANDLER_NOEC(vector5, T_BOUND)
TRAPHANDLER_NOEC(vector6, T_ILLOP)
TRAPHANDLER_NOEC(vector7, T_DEVICE)
TRAPHANDLER(vector8, T_DBLFLT)
# TRAPHANDLER_NOEC(vector9, )
TRAPHANDLER(vector10, T_TSS)
TRAPHANDLER(vector11, T_SEGNP)
TRAPHANDLER(vector12, T_STACK)
TRAPHANDLER(vector13, T_GPFLT)
TRAPHANDLER(vector14, T_PGFLT)
# TRAPHANDLER_NOEC(vector15, )
TRAPHANDLER_NOEC(vector16, T_FPERR)
TRAPHANDLER(vector17, T_ALIGN)
TRAPHANDLER_NOEC(vector18, T_MCHK)
TRAPHANDLER_NOEC(vector19, T_SIMDERR)
TRAPHANDLER_NOEC(vector20, 20)
TRAPHANDLER_NOEC(vector21, 21)
TRAPHANDLER_NOEC(vector22, 22)
TRAPHANDLER_NOEC(vector23, 23)
TRAPHANDLER_NOEC(vector24, 24)
TRAPHANDLER_NOEC(vector25, 25)
TRAPHANDLER_NOEC(vector26, 26)
TRAPHANDLER_NOEC(vector27, 27)
TRAPHANDLER_NOEC(vector28, 28)
TRAPHANDLER_NOEC(vector29, 29)
TRAPHANDLER_NOEC(vector30, 30)
TRAPHANDLER_NOEC(vector31, 31)

TRAPHANDLER_NOEC(vector32, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER_NOEC(vector33, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER_NOEC(vector34, IRQ_OFFSET+2)
TRAPHANDLER_NOEC(vector35, IRQ_OFFSET+3)
TRAPHANDLER_NOEC(vector36, IRQ_OFFSET + IRQ_SERIAL)
TRAPHANDLER_NOEC(vector37, IRQ_OFFSET+5)
TRAPHANDLER_NOEC(vector38, IRQ_OFFSET+6)
TRAPHANDLER_NOEC(vector39, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER_NOEC(vector40, IRQ_OFFSET+8)
TRAPHANDLER_NOEC(vector41, IRQ_OFFSET+9)
TRAPHANDLER_NOEC(vector42, IRQ_OFFSET+10)
TRAPHANDLER_NOEC(vector43, IRQ_OFFSET+IRQ_E1000)
TRAPHANDLER_NOEC(vector44, IRQ_OFFSET+12)
TRAPHANDLER_NOEC(vector45, IRQ_OFFSET+13)
TRAPHANDLER_NOEC(vector46, IRQ_OFFSET + IRQ_IDE)
TRAPHANDLER_NOEC(vector47, IRQ_OFFSET+15)

TRAPHANDLER_NOEC(vector48, T_SYSCALL)

/*
 * Lab 3: Your code here for _alltraps
 */
.global _alltraps;
_alltraps:
	# push to make stack like Trapframe
	pushl %ds
	pushl %es
	pushal

	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es

	pushl %esp
	call trap
	# _alltraps returns by env_pop_tf, not here


.data
.global vectors
vectors:
	.long 	vector0
	.long 	vector1
	.long 	vector2
	.long 	vector3
	.long 	vector4
	.long 	vector5
	.long 	vector6
	.long 	vector7
	.long 	vector8
	.long 	0
	.long 	vector10
	.long 	vector11
	.long 	vector12
	.long 	vector13
	.long 	vector14
	.long 	0
	.long 	vector16
	.long 	vector17
	.long 	vector18
	.long 	vector19
	.long 	vector20
	.long 	vector21
	.long 	vector22
	.long 	vector23
	.long 	vector24
	.long 	vector25
	.long 	vector26
	.long 	vector27
	.long 	vector28
	.long 	vector29
	.long 	vector30
	.long 	vector31
	.long 	vector32
	.long 	vector33
	.long 	vector34
	.long 	vector35
	.long 	vector36
	.long 	vector37
	.long 	vector38
	.long 	vector39
	.long 	vector40
	.long 	vector41
	.long 	vector42
	.long 	vector43
	.long 	vector44
	.long 	vector45
	.long 	vector46
	.long 	vector47
	.long 	vector48

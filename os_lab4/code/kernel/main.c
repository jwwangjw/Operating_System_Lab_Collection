
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

#define MAX_READER 3//最大允许读者数
#define MODE 1
#define HUNGRY 1
void init();
void protol(char name,int color);
int judge();
/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;
		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	//proc_table[0].ticks = proc_table[0].priority = 15;
	//proc_table[1].ticks = proc_table[1].priority =  5;
	//proc_table[2].ticks = proc_table[2].priority =  3;

	//k_reenter = 0;
	//ticks = 0;

	proc_table[0].ticks = proc_table[0].blocked = 0;
	proc_table[1].ticks = proc_table[1].blocked = 0;
	proc_table[2].ticks = proc_table[2].blocked = 0;
	proc_table[3].ticks = proc_table[3].blocked = 0;
	proc_table[4].ticks = proc_table[4].blocked = 0;
	proc_table[5].ticks = proc_table[5].blocked = 0;
	init();
	//Init --End
	k_reenter = 0;
	ticks = 0;
	p_proc_ready = proc_table;
	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));
	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */
	restart();
	while (1){ }
}
void init() {
	setS(&rmutex, 1);
	setS(&wmutex, 1);
	setS(&r_lock, 1);
	setS(&w_lock, 1);
	setS(&fair_lock, 1);
	setS(&rmax, MAX_READER);
	readerCount = writerCount = 0;
	Mode = MODE;
	clearscreen();
}
/*======================================================================*
                               TestA
 *======================================================================*/
void ReaderA()
{
	protol('A', COLOR_A);
}

void ReaderB()
{
	protol('B', COLOR_B);
}

void ReaderC()
{
	protol('C', COLOR_C);
}
void WriterD()
{
	protol('D', COLOR_D);
}

void WriterE()
{
	protol('E', COLOR_E);
}
void protol(char name,int color) {
	if (color < 3) {
		while (1)
		{
			if (!HUNGRY)
				P(&fair_lock, color);
			if (Mode == 1)
				P(&rmutex, color);
			P(&r_lock, color);
			if (!readerCount)
				P(&wmutex, color);
			readerCount++;
			V(&r_lock);
			if (Mode == 1)
				V(&rmutex);
			if (!HUNGRY)
				V(&fair_lock);

			P(&rmax, color);
			char out1[20] = "Reader   Start!\n\0";
			char p1 = name+color;
			out1[7] = p1;
			switch (name) {
			case 'A':
				color_print(out1, RED);
				break;
			case 'B':
				color_print(out1, BLUE);
				break;
			case 'C':
				color_print(out1, COLOR1);
				break;
			default:
				break;
			}
			currentProc = name + color;
			char out2[20] = "  is Reading!\n\0";
			char p2 = name+color;
			out2[0] = p2;
			color_print(out2, COLOR2);
			switch (name) {
			case 'A':
				milli_delay(2 * TIMECOUNT); 
				break;
			case 'B':
				milli_delay(3 * TIMECOUNT);
				break;
			case 'C':
				milli_delay(3 * TIMECOUNT);
				break;
			default:
				break;
			}
			char out3[20] = "Reader   End!\n\0";
			char p3 = name+color;
			out3[7] = p3;
			color_print(out3, COLOR4);
			V(&rmax);

			P(&r_lock, color);
			readerCount--;
			if (!readerCount)
			{
				V(&wmutex);
			}
			V(&r_lock);
		}
	}
	else {
		while (1)
		{
			if (!HUNGRY)
				P(&fair_lock, color);
			if (Mode == 1)
			{
				P(&w_lock, color);
				writerCount++;
				if (writerCount == 1)
					P(&rmutex, color);
				V(&w_lock);
			}
			P(&wmutex, color);
			char out4[20] = "Writer   Start!\n\0";
			char p4 = name+color;
			out4[7] = p4;
			switch (name) {
			case 'D':
				color_print(out4, GREEN);
				break;
			case 'E':
				color_print(out4, COLOR5);
				break;
			default:
				break;
			}
			currentProc = name + color;
			char out5[20] = "  is Writing!\n\0";
			char p5= name + color;
			out5[0] = p5;
			color_print(out5, COLOR5);
			switch (name) {
			case 'D':
				milli_delay(3 * TIMECOUNT);
				break;
			case 'E':
				milli_delay(4 * TIMECOUNT);
				break;
			default:
				break;
			}
			char out6[20] = "Writer   End!\n\0";
			char p6 = name+color;
			out6[7] = p6;
			color_print(out6, GREEN);
			V(&wmutex);

			if (Mode == 1)
			{
				P(&w_lock, color);
				writerCount--;
				if (writerCount == 0)
					V(&rmutex);
				V(&w_lock);
			}
			if (!HUNGRY)
				V(&fair_lock);
		}
	}
}

void F()
{
	while (1)
	{
		newdelay(1 * TIMECOUNT);
		int flag = judge();
		if(flag)
		{
			char out[20] = "readerCount:  \n\0";
			out[13] = '0' + readerCount; //TODO
			print(out);
		}
	}
}
int judge() {
	int flag=1;
	switch (currentProc) {
	case 'A':flag = 1;
	case 'B':flag = 1;
	case 'C':flag = 1;
	case 'D':flag = 0;
	default:break;
	}
	return flag;
}
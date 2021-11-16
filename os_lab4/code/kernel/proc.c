
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	for (p = proc_table; p < proc_table + NR_TASKS; p++)
		if (p->ticks > 0)
			p->ticks--;
	while (1)
	{
		p_proc_ready = (p_proc_ready + 1 - proc_table) % NR_TASKS + proc_table;
		if (p_proc_ready->ticks <= 0 && p_proc_ready->blocked != 1)
			break;
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}
/*======================================================================*
						   clearscreen
 *======================================================================*/
PUBLIC void clearscreen()
{
	int i = 0;
	disp_pos = 0;
	for (i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
}
/*======================================================================*
						   sys_disp_color_str
 *======================================================================*/
PUBLIC void sys_disp_color_str(char* info, int color)
{
	disp_color_str(info, color);
	if (disp_pos >= 2 * 80 * 25)
	{
		clearscreen();
	}
}
/*======================================================================*
						   sys_disp_str
 *======================================================================*/
PUBLIC void sys_disp_str(char* info)
{
	disp_str(info);
	if (disp_pos >= 2 * 80 * 25)
	{
		clearscreen();
	}
}
/*======================================================================*
						   sys_mydelay
 *======================================================================*/
PUBLIC void sys_newdelay(int milli_sec)
{
	p_proc_ready->ticks = milli_sec * HZ / 1000;
	while (1)
	{
		p_proc_ready = (p_proc_ready + 1 - proc_table) % NR_TASKS + proc_table;
		if (p_proc_ready->ticks <= 0 && p_proc_ready->blocked != 1)
			break;
	}
}
/*======================================================================*
						   sys_P
 *======================================================================*/
PUBLIC void sys_P(Semaphore* s, int proc)
{
	s->value--;
	if (s->value < 0)
	{
		s->Q[s->phead] = proc;
		p_proc_ready->blocked = 1;
		while (1)
		{
			p_proc_ready = (p_proc_ready + 1 - proc_table) % NR_TASKS + proc_table;
			if (p_proc_ready->ticks <= 0 && p_proc_ready->blocked != 1)
				break;
		}//阻塞当前程序并寻找下一能运行程序
		s->phead = (s->phead + 1) % Queue_SIZE;
	}
}
/*======================================================================*
						   sys_V
 *======================================================================*/
PUBLIC void sys_V(Semaphore* s)
{
	s->value++;
	if (s->value <= 0)
	{
		int proc = s->Q[s->ptail];
		proc_table[proc].blocked = 0;
		p_proc_ready = proc_table + proc; //唤醒队尾程序
		s->ptail = (s->ptail + 1) % Queue_SIZE;
	}
}

PUBLIC void setS(Semaphore* s, int value)
{
	s->phead = s->ptail = 0;
	s->value = value;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)
#define InputMode 0
#define SearchMode 1
PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);
PRIVATE void ESC_handler(TTY* p_tty);
PUBLIC void Ctrl_z(u32 key);
PUBLIC void initial_tty();
PRIVATE int str_len;
PUBLIC int currentMode;
char temp_list_input[1024] = { 0 };
char *ptr_in;
char temp_list_esc[1024] = { 0 };
char *ptr_esc;
int esc_str_len;
int esc_base;
PRIVATE int isLocked;

/*======================================================================*
						   initial_tty
 *======================================================================*/
PUBLIC void initial_tty() {
	int base = V_MEM_BASE;
	ptr_in = temp_list_input;
	int esc_str_len = 0;
	ptr_esc = temp_list_esc;
	u8 *p_vmem = (u8 *)(V_MEM_BASE + disp_pos - 1);
	while (p_vmem > base)
	{
		*p_vmem-- = 0x07;
		*p_vmem-- = ' ';
	}
	disp_pos = base - V_MEM_BASE;
	set_cursor(disp_pos);
	set_video_start_addr(0);
	str_len = 0;
	currentMode = InputMode;
	isLocked = 0;
	init_keyboard();
}
/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	int t = get_ticks();
	int interval = 69000;
	TTY*	p_tty;
	initial_tty();
	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	while (1) {
		if (currentMode!=SearchMode && ((get_ticks() - t) * 1000 / HZ) >= interval)
		{ 
			int base = V_MEM_BASE;
			u8 *p_vmem = (u8 *)(V_MEM_BASE + disp_pos - 1);
			while (p_vmem > base)
			{
				*p_vmem-- = 0x07;
				*p_vmem-- = ' ';
			}
			disp_pos = base - V_MEM_BASE;
			set_cursor(disp_pos);
			set_video_start_addr(0);
			t = get_ticks();
		}
		else {
			t = get_ticks();
		}
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
        char output[2] = {'\0', '\0'};
		
        if (!(key & FLAG_EXT)) {
			if(!isLocked)
				put_key(p_tty, key);
			if(currentMode==SearchMode)
				str_len++;
        }
        else {
        	int raw_code = key & MASK_RAW;
            switch(raw_code) {
                case ENTER:
					if(currentMode==SearchMode) {
						u8* ptr = (u8*)(V_MEM_BASE + p_tty->p_console->cursor * 2 - str_len * 2);
						for (u8* i = (u8*)(V_MEM_BASE); i < ptr; i += 2) {
							int isEqual = 1;
							for (int j = 0; j < str_len; j++) {
								if (*(i + j * 2) != *(ptr + j * 2) || i + j * 2 >= ptr) {
									isEqual = 0;
									break;
								}
							}

							if (isEqual) {
								for (int j = 0; j < str_len; j++) {
									*(i + j * 2 + 1) = RED;
								}

							}

						}
						flush(p_tty->p_console);
						isLocked = 1;
					} else {
						if(!isLocked)
							put_key(p_tty, '\n');
						*ptr_in ++= '\n';
					}
					break;
                case BACKSPACE:
					if(currentMode==SearchMode) {
						if(str_len > 0){	
							str_len--;
							if(!isLocked)
								put_key(p_tty, '\b');
						}
					} else {
						if(!isLocked)
							put_key(p_tty, '\b');
					}
					
					break;
                case UP:
                    			if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_DN);
                        	}
					break;
		case DOWN:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
					scroll_screen(p_tty->p_console, SCR_UP);
				}
					break;
		case F1:
		case F2:
		case F3:
		case F4:
		case F5:
		case F6:
		case F7:
		case F8:
		case F9:
		case F10:
		case F11:
		case F12:
				/* Alt + F1~F12 */
				if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
					select_console(raw_code - F1);
				}
					break;
		case TAB:
				if(!isLocked)
					put_key(p_tty, '\t');
				break;
					
		case ESC:
			ESC_handler(p_tty);
				break;
                default:
                    break;
                }
        }
}
PRIVATE void ESC_handler(TTY* p_tty) {
	if (currentMode == SearchMode) {
		currentMode = InputMode;
		for (int i = 0; i < str_len; i++) {
			put_key(p_tty, '\b');
		}
		str_len = 0;
		u8* p_vmem = (u8*)(V_MEM_BASE + p_tty->p_console->cursor * 2);
		for (u8* i = (u8*)(V_MEM_BASE); i < p_vmem; i += 2) {
			if (*i != '\t' && *i != '\n') {
				*(i + 1) = DEFAULT_CHAR_COLOR;
			}
		}
		flush(p_tty->p_console);
		isLocked = 0;
	}
	else {
		str_len = 0;
		currentMode = SearchMode;
	}

}
PUBLIC void Ctrl_z(u32 key)
{
	char output[2] = { '\0', '\0' };
	if ((key & FLAG_CTRL_L) || (key & FLAG_CTRL_R))
	{
		if (output[0] == 'z')
		{
		}
		else if (output[0] == 'c')
		{
		}
		set_cursor(disp_pos / 2);
	}
}
/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}



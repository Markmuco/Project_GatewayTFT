/*!
 * \file  shell.c
 * \brief This module provides functions command shell
 *
 * 1.0
 * 1.1
 * 1.2 macros dor VT100 commands include colors
 * 1.3 functions in external file shell_usercmd.c
 
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <shell.h>

#include "shell_usercmd.h"
#include "main.h"
#include "uart_sci.h"
#include "ms_timers.h"


#if LOGIN_ENABLE>0
#include "md5.h"
bool logged_in = false;
static uint8_t login_tmr = NO_TIMER;
#endif

// Private variables.
static uint8_t tmr = NO_TIMER;

static char mem_pool[SHELL_CMDLINE_SIZE * (SHELL_CMDLINE_HIST_COUNT + 1)];
static cmdline_obj_t cmdline;


/* USER CODE BEGIN 1 */
/* USER CODE END 1 */

// Internal Functions
static bool shell_strcmp(const char *s1, const char *s2);
static void shell_cmdline_init(cmdline_obj_t *object);
static bool shell_cmdline_is_duplicate(cmdline_obj_t *object);
static void shell_cmdline_add(cmdline_obj_t *object);
bool shell_next_argv(char ** p_buf);
static bool shell_getc(uint8_t *c);
#if (SHELL_CMDLINE_HIST_COUNT > 0)
static bool shell_cmdline_is_duplicate(cmdline_obj_t *object);
static void shell_cmdline_get(cmdline_obj_t *object);
static void shell_cmdline_get_prev(cmdline_obj_t *object);
static void shell_cmdline_get_next(cmdline_obj_t *object);
#endif
static bool shell_cmdline_execute(cmdline_obj_t *object);
static bool shell_tabline_find(cmdline_obj_t *object);
static void shell_tabline_get(cmdline_obj_t *object);
#if LOGIN_ENABLE>0
static bool shell_cmdline_login(cmdline_obj_t *object);
#endif

// Private constants.
// Command registry (runtime)
#ifndef SHELL_MAX_CMDS
#define SHELL_MAX_CMDS 64
#endif

static cmd_tbl_t cmd_tbl[SHELL_MAX_CMDS];
static uint8_t cmd_tbl_count = 0;

/*
 * \brief Register new command in the shell.
 */
int add_cmd(const char *cmd, const char *desc, void (*fxn)(char *param))
{
    if (!cmd || !fxn) return -1;
    if (cmd_tbl_count >= SHELL_MAX_CMDS) return -2;

    // prevent duplicates
    for (uint8_t i = 0; i < cmd_tbl_count; i++)
    {
        if (cmd_tbl[i].cmd && (strcmp(cmd_tbl[i].cmd, cmd) == 0))
            return -3;
    }

    cmd_tbl[cmd_tbl_count].cmd  = (char *)cmd;
    cmd_tbl[cmd_tbl_count].desc = (char *)(desc ? desc : "");
    cmd_tbl[cmd_tbl_count].fxn  = fxn;
    cmd_tbl_count++;
    return 0;
}

uint8_t shell_cmd_count(void)
{
    return cmd_tbl_count;
}

const cmd_tbl_t *shell_cmd_table(void)
{
    return cmd_tbl;
}

/*
 * \brief This function sets the shell prompt to the given name.
 */
static void shell_hostname(char * name)
{
	static char g_hostname_data[SHELL_HOSTNAME_SIZE + 1] = "";

	if (name != NULL)
	{
		strncpy(g_hostname_data, name, SHELL_HOSTNAME_SIZE);
	}

	if (g_hostname_data[0] != '\0')
	{
		tty_puts(VT100_COLOR_RED);
		tty_putc('[');
		tty_puts(g_hostname_data);
		tty_puts("]$");
		tty_puts(VT100_COLOR_DEFAULT);
	}
}

/*!
 * \brief	This function opens the shell using SCI1
 *
 * \ Parameter	vars pointer to vars_t
 *
 * \return -.
 */
void shell_open(char * name)
{
	if (tmr == NO_TIMER)
	{
		tmr = timer_get();
	}

	// (Re)build command table
	cmd_tbl_count = 0;
	shell_register_cmds();

	shell_cmdline_init(&cmdline);

	shell_hostname(name);

#if FLEXIBLE_SCI1 || FLEXIBLE_SCI2 || FLEXIBLE_SCI3 || FLEXIBLE_SCI4 || FLEXIBLE_SCI5 || FLEXIBLE_SCI6 || FLEXIBLE_SCI7 || FLEXIBLE_SCI8
	shell_use_sci1();
#endif
}

/*!
 * \brief This function closes the shell.
 *
 * \ Parameter -.
 *
 * \return -.
 */
void shell_close(void)
{
	timer_free(&tmr);
}

/*!
 * \brief This function handles the shell.
 *
 * \ Parameter -.
 *
 * \return -.
 */
void shell_process(void)
{
	uint8_t c;
	static uint8_t left_cursor = 0;

	if (shell_getc(&c))
	{
		switch (c)
		{
#if (SHELL_CMDLINE_HIST_COUNT > 0)
		case VK_UPARROW:
			left_cursor = 0;
#if LOGIN_ENABLE>0
			if (logged_in)
#endif
			{
				// Get the previous item from the history list.
				shell_cmdline_get_prev(&cmdline);

				// Handle cursor position.
				tty_puts(VT100_ERASE_LINE); // VT100 erase line.
				tty_putc('\r');
				shell_hostname(NULL);
				tty_putsn(cmdline.p_cmdline->str, cmdline.p_cmdline->size);
			}
			break;

		case VK_DOWNARROW:
			left_cursor = 0;
#if LOGIN_ENABLE>0
			if (logged_in)
#endif
			{
				// Get the next item from the history list.
				shell_cmdline_get_next(&cmdline);

				// Handle cursor position.
				tty_puts(VT100_ERASE_LINE); // VT100 erase line.
				tty_putc('\r');
				shell_hostname(NULL);
				tty_putsn(cmdline.p_cmdline->str, cmdline.p_cmdline->size);
			}
			break;
#endif
		case VK_BS:
			if (cmdline.p_cmdline->size)
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
				tty_puts(VT100_CURSOR_BACK); // VT100 cursor backward.
#if ADVANCED_ENABLE>0
				// cursor left is used print again from current position
				if (left_cursor > 0)
				{
					for (int var = cmdline.p_cmdline->size - left_cursor; var < cmdline.p_cmdline->size; ++var)
					{
						tty_putc(cmdline.p_cmdline->str[var]);
						// move the captured chars back
						cmdline.p_cmdline->str[var - 1] = cmdline.p_cmdline->str[var];
					}
				}
				tty_puts(VT100_ERASE_EOL); // VT100 erase end of line.

				for (int var = cmdline.p_cmdline->size - left_cursor; var < cmdline.p_cmdline->size; ++var)
					tty_puts(VT100_CURSOR_LEFT); // VT100 cursor left pn times - stop at far left

#else
					tty_puts(VT100_ERASE_EOL); // VT100 erase end of line.

#endif
				cmdline.p_cmdline->size--;
			}
			break;

#if ADVANCED_ENABLE>0
		case VK_DEL:
			if (cmdline.p_cmdline->size)
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);

				// cursor left is used print again from current position
				if (left_cursor > 0)
				{
					for (int var = cmdline.p_cmdline->size - left_cursor + 1; var < cmdline.p_cmdline->size; ++var)
					{
						tty_putc(cmdline.p_cmdline->str[var]);
						// move the captured chars back
						cmdline.p_cmdline->str[var - 1] = cmdline.p_cmdline->str[var];
					}
				}
				tty_puts(VT100_ERASE_EOL); // VT100 erase end of line.

				for (int var = cmdline.p_cmdline->size - left_cursor + 1; var < cmdline.p_cmdline->size; ++var)
					tty_puts(VT100_CURSOR_LEFT); // VT100 cursor left pn times - stop at far left

				cmdline.p_cmdline->size--;
				left_cursor--;
			}
			break;

		case VK_LEFTARROW:
			// cursor to first char
			if (cmdline.p_cmdline->size - left_cursor > 0)
			{
				left_cursor++;
				tty_puts(VT100_CURSOR_LEFT); // VT100 cursor left pn times - stop at far left
			}
			break;

		case VK_RIGHTARROW:
			// cursor limit to end
			if (left_cursor > 0)
			{
				left_cursor--;
				tty_puts(VT100_CURSOR_RIGHT); // VT100 cursor right pn times - stop at far right
			}
			break;

		case VK_HOME:
			// cursor to home
			for (int var = 0; var < cmdline.p_cmdline->size - left_cursor; ++var)
				tty_puts(VT100_CURSOR_LEFT); // VT100 cursor left pn times - stop at far left

			left_cursor = cmdline.p_cmdline->size;
			break;

		case VK_END:
			// cursor to end
			if (left_cursor > 0)
			{
				for (int var = cmdline.p_cmdline->size - left_cursor; var < cmdline.p_cmdline->size; ++var)
					tty_puts(VT100_CURSOR_RIGHT); // VT100 cursor right pn times - stop at far right

				left_cursor = 0;
			}
			break;
#endif

		case VK_TAB:
#if LOGIN_ENABLE>0
			if (logged_in)
#endif
		{
			if (cmdline.p_cmdline->size)
			{
				if (shell_tabline_find(&cmdline))
				{
					// Handle cursor position.
					tty_puts(VT100_ERASE_LINE); // VT100 erase line.
					tty_putc('\r');
					shell_hostname(NULL);
					tty_putsn(cmdline.tabline.str, cmdline.tabline.size);
				}
			}
		}
			break;

		case VK_CR:
			left_cursor = 0;
#if LOGIN_ENABLE>0
			// Handle cursor position.
			if (logged_in)
#endif
			{
				tty_puts("\r\n");
				if (cmdline.p_cmdline->size)
				{
					//tty_printf("(s=%d)", cmdline.p_cmdline->size);
					// Copy tab line to command line.
					shell_tabline_get(&cmdline);
					cmdline.p_cmdline->str[cmdline.p_cmdline->size] = '\0';

					if (!shell_cmdline_execute(&cmdline))
						tty_printf("'%s' is not recognized as an internal command.\r\n", cmdline.p_cmdline->str);

					shell_cmdline_add(&cmdline);
				}
				shell_hostname(NULL);
			}
#if LOGIN_ENABLE>0
			else
			{
				tty_puts("\r\nLogin: ");
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);
				cmdline.p_cmdline->str[cmdline.p_cmdline->size] = '\0';

				if (!shell_cmdline_login(&cmdline))
				tty_printf("'%s' is not recognised as an internal command.\r\n", cmdline.p_cmdline->str);

				// do not store passwords
				cmdline.cmd_idx = 0;
				cmdline.p_cmdline->str[0] = 0;
				cmdline.p_cmdline->size = 0;
			}
#endif
			break;

		case VK_ESC:
			left_cursor = 0;
			if (cmdline.p_cmdline->size)
			{
				// Erase tab line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
				tty_puts(VT100_ERASE_LINE); // VT100 erase line.
				tty_putc('\r');
				shell_hostname(NULL);
				cmdline.p_cmdline->size = 0;
			}
			break;

		default:
			if ((cmdline.p_cmdline->size < (SHELL_CMDLINE_SIZE - 1)) && (c >= 32) && (c < 127))
			{
				// Copy tab line to command line.
				shell_tabline_get(&cmdline);

				// Handle cursor position.
#if LOGIN_ENABLE>0
				if (logged_in)
				tty_putc(c);
				else
				tty_putc('*');
#else
				tty_putc(c);
#endif

#if ADVANCED_ENABLE==0
				// add the char to the input position
				cmdline.p_cmdline->str[cmdline.p_cmdline->size] = c;
#else
				// cursor left is used print again from current position
				if (left_cursor > 0)
				{
					cmdline.p_cmdline->size++;

					// push chars forward
					for (int var = cmdline.p_cmdline->size; var > cmdline.p_cmdline->size - left_cursor - 1; --var)
						cmdline.p_cmdline->str[var] = cmdline.p_cmdline->str[var - 1];

					// insert new char
					cmdline.p_cmdline->str[cmdline.p_cmdline->size - left_cursor - 1] = c;

					// print line
					for (int var = cmdline.p_cmdline->size - left_cursor; var < cmdline.p_cmdline->size; ++var)
						tty_putc(cmdline.p_cmdline->str[var]);

					// cursor to input position
					for (int var = cmdline.p_cmdline->size - left_cursor; var < cmdline.p_cmdline->size; ++var)
						tty_puts(VT100_CURSOR_LEFT); // VT100 cursor left pn times - stop at far left
				}
				else
					// add the char to the input position
					cmdline.p_cmdline->str[cmdline.p_cmdline->size - left_cursor] = c;

				if (left_cursor == 0)
#endif
					// cursor on the end add length
					cmdline.p_cmdline->size++;
			}
			break;
		}
	}
}

/*!
 * \brief This function reads and filters characters from the TTY port.
 *
 * \ Parameter c Pointer to the character read.
 *
 * \return True if successful, false on error.
 *
 * \note The following virtual keys are supported:
 *
 *   1B 4F 50       : NUMLOCK
 *   1B 4F 51       : SLASH
 *   1B 4F 52       : MULTIPLY
 *   1B 4F 53       : MINUS
 *   1B 5B 31 7E    : HOME
 *   1B 5B 31 31 7E : F1
 *   1B 5B 31 32 7E : F2
 *   1B 5B 31 33 7E : F3
 *   1B 5B 31 34 7E : F4
 *   1B 5B 31 35 7E : F5
 *   1B 5B 31 37 7E : F6
 *   1B 5B 31 38 7E : F7
 *   1B 5B 31 39 7E : F8
 *   1B 5B 32 7E    : INS
 *   1B 5B 32 30 7E : F9
 *   1B 5B 32 31 7E : F10
 *   1B 5B 32 33 7E : F11
 *   1B 5B 32 34 7E : F12
 *   1B 5B 34 7E    : END
 *   1B 5B 35 7E    : PGUP
 *   1B 5B 36 7E    : PGDN
 *   1B 5B 41       : UPARROW
 *   1B 5B 42       : DOWNARROW
 *   1B 5B 43       : RIGHTARROW
 *   1B 5B 44       : LEFTARROW
 */
static bool shell_getc(uint8_t *c)
{
	static uint8_t state = 0;
	static uint8_t c_tmp;
	char c_raw;
	bool res = false;

	if (tty_getc(&c_raw))
	{
		switch (state)
		{
		case 0:
			switch (c_raw)
			{
			case 0x1B:
				timer_start(tmr, ESC_DELAY, NULL);
				state = 1;
				break;

			default:
				*c = c_raw;
				res = true;
				break;
			}
			break;

		case 1:
			switch (c_raw)
			{
			case 0x4F:
				state = 2;
				break;

			case 0x5B:
				state = 3;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 2:
			switch (c_raw)
			{
			case 0x50:
				*c = VK_NUMLOCK;
				res = true;
				state = 0;
				break;

			case 0x51:
				*c = VK_KP_SLASH;
				res = true;
				state = 0;
				break;

			case 0x52:
				*c = VK_KP_MULTIPLY;
				res = true;
				state = 0;
				break;

			case 0x53:
				*c = VK_KP_MINUS;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 3:
			switch (c_raw)
			{
			case 0x31:
				c_tmp = VK_HOME;
				state = 4;
				break;

			case 0x32:
				c_tmp = VK_INS;
				state = 5;
				break;

			case 0x34:
				c_tmp = VK_END;
				state = 6;
				break;

			case 0x35:
				c_tmp = VK_PGUP;
				state = 6;
				break;

			case 0x36:
				c_tmp = VK_PGDN;
				state = 6;
				break;

			case 0x41:
				*c = VK_UPARROW;
				res = true;
				state = 0;
				break;

			case 0x42:
				*c = VK_DOWNARROW;
				res = true;
				state = 0;
				break;

			case 0x43:
				*c = VK_RIGHTARROW;
				res = true;
				state = 0;
				break;

			case 0x44:
				*c = VK_LEFTARROW;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 4:
			switch (c_raw)
			{
			case 0x31:
				c_tmp = VK_F1;
				state = 6;
				break;

			case 0x32:
				c_tmp = VK_F2;
				state = 6;
				break;

			case 0x33:
				c_tmp = VK_F3;
				state = 6;
				break;

			case 0x34:
				c_tmp = VK_F4;
				state = 6;
				break;

			case 0x35:
				c_tmp = VK_F5;
				state = 6;
				break;

			case 0x37:
				c_tmp = VK_F6;
				state = 6;
				break;

			case 0x38:
				c_tmp = VK_F7;
				state = 6;
				break;

			case 0x39:
				c_tmp = VK_F8;
				state = 6;
				break;

			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 5:
			switch (c_raw)
			{
			case 0x30:
				c_tmp = VK_F9;
				state = 6;
				break;

			case 0x31:
				c_tmp = VK_F10;
				state = 6;
				break;

			case 0x33:
				c_tmp = VK_F11;
				state = 6;
				break;

			case 0x34:
				c_tmp = VK_F12;
				state = 6;
				break;

			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;

		case 6:
			switch (c_raw)
			{
			case 0x7E:
				*c = c_tmp;
				res = true;
				state = 0;
				break;

			default:
				state = 0;
				break;
			}
			break;
		}
	}
	else if (timer_elapsed(tmr) && (state == 1))
	{
		*c = VK_ESC;
		res = true;
		state = 0;
	}
	return (res);
}

/*!
 * \brief This function compares two strings.
 *
 * \ Parameter s1 A pointer to a string.
 * \ Parameter s2 A pointer to a string.
 *
 * \return True if successful, false on error.
 */
static bool shell_strcmp(const char *s1, const char *s2)
{
	// comparing until table ends
	for (; *s1 == *s2; s1++, s2++)
	{
		if (*s1 == '\0')
			return (true);
	}

	// MUR 22-5-2022 table must be ended and argveter follows
	if ((*s1 == 0) && ((*s2 == ' ') || (*s2 == '\f') || (*s2 == '\n') || (*s2 == '\r') || (*s2 == '\t') || (*s2 == '\v')))
			return (true);

	return (false);
}

/*!
 * \brief This function initializes the command line object.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_init(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		object->cmdline[i].id = i;
		object->cmdline[i].str = &mem_pool[SHELL_CMDLINE_SIZE * i];
		object->cmdline[i].size = 0;

		if (object->cmdline[i].id == SHELL_CMDLINE_HIST_COUNT)
		{
			object->p_cmdline = &object->cmdline[i];
			object->p_cmdline->size = 0;
		}
	}
#if (SHELL_CMDLINE_HIST_COUNT > 0)
	object->cmd_idx = 0;
#endif
	object->tabline.str = NULL;
	object->tabline.size = 0;
	object->tab_idx = 0;
}


/*!
 * \brief This function adds an item to the command line object.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_add(cmdline_obj_t *object)
{
	uint8_t i;

#if (SHELL_CMDLINE_HIST_COUNT > 0)
    /* Niet opslaan als hetzelfde commando net al in history staat */
    if (object->p_cmdline->size > 0)
    {
        if (shell_cmdline_is_duplicate(object) != 0)
        {
            object->cmd_idx = 0;
            object->p_cmdline->size = 0;
            return;
        }
    }
#endif

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		object->cmdline[i].id++;
		if (object->cmdline[i].id > SHELL_CMDLINE_HIST_COUNT)
			object->cmdline[i].id = 0;

		if (object->cmdline[i].id == SHELL_CMDLINE_HIST_COUNT)
		{
			object->p_cmdline = &object->cmdline[i];
			object->p_cmdline->size = 0;
		}
	}
#if (SHELL_CMDLINE_HIST_COUNT > 0)
	object->cmd_idx = 0;
#endif
}

#if (SHELL_CMDLINE_HIST_COUNT > 0)
/*!
 * \brief This function checks if the current command line is a duplicate of the last entered command.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return - true if the current command line is a duplicate of the last entered command, false otherwise.
 */
static bool shell_cmdline_is_duplicate(cmdline_obj_t *object)
{
    uint8_t i;
    uint8_t last_idx = 0xFF;

    if (object == NULL)
        return false;

    if (object->p_cmdline == NULL)
        return false;

    if (object->p_cmdline->size == 0)
        return false;

    /* In deze history-implementatie staat de meest recent opgeslagen  command op id == 0. */
    for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
    {
        if (object->cmdline[i].id == 0)
        {
            last_idx = i;
            break;
        }
    }

    if (last_idx == 0xFF)
        return false;

    if (object->cmdline[last_idx].size == 0)
        return false;

    if (object->cmdline[last_idx].size != object->p_cmdline->size)
        return false;

    if (memcmp(object->cmdline[last_idx].str, object->p_cmdline->str, object->p_cmdline->size) != 0)
        return false;

    return true;
}

/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < (SHELL_CMDLINE_HIST_COUNT + 1); i++)
	{
		if (object->cmdline[i].id == object->cmd_idx)
		{
			memcpy(object->p_cmdline->str, object->cmdline[i].str, object->cmdline[i].size);
			object->p_cmdline->size = object->cmdline[i].size;
			break;
		}
	}
}

/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get_prev(cmdline_obj_t *object)
{
	shell_cmdline_get(object);
	if (!object->p_cmdline->size)
	{
		object->cmd_idx--;
		shell_cmdline_get(object);
	}

	if (object->cmd_idx < (SHELL_CMDLINE_HIST_COUNT - 1))
		object->cmd_idx++;

}

/*!
 * \brief This function retrieves an item from the command line object.
 *
 * \ Parameter object A pointer to a command line object.
 *
 * \return -.
 */
static void shell_cmdline_get_next(cmdline_obj_t *object)
{
	if (object->p_cmdline->size)
	{
		if (object->cmd_idx > 0)
			object->cmd_idx--;

		shell_cmdline_get(object);
		if (!object->p_cmdline->size)
		{
			object->cmd_idx++;
			shell_cmdline_get(object);
		}
	}
}
#endif

static bool shell_cmdline_execute(cmdline_obj_t *object)
{
	uint8_t i;

	for (i = 0; i < cmd_tbl_count; i++)
	{
		if (shell_strcmp(cmd_tbl[i].cmd, object->p_cmdline->str))
			break;
	}

	if (i < cmd_tbl_count)
	{
		if (cmd_tbl[i].fxn)
		{
			char * p;
			p = strchr(cmdline.p_cmdline->str, ' ');
			
			// put pointer p to the end of the input
			if (p == NULL)
				p = cmdline.p_cmdline->str + cmdline.p_cmdline->size;
			
			cmd_tbl[i].fxn(p);
		}

		return (true);
	}

	return (false);
}

#if LOGIN_ENABLE>0
static bool shell_cmdline_login(cmdline_obj_t *object)
{
	uint8_t i;

	// when logged in search else command was login
	for (i = 0; i < CMD_TBL_COUNT; i++)
	{
		if (shell_strcmp(cmd_tbl[i].cmd, "login"))
		break;
	}

	if (i < CMD_TBL_COUNT)
	{
		if (cmd_tbl[i].fxn)
		{
			char * p;
			p = object->p_cmdline->str;
			cmd_tbl[i].fxn(p);
		}

		return (true);
	}

	return (false);
}

/*!
 * \brief Compares string array with const array
 *
 * \ Parameter
 *
 * \return - TRUE when match
 */
static bool md5compare(char * pwd, uint8_t * arry)
{
    char a[2];

     for (int i = 0; i < MD5_DIGEST_SIZE; i++)
     {
         a[0] = pwd[i*2];
         a[1] = pwd[i*2+1];

         if (strtol(a, 0, 16) != arry[i])
        	 return false;
     }
    return true;
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
void sh_login(char *argv)
{
	static uint8_t tries = 0;
	MD5_CTX mdContext;

	// release the login timer
	if (timer_elapsed(login_tmr))
	{
		timer_free(&login_tmr);
		tries = 0;
	}

	if (argv[0])
	{
        MD5Init(&mdContext);
        MD5Update(&mdContext, (uint8_t *)argv, strlen(argv));
        MD5Final(&mdContext);

		if (login_tmr == NO_TIMER)
		{
			if (md5compare(PASSWORD_1, mdContext.digest) || md5compare(PASSWORD_2, mdContext.digest))
			{
				tries = 0;
				logged_in = true;
				tty_printf(WELCOME_TXT);
				shell_hostname(NULL);
			}
			else
			{
				tty_printf("Error login\r\n");
				if (tries++ == 4)
				{
					login_tmr = timer_get();
					timer_start(login_tmr, 5000, NULL);
				}
			}
		}
		else
		{
			timer_start(login_tmr, 5000, NULL);
			tty_printf("Please wait..\r\n");
		}
	}
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
void sh_logout(char *argv)
{
	logged_in = false;
}


#endif

/*!
 * \brief This function finds a (partial) command in a table.
 *
 * \ Parameter object A pointer to a tab line object.
 *
 * \return -.
 */
static bool shell_tabline_find(cmdline_obj_t *object)
{
	uint8_t i;

	for (object->tabline.str = NULL, i = 0; (object->tabline.str == NULL) && (i < cmd_tbl_count); i++)
	{
		if (!strncmp(cmd_tbl[object->tab_idx].cmd, object->p_cmdline->str, object->p_cmdline->size))
		{
			object->tabline.str = (char *) cmd_tbl[object->tab_idx].cmd;
			object->tabline.size = strlen(object->tabline.str);
		}

		object->tab_idx++;
		if (object->tab_idx >= cmd_tbl_count)
			object->tab_idx = 0;
	}

	return (object->tabline.str != NULL);
}

/*!
 * \brief This function copies the last tab line to the command line.
 *
 * \ Parameter object A pointer to a tab line object.
 *
 * \return -.
 */
static void shell_tabline_get(cmdline_obj_t *object)
{
	if (object->tabline.str)
	{
		memcpy(object->p_cmdline->str, object->tabline.str, object->tabline.size);
		object->p_cmdline->size = object->tabline.size;

		object->tabline.str = NULL;
	}
	object->tab_idx = 0;
}

/*!
 * \brief Browse pointer to next argveter
 *
 * \ Parameter
 *
 * \return -.
 */
bool shell_next_argv(char ** p_buf)
{
		// strchr did not found a space
	if ((*p_buf) == 0)
		return false;
	
	// end of line
	if (*(*p_buf) == 0)
		return false;

	// browse to separator (space, dot, semicolum)
	while (*(*p_buf) != 0 && *(*p_buf) != ' ' && *(*p_buf) != '.' && *(*p_buf) != ':')
		(*p_buf)++;

	// another separator?
	while ((*(*p_buf) == ' ' || (*(*p_buf) == '.') || (*(*p_buf) == ':')) && (*(*p_buf) != 0))
		(*p_buf)++;

	// next character must be available
	if (*(*(p_buf)) != 0)
		return true;

	return false;
}

/*
 * ---------------------------------- Default Shell functions ----------------------------------
 */



/*!
 * \brief This function handles the shell help command.
 *
 * \ Parameter -.
 *
 * \return -.
 */
void sh_help(char *argv)
{
	uint8_t i;

	tty_printf("Available commands:\r\n");
	for (i = 0; i < cmd_tbl_count; i++)
		tty_printf("%-15s   %s\r\n", cmd_tbl[i].cmd, cmd_tbl[i].desc);
}

/*!
 * \brief
 *
 * \ Parameter
 *
 * \return -.
 */
void sh_reboot(char *argv)
{
	uint8_t nr;

	if (shell_next_argv(&argv))
	{
		nr = strtol(argv, 0, 10);
	}
	tty_printf("NVICReset\r\n");
	HAL_Delay(10);
	NVIC_SystemReset();
}




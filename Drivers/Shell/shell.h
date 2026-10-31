/*
 * shell.h
 *
 *  Created on: 16 mrt. 2018
 *      Author: Mark
 */

#ifndef SHELL_H_
#define SHELL_H_

#include "main.h"




// Function of first login
#define ADVANCED_ENABLE				(1)	// using left and right arrow, HOME, END and DEL
#define	LOGIN_ENABLE				(0)
#if LOGIN_ENABLE>0
// use https://www.md5hashgenerator.com/
#define PASSWORD_1					"98c7242894844ecd6ec94af67ac8247d" // 1968
#define PASSWORD_2					"118557af69c0b994086ed89d2ffb5a76" // VanMoofs default
#define WELCOME_TXT					"\r\nWelcome to Shell\r\n"
#endif 

// Command line size.
#define SHELL_CMDLINE_SIZE         (80+1)
#define SHELL_HOSTNAME_SIZE        (16)

// Number of history lines.
#define SHELL_CMDLINE_HIST_COUNT   (8)

// Time for an delayed ESC in ms.
#define ESC_DELAY                  (10)

// VT100 special commands
#define VT100_ERASE_SCREEN		 	"\x1B[2J"
#define VT100_CURSOR_HOME		 	"\x1B[H"
#define VT100_ERASE_LINE		 	"\x1B[2K"
#define VT100_CURSOR_OFF		 	"\x1B[?25l"
#define VT100_CURSOR_ON			 	"\x1B[?25h"

#define VT100_ERASE_LINE		 	"\x1B[2K" // VT100 erase line.
#define VT100_CURSOR_LEFT			"\x1B[1D"  
#define VT100_ERASE_EOL				"\x1B[K"
#define VT100_CURSOR_RIGHT			"\x1B[1C"
#define VT100_CURSOR_BACK			"\x1B[D"
				
#define VT100_COLOR_DEFAULT			"\e[39m"
#define VT100_COLOR_BLACK			"\e[30m"
#define VT100_COLOR_RED				"\e[31m"
#define VT100_COLOR_GREEN			"\e[32m"
#define VT100_COLOR_YELLOW			"\e[33m"
#define VT100_COLOR_BLUE			"\e[34m"
#define VT100_COLOR_MAGENTA			"\e[35m"
#define VT100_COLOR_CYAN			"\e[36m"
#define VT100_COLOR_LIGHT_GRAY		"\e[37m"
#define VT100_COLOR_DARK_GRAY		"\e[90m"
#define VT100_COLOR_LIGHT_RED		"\e[91m"
#define VT100_COLOR_LIGHT_GREEN		"\e[92m"
#define VT100_COLOR_LIGHT_YELLOW	"\e[93m"
#define VT100_COLOR_LIGHT_BLUE		"\e[94m"
#define VT100_COLOR_LIGHT_MAGENTA	"\e[95m"
#define VT100_COLOR_LIGHT_CYAN		"\e[96m"
#define VT100_COLOR_WHITE			"\e[97m"

#define VT100_STORE_POS				"\x1B[s"	// Save current cursor position
#define VT100_MOVE_TOP				"\x1B[1;100H" // Move to top-left (row 1, column 20)
#define VT100_RESTORE_POS			"\x1B[u"	// Restore previous cursor position
#define VT100_POS_100				"\x1B[100G"	// Move to column 100 of current line

// Virtual key codes.
enum
{
	// Regular keys.
	VK_BEL = '\a',
	VK_BS = '\b',
	VK_TAB = '\t',
	VK_LF = '\n',
	VK_VT = '\v',
	VK_FF = '\f',
	VK_CR = '\r',
	VK_ESC = 0x1B,
	VK_DEL = 0x7F,
	// Function keys.
	VK_F1 = 0x80,
	VK_F2,
	VK_F3,
	VK_F4,
	VK_F5,
	VK_F6,
	VK_F7,
	VK_F8,
	VK_F9,
	VK_F10,
	VK_F11,
	VK_F12,
	// Navigation keys.
	VK_HOME,
	VK_INS,
	VK_END,
	VK_PGUP,
	VK_PGDN,
	VK_UPARROW,
	VK_DOWNARROW,
	VK_RIGHTARROW,
	VK_LEFTARROW,
	// Keypad keys.
	VK_NUMLOCK,
	VK_KP_SLASH,
	VK_KP_MULTIPLY,
	VK_KP_MINUS
};

typedef struct
{
	char *cmd;
	char *desc;
	void (*fxn)(char *param);
} cmd_tbl_t;

typedef struct
{
	uint8_t id;
	char *str;
	uint8_t size;
} line_obj_t;

typedef struct
{
	line_obj_t cmdline[SHELL_CMDLINE_HIST_COUNT + 1];
	line_obj_t *p_cmdline;
#if (SHELL_CMDLINE_HIST_COUNT > 0)
	uint8_t cmd_idx;
#endif
	line_obj_t tabline;
	uint8_t tab_idx;
} cmdline_obj_t;


// Command registry API
int add_cmd(const char *cmd, const char *desc, void (*fxn)(char *param));

// Argument helper (was internal, now shared with user commands)
bool shell_next_argv(char ** p_buf);


#define SHELL_OPEN_HEX(x)                     \
    do {                                      \
        char __buf[16];                       \
        snprintf(__buf, sizeof(__buf), "%X", (int)(x)); \
        shell_open(__buf);                    \
    } while(0)

// Public function prototypes.
void shell_open(char * name);
void shell_close(void);
void shell_off(void);
void shell_process(void);

void sh_help(char *argv);


#if LOGIN_ENABLE>0
void sh_login(char *argv);
void sh_logout(char *argv);
#endif

#endif /* SHELL_H_ */

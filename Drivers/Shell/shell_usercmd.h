#ifndef SHELL_USERCMD_H_
#define SHELL_USERCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

// Register all commands (core + application/user commands).
// shell.c calls only this function, so you never have to touch shell.c again.
void shell_register_cmds(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_USERCMD_H_ */

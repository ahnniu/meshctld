#ifndef CMDS_H
#define CMDS_H

typedef void (*cmd_cb_t)(int argc, char *argv[]);

typedef struct cmds_entry {
	const char *cmd;
	const char *arg;
	cmd_cb_t func;
	const char *desc;
} cmds_entry;

extern const struct cmds_entry onoff_cmds[];
extern const struct cmds_entry cfg_cmds[];
extern const struct cmds_entry main_cmds[];

void parse_cmds(char * cmdline);

#endif /* CMDS_H */

#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include <string.h>

#include "mesh/cmds.h"


static void exec_cmds(const cmds_entry * entry, char * cmdline)
{
	wordexp_t p;

	wordexp(cmdline, &p, WRDE_NOCMD);
	if (p.we_wordc == 0) {
		wordfree(&p);
		return;
	}
	for (; entry->cmd; entry++) {
		if (strcmp(p.we_wordv[0], entry->cmd))
			continue;

		entry->func(p.we_wordc, p.we_wordv);
	}

	wordfree(&p);
}

void parse_cmds(char * cmdline)
{
	exec_cmds(main_cmds, cmdline);
	exec_cmds(cfg_cmds, cmdline);
	exec_cmds(onoff_cmds, cmdline);
}

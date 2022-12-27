#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "codybot.h"

void Cal(void) {
	// Remove highlighted (current) day, doesn't display right in the chat
	system("cal --color=always -3 | sed 's/\x5F\x8\\([0-9]\\)/\00308\\1\003/g' > cmd.output");

	FILE *fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buffer, "##codybot::Cal() error: Cannot open cmd.output: %s\n",
			strerror(errno));
		Msg(buffer);
		return;
	}

	size_t size = 1024;
	ssize_t ret;
	char *line = malloc(size);
	while (1) {
		memset(line, 0, size);
		ret = getline(&line, &size, fp);
		if (ret <= 0)
			break;
		else
			Msg(line);
	}

	free(line);
	fclose(fp);
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "codybot.h"

void Cal(void) {
	// Remove highlighted (current) day, doesn't display right in the chat
	system("cal -3 | sed 's/\x5F\x8\\([0-9]\\)/\00308\\1\003/g' > cmd.output");

	FILE *fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		char buffer[1024];
		sprintf(buffer, "##codybot::Cal() error: Cannot open cmd.output: %s\n",
			strerror(errno));
		Msg(buffer);
		return;
	}

	int cnt = 0;
	size_t size = 1024;
	char *line = malloc(size);
	while (++cnt <= 7) {
		memset(line, 0, 1024);
		getline(&line, &size, fp);
		Msg(line);
	}

	fclose(fp);
}


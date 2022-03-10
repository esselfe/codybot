#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "codybot.h"

void Date(void) {
	system("date > cmd.output");

	FILE *fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buffer, "##codybot::Date() error: Cannot open cmd.output: %s\n",
			strerror(errno));
		Msg(buffer);
		return;
	}

	size_t size = 1024;
	char *line = malloc(size);
	memset(line, 0, 1024);
	getline(&line, &size, fp);
	Msg(line);
}


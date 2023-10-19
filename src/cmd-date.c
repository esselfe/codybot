#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "codybot.h"

void Date(int offset) {
	if (offset == 0)
		sprintf(buffer, "TZ=UTC date > cmd.output");
	else if (offset < 0)
		sprintf(buffer,
			"TZ=UTC+%d date | sed 's/ UTC//' > cmd.output",
			-offset);
	else if (offset > 0)
		sprintf(buffer,
			"TZ=UTC-%d date | sed 's/ UTC//' > cmd.output",
			offset);

	system(buffer);

	FILE *fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buffer, "##codybot::Date() error: Cannot open cmd.output: %s\n",
			strerror(errno));
		Msg(buffer);
		return;
	}

	size_t size = 1024;
	char *line = malloc(size);
	memset(line, 0, size);
	getline(&line, &size, fp);
	fclose(fp);
	Msg(line);
	free(line);
}


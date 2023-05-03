#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <magic.h>

#include "codybot.h"

// Execute a shell command as a new thread.
void ShRunStart(char *command) {
	pthread_t thr;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thr, &attr, ShRunFunc, (void *)command);
	pthread_detach(thr);
	pthread_attr_destroy(&attr);
}

void *ShRunFunc(void *argp) {
	char *text = strdup(raw.text);
	printf("&& Thread started ::%s::\n", text);
	char *cp = text;
	char buf[4096]; // don't clash with the global buffer
	char cmd[4096];
	sprintf(cmd, "timeout %ds bash -c 'cd tmp; ", cmd_timeout);
	unsigned int cnt = strlen(cmd);
	while (1) {
		if (*cp == '\n' || *cp == '\0') {
			cmd[cnt] = '\0';
			break;
		}
/*		else if (*cp == '"') {
			cmd[cnt++] = '\\';
			cmd[cnt++] = '"';
		}
*/		else {
			cmd[cnt] = *cp;
			++cnt;
		}
		++cp;
	}
	strcat(cmd, "' &> cmd.output; echo $? >cmd.ret");
	Log(LOCAL, cmd);
	system(cmd);

	FILE *fp = fopen("cmd.ret", "r");
	if (fp == NULL) {
		sprintf(buf, "codybot::ShRunFunc() error: Cannot open cmd.ret: %s",
			strerror(errno));
		Msg(buf);
	}
	fgets(buf, 4096, fp);
	fclose(fp);

	ret = atoi(buf);
	if (ret == 124) {
		sprintf(buf, "sh: %s: timed out", text);
		Msg(buf);
		return NULL;
	}

	struct stat sto;
	stat("cmd.output", &sto);
	if (sto.st_size == 0) {
		Msg("(No output)");
		return NULL;
	}

	fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buf, "codybot::ShRunFunc() error: Cannot open cmd.output: %s",
			strerror(errno));
		Msg(buf);
		return NULL;
	}

	// count the line number
	char c;
	unsigned int lines_total = 0;
	while (1) {
		c = fgetc(fp);
		if (c == -1)
			break;
		else if (c == '\n')
			++lines_total;
	}
	fseek(fp, 0, SEEK_SET);

	if (sto.st_size == 1 && lines_total == 1) {
		Msg("(No output)");
		return NULL;
	}

	unsigned int lines_max = 4;
	if (strcmp(raw.channel, "#codybot") == 0)
		lines_max = 10;
	if (lines_total <= lines_max) {
		char *result = (char *)malloc(4096);
		memset(result, 0, 4096);
		size_t size = 4095;
		while (1) {
			int ret2 = getline(&result, &size, fp);
			if (ret2 < 0) break;
			else { // Change all tab characters with a space since it
				// doesn't display right on IRC
				char tabstr[4096];
				memset(tabstr, 0, 4096);
				sprintf(tabstr, "%s", result);
				char *c = tabstr;
				unsigned int cnt = 0;
				while (cnt < 4096) {
					if (*c == '\t') {
						result[cnt++] = ' ';
						result[cnt++] = ' ';
						result[cnt++] = ' ';
						result[cnt++] = ' ';
					}
					else {
						result[cnt++] = *c;
					}

					++c;
					if (*c == '\0')
						break;
				}
			}

			if (result[strlen(result)-1] == '\n')
				result[strlen(result)-1] = '\0';
			Msg(result);
		}
	}
	else if (lines_total >= lines_max+1) {
		system("cat cmd.output |nc termbin.com 9999 > cmd.url");
		FILE *fp2 = fopen("cmd.url", "r");
		if (fp2 == NULL)
			fprintf(stderr, "##codybot::ShRunFunc() error: Cannot open cmd.url: %s\n",
				strerror(errno));
		else {
			char url[1024];
			fgets(url, 1023, fp2);
			fclose(fp2);
			Msg(url);
		}
	}

	fclose(fp);

	printf("&& Thread stopped, ret: %d ::%s::\n", ret, text);

	return NULL;
}

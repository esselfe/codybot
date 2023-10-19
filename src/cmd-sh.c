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

int ShCheckEmptyOutput(void) {
	struct stat sto;
	stat("cmd.output", &sto);
	if (sto.st_size == 0) {
		Msg("(No output)");
		return 1;
	}
	
	return 0;
}

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
	char buf[4096];
	char cmd[1024];
	char *text = strdup(raw.text);
	printf("&& Thread started ::%s::\n", text);
	
	FILE *fp = fopen("prog.sh", "w+");
	if (fp == NULL) {
		sprintf(buf, "codybot::ShRunFunc() error: Cannot open prog.sh: %s",
			strerror(errno));
		Msg(buf);
	}
	fputs(text, fp);
	fclose(fp);
	
	sprintf(cmd, "bash -c 'timeout %ds bash prog.sh &> cmd.output; echo $? >cmd.ret'",
		cmd_timeout);
	Log(LOCAL, cmd);
	system(cmd);

	fp = fopen("cmd.ret", "r");
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
		free(text);
		return NULL;
	}

	if (ShCheckEmptyOutput()) {
		free(text);
		return NULL;
	}

	fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buf, "codybot::ShRunFunc() error: Cannot open cmd.output: %s",
			strerror(errno));
		Msg(buf);
		free(text);
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
	
	struct stat sto;
	stat("cmd.output", &sto);
	if (sto.st_size == 1 && lines_total == 1) {
		Msg("(No output)");
		free(text);
		fclose(fp);
		return NULL;
	}

	unsigned int lines_max = 4;
	if (strcmp(raw.channel, "#codybot") == 0)
		lines_max = 10;
	if (lines_total <= lines_max) {
		size_t size = 4095;
		char *result = (char *)malloc(size+1);
		while (1) {
			memset(result, 0, size+1);
			int ret2 = getline(&result, &size, fp);
			if (ret2 < 0) break;
			else { // Change all tab characters with a space since it
				// doesn't display right on IRC
				// Also remove the newline
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
					else if (*c == '\n')
						break;
					else {
						result[cnt++] = *c;
					}

					++c;
					if (*c == '\0')
						break;
				}
			}
			
			Msg(result);
		}
	}
	else if (lines_total >= lines_max+1) {
		system("bash -c 'cat cmd.output | nc termbin.com 9999 > cmd.url'");
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

	free(text);

	return NULL;
}


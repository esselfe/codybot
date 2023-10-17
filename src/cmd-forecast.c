#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "codybot.h"

// Array containing time at which !forecast have been run
static unsigned long forecast_usage[10];

// Pop the first item
static void ForecastDecayUsage(void) {
	int cnt;
	for (cnt = 0; cnt < 9; cnt++)
		forecast_usage[cnt] = forecast_usage[cnt+1];

	forecast_usage[cnt] = 0;
}

// Return true if permitted, false if quota reached
static int ForecastCheckUsage(void) {
	int cnt;
	for (cnt = 0; cnt < 10; cnt++) {
		// If there's available slot
		if (forecast_usage[cnt] == 0) {
			forecast_usage[cnt] = time(NULL);
			return 1;
		}
		// If usage is complete and first item dates from over 30 minutes
		else if (cnt == 9 && forecast_usage[0] < (time(NULL) - (60*30))) {
			ForecastDecayUsage();
			forecast_usage[cnt] = time(NULL);
			return 1;
		}
	}

	return 0;
}

static void *ForecastFunc(void *ptr) {
	struct raw_line *rawp = RawLineDup((struct raw_line *)ptr);
	char buf[4096];
	memset(buf, 0, 4096);

	if (!ForecastCheckUsage()) {
		Msg("Forecast quota reached, maximum 10 times every 30 minutes.");
		return NULL;
	}

	unsigned int cnt = 0, cnt_conv = 0;
	char city[128], city_conv[128], *cp = rawp->text + strlen("!forecast ");
	memset(city, 0, 128);
	memset(city_conv, 0, 128);
	while (1) {
		if (*cp == '\n' || *cp == '\0' || cp - rawp->text >= 128)
			break;
		else if (cnt == 0 && *cp == ' ') {
			++cp;
			continue;
		}
		else if (*cp == '"' || *cp == '$' || *cp == '/' || *cp == '\\') {
			++cp;
			continue;
		}
		else if (*cp == ' ') {
			city[cnt++] = ' ';
			city_conv[cnt_conv++] = '%';
			city_conv[cnt_conv++] = '2';
			city_conv[cnt_conv++] = '0';
			++cp;
			continue;
		}
		
		city[cnt] = *cp;
		city_conv[cnt_conv] = *cp;
		++cnt;
		++cnt_conv;
		++cp;
	}
	RawLineFree(rawp);
	
	// Give the city to the api-fetcher program
	FILE *fp = fopen("api-fetch", "w");
	if (fp == NULL) {
		sprintf(buffer, "codybot error: Cannot open api-fetch: %s",
			strerror(errno));
		Msg(buffer);
		return NULL;
	}
	char str2[256];
	sprintf(str2, "f %s", city_conv);
	fputs(str2, fp);
	fclose(fp);
	
	// Wait until it's ready
	fp = fopen("api-fetch", "r");
	if (fp == NULL) {
		sprintf(buffer, "codybot error: Cannot open api-fetch: %s",
			strerror(errno));
		Msg(buffer);
		return NULL;
	}
	char *str = malloc(1024);
	memset(str, 0, 1024);
	fgets(str, 1023, fp);
	fclose(fp);
	
	// Read and send results
	fp = fopen("cmd.output", "r");
	if (fp == NULL) {
		sprintf(buffer, "codybot error: Cannot open cmd.output: %s",
			strerror(errno));
		Msg(buffer);
		return NULL;
	}
	memset(str, 0, 1024);
	fgets(str, 1023, fp);
	fclose(fp);
	Msg(str);
	free(str);
	
	return NULL;
}

void Forecast(struct raw_line *rawp) {
	pthread_t thr;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thr, &attr, ForecastFunc, (void *)rawp);
	pthread_detach(thr);
	pthread_attr_destroy(&attr);
}

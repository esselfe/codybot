#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "codybot.h"

// Array containing time at which !weather have been run
unsigned long weather_usage[10];

// Pop the first item
void WeatherDecayUsage(void) {
	int cnt;
	for (cnt = 0; cnt < 9; cnt++)
		weather_usage[cnt] = weather_usage[cnt+1];

	weather_usage[cnt] = 0;
}

// Return true if permitted, false if quota reached
int WeatherCheckUsage(void) {
	int cnt;
	for (cnt = 0; cnt < 10; cnt++) {
		// If there's available slot
		if (weather_usage[cnt] == 0) {
			weather_usage[cnt] = time(NULL);
			return 1;
		}
		// If usage is complete and first item dates from over 30 minutes
		else if (cnt == 9 && weather_usage[0] < (time(NULL) - (60*30))) {
			WeatherDecayUsage();
			weather_usage[cnt] = time(NULL);
			return 1;
		}
	}

	return 0;
}

void *WeatherFunc(void *ptr) {
	struct raw_line *rawp = RawLineDup((struct raw_line *)ptr);
	char buf[4096];
	memset(buf, 0, 4096);

	if (!WeatherCheckUsage()) {
		Msg("Weather quota reached, maximum 10 times every 30 minutes.");
		return NULL;
	}

	// Check for "kill" found in ",weather `pkill${IFS}codybot`" which kills the bot
	char *c = rawp->text;
	while (1) {
		if (*c == '\0' || *c == '\n')
			break;
		if (strlen(c) >= 5 && strncmp(c, "kill", 4) == 0) {
			Msg("weather: contains a blocked term...");
			return NULL;
		}
		++c;
	}

	unsigned int cnt = 0, cnt_conv = 0;
	char city[128], city_conv[128], *cp = rawp->text + strlen("!weather ");
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
	
	APIFetch(city_conv);

	RawLineFree(rawp);
	
	return NULL;
}

void Weather(struct raw_line *rawp) {
	pthread_t thr;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thr, &attr, WeatherFunc, (void *)rawp);
	pthread_detach(thr);
	pthread_attr_destroy(&attr);
}


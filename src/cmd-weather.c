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
	memset(rawp->text, 0, strlen(rawp->text));
	
	APIFetch(city_conv);

//	char filename[1024];
//	sprintf(filename, "/tmp/codybot-weather-%s.txt", city_conv);
//	sprintf(buf, "wget -t 1 -T 24 https://wttr.in/%s?format=%%C:%%t:%%f:%%w:%%p "
//		"-O %s", city_conv, filename);
//	system(buf);

	/*temp2[strlen(temp2)-1] = ' ';
	int deg_celsius = atoi(temp2);
	int deg_farenheit = (deg_celsius * 9 / 5) + 32;
	sprintf(buffer_cmd, "%s: %s %dC/%dF", city, temp, deg_celsius, deg_farenheit);
	Msg(buffer_cmd);*/

/*	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		sprintf(buf, "codybot error: Cannot open %s: %s",
			filename, strerror(errno));
		Msg(buf);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	unsigned long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = malloc(filesize+1);
	char *str2 = malloc(filesize+128);
	memset(str2, 0, filesize+128);
	fgets(str, filesize, fp);
	cnt = 0;
	int cnt2 = 0;
	int reading_conditions = 1, reading_temp = 0, reading_feelslike = 0,
		reading_wind = 0, reading_precip = 0;
	while (1) {
		if (str[cnt] == '\0') {
			str2[cnt2] = 'm';
			str2[cnt2+1] = '\n';
			str2[cnt2+2] = '\0';
			break;
		}
		else if (str[cnt] == '+') {
			++cnt;
			continue;
		}
		else if (str[cnt] == ':') {
			if (reading_conditions) {
				reading_conditions = 0;
				reading_temp = 1;
			}
			else if (reading_temp) {
				reading_temp = 0;
				reading_feelslike = 1;

				// Partly cloudy:+28°C:+28°C:↓6km/h:0.0mm
				int isminus = 0;
				char strtemp[128];
				memset(strtemp, 0, 128);
				if (str[cnt-6] == '+' || str[cnt-6] == '-') {
					if (str[cnt-6] == '-')
						isminus = 1;
					strtemp[0] = str[cnt-5];
					strtemp[1] = str[cnt-4];
				}
				else if (str[cnt-5] == '+' || str[cnt-5] == '-') {
					if (str[cnt-5] == '-')
						isminus = 1;
					strtemp[0] = str[cnt-4];
				}
				int temp = atoi(strtemp);
				float tempF;
				if (isminus)
					tempF = (float)(-temp)*9/5+32;
				else
					tempF = (float)temp*9/5+32;
				sprintf(strtemp, "/%.1f*F ", tempF);
				strcat(str2, strtemp);
				cnt2 += strlen(strtemp);
				strcat(str2, "feels like ");
				cnt2 += 11;

				++cnt;
				continue;
			}
			else if (reading_feelslike) {
				reading_feelslike = 0;
				reading_wind = 1;

				int isminus = 0;
				char strtemp[128];
				memset(strtemp, 0, 128);
				if (str[cnt-6] == '+' || str[cnt-6] == '-') {
					if (str[cnt-6] == '-')
						isminus = 1;
					strtemp[0] = str[cnt-5];
					strtemp[1] = str[cnt-4];
				}
				else if (str[cnt-5] == '+' || str[cnt-5] == '-') {
					if (str[cnt-5] == '-')
						isminus = 1;
					strtemp[0] = str[cnt-4];
				}
				int temp = atoi(strtemp);
				float tempF;
				if (isminus)
					tempF = (float)(-temp)*9/5+32;
				else
					tempF = (float)temp*9/5+32;
				sprintf(strtemp, "/%.1f*F ", tempF);
				strcat(str2, strtemp);
				cnt2 += strlen(strtemp);

				++cnt;
				continue;
			}
			else if (reading_wind) {
				reading_wind = 0;
				reading_precip = 1;
			}
			else if (reading_precip) {
				reading_precip = 0;
			}

			str2[cnt2++] = ' ';
			++cnt;
			continue;
		}
		// The degree symbol doesn't display correctly, so replace
		else if (str[cnt] == -62 && str[cnt+1] == -80) {
			str2[cnt2] = '*';
			cnt += 2;
			++cnt2;
			continue;
		}
		else if (str[cnt] < 32 || str[cnt] > 126) {
			++cnt;
			continue;
		}

		str2[cnt2] = str[cnt];
		++cnt;
		++cnt2;
	}
	sprintf(buf, "%s: %s", city, str2);
	Msg(buf);
*/
	/*FILE *fw = fopen("tmp/data", "w");
	for (c = str; *c != '\0'; c++)
		fprintf(fw, ":%c:%d\n", *c, (int)*c);
	fclose(fw);*/

//	free(str);
//	free(str2);
	RawLineFree(rawp);
/*	
	if (!debug) {
		sprintf(buf, "rm %s", filename);
		system(buf);
		memset(buf, 0, 4096);
	}
*/
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


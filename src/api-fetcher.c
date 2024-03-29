#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "codybot.h"

#define API_REQ_TYPE_UNSET    0
#define API_REQ_TYPE_ASTRO    1
#define API_REQ_TYPE_FORECAST 2
#define API_REQ_TYPE_TIME     3
#define API_REQ_TYPE_WEATHER  4

static void APIKeyStripNewLine(char *key) {
	char *cp = key;
	while (*cp != '\0') {
		if (*cp == '\n') {
			*cp = '\0';
			break;
		}
		++cp;
	}
}

static char *APIGetKey(void) {
	FILE *fp = fopen("api.key", "r");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api.key: %s\n",
			strerror(errno));
		return NULL;
	}
	char *key = malloc(1024);
	memset(key, 0, 1024);	
	fgets(key, 1023, fp);
	fclose(fp);
	
	APIKeyStripNewLine(key);
	
	return key;
}

static char *APIAstro(char *city) {
	// Retrieve API key
	///////////////////
	char *key = APIGetKey();
	if (key == NULL)
		return NULL;
	
	// Perform curl request
	///////////////////////
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	
	char datestr[64];
	memset(datestr, 0, 64);
	time_t t1 = time(NULL);
	struct tm *tm1 = malloc(sizeof(struct tm));
	localtime_r(&t1, tm1);
	sprintf(datestr, "%d-%02d-%02d", tm1->tm_year+1900, tm1->tm_mon+1,
		tm1->tm_mday);
	free(tm1);
	
	char url[4096];
	memset(url, 0, 4096);
	char *city_conv = curl_easy_escape(handle, city, 128);
	sprintf(url, "https://api.weatherapi.com/v1/astronomy.json"
		"?key=%s&q=%s&dt=%s", key, city_conv, datestr);
	curl_free(city_conv);
	free(key);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	
	FILE *fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s\n",
			strerror(errno));
		curl_easy_cleanup(handle);
		return NULL;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
	
	CURLcode ret2 = curl_easy_perform(handle);
	if (ret2 != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret2));
		curl_easy_cleanup(handle);
		return NULL;
	}
	fclose(fp);
	curl_easy_cleanup(handle);

	// Parse the json results
	/////////////////////////
	json_object *root = json_object_from_file("cmd.output");
	if (root == NULL) {
		char *str = malloc(32);
		sprintf(str, "No results.");
		return str;
	}
	
	json_object *location = json_object_object_get(root, "location");
	if (location == NULL) {
		json_object *error = json_object_object_get(root, "error");
		if (error == NULL) {
			json_object_put(root);
			char *errstr = malloc(128);
			sprintf(errstr, "No location found in response.");
			return errstr;
		}
		else {
			json_object *message = json_object_object_get(error, "message");
			char *val = strdup(json_object_get_string(message));
			json_object_put(root);
			return val;
		}
	}
	json_object *name = json_object_object_get(location, "name");
	json_object *region = json_object_object_get(location, "region");
	json_object *country = json_object_object_get(location, "country");
	
	json_object *parent = json_object_object_get(root, "astronomy");
	json_object *astro = json_object_object_get(parent, "astro");
	json_object *sunrise = json_object_object_get(astro, "sunrise");
	json_object *sunset = json_object_object_get(astro, "sunset");
	json_object *moonrise = json_object_object_get(astro, "moonrise");
	json_object *moonset = json_object_object_get(astro, "moonset");
	json_object *moonphase = json_object_object_get(astro, "moon_phase");
	json_object *moonillum = json_object_object_get(astro, "moon_illumination");
	
	// Create final output string
	/////////////////////////////
	char *str = malloc(4096);
	memset(str, 0, 4096);
	
	if (name != NULL)
		sprintf(str, "%s, ", (char *)json_object_get_string(name));
	
	if (region != NULL) {
		strcat(str, (char *)json_object_get_string(region));
		strcat(str, ", ");
	}
	if (country != NULL) {
		strcat(str, (char *)json_object_get_string(country));
		strcat(str, ": ");
	}
	if (sunrise != NULL) {
		strcat(str, "Sunrise ");
		strcat(str, (char *)json_object_get_string(sunrise));
		strcat(str, ", ");
	}
	if (sunset != NULL) {
		strcat(str, "sunset ");
		strcat(str, (char *)json_object_get_string(sunset));
		strcat(str, ", ");
	}
	if (moonrise != NULL) {
		strcat(str, "moonrise ");
		strcat(str, (char *)json_object_get_string(moonrise));
		strcat(str, ", ");
	}
	if (moonset != NULL) {
		strcat(str, "moonset ");
		strcat(str, (char *)json_object_get_string(moonset));
		strcat(str, ", ");
	}
	if (moonphase != NULL) {
		strcat(str, "phase ");
		strcat(str, (char *)json_object_get_string(moonphase));
		strcat(str, ", ");
	}
	if (moonillum != NULL) {
		strcat(str, "illumination ");
		strcat(str, (char *)json_object_get_string(moonillum));
		strcat(str, "%");
	}
	
	json_object_put(root);
	
	return str;
}

static char *APIForecast(char *city) {
	// Retrieve API key
	///////////////////
	char *key = APIGetKey();
	if (key == NULL)
		return NULL;
	
	// Perform curl request
	///////////////////////
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	
	char url[4096];
	memset(url, 0, 4096);
	char *city_conv = curl_easy_escape(handle, city, 128);
	sprintf(url, "https://api.weatherapi.com/v1/forecast.json"
		"?key=%s&q=%s&days=3&aqi=no&alerts=no", key, city_conv);
	curl_free(city_conv);
	free(key);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	
	FILE *fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s\n",
			strerror(errno));
		curl_easy_cleanup(handle);
		return NULL;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
	
	CURLcode ret2 = curl_easy_perform(handle);
	if (ret2 != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret2));
		curl_easy_cleanup(handle);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	curl_easy_cleanup(handle);

	// Parse the json results
	/////////////////////////
	json_object *root = json_object_from_file("cmd.output");
	if (root == NULL) {
		char *str = malloc(32);
		sprintf(str, "No results.");
		return str;
	}
	
	json_object *location = json_object_object_get(root, "location");
	if (location == NULL) {
		json_object *error = json_object_object_get(root, "error");
		if (error == NULL) {
			json_object_put(root);
			char *errstr = malloc(128);
			sprintf(errstr, "No location found in response.");
			return errstr;
		}
		else {
			json_object *message = json_object_object_get(error, "message");
			char *val = strdup(json_object_get_string(message));
			json_object_put(root);
			return val;
		}
	}
	json_object *name = json_object_object_get(location, "name");
	json_object *region = json_object_object_get(location, "region");
	json_object *country = json_object_object_get(location, "country");
	
	json_object *forecast = json_object_object_get(root, "forecast");
	json_object *forecastday = json_object_object_get(forecast, "forecastday");
	
	json_object *item1 = json_object_array_get_idx(forecastday, 1);
	json_object *date1 = json_object_object_get(item1, "date");
	json_object *day1 = json_object_object_get(item1, "day");
	json_object *condition1 = json_object_object_get(day1, "condition");
	json_object *text1 = json_object_object_get(condition1, "text");
	json_object *mintemp_c1 = json_object_object_get(day1, "mintemp_c");
	json_object *mintemp_f1 = json_object_object_get(day1, "mintemp_f");
	json_object *maxtemp_c1 = json_object_object_get(day1, "maxtemp_c");
	json_object *maxtemp_f1 = json_object_object_get(day1, "maxtemp_f");
	json_object *totalprecip_mm1 = json_object_object_get(day1, "totalprecip_mm");
        json_object *totalprecip_in1 = json_object_object_get(day1, "totalprecip_in");
        json_object *totalsnow_cm1 = json_object_object_get(day1, "totalsnow_cm");
	
	json_object *item2 = json_object_array_get_idx(forecastday, 2);
	json_object *date2 = json_object_object_get(item2, "date");
	json_object *day2 = json_object_object_get(item2, "day");
	json_object *condition2 = json_object_object_get(day2, "condition");
	json_object *text2 = json_object_object_get(condition2, "text");
	json_object *mintemp_c2 = json_object_object_get(day2, "mintemp_c");
	json_object *mintemp_f2 = json_object_object_get(day2, "mintemp_f");
	json_object *maxtemp_c2 = json_object_object_get(day2, "maxtemp_c");
	json_object *maxtemp_f2 = json_object_object_get(day2, "maxtemp_f");
	json_object *totalprecip_mm2 = json_object_object_get(day2, "totalprecip_mm");
        json_object *totalprecip_in2 = json_object_object_get(day2, "totalprecip_in");
        json_object *totalsnow_cm2 = json_object_object_get(day2, "totalsnow_cm");
        
	// Create final output string
	/////////////////////////////
	char *str = malloc(4096);
	memset(str, 0, 4096);
	if (name != NULL)
		sprintf(str, "%s, ", (char *)json_object_get_string(name));
	if (region != NULL) {
		strcat(str, (char *)json_object_get_string(region));
		strcat(str, ", ");
	}
	if (country != NULL) {
		strcat(str, (char *)json_object_get_string(country));
		strcat(str, ": ");
	}
	if (date1 != NULL) {
		strcat(str, (char *)json_object_get_string(date1));
		strcat(str, ": ");
	}
	if (text1 != NULL) {
		strcat(str, (char *)json_object_get_string(text1));
		strcat(str, ", ");
	}
	if (mintemp_c1 != NULL) {
		strcat(str, "minimum ");
		strcat(str, (char *)json_object_get_string(mintemp_c1));
		strcat(str, "C/");
	}
	if (mintemp_f1 != NULL) {
		strcat(str, (char *)json_object_get_string(mintemp_f1));
		strcat(str, "F, ");
	}
	if (maxtemp_c1 != NULL) {
		strcat(str, "maximum ");
		strcat(str, (char *)json_object_get_string(maxtemp_c1));
		strcat(str, "C/");
	}
	if (maxtemp_f1 != NULL) {
		strcat(str, (char *)json_object_get_string(maxtemp_f1));
		strcat(str, "F ");
	}
	if (totalprecip_mm1 != NULL) {
		strcat(str, "total precip. ");
		strcat(str, (char *)json_object_get_string(totalprecip_mm1));
		strcat(str, "mm/");
	}
	if (totalprecip_in1 != NULL) {
		strcat(str, (char *)json_object_get_string(totalprecip_in1));
		strcat(str, "in, ");
	}
	if (totalsnow_cm1 != NULL) {
		strcat(str, "total snow ");
		strcat(str, (char *)json_object_get_string(totalsnow_cm1));
		strcat(str, "cm; ");
	}
	
	if (date2 != NULL) {
		strcat(str, (char *)json_object_get_string(date2));
		strcat(str, ": ");
	}
	if (text2 != NULL) {
		strcat(str, (char *)json_object_get_string(text2));
		strcat(str, ", ");
	}
	if (mintemp_c2 != NULL) {
		strcat(str, "minimum ");
		strcat(str, (char *)json_object_get_string(mintemp_c2));
		strcat(str, "C/");
	}
	if (mintemp_f2 != NULL) {
		strcat(str, (char *)json_object_get_string(mintemp_f2));
		strcat(str, "F, ");
	}
	if (maxtemp_c2 != NULL) {
		strcat(str, "maximum ");
		strcat(str, (char *)json_object_get_string(maxtemp_c2));
		strcat(str, "C/");
	}
	if (maxtemp_f2 != NULL) {
		strcat(str, (char *)json_object_get_string(maxtemp_f2));
		strcat(str, "F ");
	}
	if (totalprecip_mm2 != NULL) {
		strcat(str, "total precip. ");
		strcat(str, (char *)json_object_get_string(totalprecip_mm2));
		strcat(str, "mm/");
	}
	if (totalprecip_in2 != NULL) {
		strcat(str, (char *)json_object_get_string(totalprecip_in2));
		strcat(str, "in, ");
	}
	if (totalsnow_cm2 != NULL) {
		strcat(str, "total snow ");
		strcat(str, (char *)json_object_get_string(totalsnow_cm2));
		strcat(str, "cm");
	}
	
	json_object_put(root);
	
	return str;
}

static char *APITime(char *city) {
	// Retrieve API key
	///////////////////
	char *key = APIGetKey();
	if (key == NULL)
		return NULL;
	
	// Perform curl request
	///////////////////////
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	
	char url[4096];
	memset(url, 0, 4096);
	char *city_conv = curl_easy_escape(handle, city, 128);
	sprintf(url, "https://api.weatherapi.com/v1/timezone.json"
		"?key=%s&q=%s", key, city_conv);
	curl_free(city_conv);
	free(key);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	
	FILE *fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s\n",
			strerror(errno));
		curl_easy_cleanup(handle);
		return NULL;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
	
	CURLcode ret2 = curl_easy_perform(handle);
	if (ret2 != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret2));
		curl_easy_cleanup(handle);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	curl_easy_cleanup(handle);

	// Parse the json results
	/////////////////////////
	json_object *root = json_object_from_file("cmd.output");
	if (root == NULL) {
		char *str = malloc(32);
		sprintf(str, "No results.");
		return str;
	}
	
	json_object *location = json_object_object_get(root, "location");
	if (location == NULL) {
		json_object *error = json_object_object_get(root, "error");
		if (error == NULL) {
			json_object_put(root);
			char *errstr = malloc(128);
			sprintf(errstr, "No location found in response.");
			return errstr;
		}
		else {
			json_object *message = json_object_object_get(error, "message");
			char *val = strdup(json_object_get_string(message));
			json_object_put(root);
			return val;
		}
	}
	json_object *name = json_object_object_get(location, "name");
	json_object *region = json_object_object_get(location, "region");
	json_object *country = json_object_object_get(location, "country");
	json_object *tz_id = json_object_object_get(location, "tz_id");
	json_object *time_string = json_object_object_get(location, "localtime");
        
	// Create final output string
	/////////////////////////////
	char *str = malloc(4096);
	memset(str, 0, 4096);
	if (name != NULL)
		sprintf(str, "%s, ", (char *)json_object_get_string(name));
	if (region != NULL) {
		strcat(str, (char *)json_object_get_string(region));
		strcat(str, ", ");
	}
	if (country != NULL) {
		strcat(str, (char *)json_object_get_string(country));
		strcat(str, ", ");
	}
	if (tz_id != NULL) {
		strcat(str, "timezone ");
		strcat(str, (char *)json_object_get_string(tz_id));
		strcat(str, ", ");
	}
	if (time_string != NULL)
		strcat(str, (char *)json_object_get_string(time_string));
	
	json_object_put(root);
	
	return str;
}

static char *APIWeather(char *city) {
	// Retrieve API key
	///////////////////
	char *key = APIGetKey();
	if (key == NULL)
		return NULL;
	
	// Perform curl request
	///////////////////////
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	
	char url[4096];
	memset(url, 0, 4096);
	char *city_conv = curl_easy_escape(handle, city, 128);
	sprintf(url, "https://api.weatherapi.com/v1/current.json"
		"?key=%s&q=%s", key, city_conv);
	curl_free(city_conv);
	free(key);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	
	FILE *fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s\n",
			strerror(errno));
		curl_easy_cleanup(handle);
		return NULL;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
	
	CURLcode ret2 = curl_easy_perform(handle);
	if (ret2 != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret2));
		curl_easy_cleanup(handle);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	curl_easy_cleanup(handle);

	// Parse the json results
	/////////////////////////
	json_object *root = json_object_from_file("cmd.output");
	if (root == NULL) {
		char *str = malloc(32);
		sprintf(str, "No results.");
		return str;
	}
	
	json_object *location = json_object_object_get(root, "location");
	if (location == NULL) {
		json_object *error = json_object_object_get(root, "error");
		if (error == NULL) {
			json_object_put(root);
			char *errstr = malloc(128);
			sprintf(errstr, "No location found in response.");
			return errstr;
		}
		else {
			json_object *message = json_object_object_get(error, "message");
			char *val = strdup(json_object_get_string(message));
			json_object_put(root);
			return val;
		}
	}
	json_object *name = json_object_object_get(location, "name");
	json_object *region = json_object_object_get(location, "region");
	json_object *country = json_object_object_get(location, "country");
	
	json_object *current = json_object_object_get(root, "current");
	json_object *condition = json_object_object_get(current, "condition");
	json_object *text = json_object_object_get(condition, "text");
	json_object *temp_c = json_object_object_get(current, "temp_c");
	json_object *temp_f = json_object_object_get(current, "temp_f");
	json_object *feels_c = json_object_object_get(current, "feelslike_c");
	json_object *feels_f = json_object_object_get(current, "feelslike_f");
	json_object *wind_k = json_object_object_get(current, "wind_kph");
	json_object *wind_m = json_object_object_get(current, "wind_mph");
	json_object *gust_k = json_object_object_get(current, "gust_kph");
	json_object *gust_m = json_object_object_get(current, "gust_mph");
	json_object *precip = json_object_object_get(current, "precip_mm");
	
	// Create final output string
	/////////////////////////////
	char *str = malloc(4096);
	memset(str, 0, 4096);
	if (name != NULL)
		sprintf(str, "%s, ", (char *)json_object_get_string(name));
	if (region != NULL) {
		strcat(str, (char *)json_object_get_string(region));
		strcat(str, ", ");
	}
	if (country != NULL) {
		strcat(str, (char *)json_object_get_string(country));
		strcat(str, ": ");
	}
	if (text != NULL) {
		strcat(str, (char *)json_object_get_string(text));
		strcat(str, ", ");
	}
	if (temp_c != NULL) {
		strcat(str, (char *)json_object_get_string(temp_c));
		strcat(str, "C/");
	}
	if (temp_f != NULL) {
		strcat(str, (char *)json_object_get_string(temp_f));
		strcat(str, "F ");
	}
	if (feels_c != NULL) {
		strcat(str, "feels like ");
		strcat(str, (char *)json_object_get_string(feels_c));
		strcat(str, "C/");
	}
	if (feels_f != NULL) {
		strcat(str, (char *)json_object_get_string(feels_f));
		strcat(str, "F, ");
	}
	if (wind_k != NULL) {
		strcat(str, "wind ");
		strcat(str, (char *)json_object_get_string(wind_k));
		strcat(str, "kmh/");
	}
	if (wind_m != NULL) {
		strcat(str, (char *)json_object_get_string(wind_m));
		strcat(str, "mph, ");
	}
	if (gust_k != NULL) {
		strcat(str, "gust ");
		strcat(str, (char *)json_object_get_string(gust_k));
		strcat(str, "kmh/");
	}
	if (gust_m != NULL) {
		strcat(str, (char *)json_object_get_string(gust_m));
		strcat(str, "mph, ");
	}
	if (precip != NULL) {
		strcat(str, "precip. ");
		strcat(str, (char *)json_object_get_string(precip));
		strcat(str, "mm");
	}
	
	json_object_put(root);
	
	return str;
}

int main(int argc, char **argv) {
	curl_global_init(CURL_GLOBAL_ALL);

  unsigned int request_type = API_REQ_TYPE_UNSET;	
  while(1) {
	// Retrieve city from fifo
	///////////////////
	FILE *fp = fopen("api-fetch", "r");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api-fetch: %s\n",
			strerror(errno));
		continue;
	}
	char *city = malloc(1024);
	char *str = malloc(1024);
	memset(city, 0, 1024);
	memset(str, 0, 1024);
	fgets(str, 1023, fp);
	if (*str == 'a')
		request_type = API_REQ_TYPE_ASTRO;
	else if (*str == 'f')
		request_type = API_REQ_TYPE_FORECAST;
	else if (*str == 't')
		request_type = API_REQ_TYPE_TIME;
	else if (*str == 'w')
		request_type = API_REQ_TYPE_WEATHER;
	else
		request_type = API_REQ_TYPE_UNSET;
	// The first char plus space is for the type of request, so strip it here
	sprintf(city, "%s", str+2);
	free(str);
	fclose(fp);
	
	// Perform the fetching
	///////////////////////
	char *retstr = NULL;
	if (request_type == API_REQ_TYPE_ASTRO)
		retstr = APIAstro(city);
	else if (request_type == API_REQ_TYPE_FORECAST)
		retstr = APIForecast(city);
	else if (request_type == API_REQ_TYPE_TIME)
		retstr = APITime(city);
	else if (request_type == API_REQ_TYPE_WEATHER)
		retstr = APIWeather(city);
	
	free(city);
	
	if (retstr == NULL)
		continue;
	
	// Put the final output in a file
	////////////////////////////
	fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s\n",
			strerror(errno));
		free(retstr);
		continue;
	}
	fputs(retstr, fp);
	free(retstr);
	fclose(fp);
	
	// Tell codybot it's ready
	///////////////////
	fp = fopen("api-fetch", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api-fetch: %s\n",
			strerror(errno));
		continue;
	}
	fputs("done", fp);
	fclose(fp);
  } // while (1)
	curl_global_cleanup();
}


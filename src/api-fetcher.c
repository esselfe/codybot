#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "codybot.h"

#define API_REQ_TYPE_UNSET   0
#define API_REQ_TYPE_ASTRO   1
#define API_REQ_TYPE_WEATHER 2

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
	
	// Remove the newline at the end of the key string
	if (*(key+strlen(key)-1) == '\n')
		*(key+strlen(key)-1) = '\0';
	
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
	
	char datestr[64];
	memset(datestr, 0, 64);
	time_t t0 = time(NULL);
	struct tm *tm0 = localtime(&t0);
	sprintf(datestr, "%d-%02d-%02d", tm0->tm_year+1900, tm0->tm_mon+1,
		tm0->tm_mday);
	
	char url[4096];
	memset(url, 0, 4096);
	sprintf(url, "https://api.weatherapi.com/v1/astronomy.json"
		"?key=%s&q=%s&dt=%s", key, city, datestr);
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
	
	CURLcode ret = curl_easy_perform(handle);
	if (ret != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret));
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
	char *value = (char *)json_object_get_string(name);
	sprintf(str, "%s, ", value);
	if (region != NULL) {
		value = (char *)json_object_get_string(region);
		strcat(str, value);
		strcat(str, ", ");
	}
	value = (char *)json_object_get_string(country);
	strcat(str, value);
	strcat(str, ": Sunrise ");
	value = (char *)json_object_get_string(sunrise);
	strcat(str, value);
	strcat(str, ", sunset ");
	value = (char *)json_object_get_string(sunset);
	strcat(str, value);
	strcat(str, ", moonrise ");
	value = (char *)json_object_get_string(moonrise);
	strcat(str, value);
	strcat(str, ", moonset ");
	value = (char *)json_object_get_string(moonset);
	strcat(str, value);
	strcat(str, ", phase ");
	value = (char *)json_object_get_string(moonphase);
	strcat(str, value);
	strcat(str, ", illumination ");
	value = (char *)json_object_get_string(moonillum);
	strcat(str, value);
	strcat(str, "%");
	
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
	
	char url[4096];
	memset(url, 0, 4096);
	sprintf(url, "https://api.weatherapi.com/v1/current.json"
		"?key=%s&q=%s", key, city);
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
	
	CURLcode ret = curl_easy_perform(handle);
	if (ret != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret));
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
	char *value = (char *)json_object_get_string(name);
	sprintf(str, "%s, ", value);
	if (region != NULL) {
		value = (char *)json_object_get_string(region);
		strcat(str, value);
		strcat(str, ", ");
	}
	value = (char *)json_object_get_string(country);
	strcat(str, value);
	strcat(str, ": ");
	value = (char *)json_object_get_string(text);
	strcat(str, value);
	strcat(str, ", ");
	value = (char *)json_object_get_string(temp_c);
	strcat(str, value);
	strcat(str, "C/");
	value = (char *)json_object_get_string(temp_f);
	strcat(str, value);
	strcat(str, "F feels like ");
	value = (char *)json_object_get_string(feels_c);
	strcat(str, value);
	strcat(str, "C/");
	value = (char *)json_object_get_string(feels_f);
	strcat(str, value);
	strcat(str, "F, wind ");
	value = (char *)json_object_get_string(wind_k);
	strcat(str, value);
	strcat(str, "kmh/");
	value = (char *)json_object_get_string(wind_m);
	strcat(str, value);
	strcat(str, "mph, gust ");
	value = (char *)json_object_get_string(gust_k);
	strcat(str, value);
	strcat(str, "kmh/");
	value = (char *)json_object_get_string(gust_m);
	strcat(str, value);
	strcat(str, "mph, precip. ");
	value = (char *)json_object_get_string(precip);
	strcat(str, value);
	strcat(str, "mm");
	
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
  }
	curl_global_cleanup();
}


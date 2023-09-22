#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "codybot.h"

int main(int argc, char **argv) {
  while(1) {
	// Retrieve API key
	///////////////////
	FILE *fp = fopen("api.key", "r");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api.key: %s",
			strerror(errno));
		continue;
	}
	char *key = malloc(1024);
	memset(key, 0, 1024);	
	fgets(key, 1023, fp);
	fclose(fp);
	
	// Remove the newline at the end of the key string
	if (*(key+strlen(key)-1) == '\n')
		*(key+strlen(key)-1) = '\0';
	
	// Retrieve city from fifo
	///////////////////
	fp = fopen("api-fetch", "r");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api-fetch: %s",
			strerror(errno));
		continue;
	}
	char *city = malloc(1024);
	memset(city, 0, 1024);	
	fgets(city, 1023, fp);
	fclose(fp);
	
	// Perform the fetching
	///////////////////////
	CURL *handle = curl_easy_init();
	
	char url[4096];
	memset(url, 0, 4096);
	sprintf(url, "https://api.weatherapi.com/v1/current.json"
		"?key=%s&q=%s", key, city);
	curl_easy_setopt(handle, CURLOPT_URL, url);
	free(key);
	
	fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s",
			strerror(errno));
		curl_easy_cleanup(handle);
		continue;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
	
	CURLcode ret = curl_easy_perform(handle);
	if (ret != CURLE_OK) {
		printf("api-fetcher error (curl): %s\n", curl_easy_strerror(ret));
		curl_easy_cleanup(handle);
		continue;
	}
	fclose(fp);
	curl_easy_cleanup(handle);

	// Parse the json results
	/////////////////////////
	json_object *root = json_object_from_file("cmd.output");
	
	json_object *location = json_object_object_get(root, "location");
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
	value = (char *)json_object_get_string(region);
	strcat(str, value);
	strcat(str, ", ");
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
	strcat(str, "F, ");
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
	strcat(str, "mph, ");
	value = (char *)json_object_get_string(precip);
	strcat(str, value);
	strcat(str, "mm");
	
	json_object_put(root);
	
	// Put the final output in a file
	////////////////////////////
	fp = fopen("cmd.output", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open cmd.output: %s",
			strerror(errno));
		continue;
	}
	fputs(str, fp);
	fclose(fp);
	
	// Tell codybot it's ready
	///////////////////
	fp = fopen("api-fetch", "w");
	if (fp == NULL) {
		printf("api-fetcher error: Cannot open api-fetch: %s",
			strerror(errno));
		continue;
	}
	fputs("done", fp);
	fclose(fp);
  }
}


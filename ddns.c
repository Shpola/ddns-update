
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ini.h"

#define VERSION "1.0"
#define CONFIGURATION_FILE_NAME "./config.ini"
#define IP_URL_UNSWER "http://ip.42.pl/raw"

void get_current_ip(char* data);
void rewrite_ini(void);
int update_dyn_dns(const char* ip, const char* user, const char* password, const char* hostname);
void show_help();

char current_ip[17];
char status[100];

typedef struct  {
	const char *user;
	const char *pass;
	const char *domain;
    const char* ip;
    const char* status;
} configuration;

configuration config;

size_t get_http_output(void *ptr, size_t size, size_t nmemb, char *output)
{
    int write_size = 20;
    if (size*nmemb < 20)
    	write_size = size*nmemb;
    strncat(output, ptr, write_size);

    return write_size;
}

void get_current_ip(char* data)
{
    CURL *curl;
    CURLcode res;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, IP_URL_UNSWER);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_http_output);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("There was an error executing curl_easy_perform()\n");
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

int update_dyn_dns(const char *ip, const char *user, const char *password, const char *hostname)
{
    char* url;
    url = malloc(200);
    sprintf(url, "https://nic.changeip.com/nic/update?ip=%s&hostname=%s", ip, hostname);
    
    char* login;
    login = malloc(30);
    sprintf(login, "%s:%s", user, password);

    CURL *curl;
	CURLcode res;
	long status_code;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {

        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        curl_easy_setopt(curl, CURLOPT_USERPWD, login);
		curl_easy_setopt(curl, CURLOPT_URL, url);
				
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

		if (status_code == 200 && status_code != CURLE_ABORTED_BY_CALLBACK) {
			printf("\n\nSuccessfully udpated.\n");
            strcpy(status, "Successfully udpated");
	
		}
        else if (status_code == 403) {
            printf("\n\nAuthorization denied by the server\n");
            strcpy(status, "Authorization denied by the server");
        }
        else {
            printf("\n\nBad response %lu\n", status_code);
            strcpy(status, "Bad response");
        }
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	
    rewrite_ini();

    return 0;
}

static int handler(void* user, const char* section, const char* name, const char* value) {
    
    configuration* pconfig = (configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("client", "user")) {
        pconfig->user = strdup(value);
    } else if (MATCH("client", "password")) {
        pconfig->pass = strdup(value);
    } else if (MATCH("client", "domain")) {
        pconfig->domain = strdup(value);
    } else if (MATCH("client", "ip")) {
        pconfig->ip = strdup(value);
    } else if (MATCH("client", "status")) {
        pconfig->status = strdup(value);

    } else {
        return 0;  /* unknown section/name, error */
    }
	
    return 1;
}

void show_help()
{
	printf("DDNS Client. Version %s\n", VERSION);
}

void rewrite_ini(void)
{
    FILE* ini;

    if ((ini = fopen(CONFIGURATION_FILE_NAME, "w")) == NULL) {
        fprintf(stderr, "iniparser: cannot create %s\n",CONFIGURATION_FILE_NAME);
        return;
    }

    fprintf(ini,
        "\n"
        "[client]\n"
        "\n"
        "user       = %s\t;\n"
        "password   = %s\t;\n"
        "domain     = %s\t;\n"
        "ip         = %s\t;\n"
        "status     = %s\t;\n",
        config.user,
        config.pass,
        config.domain,
        current_ip,
        status);
    fclose(ini);
}

int main(int argc, char *argv[])
{	

	if (ini_parse( CONFIGURATION_FILE_NAME, handler, &config) < 0) {
		printf("Can't load configuration file\n");
	    return 1;
	}

	if (argc > 1 && strcmp(argv[1], "-h") == 0) {
		show_help();
		return 0;
	}

    get_current_ip(current_ip);

    printf("Current IP : %s\n", current_ip);
    printf("Previous IP : %s\n", config.ip);
    
    if (strcmp(current_ip, config.ip) == 0 ) {
        printf("IPs are same, No need updates\n");
    }
    else {
    	printf("IPs are different need to be updated\n");
        
        update_dyn_dns(current_ip, config.user, config.pass, config.domain);
    }

    return 0;
}

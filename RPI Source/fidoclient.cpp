#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "ASM.h"
#include "base64.h"
#include "curlwrapper.h"

const char* url_endpoint = "http://163.180.118.168:58000/fidouaf/v1/public/"; // REST API endpoint URL (It must be https(TLS))
void Registration(char* username);
void Authentication();
void Deregistration();

//Sungsu Added
int fd;
char device[]= "/dev/ttyAMA0";
unsigned long baud = 9600;

//Sungsu Added
char check(){
	struct termios options;
	//Open Seiral
	if ((fd = serialOpen(device, baud)) < 0)	/* open serial port */
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
		return -1;
	}

	// set 8 stop bit and even parity
	tcgetattr(fd, &options);
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag |= PARENB;
	tcsetattr(fd, TCSANOW, &options);
	if (wiringPiSetup() == -1)					/* initializes wiringPi setup */
	{
		fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return -1;
	}

	char newChar = serialGetchar(fd);
	fflush(stdout);

	return newChar;
}

/*
	Main Function
*/
int main() 
{
	char dat;
	char username[16];
	char flag; //Sungsu Added
	int menu;

	while (1) {
		printf("-------FIDO Client------- -\n");
		printf("1. Register\n");
		printf("2. Authentication\n");
		printf("3. Deregistration\n");
		printf("4. Reset\n");

		//Sungsu Added
		while(1){
			flag = check();		
			printf("%c", flag);
			if(flag=='@'){
				printf("\nInterrupt Occured !!\n");
				menu = int(serialGetchar(fd)) - 48;
				printf("Command Number is : ");
				printf("%d\n", menu);
				close(fd);
				break;
			}
			close(fd);
		}

		//Sungsu Added
		initialize();

		switch (menu) {
			case 1:
				printf("Enter Username:");
				scanf("%s", username);
				Registration(username);
				break;
			case 2:
				Authentication();
				break;
			case 3:
				Deregistration();
				break;
			case 4:
				InitializeSE();
				break;
		}

		//Sungsu Added
		deinitialize();
	}
	return 0;
}

/** Register Function
 * This function makes registration request to API Endpoint and Make TLV Data to send to Arduino(SE).
 * And When Arduino(SE) returns registration assertion data, This function makes response data(JSON Array) and send it to FIDO Server(REST API)
 *
 * @param username username to register
 *
 */
void Registration(char* username)
{
	char regRequest_url[500] = "";
	char* req_data;
	char* b64_assertions;
	// make URL
	strcat(regRequest_url, url_endpoint);
	strcat(regRequest_url, "regRequest/");
	strcat(regRequest_url, username);

	req_data = pcurl(regRequest_url, "GET", NULL, NULL);

	if (!req_data) {
		fprintf(stderr, "Error Getting RegRequest Data\n");
		return;
	}
	printf("\n");
	printf("==== [Register] Got Data From FIDO Server ====\n");
	printf("%s\n", req_data);
	printf("\n");


	cJSON* res_json = cJSON_Parse(req_data);
	cJSON* res_json_main = cJSON_GetArrayItem(res_json, 0);
	cJSON* json_header;
	cJSON* fcParams = cJSON_CreateObject();
	cJSON* assertions = cJSON_CreateArray();
	cJSON* assertionObject = cJSON_CreateObject();
	json_header = cJSON_GetObjectItemCaseSensitive(res_json_main, "header");

	// fcParams�� { 'appID' : {appID}, 'challenge': {challenge}, 'facetID': {facetID'}  �� ���·� ����� base64 encdode�ؼ� Response�Ҷ� ���������.
	char* appID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_header, "appID"));
	char* challenge = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(res_json_main, "challenge"));
	cJSON_AddStringToObject(fcParams, "appID", appID);
	cJSON_AddStringToObject(fcParams, "challenge", challenge);
	cJSON_AddStringToObject(fcParams, "facetID", "");

	char* c_fcParams = cJSON_PrintUnformatted(fcParams);

	// base64 ��Ŵ
	size_t len_b_fcParams = base64_encoded_size(strlen(c_fcParams));
	char* b_fcParams;
	b_fcParams = (char*)malloc(sizeof(char) * (len_b_fcParams + 1));
	url_safe_base64_encode(b_fcParams, (const char*)c_fcParams, strlen(c_fcParams));
	b_fcParams[len_b_fcParams] = '\0';
	// �������

	// resgister assertion�޾ƿ�
	b64_assertions = getRegAssertions(b_fcParams, appID, username);
	if (b64_assertions != NULL) {
		// server�� response�ϱ� ���� Json���� �������.
		cJSON_DeleteItemFromObject(res_json_main, "challenge");
		cJSON_DeleteItemFromObject(res_json_main, "username");
		cJSON_DeleteItemFromObject(res_json_main, "policy");
		cJSON_AddStringToObject(res_json_main, "fcParams", b_fcParams);
		cJSON_AddStringToObject(assertionObject, "assertionScheme", "UAFV1TLV");
		cJSON_AddStringToObject(assertionObject, "assertion", b64_assertions);
		cJSON_AddItemToArray(assertions, assertionObject);
		cJSON_AddItemToObject(res_json_main, "assertions", assertions);
		char* resp_data = cJSON_PrintUnformatted(res_json);

		printf("\n");
		printf("==== [Register] Sending To FIDO Server ====\n");
		printf("%s\n", resp_data);
		printf("\n");

		char regResponse_url[500] = "";
		char* res_data;

		// make URL
		strcat(regResponse_url, url_endpoint);
		strcat(regResponse_url, "regResponse/");
		res_data = pcurl(regResponse_url, "POST", resp_data, NULL);

		if (!res_data) {
			fprintf(stderr, "Error Getting regResponse Data\n");
			return;
		}
		printf("\n");
		printf("==== [Register] Got Response From FIDO Server ====\n");
		printf("%s\n", res_data);
		printf("\n");

		free(res_data);
	}
	free(b_fcParams);
	free(req_data);
	free(b64_assertions);
	cJSON_Delete(fcParams);
	cJSON_Delete(res_json);
}

/** Authentication Function
 * This function makes authentication request to API Endpoint and Make TLV Data to send to Arduino(SE).
 * And When Arduino(SE) returns authentication assertion data, This function makes response data(JSON Array) and send it to FIDO Server(REST API)
 *
 */
void Authentication()
{
	char regRequest_url[500] = "";
	char* req_data;
	char* b64_assertions;
	// make URL
	strcat(regRequest_url, url_endpoint);
	strcat(regRequest_url, "authRequest/");

	req_data = pcurl(regRequest_url, "GET", NULL, NULL);

	if (!req_data) {
		fprintf(stderr, "Error Getting AuthRequest Data\n");
		return;
	}

	printf("\n");
	printf("==== [Authenticate] Got Data From FIDO Server ====\n");
	printf("%s\n", req_data);
	printf("\n");


	cJSON* res_json = cJSON_Parse(req_data);
	cJSON* res_json_main = cJSON_GetArrayItem(res_json, 0);
	cJSON* json_header;
	cJSON* fcParams = cJSON_CreateObject();
	cJSON* assertions = cJSON_CreateArray();
	cJSON* assertionObject = cJSON_CreateObject();
	json_header = cJSON_GetObjectItemCaseSensitive(res_json_main, "header");

	char* appID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_header, "appID"));
	char* challenge = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(res_json_main, "challenge"));
	cJSON_AddStringToObject(fcParams, "appID", appID);
	cJSON_AddStringToObject(fcParams, "challenge", challenge);
	cJSON_AddStringToObject(fcParams, "facetID", "");

	char* c_fcParams = cJSON_PrintUnformatted(fcParams);


	size_t len_b_fcParams = base64_encoded_size(strlen(c_fcParams));
	char* b_fcParams;
	b_fcParams = (char*)malloc(sizeof(char) * (len_b_fcParams + 1));

	url_safe_base64_encode(b_fcParams, (const char*)c_fcParams, strlen(c_fcParams));
	b_fcParams[len_b_fcParams] = '\0';
	
	b64_assertions = getAuthAssertions(b_fcParams, appID);
	if (b64_assertions != NULL) {
		cJSON_DeleteItemFromObject(res_json_main, "challenge");
		cJSON_DeleteItemFromObject(res_json_main, "policy");
		cJSON_AddStringToObject(res_json_main, "fcParams", b_fcParams);
		cJSON_AddStringToObject(assertionObject, "assertionScheme", "UAFV1TLV");
		cJSON_AddStringToObject(assertionObject, "assertion", b64_assertions);
		cJSON_AddItemToArray(assertions, assertionObject);
		cJSON_AddItemToObject(res_json_main, "assertions", assertions);
		char* resp_data = cJSON_PrintUnformatted(res_json);
		printf("\n");
		printf("==== [Authenticate] Sending To FIDO Server ====\n");
		printf("%s\n", resp_data);
		printf("\n");


		char regResponse_url[500] = "";
		char* res_data;

		// make URL
		strcat(regResponse_url, url_endpoint);
		strcat(regResponse_url, "authResponse/");
		res_data = pcurl(regResponse_url, "POST", resp_data, NULL);

		if (!res_data) {
			fprintf(stderr, "Error Getting authResponse Data\n");
			return;
		}
		printf("\n");
		printf("==== [Authenticate] Got Response From FIDO Server ====\n");
		printf("%s\n", res_data);
		printf("\n");
		free(res_data);
	}
	free(b_fcParams);
	free(req_data);
	free(b64_assertions);
	cJSON_Delete(fcParams);
	cJSON_Delete(res_json);
}

/** Deregistration Function
 * This function makes authentication request to API Endpoint and Make TLV Data to send to Arduino(SE).
 * And When Arduino(SE) returns authentication assertion data, This function makes response data(JSON Array) and send it to FIDO Server(REST API)
 * When FIDO Server returns KeyID of User, It Makes TLV Data with returned KeyID to deregistration and sends it to Arduino(SE).
 * When success, This Function Makes deregistration request data(JSON Array) with KeyID and send it to FIDO Server(Rest API)
 * 
 *
 */
void Deregistration() {
	// Deregistration�� Authentication�� ���� ���� �� �� KeyID�� �޾ƿ��� Deregistration ������.
	char regRequest_url[500] = "";
	char aaid[10];
	char* req_data;
	char* b64_assertions;
	// make URL
	strcat(regRequest_url, url_endpoint);
	strcat(regRequest_url, "authRequest/");

	req_data = pcurl(regRequest_url, "GET", NULL, NULL);

	if (!req_data) {
		fprintf(stderr, "Error Getting AuthRequest Data\n");
		return;
	}
	printf("\n");
	printf("==== [Dereg-Auth] Got Data From FIDO Server ====\n");
	printf("%s\n", req_data);
	printf("\n");

	cJSON* res_json = cJSON_Parse(req_data);
	cJSON* res_json_main = cJSON_GetArrayItem(res_json, 0);
	cJSON* json_header;
	cJSON* fcParams = cJSON_CreateObject();
	cJSON* assertions = cJSON_CreateArray();
	cJSON* assertionObject = cJSON_CreateObject();

	cJSON* dereg_json_list;
	cJSON* dereg_json;
	cJSON* dereg_json_authenticators_list;
	cJSON* dereg_json_authenticators;
	cJSON* dereg_json_header;
	cJSON* dereg_json_header_upv;
	json_header = cJSON_GetObjectItemCaseSensitive(res_json_main, "header");

	char* appID = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(json_header, "appID"));
	char* challenge = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(res_json_main, "challenge"));
	cJSON_AddStringToObject(fcParams, "appID", appID);
	cJSON_AddStringToObject(fcParams, "challenge", challenge);
	cJSON_AddStringToObject(fcParams, "facetID", "");

	char* c_fcParams = cJSON_PrintUnformatted(fcParams);


	size_t len_b_fcParams = base64_encoded_size(strlen(c_fcParams));
	char* b_fcParams;
	b_fcParams = (char*)malloc(sizeof(char) * (len_b_fcParams + 1));

	url_safe_base64_encode(b_fcParams, (const char*)c_fcParams, strlen(c_fcParams));
	b_fcParams[len_b_fcParams] = '\0';

	b64_assertions = getAuthAssertions(b_fcParams, appID);
	if (b64_assertions != NULL) {
		cJSON_DeleteItemFromObject(res_json_main, "challenge");
		cJSON_DeleteItemFromObject(res_json_main, "policy");
		cJSON_AddStringToObject(res_json_main, "fcParams", b_fcParams);
		cJSON_AddStringToObject(assertionObject, "assertionScheme", "UAFV1TLV");
		cJSON_AddStringToObject(assertionObject, "assertion", b64_assertions);
		cJSON_AddItemToArray(assertions, assertionObject);
		cJSON_AddItemToObject(res_json_main, "assertions", assertions);
		char* resp_data = cJSON_PrintUnformatted(res_json);
		printf("\n");
		printf("==== [Dereg-Auth] Sending To FIDO Server ====\n");
		printf("%s\n", resp_data);
		printf("\n");


		char regResponse_url[500] = "";
		char* res_data;

		// make URL
		strcat(regResponse_url, url_endpoint);
		strcat(regResponse_url, "authResponse/");
		res_data = pcurl(regResponse_url, "POST", resp_data, NULL);

		if (!res_data) {
			fprintf(stderr, "Error Getting authResponse Data\n");
			return;
		}

		printf("\n");
		printf("==== [Dereg-Auth] Got Response From FIDO Server ====\n");
		printf("%s\n", res_data);
		printf("\n");

		cJSON* authres_json = cJSON_Parse(res_data);
		cJSON* authres_json_main = cJSON_GetArrayItem(authres_json, 0);
		char* key_id = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(authres_json_main, "KeyID"));
		size_t len_b_key_id = base64_decoded_size(strlen(key_id));
		char* b_key_id;
		b_key_id = (char*)malloc(sizeof(char) * (len_b_key_id + 1));

		base64_decode_binary((unsigned char*)b_key_id, (const char*)key_id, strlen(key_id));
		b_key_id[len_b_key_id] = '\0';
		if (getDeregisterAssertions(appID, b_key_id, strlen(b_key_id)) == 1) {
			dereg_json_list = cJSON_CreateArray();
			dereg_json = cJSON_CreateObject();
			dereg_json_authenticators_list = cJSON_CreateArray();
			dereg_json_authenticators = cJSON_CreateObject();
			dereg_json_header = cJSON_CreateObject();
			dereg_json_header_upv = cJSON_CreateObject();
			cJSON_AddStringToObject(dereg_json_authenticators, "aaid", "0013#0001");
			cJSON_AddStringToObject(dereg_json_authenticators, "keyID", key_id);
			cJSON_AddItemToArray(dereg_json_authenticators_list, dereg_json_authenticators);
			cJSON_AddItemToObject(dereg_json, "authenticators", dereg_json_authenticators_list);
			cJSON_AddStringToObject(dereg_json_header, "op", "Dereg");
			cJSON_AddStringToObject(dereg_json_header, "appID", appID);
			cJSON_AddNumberToObject(dereg_json_header_upv, "major", 1);
			cJSON_AddNumberToObject(dereg_json_header_upv, "minor", 0);
			cJSON_AddItemToObject(dereg_json_header, "upv", dereg_json_header_upv);
			cJSON_AddItemToObject(dereg_json, "header", dereg_json_header);
			cJSON_AddItemToArray(dereg_json_list, dereg_json);
			char* dereg_json = cJSON_PrintUnformatted(dereg_json_list);
			printf("\n");
			printf("==== [Dereg] Sending Data To FIDO Server ====\n");
			printf("%s\n", dereg_json);
			printf("\n");
			char deregResponse_url[500] = "";
			char* dereg_res_data;
			strcat(deregResponse_url, url_endpoint);
			strcat(deregResponse_url, "deregRequest");
			dereg_res_data = pcurl(deregResponse_url, "POST", dereg_json, NULL);
			if (!dereg_res_data) {
				fprintf(stderr, "Error Getting deregResponse Data\n");
				return;
			}
			printf("\n");
			printf("==== [Dereg] Got Response From FIDO Server ====\n");
			printf("%s\n", dereg_res_data);
			printf("\n");

			free(dereg_res_data);
			cJSON_Delete(dereg_json_list);
			cJSON_Delete(authres_json);
		}
		free(b_key_id);
		free(res_data);
	}

	
	free(b_fcParams);
	free(req_data);
	free(b64_assertions);
	cJSON_Delete(fcParams);
	cJSON_Delete(res_json);
}
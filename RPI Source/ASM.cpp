#include "ASM.h"
#include "base64.h"
#include "tags.h"
int serial_port;
/** Initialize Serial Port (/dev/serial0) and wiringPi
 *
 * @returns -1 on error or 1 when success
 *
 */
int initialize() {
	
	struct termios options;
	//Open Seiral
	if ((serial_port = serialOpen("/dev/serial0", 9600)) < 0)	/* open serial port */
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
		return -1;
	}

	// set 8 stop bit and even parity
	tcgetattr(serial_port, &options);
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag |= PARENB;
	tcsetattr(serial_port, TCSANOW, &options);
	if (wiringPiSetup() == -1)					/* initializes wiringPi setup */
	{
		fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return -1;
	}
	return 1;
}

/** Read Character from serial port. it sets timeout for 1 seconds
 *
 * @param port Serial port number
 * @returns -1 on error or 1 on success
 *
 */
int timedRead(int port)
{
	int c = -1;
	int len;
	int good;
	int _timeout = 1000;
	int _startMillis = millis();
	do {
		if ((good = serialDataAvail(port)) > 0) {
			c = serialGetchar(port);
			if (c >= 0) return c;
		}
	} while (millis() - _startMillis < _timeout);
	return -1;     // -1 indicates timeout
}

/** Read Bytes from serial port and save it on buffer
 *
 * @param port Serial port number
 * @param buffer buffer to save in
 * @param length length to read
 * @returns number of bytes read
 *
 */
int readBytes(int port, unsigned char* buffer, int length) {
	int count = 0;
	while (count < length) {
		int c = timedRead(port);
		if (c < 0) break;

		*buffer++ = (unsigned char)c;
		count++;
	}
	return count;
}

/** Read Bytes from serial port and save it on buffer
 *
 * @param port Serial port number
 * @param buffer buffer to save in
 * @param length length to read
 * @returns number of bytes read
 *
 */
int deinitialize() {
	close(serial_port);
}

/** Store Short to char array, little endian
 *
 * @param data character array
 * @param id integer to store
 * @param offset offset of array to save
 *
 */
void encodeInt(char data[], int id, int offset) {
	// ���� : id�� 0x3212�� ��� data�迭�� offset�κк��� 2����Ʈ�� 0x12, 0x32������ ����
	data[offset] = (char)(id & 0x00ff);
	data[offset + 1] = (char)((id & 0xff00) >> 8);
}

/** Insert TLV Data to array
 * -------------------
 * |       TAG       |
 * |      Length     |
 * |      Value      |
 * -------------------
 *
 * @param data Array to insert TLV Data
 * @param tag  TAG Value (short)
 * @param offset offset of array to insert
 * @param value  VALUE Data
 * @param len length of data
 * @returns next offset of data array
 *
 */
int insertTLV(char data[], int tag, size_t offset, char* value, size_t len) {
	// data�迭�� offset��ġ�� TLV������ �����͸� �ְ� ���� ������ data�迭 ��ġ�� ��ȯ
	encodeInt(data, tag, offset); // Tag
	offset += 2;
	encodeInt(data, len, offset); // Length
	offset += 2;
	memcpy(data + offset, value, len); // Value
	return len + 4;
}
/** Make Register TLV Data to send to Arduino
 *
 * @param result TLV Array to store
 * @param appID appID Data to store
 * @param fcParams fcParams data to store
 * @param username username tdata to store
 * @returns Length of byte
 *
 */
int getRegisterTLV(char result[], char* appID, char* fcParams, char* username) {
	
	char h_khaccesstoken[SHA256_DIGEST_LENGTH];
	int offset = 0;
	char authn_idx[] = { 0x00 };
	char attes_type[] = { 0x07, 0x28, 0x02, 0x00, 0x3E, 0x07 }; // TAG_ATTESTATION_TYPE�� value
	char khaccesstoken[1024] = "";
	int i;

	// ó�� 	TAG_UAFV1_REGISTER_CMD tag���� ����
	encodeInt(result, TAG_UAFV1_REGISTER_CMD, offset);

	offset += 4; // Length�� ���� �𸣹Ƿ� data�κ����� offset�̵�
	SHA256_CTX ctx;

	SHA256_Init(&ctx);

	// authenticator index�� �ִµ� 0x00����
	offset += insertTLV(result, TAG_AUTHENTICATOR_INDEX, offset, authn_idx, 1);

	// khaccesstoken�� �׳� appID, ���� SE������ username�� ������ ����
	strcat(khaccesstoken, appID);

	// �̰� sha256��
	SHA256_Update(&ctx, khaccesstoken, strlen(khaccesstoken));
	SHA256_Final((unsigned char*)h_khaccesstoken, &ctx);

	

	offset += insertTLV(result, TAG_APPID, offset, appID, strlen(appID)); // TAG_APPID 

	offset += insertTLV(result, TAG_FINAL_CHALLENGE_HASH, offset, fcParams, 32); // TAG_FINAL_CHALLENGE_HASH

	offset += insertTLV(result, TAG_USERNAME, offset, username, strlen(username)); // TAG_USERNAME

	offset += insertTLV(result, TAG_ATTESTATION_TYPE, offset, attes_type, 6); // TAG_ATTESTATION_TYPE

	offset += insertTLV(result, TAG_KEYHANDLE_ACCESS_TOKEN, offset, h_khaccesstoken, SHA256_DIGEST_LENGTH); // TAG_KEYHANDLE_ACCESS_TOKEN

	encodeInt(result, offset - 4, 2); // �� ���� �� ���� offset - 4�� �������� ���� �̹Ƿ� TAG_UAFV1_REGISTER_CMD�� length��ġ�� �� ���� �־���

	printf("\n");
	printf("==== Sending Register TLV To SE ====\n");
	printf("appID: %s\n", appID);
	printf("Final Challenge Hash: ");
	for (i = 0; i < 32; i++) {
		printf("%02x ", fcParams[i]);
	}
	printf("\n");
	printf("username: %s\n", username);
	printf("Data: ");
	for (i = 0; i < offset; i++) {
		printf("%02x", result[i]);
	}
	printf("\n");
	printf("\n\n");
	// ��ġ(����) ��ȯ
	return offset;

}

/** Make Authenticate TLV Data to send to Arduino
 *
 * @param result TLV Array to store
 * @param appID appID Data to store
 * @param fcParams fcParams data to store
 * @returns Length of byte
 *
 */
int getAuthenticateTLV(char result[], char* appID, char* fcParams) {
	char h_khaccesstoken[SHA256_DIGEST_LENGTH];
	int offset = 0;
	char authn_idx[] = { 0x00 };
	char attes_type[] = { 0x07, 0x28, 0x02, 0x00, 0x3E, 0x07 };
	char khaccesstoken[1024] = "";
	int i;
	SHA256_CTX ctx;
	SHA256_Init(&ctx);


	strcat(khaccesstoken, appID);
	SHA256_Update(&ctx, khaccesstoken, strlen(khaccesstoken));
	SHA256_Final((unsigned char*)h_khaccesstoken, &ctx);
	encodeInt(result, TAG_UAFV1_SIGN_CMD, offset);
	offset += 4;

	offset += insertTLV(result, TAG_AUTHENTICATOR_INDEX, offset, authn_idx, 1);

	offset += insertTLV(result, TAG_APPID, offset, appID, strlen(appID));

	offset += insertTLV(result, TAG_FINAL_CHALLENGE_HASH, offset, fcParams, 32);

	offset += insertTLV(result, TAG_ATTESTATION_TYPE, offset, attes_type, 6);

	offset += insertTLV(result, TAG_KEYHANDLE_ACCESS_TOKEN, offset, h_khaccesstoken, SHA256_DIGEST_LENGTH);

	encodeInt(result, offset - 4, 2);

	printf("\n");
	printf("==== Sending Authenticate TLV To SE ====\n");
	printf("appID: %s\n", appID);
	printf("Final Challenge Hash: ");
	for (i = 0; i < 32; i++) {
		printf("%02x", fcParams[i]);
	}
	printf("\n");
	printf("Data: ");
	for (i = 0; i < offset; i++) {
		printf("%02x", result[i]);
	}
	printf("\n");
	printf("\n\n");
	return offset;

}

/** Make Deregister TLV Data to send to Arduino
 *
 * @param result TLV Array to store
 * @param appID appID Data to store
 * @param keyID keyID to find user
 * @param len_keyID keyID length
 * @returns Length of byte
 *
 */
int getDeregisterTLV(char result[], char* appID, char* keyID, int len_keyID) {
	char h_khaccesstoken[SHA256_DIGEST_LENGTH];
	int offset = 0;
	char authn_idx[] = { 0x00 };
	char attes_type[] = { 0x07, 0x28, 0x02, 0x00, 0x3E, 0x07 };
	char khaccesstoken[1024] = "";
	int i;
	SHA256_CTX ctx;
	SHA256_Init(&ctx);

	strcat(khaccesstoken, appID);
	SHA256_Update(&ctx, khaccesstoken, strlen(khaccesstoken));
	SHA256_Final((unsigned char*)h_khaccesstoken, &ctx);
	encodeInt(result, TAG_UAFV1_DEREGISTER_CMD, offset);
	offset += 4;

	offset += insertTLV(result, TAG_AUTHENTICATOR_INDEX, offset, authn_idx, 1);

	offset += insertTLV(result, TAG_APPID, offset, appID, strlen(appID));

	offset += insertTLV(result, TAG_KEYID, offset, keyID, len_keyID);

	offset += insertTLV(result, TAG_KEYHANDLE_ACCESS_TOKEN, offset, h_khaccesstoken, SHA256_DIGEST_LENGTH);

	encodeInt(result, offset - 4, 2);

	printf("\n");
	printf("==== Sending Deregister TLV To SE ====\n");
	printf("appID: %s\n", appID);
	printf("KeyID: %s\n", keyID);
	printf("Data: ");
	for (i = 0; i < offset; i++) {
		printf("%02x", result[i]);
	}
	printf("\n");
	printf("\n\n");
	return offset;

}

/** Make Reset TLV Data to send to Arduino (For Debugging, Erase all data from SE, Fingerprint Sensor)
 *
 * @param result TLV Array to store
 *
 */
int getResetTLV(char result[]) {
	char h_khaccesstoken[SHA256_DIGEST_LENGTH];
	int offset = 0;
	char authn_idx[] = { 0x00 };
	encodeInt(result, 0x3401, offset);
	offset += 4;
	offset += insertTLV(result, TAG_AUTHENTICATOR_INDEX, offset, authn_idx, 1);

	encodeInt(result, offset - 4, 2);

	return offset;
}


/** Get Registeration Assertions from SE(Arduino)
 *
 * @param b_fcParams fcParams base64 data
 * @param appID appID Data
 * @param username username data
 * @returns base64 url encoded assertions
 *
 */
char* getRegAssertions(char* b_fcParams, char* appID, char* username) {

	// fcParams�� Hash�Ͽ� ������.
	char fc_hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, b_fcParams, strlen(b_fcParams));
	SHA256_Final((unsigned char*)fc_hash, &ctx);

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		printf("%02x", fc_hash[i]);
	}
	printf("\n");
	// �������

	// TLV�޾ƿ�. �ȿ� �Լ� ���ߵ�
	char registerTLV[1024];
	int tlv_length = getRegisterTLV(registerTLV, appID, fc_hash, username);

	// �ø���� ������.
	for (int i = 0; i < tlv_length; i++) {
		serialPutchar(serial_port, registerTLV[i]);
	}
	while (!serialDataAvail(serial_port));

	// Response�� �޾ƿ´�.
	unsigned char responseTLV[1024];
	int len_restlv = readBytes(serial_port, responseTLV, 1024);
	printf("\n");
	printf("==== [Register] got %d bytes from SE ====\n", len_restlv);
	for (int i = 0; i < len_restlv; i++) {
		printf("%02x", responseTLV[i]);

	}
	printf("\n");
	// ���� �߻��� ó��
	if (responseTLV[0] == 0x45 && responseTLV[1] == 0x72 && responseTLV[2] == 0x72 && responseTLV[3] == 0x6f && responseTLV[4] == 0x72) {
		responseTLV[len_restlv] = '\0';
		printf("%s\n", responseTLV);
		return NULL;
	}

	// ���� assertions��ġ�� ã�� base64 encode�Ͽ� ��ȯ��
	size_t len_assertions = base64_encoded_size(len_restlv - 10);
	char* b64_assertions;
	b64_assertions = (char*)malloc(sizeof(char) * (len_assertions + 1));
	url_safe_base64_encode(b64_assertions, (const char*)(responseTLV + 10), len_restlv - 10);
	b64_assertions[len_assertions] = '\0';

	printf("\n");
	printf("==== [Register] Base64 Encoded Assertions ====\n");
	printf("%s\n", b64_assertions);
	printf("\n");
	return b64_assertions;
}

/** Get Authentication Assertions from SE(Arduino)
 *
 * @param b_fcParams fcParams base64 data
 * @param appID appID Data
 * @returns base64 url encoded assertions
 *
 */
char* getAuthAssertions(char* b_fcParams, char* appID) {
	char fc_hash[SHA256_DIGEST_LENGTH];
	int offset = 0;
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, b_fcParams, strlen(b_fcParams));
	SHA256_Final((unsigned char*)fc_hash, &ctx);

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		printf("%02x", fc_hash[i]);
	}
	printf("\n");
	char registerTLV[1024];
	int tlv_length = getAuthenticateTLV(registerTLV, appID, fc_hash);

	for (int i = 0; i < tlv_length; i++) {
		serialPutchar(serial_port, registerTLV[i]);
	}
	while (!serialDataAvail(serial_port));
	unsigned char responseTLV[1024];
	int len_restlv = readBytes(serial_port, responseTLV, 1024);
	printf("\n");
	printf("==== [Authenticate] got %d bytes from SE ====\n", len_restlv);
	for (int i = 0; i < len_restlv; i++) {
		printf("%02x", responseTLV[i]);

	}
	printf("\n");
	if (responseTLV[0] == 0x45 && responseTLV[1] == 0x72 && responseTLV[2] == 0x72 && responseTLV[3] == 0x6f && responseTLV[4] == 0x72) {
		responseTLV[len_restlv] = '\0';
		printf("%s\n", responseTLV);
		return NULL;
	}

	size_t len_assertions = base64_encoded_size(len_restlv - 10);
	char* b64_assertions;
	b64_assertions = (char*)malloc(sizeof(char) * (len_assertions + 1));
	url_safe_base64_encode(b64_assertions, (const char*)(responseTLV + 10), len_restlv - 10);
	b64_assertions[len_assertions] = '\0';
	printf("\n");
	printf("==== [Authenticate] Base64 Encoded Assertions ====\n");
	printf("%s\n", b64_assertions);
	printf("\n");
	return b64_assertions;
}

/** Get Deregistration Assertions from SE(Arduino)
 *
 * @param b_fcParams fcParams base64 data
 * @param appID appID Data
 * @param keyID keyID Data
 * @param len_keyID length of keyID
 * @returns base64 url encoded assertions
 *
 */
int getDeregisterAssertions(char* appID, char* keyID, int len_keyID) {
	char registerTLV[1024];
	int tlv_length = getDeregisterTLV(registerTLV, appID, keyID, len_keyID);
	for (int i = 0; i < tlv_length; i++) {
		serialPutchar(serial_port, registerTLV[i]);
	}
	while (!serialDataAvail(serial_port));
	unsigned char responseTLV[1024];
	int len_restlv = readBytes(serial_port, responseTLV, 1024);
	printf("\n");
	printf("==== [Deregister] got %d bytes from SE ====\n", len_restlv);
	for (int i = 0; i < len_restlv; i++) {
		printf("%02x", responseTLV[i]);

	}
	printf("\n");
	if (responseTLV[0] == 0x45 && responseTLV[1] == 0x72 && responseTLV[2] == 0x72 && responseTLV[3] == 0x6f && responseTLV[4] == 0x72) {
		responseTLV[len_restlv] = '\0';
		printf("%s\n", responseTLV);
		return NULL;
	}

	if (responseTLV[8] == 0 && responseTLV[9] == 0) {
		return 1;
	}
	else {
		return 0;
	}
}

/** Initialize SE and fingerPrintSensor(For Debugging)
 *
 * @returns 1 when success
 *
 */
int InitializeSE() {
	char registerTLV[1024];
	int tlv_length = getResetTLV(registerTLV);

	for (int i = 0; i < tlv_length; i++) {
		printf("%02x", registerTLV[i]);
		serialPutchar(serial_port, registerTLV[i]);
	}
	printf("\n");
	while (!serialDataAvail(serial_port));
	unsigned char responseTLV[1024];
	int len_restlv = readBytes(serial_port, responseTLV, 1024);
	printf("%d\n", len_restlv);
	for (int i = 0; i < len_restlv; i++) {
		printf("%02x ", responseTLV[i]);

	}
	printf("\n");
	return 1;
}
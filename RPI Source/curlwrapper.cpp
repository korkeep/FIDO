#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>


struct ResData {
    char* data;
    size_t size;
};


void init_string(struct ResData* s) {
    s->size = 0;
    s->data = (char *) malloc(s->size + 1);
    if (s->data == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->data[0] = '\0';
}

// Curl CallBack Function
static size_t memwrite(void* contents, size_t size, size_t nmemb, struct ResData* userp) {
    size_t new_len = userp->size + size * nmemb;
    userp->data = (char *)realloc(userp->data, new_len + 1);
    if (userp->data == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(userp->data + userp->size, contents, size * nmemb);
    userp->data[new_len] = '\0';
    userp->size = new_len;

    return size * nmemb;
}

/** curl wrapper function, get and post method support
 *
 * @param url url data
 * @param method GET or POST
 * @param postfield post data field
 * @param header HTTP Headers
 * @returns response data
 *
 */
char* pcurl(const char* url, const char* method, const char* postfield, const char* header) {
    CURL* handle;
    CURLcode res_code;
    struct curl_slist* headers = NULL;
    struct ResData res;
    init_string(&res);

    handle = curl_easy_init();
    if (handle) {
        headers = curl_slist_append(headers, "Accpet: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        if (header && *header != '\0')
            headers = curl_slist_append(headers, header);

        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, memwrite);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&res);

        if (!strcmp(method, "POST")) {
            curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postfield);
            curl_easy_setopt(handle, CURLOPT_POST, 1);
        }
        res_code = curl_easy_perform(handle);
        if (res_code != CURLE_OK)
        {
            printf("%s\n", url);
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res_code));
        }
            
    }
    else {
        printf("ERROR\n");
        return NULL;
    }

    curl_easy_cleanup(handle);
    return (char*)res.data;

}
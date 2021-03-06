/*
 * @Author: CrazyIvanPro
 * @Date: 2018-12-30 08:11:41
 * @LastEditors: CrazyIvanPro
 * @LastEditTime: 2019-01-24 19:38:19
 * @Description: craweler.c
 */
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <tidy.h>
#include <tidybuffio.h>
#include "crawler.h"

// init vars
int currentIndex = 0;

// this function is used to write website contents to an output buffer
// built from bufferStruct
size_t bufferCallback(char *buffer, size_t size, size_t nmemb,
                      TidyBuffer *tidyBuffer) {
    // append response to the tidyBuffer
    size_t newSize = size * nmemb;
    tidyBufAppend(tidyBuffer, buffer, newSize);

    // return size of response
    return newSize;
};

// output data to file
int my_write(char **output) {
    for (int i = 0; i < MAX_LINKS; i++) {
        if (output[i] && (strlen(output[i]) > 1)) {
            printf("writing %d: %s\n", i, output[i]);
            open_and_write(OUTPUT_PATH, output[i]);
        }
    }
    return 0;
}

/*
 * parse - parse website content in Tidy form
 */
void parse(TidyNode node, char **output) {
    TidyNode child;

    // for each child, recursively parse all of their children
    for (child = tidyGetChild(node); child != NULL;
         child = tidyGetNext(child)) {
        // if href exists, output it
        TidyAttr hrefAttr = tidyAttrGetById(child, TidyAttr_HREF);
        if (hrefAttr) {
            if (currentIndex < MAX_LINKS) {
                if (strlen(tidyAttrValue(hrefAttr)) < MAX_URL_LEN) {
                    strcpy(output[currentIndex], tidyAttrValue(hrefAttr));
                    currentIndex++;
                }
            }
            if (tidyAttrValue(hrefAttr))
                printf("Url found: %s\n", tidyAttrValue(hrefAttr));
        }

        // recursive call for tree traversing
        parse(child, output);
    }
}

// get content of a website and store it in a buffer
int getContent(Crawler crawler) {
    // if crawler exists
    if (crawler.url) {
        // intitialize cURL vars
        CURL *handle;
        handle = curl_easy_init();
        char errBuff[CURL_ERROR_SIZE];
        int res;
        TidyDoc parseDoc;
        TidyBuffer tidyBuffer = {0};

        // if initialized correctly
        if (handle) {
            // set up cURL options
            curl_easy_setopt(handle, CURLOPT_URL, crawler.url);  // set URL
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,
                             bufferCallback);  // set output callback function
            curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuff);

            // set up Tidy Buffer and Tidy Doc
            parseDoc = tidyCreate();
            tidyBufInit(&tidyBuffer);
            tidyOptSetInt(parseDoc, TidyWrapLen, 2048);      // set max length
            tidyOptSetBool(parseDoc, TidyForceOutput, yes);  // force output

            curl_easy_setopt(handle, CURLOPT_WRITEDATA,
                             &tidyBuffer);  // identify buffer to store data in

            // execute request, return status code to res
            res = curl_easy_perform(handle);

            // check success
            if (res == CURLE_OK) {
                printf("successful crawl of %s\n", crawler.url);

                // parse webpage so it is readable by Tidy
                tidyParseBuffer(parseDoc, &tidyBuffer);

                // alloc output array
                for (int i = 0; i < MAX_LINKS; i++) {
                    crawler.parsedUrls[i] =
                        (char *)malloc(MAX_URL_LEN * sizeof(char *));
                }
                parse(tidyGetBody(parseDoc),
                      crawler.parsedUrls);  // parse results
                crawler.parsedUrls = crawler.parsedUrls;
            } else {
                printf("crawl failed for %s\n", crawler.url);
                return 0;  // failure
            }

            // clean up, close connections
            curl_easy_cleanup(handle);
            tidyBufFree(&tidyBuffer);
            tidyRelease(parseDoc);

            return 1;  // success
        }
        return 0;  // failure
    }
    return 0;  // failure
}

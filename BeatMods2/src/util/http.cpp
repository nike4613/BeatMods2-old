#include "util/http.h"
#include <iostream>

namespace BeatMods::http {
    CURLcode curl_error::code() {
        return cc;
    }

    curl_error::curl_error(std::string const& arg, CURLcode cc) : runtime_error(arg) {
        this->cc = cc;
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    response GET_response(std::string const& url) {
        CURL *curl;
        CURLcode cc;

        response res;
        res.header = "";
        res.body = "";

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &res.header);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);

            cc = curl_easy_perform(curl);
            if (cc != CURLE_OK) {
                throw curl_error(curl_easy_strerror(cc), cc);
            }

            curl_easy_cleanup(curl);
        }

        return res;
    }

    response POST_response(std::string const& url, std::string const& data) {
        CURL *curl;
        CURLcode cc;

        response res;
        res.header = "";
        res.body = "";

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &res.header);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);

            cc = curl_easy_perform(curl);
            if (cc != CURLE_OK) {
                throw curl_error(curl_easy_strerror(cc), cc);
            }

            curl_easy_cleanup(curl);
        }

        return res;
    }
}
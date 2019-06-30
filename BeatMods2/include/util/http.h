#ifndef BEATMODS2_HTTP_H
#define BEATMODS2_HTTP_H

#include <string>
#include <stdexcept>
#include <curl/curl.h>

namespace BeatMods::http {
    class curl_error : public std::runtime_error {
    private:
        CURLcode cc;
    public:
        curl_error(std::string const&, CURLcode);
        CURLcode code();
    };

    struct response {
        std::string header;
        std::string body;
    };

    response GET_response(std::string const&);
    response POST_response(std::string const&, std::string const&);
}

#endif //BEATMODS2_HTTP_H

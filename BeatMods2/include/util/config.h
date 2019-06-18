#ifndef BEATMODS2_UTIL_CONFIG
#define BEATMODS2_UTIL_CONFIG

#include <cstddef>
#include <cstdint>
#include <string>
#include "util/json.h"
#include "logging/Logging.h"
#include <iostream>
#include <restbed>

namespace BeatMods {

    class Config {
    public:
        Config();

        uint16_t port = 7000;
        std::string bind_address = "[::]";

        uint worker_limit_num = 3;
        uint worker_limit_den = 4;

        bool use_ssl = false;
        bool ssl_https_only = false;
        restbed::Uri ssl_private_key {"file:///"};
        restbed::Uri ssl_certificate {"file:///"};
        restbed::Uri ssl_diffie_hellman {"file:///"};
        // TODO: add more SSL settings

        rapidjson::Document vue_config;

        static Config Parse(rapidjson::Document const&, UnifiedLogger&);
        static Config Parse(std::string_view, UnifiedLogger&);
        static Config Parse(std::istream&, UnifiedLogger&);

        template<typename Handler>
        void Serialize(Handler& write) const
        {
            write.StartObject();

            write.String("port");
            write.Uint(port);

            write.String("bind_address");
            write.String(bind_address);

            write.String("worker_limit_num");
            write.Uint(worker_limit_num);
            write.String("worker_limit_den");
            write.Uint(worker_limit_den);

            write.String("use_ssl");
            write.Bool(use_ssl);

            write.String("ssl");
            {
                write.StartObject();

                write.String("https_only");
                write.Bool(ssl_https_only);

                write.String("private_key");
                write.String(ssl_private_key.to_string());

                write.String("certificate");
                write.String(ssl_certificate.to_string());

                write.String("diffie_hellman");
                write.String(ssl_diffie_hellman.to_string());

                write.EndObject();
            }

            write.String("vue_config");
            vue_config.Accept(write);

            write.EndObject();
        }
    };

}

#endif
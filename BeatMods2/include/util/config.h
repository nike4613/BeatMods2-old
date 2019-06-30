#ifndef BEATMODS2_UTIL_CONFIG_H
#define BEATMODS2_UTIL_CONFIG_H

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
        std::string bind_address = "127.0.0.1";

        unsigned int worker_limit_num = 3;
        unsigned int worker_limit_den = 4;

        bool use_ssl = false;
        struct SSL {
            bool https_only = false;
            restbed::Uri private_key {"file:///"};
            restbed::Uri certificate {"file:///"};
            restbed::Uri diffie_hellman {"file:///"};
        } ssl;

        struct Postgres {
            std::string connection_string = ""; // libpq connection string
        } postgres;

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
                write.Bool(ssl.https_only);

                write.String("private_key");
                write.String(ssl.private_key.to_string());

                write.String("certificate");
                write.String(ssl.certificate.to_string());

                write.String("diffie_hellman");
                write.String(ssl.diffie_hellman.to_string());

                write.EndObject();
            }

            write.String("postgres");
            {
                write.StartObject();

                write.String("connection_string");
                write.String(postgres.connection_string);

                write.EndObject();
            }

            write.String("vue_config");
            vue_config.Accept(write);

            write.EndObject();
        }
    };

}

#endif
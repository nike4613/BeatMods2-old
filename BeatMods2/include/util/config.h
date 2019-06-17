#ifndef BEATMODS2_UTIL_CONFIG
#define BEATMODS2_UTIL_CONFIG

#include <cstddef>
#include <cstdint>
#include <string>
#include "util/json.h"
#include "logging/Logging.h"
#include <iostream>

namespace BeatMods {

    class Config {
    public:
        Config();

        uint16_t port = 7000;
        std::string bind_address = "[::]";

        bool use_ssl = false; // currently unused
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

            write.String("use_ssl");
            write.Bool(use_ssl);

            write.String("vue_config");
            vue_config.Accept(write);

            write.EndObject();
        }
    };

}

#endif
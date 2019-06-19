#include "util/config.h"
#include <restbed>
#include <utility>
#include <string>

using namespace BeatMods;
using namespace rapidjson;
using namespace restbed;
using restbed::Logger;

Config::Config() : vue_config{rapidjson::Type::kObjectType} {}

Config Config::Parse(rapidjson::Document const& doc, UnifiedLogger& logger)
{
    Config cfg {};

#define TRY_READ_KEY_A(_name, _type, ...) \
    if (name == #_name) \
    { \
        if (!value.Is##_type()) \
        { \
            logger.write(Logger::Level::WARNING, "'" #_name "' not a " #_type "; ignoring"); \
            continue; \
        } \
        __VA_ARGS__; \
    }
#define EXP(...) __VA_ARGS__
#define TRY_READ_KEY(_name, _target, _type, ...) TRY_READ_KEY_A(_name, _type, cfg._target = __VA_ARGS__)
#define TRY_READ_KEY_SN(_name, _type, ...) TRY_READ_KEY(_name, _name, _type, __VA_ARGS__)
#define TRY_READ_KEY_SC(_name, _target, _type) TRY_READ_KEY(_name, _target, _type, value.Get##_type())
#define TRY_READ_KEY_SNC(_name, _type) TRY_READ_KEY_SC(_name, _name, _type)

    for (auto const& [key, value] : doc.GetObject())
    {
        auto const name = json::get_string(key);

        TRY_READ_KEY_SNC(port, Uint)
        else TRY_READ_KEY_SN(bind_address, String, json::get_string(value))
        else TRY_READ_KEY_SNC(worker_limit_num, Uint)
        else TRY_READ_KEY_SNC(worker_limit_den, Uint)
        else TRY_READ_KEY_SNC(use_ssl, Bool)
        else TRY_READ_KEY_A(ssl, Object, {
            for (auto const& [key, value] : value.GetObject())
            {
                auto const name = json::get_string(key);

                TRY_READ_KEY_SC(https_only, ssl.https_only, Bool)
                else TRY_READ_KEY_A(private_key, String, 
                    try {
                        cfg.ssl.private_key = restbed::Uri(std::string {json::get_string(value)}, true);
                    } catch (std::invalid_argument const& e) {
                        logger.log(Logger::Level::WARNING, 
                            "Field 'ssl.private_key' ('%s') not a valid URI; ignoring", 
                            json::get_string(value).data());
                        continue;
                    })
                else TRY_READ_KEY_A(certificate, String, 
                    try {
                        cfg.ssl.certificate = restbed::Uri(std::string {json::get_string(value)}, true);
                    } catch (std::invalid_argument const& e) {
                        logger.log(Logger::Level::WARNING, 
                            "Field 'ssl.certificate' ('%s') not a valid URI; ignoring", 
                            json::get_string(value).data());
                        continue;
                    })
                else TRY_READ_KEY_A(diffie_hellman, String, 
                    try {
                        cfg.ssl.diffie_hellman = restbed::Uri(std::string {json::get_string(value)}, true);
                    } catch (std::invalid_argument const& e) {
                        logger.log(Logger::Level::WARNING, 
                            "Field 'ssl.diffie_hellman' ('%s') not a valid URI; ignoring", 
                            json::get_string(value).data());
                        continue;
                    })
                else
                {
                    logger.log(Logger::Level::WARNING, "Unknown key '%s' in 'ssl' block; ignoring", name.data());
                    continue;
                }
            }})
        else TRY_READ_KEY_A(postgres, Object, {
            for (auto const& [key, value] : value.GetObject())
            {
                auto const name = json::get_string(key);

                TRY_READ_KEY(connection_string, postgres.connection_string, String, json::get_string(value))
                else
                {
                    logger.log(Logger::Level::WARNING, "Unknown key '%s' in 'postgres' block; ignoring", name.data());
                    continue;
                }
            }})
        else TRY_READ_KEY_A(vue_config, Object, cfg.vue_config.CopyFrom(value, cfg.vue_config.GetAllocator()))
        else
        {
            logger.log(Logger::Level::WARNING, "Unknown key '%s'; ignoring", name.data());
            continue;
        }
    }
    
    return cfg;
}

Config Config::Parse(std::string_view text, UnifiedLogger& logger) 
{
    char json[text.length()];
    memcpy(json, text.data(), text.length());

    Document doc;
    if (doc.ParseInsitu(json).HasParseError())
    {
        logger.write(Logger::Level::WARNING, "Could not parse JSON; initializing config to defaults");
        return {};
    }

    return Parse(doc, logger);
}

Config Config::Parse(std::istream& stream, UnifiedLogger& logger)
{
    IStreamWrapper wrap {stream};

    Document doc;
    if (doc.ParseStream(wrap).HasParseError())
    {
        logger.write(Logger::Level::WARNING, "Could not parse JSON; initializing config to defaults");
        return {};
    }

    return Parse(doc, logger);
}
#include "util/config.h"
#include <restbed>
#include <utility>

using namespace BeatMods;
using namespace rapidjson;
using namespace restbed;
using restbed::Logger;

Config::Config() : vue_config{rapidjson::Type::kObjectType} {}

Config Config::Parse(rapidjson::Document const& doc, UnifiedLogger& logger)
{
    Config cfg {};

    for (auto const& [key, value] : doc.GetObject())
    {
        if (!key.IsString()) 
        {
            logger.write(Logger::Level::WARNING, "JSON config key not string; ignoring");
            continue;
        }
        auto const name = json::get_string(key);

        if (name == "port")
        {
            if (!value.IsUint()) 
            {
                logger.write(Logger::Level::WARNING, "'port' not an unsigned integer; ignoring");
                continue;
            }
            cfg.port = static_cast<uint16_t>(value.GetUint());
        }
        else if (name == "bind_address")
        {
            if (!value.IsString()) 
            {
                logger.write(Logger::Level::WARNING, "'bind_address' not a string; ignoring");
                continue;
            }
            cfg.bind_address = json::get_string(value);
        }
        else if (name == "use_ssl")
        {
            if (!value.IsBool()) 
            {
                logger.write(Logger::Level::WARNING, "'use_ssl' not a bool; ignoring");
                continue;
            }
            cfg.use_ssl = value.GetBool();
        }
        // TODO: add the rest of the SSL config settings
        else if (name == "vue_config")
        {
            if (!value.IsObject()) 
            {
                logger.write(Logger::Level::WARNING, "'vue_config' not an object; ignoring");
                continue;
            }
            cfg.vue_config.CopyFrom(value, cfg.vue_config.GetAllocator());
        }
        else
        {
            logger.log(Logger::Level::WARNING, "Unknown key %s; ignoring", name.data());
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
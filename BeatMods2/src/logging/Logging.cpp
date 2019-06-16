#include "logging/Logging.h"
#include <iostream>
#include <cassert>

using namespace BeatMods;

std::string std::to_string(restbed::Logger::Level const& level)
{
    switch (level)
    {
    case restbed::Logger::Level::DEBUG: return "Debug";
    case restbed::Logger::Level::ERROR: return "Error";
    case restbed::Logger::Level::WARNING: return "Warning";
    case restbed::Logger::Level::FATAL: return "Fatal";
    case restbed::Logger::Level::INFO: return "Info";
    case restbed::Logger::Level::SECURITY: return "Security";
    }
    
    assert("Invalid level");
}

void UnifiedLogger::start(std::shared_ptr<restbed::Settings const> const& settings)
{
}

void UnifiedLogger::stop()
{
}

void UnifiedLogger::log(Level const level, char const* format, ...)
{
    char buf[LOGGER_BUFFER_SIZE];
    
    va_list args;
    va_start(args, format);
    auto len = vsprintf(buf, format, args);
    va_end(args);
    
    write(level, { buf, static_cast<size_t>(len) });
}

void BeatMods::UnifiedLogger::log_if(bool expression, Level const level, char const* format, ...)
{
    if (expression) {
        char buf[LOGGER_BUFFER_SIZE];

        va_list args;
        va_start(args, format);
        auto len = vsprintf(buf, format, args);
        va_end(args);

        write(level, { buf, static_cast<size_t>(len) });
    }
}

void StdoutLogger::write(Level const level, std::string_view text)
{
    std::cout << "[" << std::to_string(level) << "] " << text << std::endl;
}

std::shared_ptr<StdoutLogger> BeatMods::StdoutLogger::New()
{
    return std::make_shared<StdoutLogger>();
}

void MultiLogger::write(Level const level, std::string_view text)
{
    for (auto const& log : loggers)
        log->write(level, text);
}
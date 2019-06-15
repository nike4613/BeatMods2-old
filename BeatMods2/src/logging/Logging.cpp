#include "logging/Logging.h"
#include <iostream>

using namespace BeatMods;

std::string std::to_string(const restbed::Logger::Level& level)
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
}

void UnifiedLogger::start(const std::shared_ptr<const restbed::Settings>& settings)
{
}

void UnifiedLogger::stop()
{
}

void UnifiedLogger::log(const Level level, const char* format, ...)
{
	char buf[LOGGER_BUFFER_SIZE];

	va_list args;
	va_start(args, format);
	auto len = vsprintf(buf, format, args);
	va_end(args);

	write(level, { buf, static_cast<size_t>(len) });
}

void BeatMods::UnifiedLogger::log_if(bool expression, const Level level, const char* format, ...)
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

void StdoutLogger::write(const Level level, std::string_view text)
{
	std::cout << "[" << std::to_string(level) << "] " << text << std::endl;
}

std::shared_ptr<StdoutLogger> BeatMods::StdoutLogger::New()
{
	return std::make_shared<StdoutLogger>();
}

void MultiLogger::write(const Level level, std::string_view text)
{
	for (const auto& log : loggers)
		log->write(level, text);
}
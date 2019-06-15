#ifndef BEATMODS_2_LOGGING_H
#define BEATMODS_2_LOGGING_H

#include <restbed>
#include <string>
#include <vector>
#include <utility>

#define LOGGER_BUFFER_SIZE 2048

namespace std {
	std::string to_string(const restbed::Logger::Level& level);
}

namespace BeatMods {
	class UnifiedLogger : public restbed::Logger {
	public:
		void start(const std::shared_ptr<const restbed::Settings>& settings) override;
		void stop() override; // provides empty defaults
		void log(const Level level, const char* format, ...) override;
		void log_if(bool expression, const Level level, const char* format, ...) override;

		virtual void write(const Level level, std::string_view) = 0;
	};

	class StdoutLogger : public UnifiedLogger {
	public:
		void write(const Level level, std::string_view) override;
		static std::shared_ptr<StdoutLogger> New();
	};

	class MultiLogger : public UnifiedLogger {
		std::vector<std::shared_ptr<UnifiedLogger>> loggers;

		MultiLogger() = default;
		MultiLogger(std::initializer_list<std::shared_ptr<UnifiedLogger>>&& loggers) : loggers(loggers) {}
	public:
		void write(const Level level, std::string_view) override;

		template<typename...T> MultiLogger(const std::shared_ptr<T>&... Args) 
			: MultiLogger( { (static_cast<std::shared_ptr<UnifiedLogger>>(Args), ...) } ) {}

		template<typename...T> static std::shared_ptr<MultiLogger> New(const std::shared_ptr<T>&... Args) 
			{ return std::make_shared<MultiLogger>(Args...); }
	};
}

#endif
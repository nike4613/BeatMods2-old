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
        void start(std::shared_ptr<restbed::Settings const> const& settings) override;
        void stop() override; // provides empty defaults
        void log(Level const level, char const* format, ...) override;
        void log_if(bool expression, Level const level, char const* format, ...) override;

        virtual void write(Level const level, std::string_view) = 0;
    };

    class StdoutLogger : public UnifiedLogger {
    public:
        void write(Level const level, std::string_view) override;
        static std::shared_ptr<StdoutLogger> New();
    };

    class MultiLogger : public UnifiedLogger {
        std::vector<std::shared_ptr<UnifiedLogger>> loggers;

        MultiLogger() = default;
        constexpr MultiLogger(std::initializer_list<std::shared_ptr<UnifiedLogger>>&& loggers) : loggers(loggers) {}
    public:
        void write(Level const level, std::string_view) override;

        template<typename...T> constexpr MultiLogger(std::shared_ptr<T> const&... Args)
            : MultiLogger( { (static_cast<std::shared_ptr<UnifiedLogger>>(Args), ...) } ) {}

        template<typename...T> static std::shared_ptr<MultiLogger> New(std::shared_ptr<T> const&... Args)
            { return std::make_shared<MultiLogger>(Args...); }
    };
}

#endif
// BeatMods2.cpp : Defines the entry point for the application.
//

#include "BeatMods2.h"
#include <restbed>
#include <string>
#include <memory>
#include <cstdlib>

namespace {
	void GET_root_handler(const std::shared_ptr<restbed::Session> session)
	{
		const auto& req = session->get_request();

		const std::string body = req->get_path_parameter("path");
		session->close(restbed::OK, body, { { "Content-Length", std::to_string(body.size()) } });
	}
}

#define LOGGER_BUFFER_SIZE 2048
class StdoutLogger : public restbed::Logger {
private:
	void log(const Level level, const std::string_view& msg)
	{
		std::string lvlName;
		switch (level)
		{
		case Level::DEBUG: lvlName = "Debug"; break;
		case Level::ERROR: lvlName = "Error"; break;
		case Level::WARNING: lvlName = "Warning"; break;
		case Level::FATAL: lvlName = "Fatal"; break;
		case Level::INFO: lvlName = "Info"; break;
		case Level::SECURITY: lvlName = "Security"; break;
		}

		std::cout << "[" << lvlName << "] " << msg << std::endl;
	}

public:
	// Inherited via Logger
	virtual void stop(void) override
	{
	}
	virtual void start(const std::shared_ptr<const restbed::Settings>& settings) override
	{
	}
	virtual void log(const Level level, const char* format, ...) override
	{
		char buf[LOGGER_BUFFER_SIZE];

		va_list args;
		va_start(args, format);
		auto len = vsprintf(buf, format, args);
		va_end(args);

		log(level, { buf, static_cast<size_t>(len) });
	}
	virtual void log_if(bool expression, const Level level, const char* format, ...) override
	{
		if (expression) {
			char buf[LOGGER_BUFFER_SIZE];

			va_list args;
			va_start(args, format);
			auto len = vsprintf(buf, format, args);
			va_end(args);

			log(level, { buf, static_cast<size_t>(len) });
		}
	}
};

int main()
{
	auto resc = std::make_shared<restbed::Resource>();
	resc->set_path("{path: .*}");
	resc->set_method_handler("GET", GET_root_handler);

	auto settings = std::make_shared<restbed::Settings>();
	settings->set_port(7000);
	settings->set_default_header("Connection", "close");

	auto logger = std::make_shared<StdoutLogger>();

	restbed::Service srv;
	srv.publish(resc);
	srv.set_logger(logger);
	srv.start(settings);

	return 0;
}

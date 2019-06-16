﻿// BeatMods2.cpp : Defines the entry point for the application.
//

#include "BeatMods2.h"
#include <restbed>
#include <string>
#include <memory>
#include <thread>
#include <cstdlib>
#include <pqxx/pqxx>

#include "logging/Logging.h"

namespace {
    void GET_root_handler(const std::shared_ptr<restbed::Session> session)
    {
        const auto& req = session->get_request();

        std::stringstream ss;
        ss << req->get_path_parameter("path") << " - thread " << std::this_thread::get_id();

        const std::string body = ss.str();
        session->close(restbed::OK, body, { { "Content-Length", std::to_string(body.size()) } });
    }
}

using namespace BeatMods;

int main()
{
    auto resc = std::make_shared<restbed::Resource>();
    resc->set_path("{path: .*}");
    resc->set_method_handler("GET", GET_root_handler);

    unsigned int worker_limit = std::thread::hardware_concurrency() / 4 * 3;

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(7000);
    settings->set_worker_limit(worker_limit);
    settings->set_default_header("Connection", "close");

    auto logger1 = StdoutLogger::New();

    restbed::Service srv;
    srv.publish(resc);
    srv.set_logger(MultiLogger::New(logger1));
    srv.start(settings);

    return 0;
}

// BeatMods2.cpp : Defines the entry point for the application.
//

#include "BeatMods2.h"
#include <restbed>
#include <string>
#include <memory>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <pqxx/pqxx>
#include <semver200.h>

#include "util/json.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "logging/Logging.h"

#include "util/config.h"

using namespace BeatMods;

namespace {
    // TODO: find a good way to make this not global
    Config conf;

    void GET_config(std::shared_ptr<restbed::Session> const session)
    {
        auto const& req = session->get_request();

        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer {sb};

        conf.Serialize(writer);

        std::string const body = sb.GetString();
        session->close(restbed::OK, body, { { "Content-Length", std::to_string(body.size()) } });
    }

    void GET_root_handler(std::shared_ptr<restbed::Session> const session)
    {
        auto const& req = session->get_request();

        std::stringstream ss;
        ss << req->get_path_parameter("path") << " - thread " << std::this_thread::get_id() << std::endl;

        auto const json_str = req->get_query_parameter("json", "{}");

        rapidjson::Document doc;
        if (doc.Parse(json_str).HasParseError())
            ss << "Error parsing json query parameter" << std::endl;
        else {
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            doc.Accept(writer);
            ss << sb.GetString() << std::endl;
        }

        std::string const body = ss.str();
        session->close(restbed::OK, body, { { "Content-Length", std::to_string(body.size()) } });
    }
}

int main()
{
    auto logger = StdoutLogger::New();

    {
        std::ifstream configJson {"config.json"};
        if (configJson)
            conf = Config::Parse(configJson, *logger);
    }

    auto resc = std::make_shared<restbed::Resource>();
    resc->set_path("/{path: [^c].*}");
    resc->set_method_handler("GET", GET_root_handler);
    
    auto config = std::make_shared<restbed::Resource>();
    config->set_path("/config");
    config->set_method_handler("GET", GET_config);

    unsigned int worker_limit = std::thread::hardware_concurrency() / 4 * 3;

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(conf.port);
    settings->set_bind_address(conf.bind_address);
    //if (conf.use_ssl)
    //    settings->set_ssl_settings
    settings->set_worker_limit(worker_limit);
    settings->set_default_header("Connection", "close");

    {
        std::ofstream configJson {"config.json"};
        rapidjson::OStreamWrapper wrap {configJson};
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer {wrap};

        conf.Serialize(writer);
    }

    restbed::Service srv;
    srv.publish(config);
    srv.publish(resc);
    srv.set_logger(MultiLogger::New(logger));
    srv.start(settings);

    return 0;
}

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

#include "db/engine.h"
#include "db/string_traits.h"

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

        ss << session->get_destination() << std::endl;
        ss << req->get_path() << std::endl;

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

    try
    {
        pqxx::connection conn {conf.postgres.connection_string};

        pqxx::work transaction {conn};
        
        db::NewsItem lookupValues;
        auto author = std::make_shared<db::User>();
        lookupValues.author = author;
        author->name = "DaNike";

        auto result = db::lookup<db::User>(
            transaction, 
            {.id = true, .name = true, .profile = true, .created = true},
            "",
            { .name = true }, 
            author.get(), 
            db::PgCompareOp::Like);
        for (auto user : result) {
            std::cout << user->name << " (" << user->id << ") created " << pqxx::to_string(user->created) << std::endl << user->profile << std::endl;
        }

        auto result2 = db::lookup<db::NewsItem>(
            transaction, 
            {.id = true, .title = true, .author_resolve = true, .author_request = {.id = true, .name = true}}, 
            "",
            {.author_resolve = true, .author_request = {.name = true}},
            &lookupValues, 
            db::PgCompareOp::Like);

        for (auto news : result2) {
            auto user = db::get_resolved(news->author); // notice that creation time was not requested by this last request! the same object was used from the previous one
            std::cout << news->title << " (" << news->id << ") by " << user->name << " (" << user->id << ") created " << pqxx::to_string(user->created) << std::endl;
        }

        auto result3 = db::lookup<db::GameVersion>(
            transaction, 
            {.id = true, .version = true, .steamBuildId = true, .visibility = true});

        for (auto gv : result3) {
            std::cout << gv->version << " (" << gv->id << ") " << gv->steamBuildId << " " << std::to_string(gv->visibility) << std::endl;
        }
    }
    catch (std::exception const& e)
    {    
        std::cerr << e.what() << std::endl;
        return 1;
    }

    auto resc = std::make_shared<restbed::Resource>();
    resc->set_path("/{path: [^c].*}");
    resc->set_method_handler("GET", GET_root_handler);
    
    auto config = std::make_shared<restbed::Resource>();
    config->set_path("/config");
    config->set_method_handler("GET", GET_config);

    unsigned int worker_limit = std::thread::hardware_concurrency() / conf.worker_limit_den * conf.worker_limit_num;

    auto settings = std::make_shared<restbed::Settings>();
    settings->set_port(conf.port);
    settings->set_bind_address(conf.bind_address);
    settings->set_worker_limit(worker_limit);
    settings->set_default_header("Connection", "close");

    if (conf.use_ssl)
    {
        auto ssl_settings = std::make_shared<restbed::SSLSettings>();
        ssl_settings->set_http_disabled(!conf.ssl.https_only);
        ssl_settings->set_private_key(conf.ssl.private_key);
        ssl_settings->set_certificate(conf.ssl.certificate);
        ssl_settings->set_temporary_diffie_hellman(conf.ssl.diffie_hellman);
        
        settings->set_ssl_settings(ssl_settings);
    }

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

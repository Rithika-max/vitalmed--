#include <drogon/drogon.h>
#include "server/HttpServer.hpp"
#include "core/Logger.hpp"
#include "core/StorageManager.hpp"

static constexpr int BACKEND_PORT = 3003;

int main() {
    // Initialize storage
    vetalmed::core::StorageManager::init();
    
    // Diagnostic: report resolved storage root and metadata path
    std::cout << "Resolved storage root: " << vetalmed::core::StorageManager::getStorageRoot() << std::endl;
    std::cout << "Metadata database path: " << vetalmed::core::StorageManager::metadataPath() << std::endl;

    // Initialize logger
    vetalmed::core::Logger::init();
    vetalmed::core::Logger::info("Main", "=== Vetalmed Backend Starting ===");
    vetalmed::core::Logger::info("Main", "Storage folders initialized");
    
    try {
        vetalmed::server::HttpServer::start(BACKEND_PORT);
        vetalmed::core::Logger::info("Main", "Backend listener configured on port " + std::to_string(BACKEND_PORT));
        std::cout << "Backend listening on port " << BACKEND_PORT << std::endl;
    } catch (const std::exception &ex) {
        vetalmed::core::Logger::error("Main", std::string("Failed starting backend: ") + ex.what());
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    drogon::app().registerHandler("/.*", [](const drogon::HttpRequestPtr &req,
                                             std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS,DELETE");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        callback(resp);
    }, {drogon::Options});
    
    vetalmed::core::Logger::info("Main", "Drogon app configured");
    drogon::app().run();
    
    vetalmed::core::Logger::info("Main", "Backend shutdown");
    return 0;
}

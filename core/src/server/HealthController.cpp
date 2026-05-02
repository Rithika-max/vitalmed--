#include "server/HealthController.hpp"
#include "core/OllamaHealthChecker.hpp"
#include "core/StorageManager.hpp"
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <filesystem>

using namespace vetalmed::server;
using namespace vetalmed::core;

namespace fs = std::filesystem;

void HealthController::health(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Logger::info("HealthController", "Health check called");

    auto ollamaStatus = OllamaHealthChecker::getHealthStatus();
    bool ollamaOnline = ollamaStatus["ollama"] == "online";

    bool storageOk = fs::exists(StorageManager::uploadsPath()) &&
                     fs::exists(StorageManager::extractedPath()) &&
                     fs::exists(StorageManager::indexPath()) &&
                     fs::exists(StorageManager::logsPath());

    bool datasetsLoaded = false;
    int datasetCount = 0;
    try {
        sqlite3 *db = nullptr;
        if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) == SQLITE_OK) {
            sqlite3_stmt *stmt = nullptr;
            if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM files_metadata;", -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    datasetCount = sqlite3_column_int(stmt, 0);
                    datasetsLoaded = true;
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }
    } catch (...) {
        datasetsLoaded = false;
    }

    nlohmann::json response = {
        {"backend", "running"},
        {"status", "ok"},
        {"port", 3003},
        {"storage", storageOk ? "ok" : "error"},
        {"datasets_loaded", datasetsLoaded},
        {"dataset_count", datasetCount},
        {"ollama", ollamaOnline ? "online" : "offline"}
    };

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

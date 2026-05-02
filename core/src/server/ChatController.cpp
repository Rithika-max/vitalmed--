#include "server/ChatController.hpp"
#include "core/Validator.hpp"
#include "core/MemoryHook.hpp"
#include "core/IntentParser.hpp"
#include "core/DatasetSelector.hpp"
#include "core/DecisionRouter.hpp"
#include "core/ResponseComposer.hpp"
#include "core/Logger.hpp"
#include "core/OllamaHealthChecker.hpp"
#include "core/FallbackResponseComposer.hpp"
#include "core/StorageManager.hpp"
#include "engines/ApiEngine.hpp"
#include "engines/StructuredEngine.hpp"
#include "engines/RagEngine.hpp"
#include "engines/HybridEngine.hpp"
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace vetalmed::server;
using namespace vetalmed::core;
using namespace vetalmed::engines;

static MemoryHook memoryHook;

void ChatController::chat(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    nlohmann::json body;
    try {
        body = nlohmann::json::parse(req->body());
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse({{"summary", ""}, {"data", nlohmann::json::array()}, {"insights", nlohmann::json::array()}, {"recommendation", ""}, {"confidence", 0.0}, {"source_type", "insufficient_data"}});
        resp->setStatusCode(drogon::k400BadRequest);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    std::string query = body.value("query", "");
    std::string sessionId = body.value("session_id", "default");
    std::string error;
    
    Logger::info("ChatController", "Query received: " + query);
    
    if (!Validator::validateQuery(query, error)) {
        Logger::warn("ChatController", "Invalid query: " + error);
        auto resp = drogon::HttpResponse::newHttpJsonResponse({{"summary", ""}, {"data", nlohmann::json::array()}, {"insights", nlohmann::json::array()}, {"recommendation", error}, {"confidence", 0.0}, {"source_type", "insufficient_data"}});
        resp->setStatusCode(drogon::k400BadRequest);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    memoryHook.addMemory(sessionId, query);
    const auto intent = IntentParser::parseIntent(query);
    const auto selectedDataset = DatasetSelector::selectBestDataset(query);
    
    if (selectedDataset == "insufficient_data") {
        Logger::info("ChatController", "No relevant dataset found");
        
        // Check if Ollama is available for enhancement
        bool ollamaOnline = OllamaHealthChecker::isOllamaOnline();
        
        auto resp = drogon::HttpResponse::newHttpJsonResponse({
            {"summary", "No relevant data found for this question."},
            {"data", nlohmann::json::array()},
            {"insights", nlohmann::json::array()},
            {"recommendation", "Please upload relevant data or ask a question based on available data."},
            {"confidence", 0.1},
            {"source_type", "insufficient_data"},
            {"ollama_status", ollamaOnline ? "online" : "offline"}
        });
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    const bool hasStructured = selectedDataset == "structured" || selectedDataset == "hybrid";
    const bool hasUnstructured = selectedDataset == "rag" || selectedDataset == "hybrid";
    const bool hasApiData = selectedDataset == "api";
    auto route = DecisionRouter::route(intent, hasStructured, hasUnstructured, hasApiData);
    if (route == IntentType::UNKNOWN) {
        if (selectedDataset == "structured") {
            route = IntentType::STRUCTURED;
        } else if (selectedDataset == "rag") {
            route = IntentType::RAG;
        } else if (selectedDataset == "api") {
            route = IntentType::API;
        } else if (selectedDataset == "hybrid") {
            route = IntentType::HYBRID;
        }
    }

    nlohmann::json result;
    std::string sourceType = "insufficient_data";
    switch (route) {
        case IntentType::API:
            result = ApiEngine::execute(query);
            sourceType = "api";
            break;
        case IntentType::STRUCTURED:
            result = StructuredEngine::execute(query);
            sourceType = "structured";
            break;
        case IntentType::RAG:
            result = RagEngine::execute(query);
            sourceType = "rag";
            break;
        case IntentType::HYBRID: {
            result = HybridEngine::execute(query);
            sourceType = "hybrid";
            break;
        }
        default:
            result = nlohmann::json::object();
            sourceType = "insufficient_data";
            break;
    }

    if (result.empty()) {
        Logger::info("ChatController", "No result from engine");
        auto resp = drogon::HttpResponse::newHttpJsonResponse({
            {"summary", "No relevant data found for this question."},
            {"data", nlohmann::json::array()},
            {"insights", nlohmann::json::array()},
            {"recommendation", "Please upload relevant data or ask a question based on available data."},
            {"confidence", 0.1},
            {"source_type", "insufficient_data"}
        });
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    // Grounding check: Ensure the response is based on actual data
    bool isGrounded = false;
    if (result.contains("data") && !result["data"].empty()) {
        isGrounded = true;
    }

    // Additional validation: Check if summary mentions actual data sources
    if (result.contains("summary") && !result["summary"].empty()) {
        std::string summary = result["summary"];
        // Summary should reference actual files or data sources, not be generic
        if (summary.find("Found") != std::string::npos ||
            summary.find("uploaded") != std::string::npos ||
            summary.find(".pdf") != std::string::npos ||
            summary.find(".txt") != std::string::npos ||
            summary.find(".csv") != std::string::npos ||
            summary.find(".json") != std::string::npos) {
            isGrounded = true;
        }
    }

    if (!isGrounded) {
        Logger::warn("ChatController", "Response not grounded in data");
        auto resp = drogon::HttpResponse::newHttpJsonResponse({
            {"summary", "No relevant data found for this question."},
            {"data", nlohmann::json::array()},
            {"insights", nlohmann::json::array()},
            {"recommendation", "Please upload relevant data or ask a question based on available data."},
            {"confidence", 0.1},
            {"source_type", "insufficient_data"}
        });
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    Logger::info("ChatController", "Response generated successfully");
    auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

void ChatController::datasets(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    nlohmann::json response = nlohmann::json::array();
    try {
        sqlite3 *db;
        if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) == SQLITE_OK) {
            const char *sql = "SELECT id, filename, file_type, file_path, extracted_path, extracted_size, processed_status, uploaded_at FROM files_metadata ORDER BY uploaded_at DESC";
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    nlohmann::json item;
                    item["id"] = sqlite3_column_int(stmt, 0);
                    item["file_name"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                    item["file_type"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                    item["original_path"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
                    item["extracted_path"] = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
                    item["extracted_size"] = sqlite3_column_int64(stmt, 5);
                    
                    const char *status = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
                    item["processed_status"] = status ? status : "processed";
                    item["status"] = status ? status : "processed";
                    
                    const char *uploadedAt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
                    item["uploaded_at"] = uploadedAt ? uploadedAt : "";
                    
                    response.push_back(item);
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }
    } catch (...) {
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
}

void ChatController::deleteDataset(const drogon::HttpRequestPtr &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                   int id) {
    try {
        sqlite3 *db;
        if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) == SQLITE_OK) {
            // Get file paths before deletion
            sqlite3_stmt *selectStmt;
            const char *selectSql = "SELECT file_path, extracted_path FROM files_metadata WHERE id = ?";
            std::string filePath, extractedPath;
            
            if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(selectStmt, 1, id);
                if (sqlite3_step(selectStmt) == SQLITE_ROW) {
                    const char *path1 = reinterpret_cast<const char *>(sqlite3_column_text(selectStmt, 0));
                    const char *path2 = reinterpret_cast<const char *>(sqlite3_column_text(selectStmt, 1));
                    filePath = path1 ? path1 : "";
                    extractedPath = path2 ? path2 : "";
                }
                sqlite3_finalize(selectStmt);
            }
            
            // Delete from database
            sqlite3_stmt *deleteStmt;
            const char *deleteSql = "DELETE FROM files_metadata WHERE id = ?";
            if (sqlite3_prepare_v2(db, deleteSql, -1, &deleteStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(deleteStmt, 1, id);
                if (sqlite3_step(deleteStmt) != SQLITE_DONE) {
                    sqlite3_finalize(deleteStmt);
                    sqlite3_close(db);
                    auto resp = drogon::HttpResponse::newHttpJsonResponse({{"success", false}, {"message", "Failed to delete from database"}});
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    callback(resp);
                    return;
                }
                sqlite3_finalize(deleteStmt);
            }
            
            sqlite3_close(db);
            
            // Delete files from storage
            if (!filePath.empty() && std::filesystem::exists(filePath)) {
                std::filesystem::remove(filePath);
            }
            if (!extractedPath.empty() && std::filesystem::exists(extractedPath)) {
                std::filesystem::remove(extractedPath);
            }
            
            auto resp = drogon::HttpResponse::newHttpJsonResponse({{"success", true}, {"message", "Dataset deleted successfully"}});
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        } else {
            auto resp = drogon::HttpResponse::newHttpJsonResponse({{"success", false}, {"message", "Database error"}});
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        }
    } catch (const std::exception &ex) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse({{"success", false}, {"message", std::string("Error: ") + ex.what()}});
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    }
}

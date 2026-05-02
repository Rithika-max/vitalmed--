#include "server/UploadController.hpp"
#include "upload/FileReceiver.hpp"
#include "upload/FileValidator.hpp"
#include "upload/FileTypeDetector.hpp"
#include "upload/ContentExtractor.hpp"
#include "upload/StructureDetector.hpp"
#include "upload/SchemaInferenceEngine.hpp"
#include "upload/DataQualityChecker.hpp"
#include "upload/AutoPipelineRouter.hpp"
#include "upload/StorageRouter.hpp"
#include <json/json.h>
#include "core/Logger.hpp"
#include <nlohmann/json.hpp>

using namespace vetalmed::server;
using namespace vetalmed::upload;
using namespace vetalmed::core;

void UploadController::upload(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    try {
        std::string filename;
        std::string error;
        const std::string path = FileReceiver::receive(req, filename, error);
    
    if (path.empty()) {
        Logger::warn("UploadController", "File receive failed: " + error);
        Json::Value errorResp(Json::objectValue);
        errorResp["success"] = false;
        errorResp["message"] = error;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResp);
        resp->setStatusCode(drogon::k400BadRequest);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }
    
    Logger::info("UploadController", "File received: " + filename + " at " + path);

    if (!FileValidator::validate(filename, path, error)) {
        Logger::warn("UploadController", "File validation failed: " + error);
        Json::Value errorResp(Json::objectValue);
        errorResp["success"] = false;
        errorResp["message"] = error;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResp);
        resp->setStatusCode(drogon::k400BadRequest);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    Logger::info("UploadController", "File validation passed");
    
    const std::string fileType = FileTypeDetector::detectType(filename, path);
    Logger::info("UploadController", "Detected file type: " + fileType);
    
    const std::string content = ContentExtractor::extract(fileType, path);
    Logger::info("UploadController", "Content extracted, size: " + std::to_string(content.size()) + " bytes");
    if (content.empty()) {
        error = "Failed to extract file content from uploaded file.";
        Logger::error("UploadController", error);
        Json::Value errorResp(Json::objectValue);
        errorResp["success"] = false;
        errorResp["message"] = error;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResp);
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }
    
    const bool structured = StructureDetector::isStructured(fileType, content);
    Logger::info("UploadController", "Structure detected: " + std::string(structured ? "structured" : "unstructured"));
    
    const std::string schema = SchemaInferenceEngine::infer(fileType, content);
    Logger::info("UploadController", "Schema inferred: " + schema);
    
    std::string report;
    if (!DataQualityChecker::check(content, report)) {
        report = "Data quality warning: " + report;
        Logger::warn("UploadController", report);
    }

    int metadataId = -1;
    if (!StorageRouter::persist(filename, fileType, content, schema, metadataId, error)) {
        Logger::error("UploadController", "Failed to persist metadata: " + error);
        Json::Value errorResp(Json::objectValue);
        errorResp["success"] = false;
        errorResp["message"] = error;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResp);
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
        return;
    }

    Logger::info("UploadController", "File metadata saved with id: " + std::to_string(metadataId));

    const std::string pipeline = (structured ? "structured" : "unstructured");
    Json::Value payload(Json::objectValue);
    payload["success"] = true;
    payload["message"] = "File uploaded and processed successfully.";
    payload["filename"] = filename;
    payload["file_type"] = fileType;
    payload["status"] = "processed";
    payload["pipeline"] = pipeline;
    payload["metadata_id"] = metadataId;
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(payload);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    callback(resp);
    } catch (...) {
        Logger::error("UploadController", "Unknown exception in upload");
        Json::Value errorResp(Json::objectValue);
        errorResp["success"] = false;
        errorResp["message"] = "Internal server error during upload";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResp);
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    }
}

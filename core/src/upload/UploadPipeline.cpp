#include "upload/FileReceiver.hpp"
#include "upload/FileValidator.hpp"
#include "upload/FileTypeDetector.hpp"
#include "upload/ContentExtractor.hpp"
#include "upload/StructureDetector.hpp"
#include "upload/SchemaInferenceEngine.hpp"
#include "upload/DataQualityChecker.hpp"
#include "upload/AutoPipelineRouter.hpp"
#include "upload/StorageRouter.hpp"
#include "core/Logger.hpp"
#include <drogon/MultiPart.h>
#define NOMINMAX
#include <drogon/HttpAppFramework.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <sqlite3.h>
#include <algorithm>

namespace fs = std::filesystem;
using vetalmed::core::StorageManager;

namespace vetalmed::upload {

std::string FileReceiver::receive(const drogon::HttpRequestPtr &req, std::string &filename, std::string &error) {
    Logger::info("FileReceiver", "Starting file receive");
    drogon::MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0) {
        error = "Failed to parse multipart request.";
        Logger::error("FileReceiver", error);
        return {};
    }
    if (fileUpload.getFiles().size() != 1) {
        error = "Expected exactly one file, got " + std::to_string(fileUpload.getFiles().size());
        Logger::error("FileReceiver", error);
        return {};
    }
    const auto &file = fileUpload.getFiles()[0];
    filename = file.getFileName();
    Logger::info("FileReceiver", "Received file: " + filename);
    const auto uploadsDir = StorageManager::uploadsPath();
    Logger::info("FileReceiver", "Uploads dir: " + uploadsDir.string());
    fs::create_directories(uploadsDir);
    const auto destination = uploadsDir / filename;
    Logger::info("FileReceiver", "Destination: " + destination.string());
    if (file.saveAs(destination.string()) != 0) {
        error = "File save failed.";
        Logger::error("FileReceiver", error);
        return {};
    }
    Logger::info("FileReceiver", "File saved successfully");
    return destination.string();
}

bool FileValidator::validate(const std::string &filename, const std::string &path, std::string &error) {
    if (filename.empty()) {
        error = "Filename missing.";
        return false;
    }
    if (!fs::exists(path)) {
        error = "Stored file not found.";
        return false;
    }
    const auto size = fs::file_size(path);
    if (size == 0) {
        error = "Uploaded file is empty.";
        return false;
    }
    return true;
}

std::string FileTypeDetector::detectType(const std::string &filename, const std::string &path) {
    auto lower = filename;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto endsWith = [&](const std::string &suffix) {
        return lower.size() >= suffix.size() && lower.compare(lower.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (endsWith(".csv")) return "csv";
    if (endsWith(".json")) return "json";
    if (endsWith(".txt")) return "txt";
    if (endsWith(".pdf")) return "pdf";
    if (endsWith(".docx")) return "docx";
    if (endsWith(".xls") || endsWith(".xlsx")) return "excel";
    return "unknown";
}

static std::string readFile(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool columnExists(sqlite3 *db, const std::string &table, const std::string &column) {
    sqlite3_stmt *stmt = nullptr;
    const std::string sql = "PRAGMA table_info('" + table + "');";
    bool exists = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            if (name && column == name) {
                exists = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }
    return exists;
}

std::string ContentExtractor::extract(const std::string &fileType, const std::string &path) {
    std::string rawContent = readFile(path);
    if (rawContent.empty()) {
        return "";
    }

    if (fileType == "csv") {
        return extractCsvContent(rawContent);
    }
    if (fileType == "json") {
        return extractJsonContent(rawContent);
    }
    if (fileType == "excel") {
        return "[excel extraction placeholder - " + std::to_string(rawContent.size()) + " bytes]";
    }
    if (fileType == "pdf") {
        return "[pdf extraction placeholder - " + std::to_string(rawContent.size()) + " bytes]";
    }
    if (fileType == "docx") {
        return "[docx extraction placeholder - " + std::to_string(rawContent.size()) + " bytes]";
    }
    // For txt and others, return as is
    return rawContent;
}

std::string ContentExtractor::extractCsvContent(const std::string &content) {
    std::istringstream ss(content);
    std::string line;
    std::vector<std::vector<std::string>> rows;
    while (std::getline(ss, line)) {
        std::vector<std::string> row;
        std::istringstream lineStream(line);
        std::string cell;
        while (std::getline(lineStream, cell, ',')) {
            // Basic CSV parsing, remove quotes if present
            if (!cell.empty() && cell.front() == '"' && cell.back() == '"') {
                cell = cell.substr(1, cell.size() - 2);
            }
            row.push_back(cell);
        }
        if (!row.empty()) {
            rows.push_back(row);
        }
    }

    if (rows.empty()) {
        return "Empty CSV file";
    }

    std::ostringstream result;
    result << "CSV Data:\n";
    result << "Columns: " << rows[0].size() << "\n";
    result << "Rows: " << rows.size() << "\n";
    result << "Schema: ";
    for (size_t i = 0; i < rows[0].size(); ++i) {
        if (i > 0) result << ", ";
        result << "Column" << (i + 1) << ": " << rows[0][i];
    }
    result << "\n\nSample Data:\n";
    size_t sampleRows = (rows.size() > 5) ? 5 : rows.size();
    for (size_t i = 0; i < sampleRows; ++i) {
        result << "Row " << (i + 1) << ": ";
        for (size_t j = 0; j < rows[i].size(); ++j) {
            if (j > 0) result << " | ";
            result << rows[i][j];
        }
        result << "\n";
    }
    return result.str();
}

std::string ContentExtractor::extractJsonContent(const std::string &content) {
    try {
        auto json = nlohmann::json::parse(content);
        std::ostringstream result;
        result << "JSON Data:\n";
        result << "Type: " << json.type_name() << "\n";
        if (json.is_object()) {
            result << "Keys: ";
            for (auto it = json.begin(); it != json.end(); ++it) {
                if (it != json.begin()) result << ", ";
                result << it.key();
            }
            result << "\n";
        }
        if (json.is_array()) {
            result << "Array length: " << json.size() << "\n";
        }
        result << "\nContent:\n" << json.dump(2);
        return result.str();
    } catch (...) {
        return "Invalid JSON content";
    }
}

bool StructureDetector::isStructured(const std::string &fileType, const std::string &content) {
    if (fileType == "csv" || fileType == "json" || fileType == "excel") {
        return true;
    }
    if (fileType == "txt" && content.find(',') != std::string::npos) {
        return true;
    }
    return false;
}

std::string SchemaInferenceEngine::infer(const std::string &fileType, const std::string &content) {
    if (fileType == "json") {
        return "json_document";
    }
    if (fileType == "csv") {
        std::istringstream ss(content);
        std::string header;
        if (std::getline(ss, header)) {
            return header;
        }
    }
    if (fileType == "txt") {
        return "unstructured_text";
    }
    return "unknown_schema";
}

bool DataQualityChecker::check(const std::string &content, std::string &report) {
    if (content.empty()) {
        report = "File content could not be extracted.";
        return false;
    }
    if (content.size() < 10) {
        report = "Extracted content is unusually short.";
        return false;
    }
    return true;
}

PipelineType AutoPipelineRouter::route(bool hasStructured, bool hasUnstructured) {
    if (hasStructured && hasUnstructured) {
        return PipelineType::HYBRID;
    }
    if (hasStructured) {
        return PipelineType::STRUCTURED;
    }
    return PipelineType::UNSTRUCTURED;
}

bool StorageRouter::persist(const std::string &filename,
                            const std::string &fileType,
                            const std::string &content,
                            const std::string &schema,
                            int &metadataId,
                            std::string &error) {
    fs::create_directories(StorageManager::uploadsPath());
    fs::create_directories(StorageManager::extractedPath());
    sqlite3 *db = nullptr;
    if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) != SQLITE_OK) {
        error = "Unable to open metadata database.";
        return false;
    }

    const char *createSql = R"(
        CREATE TABLE IF NOT EXISTS files_metadata (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT,
            file_type TEXT,
            file_path TEXT,
            extracted_path TEXT,
            uploaded_at TEXT,
            processed_status TEXT,
            schema TEXT,
            dataset_type TEXT,
            source TEXT,
            size_bytes INTEGER,
            extracted_size INTEGER
        );
    )";

    if (sqlite3_exec(db, createSql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        error = "Failed to create metadata table.";
        sqlite3_close(db);
        return false;
    }

    // Add missing columns
    if (!columnExists(db, "files_metadata", "file_path")) {
        sqlite3_exec(db, "ALTER TABLE files_metadata ADD COLUMN file_path TEXT;", nullptr, nullptr, nullptr);
    }
    if (!columnExists(db, "files_metadata", "extracted_path")) {
        sqlite3_exec(db, "ALTER TABLE files_metadata ADD COLUMN extracted_path TEXT;", nullptr, nullptr, nullptr);
    }
    if (!columnExists(db, "files_metadata", "processed_status")) {
        sqlite3_exec(db, "ALTER TABLE files_metadata ADD COLUMN processed_status TEXT;", nullptr, nullptr, nullptr);
    }
    if (!columnExists(db, "files_metadata", "dataset_type")) {
        sqlite3_exec(db, "ALTER TABLE files_metadata ADD COLUMN dataset_type TEXT;", nullptr, nullptr, nullptr);
    }
    if (!columnExists(db, "files_metadata", "extracted_size")) {
        sqlite3_exec(db, "ALTER TABLE files_metadata ADD COLUMN extracted_size INTEGER;", nullptr, nullptr, nullptr);
    }
    // Remove old extracted_content column if exists
    if (columnExists(db, "files_metadata", "extracted_content")) {
        // Note: SQLite doesn't support DROP COLUMN easily, but we can ignore it for now
    }

    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    ts << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S UTC");
    const std::string filePath = StorageManager::filePath(filename).string();
    const long long sizeBytes = std::filesystem::file_size(filePath);
    const std::string datasetType = (fileType == "csv" || fileType == "json" || fileType == "excel") ? "structured" : "unstructured";

    // Save extracted content to file
    std::string extractedPath;
    long long extractedSize = 0;
    if (content.empty()) {
        error = "Extracted content is empty.";
        sqlite3_close(db);
        return false;
    }
    if (!content.empty()) {
        extractedPath = StorageManager::extractedFilePath(filename).string();
        std::ofstream outFile(extractedPath);
        if (outFile.is_open()) {
            outFile << content;
            outFile.close();
            extractedSize = std::filesystem::file_size(extractedPath);
        } else {
            error = "Failed to save extracted content.";
            sqlite3_close(db);
            return false;
        }
    }

    const std::string processedStatus = extractedPath.empty() ? "failed" : "processed";

    sqlite3_stmt *stmt;
    const char *insertSql = "INSERT INTO files_metadata(filename,file_type,file_path,extracted_path,uploaded_at,processed_status,schema,dataset_type,source,size_bytes,extracted_size) VALUES(?,?,?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
        error = "Failed to prepare metadata insert.";
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, fileType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, extractedPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, ts.str().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, processedStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, schema.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, datasetType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, "upload", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 10, sizeBytes);
    sqlite3_bind_int64(stmt, 11, extractedSize);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        error = "Failed to save file metadata.";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    metadataId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

}

#include "core/DatasetSelector.hpp"
#include "core/StorageManager.hpp"
#include <sqlite3.h>
#include <filesystem>
#include <fstream>
#include <sstream>

using vetalmed::core::StorageManager;
#include <algorithm>

namespace {

std::string lower(const std::string &text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool matchesQuery(const std::string &text, const std::string &query) {
    if (text.empty() || query.empty()) {
        return false;
    }
    return lower(text).find(lower(query)) != std::string::npos;
}

bool fileTypeIsStructured(const std::string &fileType) {
    const std::string normalized = lower(fileType);
    return normalized == "csv" || normalized == "json" || normalized == "xls" || normalized == "xlsx";
}

bool fileTypeIsUnstructured(const std::string &fileType) {
    const std::string normalized = lower(fileType);
    return normalized == "pdf" || normalized == "docx" || normalized == "txt" || (!fileTypeIsStructured(fileType) && !normalized.empty());
}

std::string readFileContent(const std::string &path) {
    if (path.empty() || !std::filesystem::exists(path)) {
        return {};
    }
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

#include "core/StorageManager.hpp"

bool columnExists(sqlite3 *db, const std::string &table, const std::string &column) {
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

struct DatasetRecord {
    std::string filename;
    std::string fileType;
    std::string filePath;
    std::string extractedPath;
    std::string schema;
    std::string processedStatus;
};

std::vector<DatasetRecord> loadDatasetRecords() {
    std::vector<DatasetRecord> records;
    sqlite3 *db = nullptr;
    if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) != SQLITE_OK) {
        return records;
    }

    const bool hasFilePath = columnExists(db, "files_metadata", "file_path");
    const bool hasExtractedPath = columnExists(db, "files_metadata", "extracted_path");
    const bool hasSchema = columnExists(db, "files_metadata", "schema");
    const bool hasProcessedStatus = columnExists(db, "files_metadata", "processed_status");

    std::string selectColumns = "filename, file_type";
    selectColumns += hasFilePath ? ", file_path" : ", ''";
    selectColumns += hasExtractedPath ? ", extracted_path" : ", ''";
    selectColumns += hasSchema ? ", schema" : ", ''";
    selectColumns += hasProcessedStatus ? ", processed_status" : ", ''";

    std::string sql = "SELECT " + selectColumns + " FROM files_metadata";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            DatasetRecord record;
            const char *filename = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *fileType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            const char *filePath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            const char *extractedPath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            const char *schema = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            const char *processedStatus = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            record.filename = filename ? filename : "";
            record.fileType = fileType ? fileType : "";
            record.filePath = filePath ? filePath : "";
            record.extractedPath = extractedPath ? extractedPath : "";
            record.schema = schema ? schema : "";
            record.processedStatus = processedStatus ? processedStatus : "";
            records.push_back(record);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return records;
}

bool containsDataMatch(const DatasetRecord &record, const std::string &query) {
    // Only consider processed files
    if (record.processedStatus != "processed") {
        return false;
    }
    if (matchesQuery(record.filename, query) || matchesQuery(record.schema, query)) {
        return true;
    }
    // Read from extracted content file
    const std::string extractedContents = readFileContent(record.extractedPath);
    return matchesQuery(extractedContents, query);
}

bool queryHasApiIntent(const std::string &query) {
    const std::string normalized = lower(query);
    return normalized.find("erp") != std::string::npos || normalized.find("api") != std::string::npos || normalized.find("order") != std::string::npos || normalized.find("invoice") != std::string::npos || normalized.find("purchase") != std::string::npos || normalized.find("vendor") != std::string::npos;
}

bool queryHasStructuredIntent(const std::string &query) {
    const std::string normalized = lower(query);
    return normalized.find("report") != std::string::npos || normalized.find("sales") != std::string::npos || normalized.find("inventory") != std::string::npos || normalized.find("balance") != std::string::npos || normalized.find("total") != std::string::npos || normalized.find("sum") != std::string::npos || normalized.find("average") != std::string::npos || normalized.find("count") != std::string::npos || normalized.find("quantity") != std::string::npos || normalized.find("revenue") != std::string::npos || normalized.find("expense") != std::string::npos;
}

bool queryHasRagIntent(const std::string &query) {
    const std::string normalized = lower(query);
    return normalized.find("document") != std::string::npos || normalized.find("contract") != std::string::npos || normalized.find("pdf") != std::string::npos || normalized.find("note") != std::string::npos || normalized.find("agreement") != std::string::npos || normalized.find("terms") != std::string::npos || normalized.find("policy") != std::string::npos;
}

bool queryHasHybridIntent(const std::string &query) {
    const std::string normalized = lower(query);
    return normalized.find("hybrid") != std::string::npos || normalized.find("combine") != std::string::npos || normalized.find("insight") != std::string::npos || normalized.find("analysis") != std::string::npos;
}

}

namespace vetalmed::core {

std::string DatasetSelector::selectBestDataset(const std::string &query) {
    if (query.empty()) {
        return "insufficient_data";
    }

    const auto normalizedQuery = lower(query);
    const auto records = loadDatasetRecords();
    if (records.empty()) {
        return "insufficient_data";
    }

    bool hasStructured = false;
    bool hasUnstructured = false;
    bool hasApi = false;
    bool hasMatchedData = false;

    for (const auto &record : records) {
        if (!containsDataMatch(record, query)) {
            continue;
        }
        hasMatchedData = true;
        if (fileTypeIsStructured(record.fileType)) {
            hasStructured = true;
        } else {
            hasUnstructured = true;
        }
        if (queryHasApiIntent(query) || lower(record.fileType) == "json" || lower(record.filename).find("erp") != std::string::npos) {
            hasApi = true;
        }
    }

    if (!hasMatchedData) {
        return "insufficient_data";
    }

    if (queryHasHybridIntent(query) && hasStructured && hasUnstructured) {
        return "hybrid";
    }
    if (queryHasApiIntent(query) && hasApi) {
        return "api";
    }
    if (queryHasStructuredIntent(query) && hasStructured) {
        return "structured";
    }
    if (queryHasRagIntent(query) && hasUnstructured) {
        return "rag";
    }

    if (hasStructured && hasUnstructured) {
        return "hybrid";
    }
    if (hasStructured) {
        return "structured";
    }
    if (hasUnstructured) {
        return "rag";
    }
    if (hasApi) {
        return "api";
    }

    return "insufficient_data";
}

}

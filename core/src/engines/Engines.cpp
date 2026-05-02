#include "engines/ApiEngine.hpp"
#include "engines/StructuredEngine.hpp"
#include "engines/RagEngine.hpp"
#include "engines/ReasoningEngine.hpp"
#include "engines/LlmEngine.hpp"
#include "core/StorageManager.hpp"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>

namespace fs = std::filesystem;
using vetalmed::core::StorageManager;

namespace {

std::string lowerCase(const std::string &text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

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

std::vector<std::string> splitCsvLine(const std::string &line) {
    std::vector<std::string> values;
    std::istringstream iss(line);
    std::string token;
    while (std::getline(iss, token, ',')) {
        values.push_back(token);
    }
    return values;
}

std::string findSnippet(const std::string &content, const std::string &query) {
    auto normalizedContent = lowerCase(content);
    auto normalizedQuery = lowerCase(query);
    size_t pos = normalizedContent.find(normalizedQuery);
    if (pos == std::string::npos) {
        return {};
    }
    size_t start = pos > 100 ? pos - 100 : 0;
    size_t end = std::min(content.size(), pos + normalizedQuery.size() + 200);
    return content.substr(start, end - start);
}

bool matchesQuery(const std::string &text, const std::string &query) {
    if (text.empty() || query.empty()) {
        return false;
    }
    return lowerCase(text).find(lowerCase(query)) != std::string::npos;
}

bool isStructuredFileType(const std::string &fileType) {
    const auto normalized = lowerCase(fileType);
    return normalized == "csv" || normalized == "json" || normalized == "xls" || normalized == "xlsx";
}

struct DatasetRecord {
    std::string filename;
    std::string fileType;
    std::string filePath;
    std::string extractedPath;
    std::string datasetType;
    std::string processedStatus;
    std::string schema;
};

std::vector<DatasetRecord> loadDatasetRecords(const std::string &queryTypeClause = "") {
    sqlite3 *db = nullptr;
    std::vector<DatasetRecord> records;
    if (sqlite3_open(StorageManager::metadataPath().string().c_str(), &db) != SQLITE_OK) {
        return records;
    }

    bool hasFilePath = columnExists(db, "files_metadata", "file_path");
    bool hasExtractedPath = columnExists(db, "files_metadata", "extracted_path");
    bool hasDatasetType = columnExists(db, "files_metadata", "dataset_type");
    bool hasProcessedStatus = columnExists(db, "files_metadata", "processed_status");

    std::string selectColumns = "filename, file_type";
    if (hasFilePath) {
        selectColumns += ", file_path";
    }
    if (hasExtractedPath) {
        selectColumns += ", extracted_path";
    }
    if (hasDatasetType) {
        selectColumns += ", dataset_type";
    }
    if (hasProcessedStatus) {
        selectColumns += ", processed_status";
    }
    selectColumns += ", schema";

    std::string sql = "SELECT " + selectColumns + " FROM files_metadata WHERE processed_status = 'processed'";
    if (!queryTypeClause.empty()) {
        sql += " AND " + queryTypeClause;
    }

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            DatasetRecord record;
            int index = 0;
            const char *filename = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
            const char *fileType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
            record.filename = filename ? filename : "";
            record.fileType = fileType ? fileType : "";
            if (hasFilePath) {
                const char *filePath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
                record.filePath = filePath ? filePath : "";
            }
            if (hasExtractedPath) {
                const char *extractedPath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
                record.extractedPath = extractedPath ? extractedPath : "";
            }
            if (hasDatasetType) {
                const char *datasetType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
                record.datasetType = datasetType ? datasetType : "";
            }
            if (hasProcessedStatus) {
                const char *processedStatus = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
                record.processedStatus = processedStatus ? processedStatus : "";
            }
            const char *schema = reinterpret_cast<const char *>(sqlite3_column_text(stmt, index++));
            record.schema = schema ? schema : "";
            records.push_back(record);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return records;
}

std::string readFileContent(const std::string &path) {
    if (path.empty() || !fs::exists(path)) {
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

}

namespace vetalmed::engines {

nlohmann::json ApiEngine::execute(const std::string &query) {
    nlohmann::json result;
    if (query.empty()) {
        return result;
    }

    const auto datasets = loadDatasetRecords();
    for (const auto &record : datasets) {
        const auto content = readFileContent(record.extractedPath);
        bool match = matchesQuery(record.filename, query) || matchesQuery(record.schema, query) || matchesQuery(content, query);
        if (!match) {
            continue;
        }

        nlohmann::json matchData;
        matchData["document"] = record.filename;
        matchData["file_type"] = record.fileType;
        matchData["dataset_type"] = !record.datasetType.empty() ? record.datasetType : (isStructuredFileType(record.fileType) ? "structured" : "unstructured");
        matchData["snippet"] = findSnippet(content, query);
        result["data"].push_back(matchData);
    }

    if (result.empty()) {
        return result;
    }

    result["summary"] = "Found ERP-related data in uploaded content.";
    result["insights"] = nlohmann::json::array({"Matched ERP-related dataset content from the uploaded files."});
    result["recommendation"] = "Review the uploaded dataset(s) for the requested ERP information.";
    result["confidence"] = 0.75;
    result["source_type"] = "api";
    return result;
}

nlohmann::json StructuredEngine::execute(const std::string &query) {
    nlohmann::json result;
    if (query.empty()) {
        return result;
    }

    auto datasets = loadDatasetRecords("WHERE file_type IN ('csv', 'json', 'xlsx', 'xls')");
    if (datasets.empty()) {
        return result;
    }

    std::vector<nlohmann::json> extractedData;
    std::string summaryText;
    double confidence = 0.0;

    for (const auto &record : datasets) {
        const auto content = readFileContent(record.extractedPath);
        if (content.empty()) {
            continue;
        }

        if (record.fileType == "json") {
            try {
                auto jsonData = nlohmann::json::parse(content);
                for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
                    const auto key = it.key();
                    const auto &value = it.value();
                    if (matchesQuery(key, query) || matchesQuery(value.dump(), query)) {
                        extractedData.push_back({
                            {"document", record.filename},
                            {"match_key", key},
                            {"match_value", value},
                            {"source_type", "structured"}
                        });
                    }
                }
            } catch (...) {
                // invalid JSON
            }
        } else if (record.fileType == "csv") {
            std::istringstream iss(content);
            std::string headerLine;
            if (!std::getline(iss, headerLine)) {
                continue;
            }
            const auto headers = splitCsvLine(headerLine);
            std::string rowText;
            while (std::getline(iss, rowText)) {
                const auto cells = splitCsvLine(rowText);
                bool matchedRow = false;
                nlohmann::json rowJson;
                for (size_t i = 0; i < cells.size(); ++i) {
                    const auto header = i < headers.size() ? headers[i] : std::string("column_") + std::to_string(i);
                    rowJson[header] = cells[i];
                    if (matchesQuery(header, query) || matchesQuery(cells[i], query)) {
                        matchedRow = true;
                    }
                }
                if (matchedRow) {
                    extractedData.push_back({
                        {"document", record.filename},
                        {"row", rowJson},
                        {"source_type", "structured"}
                    });
                }
            }
            if (extractedData.empty() && matchesQuery(headerLine, query)) {
                extractedData.push_back({
                    {"document", record.filename},
                    {"headers", headers},
                    {"source_type", "structured"}
                });
            }
        }

        if (!extractedData.empty() && summaryText.empty()) {
            summaryText = "Found matching structured content in " + record.filename;
            confidence = 0.85;
        }
    }

    if (extractedData.empty()) {
        return result;
    }

    if (summaryText.empty()) {
        summaryText = "Found structured data that matches your query.";
        confidence = 0.75;
    }

    result["summary"] = summaryText;
    result["data"] = extractedData;
    result["insights"] = nlohmann::json::array({"Retrieved structured content that matches the uploaded data."});
    result["recommendation"] = "Use the structured dataset entries returned above to answer your question.";
    result["confidence"] = confidence;
    result["source_type"] = "structured";
    return result;
}

nlohmann::json RagEngine::execute(const std::string &query) {
    nlohmann::json result;
    if (query.empty()) {
        return result;
    }

    auto datasets = loadDatasetRecords("WHERE file_type NOT IN ('csv', 'json', 'xlsx', 'xls')");
    if (datasets.empty()) {
        return result;
    }

    std::vector<nlohmann::json> foundContent;
    std::string summaryText;
    double confidence = 0.0;

    for (const auto &record : datasets) {
        const auto content = readFileContent(record.extractedPath);
        if (content.empty()) {
            continue;
        }
        if (!matchesQuery(content, query) && !matchesQuery(record.filename, query) && !matchesQuery(record.schema, query)) {
            continue;
        }

        foundContent.push_back({
            {"document", record.filename},
            {"snippet", findSnippet(content, query)},
            {"source_type", "rag"}
        });

        if (summaryText.empty()) {
            summaryText = "Found relevant document content in " + record.filename;
        }
        confidence = std::max(confidence, 0.8);
    }

    if (foundContent.empty()) {
        return result;
    }

    result["summary"] = summaryText;
    result["data"] = foundContent;
    result["insights"] = nlohmann::json::array({"Relevant document content was retrieved from uploaded files."});
    result["recommendation"] = "Review the document snippets and original files for full context.";
    result["confidence"] = confidence;
    result["source_type"] = "rag";
    return result;
}

nlohmann::json ReasoningEngine::combine(const nlohmann::json &structuredResult,
                                        const nlohmann::json &ragResult) {
    if (structuredResult.empty() || !structuredResult.contains("data") || structuredResult["data"].empty()) {
        return ragResult;
    }
    if (ragResult.empty() || !ragResult.contains("data") || ragResult["data"].empty()) {
        return structuredResult;
    }

    nlohmann::json combined;
    std::vector<nlohmann::json> combinedData;
    for (const auto &item : structuredResult["data"]) {
        combinedData.push_back(item);
    }
    for (const auto &item : ragResult["data"]) {
        combinedData.push_back(item);
    }

    std::vector<std::string> insights;
    if (structuredResult.contains("insights")) {
        for (const auto &item : structuredResult["insights"]) {
            if (item.is_string()) {
                insights.push_back(item.get<std::string>());
            }
        }
    }
    if (ragResult.contains("insights")) {
        for (const auto &item : ragResult["insights"]) {
            if (item.is_string()) {
                insights.push_back(item.get<std::string>());
            }
        }
    }

    std::string summaryText = structuredResult.value("summary", std::string());
    if (!ragResult.value("summary", std::string()).empty()) {
        if (!summaryText.empty()) {
            summaryText += " | ";
        }
        summaryText += ragResult["summary"].get<std::string>();
    }

    combined["summary"] = summaryText.empty() ? "Combined data found in structured and document sources." : summaryText;
    combined["data"] = combinedData;
    combined["insights"] = insights.empty() ? nlohmann::json::array({std::string("Combined structured and document data matches.")}) : nlohmann::json(insights);
    combined["recommendation"] = "Use the combined dataset and document content to answer the query.";
    combined["confidence"] = std::min(std::max(structuredResult.value("confidence", 0.0), ragResult.value("confidence", 0.0)) + 0.05, 0.9);
    combined["source_type"] = "hybrid";
    return combined;
}

nlohmann::json LlmEngine::summarize(const std::string &text) {
    nlohmann::json summary;
    summary["summary"] = text.substr(0, std::min<size_t>(text.size(), 240));
    summary["confidence"] = 0.60;
    return summary;
}

}

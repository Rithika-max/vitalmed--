#pragma once
#include <filesystem>
#include <string>

namespace vetalmed::core {

class StorageManager {
public:
    static void init();
    static std::filesystem::path getStorageRoot();
    static std::filesystem::path uploadsPath();
    static std::filesystem::path extractedPath();
    static std::filesystem::path indexPath();
    static std::filesystem::path logsPath();
    static std::filesystem::path metadataPath();
    static std::filesystem::path filePath(const std::string &filename);
    static std::filesystem::path extractedFilePath(const std::string &filename);

private:
    static std::filesystem::path findRepositoryRoot();
    static std::filesystem::path storageRoot;
};

}

#include "core/StorageManager.hpp"
#include "core/Logger.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace vetalmed::core {

std::filesystem::path StorageManager::storageRoot;

static bool isRepositoryRoot(const fs::path &path) {
    return fs::exists(path / "core") &&
           (fs::exists(path / "package.json") || fs::exists(path / "storage"));
}

void StorageManager::init() {
    const auto repoRoot = findRepositoryRoot();
    storageRoot = repoRoot / "storage";
    fs::create_directories(storageRoot);
    fs::create_directories(uploadsPath());
    fs::create_directories(extractedPath());
    fs::create_directories(indexPath());
    fs::create_directories(logsPath());
}

std::filesystem::path StorageManager::getStorageRoot() {
    return storageRoot;
}

std::filesystem::path StorageManager::uploadsPath() {
    return storageRoot / "uploads";
}

std::filesystem::path StorageManager::extractedPath() {
    return storageRoot / "extracted";
}

std::filesystem::path StorageManager::indexPath() {
    return storageRoot / "index";
}

std::filesystem::path StorageManager::logsPath() {
    return storageRoot / "logs";
}

std::filesystem::path StorageManager::metadataPath() {
    return storageRoot / "metadata.db";
}

std::filesystem::path StorageManager::filePath(const std::string &filename) {
    return uploadsPath() / filename;
}

std::filesystem::path StorageManager::extractedFilePath(const std::string &filename) {
    return extractedPath() / (filename + ".txt");
}

std::filesystem::path StorageManager::findRepositoryRoot() {
    if (const char *envRoot = std::getenv("VETALMED_REPO_ROOT")) {
        fs::path path(envRoot);
        if (isRepositoryRoot(path)) {
            return path;
        }
    }

    fs::path path = fs::current_path();
    while (!path.empty()) {
        if (isRepositoryRoot(path)) {
            return path;
        }
        if (path == path.root_path()) {
            break;
        }
        path = path.parent_path();
    }

    path = fs::path(__FILE__);
    while (!path.empty()) {
        if (isRepositoryRoot(path)) {
            return path;
        }
        if (path == path.root_path()) {
            break;
        }
        path = path.parent_path();
    }

    return fs::current_path();
}

}

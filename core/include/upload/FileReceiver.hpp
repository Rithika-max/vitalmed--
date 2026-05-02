#pragma once
#include <string>
#include <drogon/HttpAppFramework.h>

namespace vetalmed::upload {

class FileReceiver {
public:
    static std::string receive(const drogon::HttpRequestPtr &req, std::string &filename, std::string &error);
};

}

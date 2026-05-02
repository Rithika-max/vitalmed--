#pragma once
#include <drogon/HttpController.h>

namespace vetalmed::server {

class UploadController : public drogon::HttpController<UploadController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UploadController::upload, "/upload", drogon::Post);
    METHOD_LIST_END

    void upload(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}

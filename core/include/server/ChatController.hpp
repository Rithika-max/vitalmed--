#pragma once
#include <drogon/HttpController.h>

namespace vetalmed::server {

class ChatController : public drogon::HttpController<ChatController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChatController::chat, "/chat", drogon::Post);
    ADD_METHOD_TO(ChatController::datasets, "/datasets", drogon::Get);
    ADD_METHOD_TO(ChatController::deleteDataset, "/datasets/{id}", drogon::Delete);
    METHOD_LIST_END

    void chat(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void datasets(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void deleteDataset(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int id);
};

}

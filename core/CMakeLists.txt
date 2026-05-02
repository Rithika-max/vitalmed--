cmake_minimum_required(VERSION 3.16)
project(erp_ai_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)
find_package(nlohmann_json REQUIRED)
find_library(SQLITE3_LIBRARY sqlite3)

if(ENABLE_FAISS)
    find_package(FAISS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/server/HealthController.cpp
    src/server/UploadController.cpp
    src/server/ChatController.cpp
    src/server/HttpServer.cpp
    src/core/Validator.cpp
    src/core/MemoryHook.cpp
    src/core/IntentParser.cpp
    src/core/DecisionRouter.cpp
    src/core/DatasetSelector.cpp
    src/core/ResponseComposer.cpp
    src/core/OllamaHealthChecker.cpp
    src/core/FallbackResponseComposer.cpp
    src/core/Logger.cpp
    src/core/StorageManager.cpp
    src/upload/UploadPipeline.cpp
    src/engines/Engines.cpp
    src/engines/HybridEngine.cpp
    src/engines/CnnEngine.cpp
)

add_executable(erp_ai_backend ${SOURCES})

target_link_libraries(erp_ai_backend
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    ${SQLITE3_LIBRARY}
)

if(ENABLE_FAISS AND FAISS_FOUND)
    target_compile_definitions(erp_ai_backend PRIVATE ENABLE_FAISS)
    target_link_libraries(erp_ai_backend PRIVATE ${FAISS_LIBRARIES})
endif()

install(TARGETS erp_ai_backend DESTINATION bin)

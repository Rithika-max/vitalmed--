#include "core/FallbackResponseComposer.hpp"
#include <sstream>
#include <algorithm>

namespace vetalmed::core {

nlohmann::json FallbackResponseComposer::composeFromData(const std::string &query, 
                                                        const nlohmann::json &searchResults) {
    nlohmann::json response;
    
    // Extract relevant data from search results
    std::vector<nlohmann::json> resultData;
    if (searchResults.is_array()) {
        for (const auto &item : searchResults) {
            resultData.push_back(item);
        }
    } else if (searchResults.is_object() && searchResults.contains("data")) {
        if (searchResults["data"].is_array()) {
            for (const auto &item : searchResults["data"]) {
                resultData.push_back(item);
            }
        }
    }
    
    // Build structured response
    std::ostringstream summary;
    summary << "Based on the available data: ";
    
    if (!resultData.empty()) {
        summary << "Found " << resultData.size() << " matching record(s).";
    } else {
        summary << "No matching records found.";
    }
    
    response["summary"] = summary.str();
    response["data"] = resultData.empty() ? nlohmann::json::array() : nlohmann::json(resultData);
    
    std::vector<std::string> insights;
    insights.push_back("Data sourced from uploaded datasets");
    insights.push_back("No LLM enhancement available (Ollama offline)");
    
    response["insights"] = nlohmann::json(insights);
    response["recommendation"] = resultData.empty() ? 
        "Please upload relevant data or refine your query." : 
        "Review the matched records above for your answer.";
    response["confidence"] = resultData.empty() ? 0.1 : 0.65;
    response["source_type"] = "rule_based";
    response["ollama_status"] = "offline";
    
    return response;
}

nlohmann::json FallbackResponseComposer::composeNoData(const std::string &query) {
    nlohmann::json response;
    
    response["summary"] = "No relevant data found for this query. Ollama is also offline, so AI enhancement is unavailable.";
    response["data"] = nlohmann::json::array();
    response["insights"] = nlohmann::json::array({"Insufficient data", "LLM unavailable"});
    response["recommendation"] = "Please upload relevant data or ask a question based on available data.";
    response["confidence"] = 0.1;
    response["source_type"] = "insufficient_data";
    response["ollama_status"] = "offline";
    
    return response;
}

}

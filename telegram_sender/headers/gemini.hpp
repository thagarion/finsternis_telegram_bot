#pragma once
#include <format>
#include <regex>
#include <string>

#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "config.hpp"

class Gemini {
    static size_t write_callback(void *contents, const size_t size, const size_t count, std::string *result) {
        result->append(static_cast<char *>(contents), size * count);
        return size * count;
    }

public:
    static inline std::string flash_model = "gemini-3-flash-preview";
    static inline std::string lite_model = "gemini-3.1-flash-lite-preview";

    static inline std::string discord_translator_prompt =
            "You are a professional, accurate, and objective translator for game news. Your task is to accurately "
            "translate the provided text segment into Russian.\n\n"
            "CRITICAL CONSTRAINTS:\n"
            "1. Translation: Translate the text *verbatim* to Russian, changing only the language. Preserve the "
            "original meaning.\n"
            "2. Formatting (CRITICAL): Use ONLY the following strict Telegram Markdown tags: *bold*, _italic_, - lists. Do not use "
            "any other formatting\n"
            "3. Formatting: Add basic formatting to key headings, dates, or important phrases where appropriate. The "
            "formatting must be seamless and not interrupt the flow of the translation.\n"
            "4. Localization: Change all dates and times to the MSK (Moscow Time) timezone in Russian format.\n"
            "5. Names: Leave names of game items, proper nouns, and technical terms in their original language "
            "(English).\n";

    static std::string gemini_request(const std::string &input, const std::string &prompt, const std::string &model) {
        const auto gemini_url = std::format(
            "https://generativelanguage.googleapis.com/v1beta/models/{}:generateContent?key={}", model,
            Config::gemini_api_key);

        std::string buffer;
        buffer.clear();

        nlohmann::json payload;

        payload["system_instruction"]["parts"]["text"] = prompt;
        payload["contents"]["parts"]["text"] = input;
        payload["generationConfig"]["temperature"] = 0.2;
        payload["generationConfig"]["topP"] = 0.5;

        const std::string body = payload.dump();

        if (CURL *curl = curl_easy_init()) {
            curl_easy_setopt(curl, CURLOPT_URL, gemini_url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

            curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            const CURLcode result = curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);

            if (result != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(result) << std::endl;
                return "";
            }
        }

        try {
            if (auto json_doc = nlohmann::json::parse(buffer); json_doc.contains("error")) {
                std::cerr << json_doc.at("error").at("message").get<std::string>() << std::endl;
                return "";
            } else {
                buffer = json_doc.at("candidates").at(0).at("content").at("parts").at(0).at("text").dump();
                buffer = std::regex_replace(buffer, std::regex("\\\\n"), "\n");
                buffer = std::regex_replace(buffer, std::regex("\""), "");
                buffer = std::regex_replace(buffer, std::regex("#{4,}"), "###");
            }
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }

        return buffer;
    }
};

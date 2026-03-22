#pragma once

#include <tgbotxx/tgbotxx.hpp>

#include "config.hpp"

class TelegramBot final {
    explicit TelegramBot(const std::string& token) : bot_api(token) {}

    tgbotxx::Api bot_api;

public:
    TelegramBot(const TelegramBot&) = delete;

    TelegramBot& operator=(const TelegramBot&) = delete;

    static TelegramBot& GetMimiMentor() {
        static TelegramBot instance(Config::mimi_chat_id.api_key);
        return instance;
    }

    static TelegramBot& GetFinsternis() {
        static TelegramBot instance(Config::finsternis_chat_id.api_key);
        return instance;
    }

    tgbotxx::Ptr<tgbotxx::Message> SendMessage(const int64_t chat_id, const int32_t thread_id, const std::string& msg,
                                               const std::string& image_link,
                                               const std::string& parse_mode = "") const {
        if (const auto& message = msg; !message.empty()) {
            if (image_link.empty()) {
                return bot_api.sendMessage(chat_id, message, thread_id, parse_mode);
            } else {
                return bot_api.sendPhoto(chat_id, image_link, thread_id, message, parse_mode);
            }
        }
        return nullptr;
    }
};

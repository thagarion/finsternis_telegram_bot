#pragma once

#include <amqpcpp.h>

#include <string>

#include "gemini.hpp"
#include "logger.hpp"
#include "amqpcpp/linux_tcp/tcpchannel.h"

struct Event {
    std::string body;
    std::string source;
    std::string author;
    std::string date;
};

template<typename Handler>
void register_consumer(AMQP::TcpChannel &channel, const std::string &queue, Handler handler) {
    channel.consume(queue)
            .onReceived([&channel, handler](const AMQP::Message &message, const uint64_t tag, bool redelivered) {
                try {
                    Event event;

                    event.body = std::string(message.body(), message.bodySize());

                    const auto &headers = message.headers();

                    if (headers.size() > 0) {
                        event.author = std::string(headers.get("author"));
                        event.source = std::string(headers.get("source"));
                        event.date = std::string(headers.get("date"));
                    }

                    handler(event);
                    channel.ack(tag);
                } catch (const std::exception &exception) {
                    LOG_ERROR("hangle error: " << exception.what());
                    channel.reject(tag);
                }
            });
}

inline void handle_discord(const Event &event) {
    const auto translated_text = Gemini::gemini_request(event.body, Gemini::discord_translator_prompt,
                                                        Gemini::lite_model);
    std::string message;
    if (!event.author.empty()) {
        message.append(std::format("[{}]: ", event.author));
    }
    message.append(translated_text);
    auto result = TelegramBot::GetFinsternis().SendMessage(
        Config::finsternis_chat_id.chat_id,
        Config::finsternis_chat_id.thread_id,
        message,
        ""
    );
}

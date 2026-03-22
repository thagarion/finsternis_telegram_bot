#pragma once
#include <filesystem>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

struct TG_Chat_ID {
    std::string api_key;
    int64_t chat_id;
    int32_t thread_id;
};

class Config {
    static inline std::string config_path = "config.yaml";

public:
    static inline std::string gemini_api_key;
    static inline std::string amqp_address;
    static inline TG_Chat_ID mimi_chat_id = {};
    static inline TG_Chat_ID finsternis_chat_id = {};

    static void load_config() {
        if (!std::filesystem::exists(config_path)) {
            std::cerr << "Config File Not Found" << std::endl;
            exit(-1);
        }

        YAML::Node config_node = YAML::LoadFile(config_path);

        gemini_api_key = config_node["gemini_api"].as<std::string>();
        amqp_address = config_node["amqp_address"].as<std::string>();

        if (config_node["mimi_bot"]) {
            const auto& node = config_node["mimi_bot"];
            mimi_chat_id.api_key = node["api"].as<std::string>();
            mimi_chat_id.chat_id = node["chat"].as<int64_t>();
            mimi_chat_id.thread_id = node["thread"].as<int32_t>();
        }

        if (config_node["finsternis_bot"]) {
            const auto& node = config_node["finsternis_bot"];
            finsternis_chat_id.api_key = node["api"].as<std::string>();
            finsternis_chat_id.chat_id = node["chat"].as<int64_t>();
            finsternis_chat_id.thread_id = node["thread"].as<int32_t>();
        }
    }
};
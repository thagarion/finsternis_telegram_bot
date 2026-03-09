#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <yaml-cpp/yaml.h>

#include "logger.hpp"

class Config {
    const std::string config_path;

public:
    std::string discord_token;
    std::string channel_id;
    int log_level = 2;

    std::string redis_address;
    std::string redis_port;

    Config() = delete;

    explicit Config(std::string path) : config_path(std::move(path)) {
        if (!std::filesystem::exists(config_path)) {
            LOG_ERROR("config file not found");
            exit(-1);
        }
        YAML::Node config_node = YAML::LoadFile(config_path);

        discord_token = config_node["discord_token"].as<std::string>();
        channel_id = config_node["channel_id"].as<std::string>();
        log_level = config_node["log_level"].as<int>(2);
        redis_address = config_node["redis_address"].as<std::string>("127.0.0.1");
        redis_port = config_node["redis_port"].as<std::string>("6379");
    }
};

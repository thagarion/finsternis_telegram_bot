#pragma once

#include <string>
#include <boost/process/env.hpp>

#include "logger.hpp"

class Config {
public:
    static inline std::string discord_token;
    static inline std::string channel_id;
    static inline int log_level = 2;

    static inline std::string amqp_address;

    static void Read() {
        auto env = boost::this_process::environment();

        if (const auto log_var = env["LOG_LEVEL"]; !log_var.empty()) {
            log_level = boost::lexical_cast<int>(log_var.to_string());
        }

        amqp_address = env["AMQP_ADDRESS"].to_string();
        discord_token = env["DISCORD_TOKEN"].to_string();
        channel_id = env["DISCORD_CHANNEL"].to_string();
    }
};

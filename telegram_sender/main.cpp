#include "logger.hpp"
#include "headers/telegram_bot.hpp"
#include "headers/config.hpp"
#include <tgbotxx/tgbotxx.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <boost/asio.hpp>

#include "rabbit_consumer.hpp"

using json = nlohmann::json;

std::vector<std::string> splitText(const std::string &text, size_t maxLen = 1000);


int main() {
    Logger::Init();
    Config::load_config();

    boost::asio::io_context io_context;
    AMQP::LibBoostAsioHandler handler(io_context);

    AMQP::TcpConnection connection(&handler, AMQP::Address(Config::amqp_address));

    AMQP::TcpChannel channel(&connection);

    channel.onReady([]() {
        LOG_INFO("Rabbit connection ready");
    });

    channel.onError([](const char *message) {
        LOG_ERROR("Rabbit error: " << message);
    });

    register_consumer(channel, "discord_messages", handle_discord);

    io_context.run();

    return 0;
}

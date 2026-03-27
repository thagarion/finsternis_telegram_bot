#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include "config.hpp"
#include "logger.hpp"

std::atomic<int> last_s{0};
std::atomic<bool> is_active{true};

void start_heartbeat(
    boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > &websocket,
    boost::asio::steady_timer &timer, int interval_ms) {
    timer.expires_after(std::chrono::milliseconds(interval_ms));
    timer.async_wait([&websocket, &timer, interval_ms](const boost::system::error_code &code) {
        if (!code && is_active) {
            try {
                const nlohmann::json heartbeat = {
                    {"op", 1},
                    {"d", last_s.load() > 0 ? nlohmann::json(last_s.load()) : nlohmann::json(nullptr)}
                };
                websocket.write(boost::asio::buffer(heartbeat.dump()));
                LOG_DEBUG("heartbeat sent");
                start_heartbeat(websocket, timer, interval_ms);
            } catch (...) { LOG_ERROR("heartbeat sending error"); }
        }
    });
}

void run_session(const std::string &token, const std::string &channel_id) {
    std::thread ioc_thread;

    boost::asio::io_service io_service;
    boost::asio::ssl::context context(boost::asio::ssl::context::tlsv13_client); // Используй TLS 1.3
    context.set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3);

    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::beast::websocket::stream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket> > websocket(
        io_service, context);

    AMQP::LibBoostAsioHandler rabbit_handler(io_service);
    AMQP::TcpConnection rabbit_connection(&rabbit_handler, Config::amqp_address);
    AMQP::TcpChannel rabbit_channel(&rabbit_connection);
    rabbit_channel.declareQueue("discord_messages", AMQP::durable);

    auto const results = resolver.resolve("gateway.discord.gg", "443");
    boost::asio::connect(boost::beast::get_lowest_layer(websocket), results);

    if (!SSL_set_tlsext_host_name(websocket.next_layer().native_handle(), "gateway.discord.gg")) {
        throw boost::beast::system_error(
            boost::beast::error_code(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()));
    }

    websocket.next_layer().handshake(boost::asio::ssl::stream_base::client);
    websocket.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type &req) {
            req.set(boost::beast::http::field::user_agent,
                    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36");

            req.set(boost::beast::http::field::accept_language, "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");
            req.set("Cache-Control", "no-cache");
            req.set("Pragma", "no-cache");

            req.set(boost::beast::http::field::host, "gateway.discord.gg");
        }
    ));
    websocket.handshake("gateway.discord.gg", "/?v=9&encoding=json");

    boost::beast::flat_buffer buffer;
    websocket.read(buffer);
    auto hello = nlohmann::json::parse(boost::beast::buffers_to_string(buffer.data()));
    const int interval = hello["d"]["heartbeat_interval"];
    LOG_INFO("heartbeat interval = " << interval);

    boost::asio::steady_timer timer(io_service);
    start_heartbeat(websocket, timer, interval);
    ioc_thread = std::thread([&io_service]() {
        try { io_service.run(); } catch (...) {
        }
    });

    nlohmann::json identify = {
        {"op", 2},
        {
            "d", {
                {"token", token},
                {"properties", {{"$os", "Windows"}, {"$browser", "chrome"}, {"$device", ""}}}
            }
        }
    };
    websocket.write(boost::asio::buffer(identify.dump()));

    try {
        boost::beast::flat_buffer flat_buffer;
        while (is_active) {
            flat_buffer.consume(flat_buffer.size());

            boost::system::error_code code;
            websocket.read(flat_buffer, code);
            if (code == boost::beast::websocket::error::closed || !is_active) { break; }
            if (code) throw boost::beast::system_error{code};

            auto data = nlohmann::json::parse(boost::beast::buffers_to_string(flat_buffer.data()));

            if (data.contains("s") && !data["s"].is_null()) { last_s = data["s"]; }

            if (data["t"] == "MESSAGE_CREATE") {
                if (auto &d = data["d"]; d["channel_id"] == channel_id) {
                    std::string content = d["content"];
                    std::string author = d["author"]["username"];
                    std::string timestamp = d["timestamp"];
                    LOG_INFO("[" << author << "]: " << content);

                    AMQP::Table headers;
                    headers.set("author", author);
                    headers.set("date", timestamp);
                    AMQP::Envelope envelope(content.data(), content.length());
                    envelope.setHeaders(headers);
                    envelope.setDeliveryMode(2);
                    rabbit_channel.publish("", "discord_messages", envelope);
                }
            }
        }
    } catch (...) {
        is_active = false;
    }

    timer.cancel();
    io_service.stop();
    if (ioc_thread.joinable()) { ioc_thread.join(); }
}

int main() {
    Config::Read();
    Logger::Init(Config::log_level);

    boost::asio::io_context signal_ioc;
    boost::asio::signal_set signals(signal_ioc);
    signals.add(SIGINT);
    signals.add(SIGTERM);
    signals.add(SIGQUIT);
    signals.add(SIGHUP);

    signals.async_wait([&](auto, int) {
        LOG_INFO("closing...")
        is_active = false;
    });

    std::thread sig_thread([&]() { signal_ioc.run(); });

    while (is_active) {
        try {
            LOG_INFO("start listening")
            run_session(Config::discord_token, Config::channel_id);
        } catch (const std::exception &exception) {
            LOG_ERROR("session lost. exception: " << exception.what());
            if (is_active) {
                LOG_INFO("reconnecting...");
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }
    }

    if (sig_thread.joinable()) { sig_thread.join(); }
}

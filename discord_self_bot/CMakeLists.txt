cmake_minimum_required(VERSION 3.28)
project(discord-self-bot VERSION 1.0 DESCRIPTION "discord-self-bot")
set(project_name "discord-self-bot")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(AMQP-CPP_LINUX_TCP ON CACHE BOOL "" FORCE)
set(AMQP-CPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)
FetchContent_Declare(
        amqpcpp
        GIT_REPOSITORY https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
        GIT_TAG v4.3.27
)
set(AMQP-CPP_BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(amqpcpp)

find_package(OpenSSL REQUIRED MODULE)
find_package(Boost CONFIG REQUIRED COMPONENTS system log log_setup)

file(GLOB_RECURSE sources "sources/**.cpp")
file(GLOB_RECURSE headers "headers/**.hpp")

include_directories(${project_name} PUBLIC headers ${JSON_INCLUDE_DIR})
add_executable(${project_name} main.cpp ${sources} ${headers})
target_link_libraries(${project_name} PUBLIC pthread yaml-cpp nlohmann_json amqpcpp
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::system
        Boost::log
        Boost::log_setup)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources} ${headers})
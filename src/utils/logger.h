#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

class Logger {
public:
    static void init(const std::string& log_file = "shadow_key.log");
    static std::shared_ptr<spdlog::logger>& get();

private:
    static std::shared_ptr<spdlog::logger> instance_;
};

#define LOG_TRACE(...)    Logger::get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    Logger::get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     Logger::get()->info(__VA_ARGS__)
#define LOG_WARN(...)     Logger::get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    Logger::get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::get()->critical(__VA_ARGS__)

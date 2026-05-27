#include "logger.h"

std::shared_ptr<spdlog::logger> Logger::instance_ = nullptr;

void Logger::init(const std::string& log_file) {
    if (instance_) return;

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true));

    instance_ = std::make_shared<spdlog::logger>("ShadowKey", sinks.begin(), sinks.end());
    instance_->set_level(spdlog::level::trace);
    instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    spdlog::set_default_logger(instance_);
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    return instance_;
}

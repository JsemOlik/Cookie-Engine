#include "Cookie/Core/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace cookie::core {

namespace {

std::string BuildTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

  std::tm local_time{};
#if defined(_WIN32)
  localtime_s(&local_time, &now_time);
#else
  localtime_r(&now_time, &local_time);
#endif

  std::ostringstream builder;
  builder << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
  return builder.str();
}

} // namespace

Logger::Logger(std::filesystem::path log_file_path) {
  const auto parent = log_file_path.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }

  stream_.open(log_file_path, std::ios::out | std::ios::trunc);
}

bool Logger::IsReady() const {
  return stream_.is_open();
}

void Logger::Info(std::string_view message) {
  WriteLine("INFO", message);
}

void Logger::Error(std::string_view message) {
  WriteLine("ERROR", message);
}

void Logger::WriteLine(std::string_view level, std::string_view message) {
  if (!stream_.is_open()) {
    return;
  }

  stream_ << '[' << BuildTimestamp() << "] [" << level << "] " << message
          << '\n';
}

} // namespace cookie::core

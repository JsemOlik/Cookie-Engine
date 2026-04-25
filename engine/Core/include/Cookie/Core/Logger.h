#pragma once

#include <filesystem>
#include <fstream>
#include <string_view>

namespace cookie::core {

class Logger {
 public:
  explicit Logger(std::filesystem::path log_file_path);

  bool IsReady() const;
  void Info(std::string_view message);
  void Error(std::string_view message);

 private:
  void WriteLine(std::string_view level, std::string_view message);

  std::ofstream stream_;
};

} // namespace cookie::core

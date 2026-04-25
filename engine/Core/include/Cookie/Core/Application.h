#pragma once

#include <string>

namespace cookie::core {

struct ApplicationConfig {
  std::string application_name;
};

class Application {
 public:
  explicit Application(ApplicationConfig config);

  int Run() const;

 private:
  ApplicationConfig config_;
};

} // namespace cookie::core

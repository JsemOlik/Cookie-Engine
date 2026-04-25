#pragma once

#include <memory>
#include <string>

#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::core {

struct ApplicationConfig {
  std::string application_name;
  std::string renderer_backend_name;
};

class Application {
 public:
  Application(ApplicationConfig config,
              std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend);

  int Run() const;

 private:
  ApplicationConfig config_;
  std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend_;
};

} // namespace cookie::core

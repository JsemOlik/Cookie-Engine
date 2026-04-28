#pragma once

#include <memory>
#include <string>

#include "Cookie/Core/AudioBackend.h"
#include "Cookie/Core/PhysicsBackend.h"
#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::core {

struct ApplicationConfig {
  std::string application_name;
  std::string engine_config_path;
  std::string core_runtime_source;
  std::string core_module_path;
  std::string core_module_name;
  int core_module_api_version = 0;
  bool strict_module_mode = false;
  bool require_core_module = false;
  bool require_renderer_module = false;
  bool require_physics_module = false;
  bool require_audio_module = false;
  std::string renderer_backend_name;
  std::string renderer_runtime_source;
  std::string renderer_module_path;
  std::string physics_runtime_source;
  std::string physics_module_path;
  std::string audio_runtime_source;
  std::string audio_module_path;
  std::string window_title;
  std::string camera_mode = "perspective";
  std::string demo_albedo_asset_id;
  int window_width = 1280;
  int window_height = 720;
  cookie::renderer::ClearColor clear_color{};
};

class Application {
 public:
  Application(ApplicationConfig config,
              std::unique_ptr<cookie::core::IAudioBackend> audio_backend,
              std::unique_ptr<cookie::core::IPhysicsBackend> physics_backend,
              std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend);

  int Run() const;

 private:
  ApplicationConfig config_;
  std::unique_ptr<cookie::core::IAudioBackend> audio_backend_;
  std::unique_ptr<cookie::core::IPhysicsBackend> physics_backend_;
  std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend_;
};

} // namespace cookie::core

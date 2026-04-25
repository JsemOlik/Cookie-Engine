#include "Cookie/Core/Application.h"

#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/Logger.h"
#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Platform/PlatformWindow.h"

namespace cookie::core {

Application::Application(
    ApplicationConfig config,
    std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend)
    : config_(std::move(config)), renderer_backend_(std::move(renderer_backend)) {}

int Application::Run() const {
  const StartupPaths paths = DiscoverStartupPaths();
  Logger logger(paths.log_file);

  if (!logger.IsReady()) {
    return 1;
  }

  logger.Info("CookieRuntime startup sequence initialized.");
  logger.Info("Application: " + config_.application_name);
  logger.Info("Selected renderer backend: " + config_.renderer_backend_name);
  logger.Info("Window title: " + config_.window_title);
  logger.Info("Window size: " + std::to_string(config_.window_width) + "x" +
              std::to_string(config_.window_height));
  logger.Info("Project root: " + paths.project_root.string());
  logger.Info("Config directory: " + paths.config_dir.string());
  logger.Info("Engine config: " + paths.engine_config.string());
  logger.Info("Graphics config: " + paths.graphics_config.string());
  logger.Info("Input config: " + paths.input_config.string());
  logger.Info("Game config: " + paths.game_config.string());

  cookie::assets::AssetRegistry assets;
  const auto base_package = paths.project_root / "content" / "base.pak";
  if (assets.MountPackage(base_package)) {
    logger.Info("Mounted asset package: " + base_package.string());
    logger.Info("Mounted package count: " +
                std::to_string(assets.GetMountedPackageCount()));
    logger.Info("Discovered asset ids: " + std::to_string(assets.GetAssetCount()));
  } else {
    logger.Error("Failed to mount asset package: " + base_package.string());
  }

  if (!renderer_backend_) {
    logger.Error("No renderer backend instance is available.");
    return 2;
  }

  logger.Info("Initializing renderer backend: " +
              std::string(renderer_backend_->Name()));
  if (!renderer_backend_->Initialize()) {
    logger.Error("Renderer backend failed to initialize.");
    return 3;
  }
  logger.Info("Renderer backend initialized successfully.");

  auto window = cookie::platform::CreatePlatformWindow({
      .title = config_.window_title,
      .width = config_.window_width,
      .height = config_.window_height,
  });
  if (!window) {
    logger.Error("Platform window creation failed.");
    renderer_backend_->Shutdown();
    return 4;
  }

  logger.Info("Platform window created successfully.");
  logger.Info("Starting frame loop.");

  int frame_count = 0;
  while (!window->ShouldClose()) {
    window->PollEvents();

    if (!renderer_backend_->BeginFrame()) {
      logger.Error("Renderer backend BeginFrame failed.");
      window->RequestClose();
      break;
    }

    renderer_backend_->Clear(config_.clear_color);
    renderer_backend_->EndFrame();

    ++frame_count;
    if (config_.max_frames > 0 && frame_count >= config_.max_frames) {
      logger.Info("Frame loop reached max_frames, requesting shutdown.");
      window->RequestClose();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  logger.Info("Frame loop ended after " + std::to_string(frame_count) + " frames.");
  renderer_backend_->Shutdown();
  logger.Info("Renderer backend shut down successfully.");
  logger.Info("Phase 4 skeleton complete. Asset mount/list contracts wired.");

  return 0;
}

} // namespace cookie::core

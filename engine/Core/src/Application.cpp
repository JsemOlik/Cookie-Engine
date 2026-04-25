#include "Cookie/Core/Application.h"

#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/GameLogicModule.h"
#include "Cookie/Core/Logger.h"
#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Platform/PlatformPaths.h"
#include "Cookie/Platform/PlatformWindow.h"

namespace cookie::core {

Application::Application(
    ApplicationConfig config,
    std::unique_ptr<cookie::core::IPhysicsBackend> physics_backend,
    std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend)
    : config_(std::move(config)),
      physics_backend_(std::move(physics_backend)),
      renderer_backend_(std::move(renderer_backend)) {}

int Application::Run() const {
  const StartupPaths paths = DiscoverStartupPaths();
  Logger logger(paths.log_file);

  if (!logger.IsReady()) {
    return 1;
  }

  logger.Info("CookieRuntime startup sequence initialized.");
  logger.Info("Application: " + config_.application_name);
  logger.Info("Selected renderer backend: " + config_.renderer_backend_name);
  if (!config_.renderer_runtime_source.empty()) {
    logger.Info("Renderer runtime source: " + config_.renderer_runtime_source);
  }
  if (!config_.renderer_module_path.empty()) {
    logger.Info("Renderer module path: " + config_.renderer_module_path);
  }
  if (!config_.physics_runtime_source.empty()) {
    logger.Info("Physics runtime source: " + config_.physics_runtime_source);
  }
  if (!config_.physics_module_path.empty()) {
    logger.Info("Physics module path: " + config_.physics_module_path);
  }
  logger.Info("Window title: " + config_.window_title);
  logger.Info("Window size: " + std::to_string(config_.window_width) + "x" +
              std::to_string(config_.window_height));
  logger.Info("Project root: " + paths.project_root.string());
  logger.Info("Config directory: " + paths.config_dir.string());
  logger.Info("Engine config: " + paths.engine_config.string());
  logger.Info("Graphics config: " + paths.graphics_config.string());
  logger.Info("Input config: " + paths.input_config.string());
  logger.Info("Game config: " + paths.game_config.string());

  GameLogicModule game_logic;
  const auto game_logic_from_repo = paths.project_root / "bin" / "GameLogic.dll";
  const auto game_logic_from_runtime =
      cookie::platform::GetWorkingDirectory() / "GameLogic.dll";
  if (game_logic.Load(game_logic_from_repo)) {
    logger.Info("Loaded game logic module: " + game_logic_from_repo.string());
  } else if (game_logic.Load(game_logic_from_runtime)) {
    logger.Info("Loaded game logic module: " + game_logic_from_runtime.string());
  } else {
    logger.Info("Game logic module not loaded. Looked in repo/bin and runtime directory.");
  }

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
  if (!physics_backend_) {
    logger.Error("No physics backend instance is available.");
    return 3;
  }

  logger.Info("Initializing physics backend.");
  if (!physics_backend_->Initialize()) {
    logger.Error("Physics backend failed to initialize.");
    return 4;
  }
  logger.Info("Physics backend initialized successfully.");

  logger.Info("Initializing renderer backend: " +
              std::string(renderer_backend_->Name()));
  if (!renderer_backend_->Initialize()) {
    logger.Error("Renderer backend failed to initialize.");
    physics_backend_->Shutdown();
    return 5;
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
    physics_backend_->Shutdown();
    return 6;
  }

  logger.Info("Platform window created successfully.");
  logger.Info("Starting frame loop.");

  if (game_logic.IsLoaded()) {
    if (game_logic.Startup()) {
      logger.Info("Game logic startup completed.");
    } else {
      logger.Error("Game logic startup reported failure.");
    }
  }

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
    const cookie::core::PhysicsStepStats step_stats =
        physics_backend_->StepSimulation({
            .delta_time_seconds = 1.0f / 60.0f,
        });

    if (game_logic.IsLoaded()) {
      game_logic.Update(1.0f / 60.0f);
    }

    if (frame_count == 1) {
      const bool has_jolt = step_stats.using_jolt_headers;
      logger.Info("Physics backend using Jolt headers: " +
                  std::string(has_jolt ? "true" : "false"));
      if (!has_jolt) {
        logger.Info(
            "Hint: configure/build with x64-debug-vcpkg preset to enable vcpkg Jolt integration.");
      }
    }

    if (config_.max_frames > 0 && frame_count >= config_.max_frames) {
      logger.Info("Frame loop reached max_frames, requesting shutdown.");
      window->RequestClose();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  logger.Info("Frame loop ended after " + std::to_string(frame_count) + " frames.");
  if (game_logic.IsLoaded()) {
    game_logic.Shutdown();
    game_logic.Unload();
    logger.Info("Game logic shutdown completed.");
  }
  renderer_backend_->Shutdown();
  logger.Info("Renderer backend shut down successfully.");
  physics_backend_->Shutdown();
  logger.Info("Physics backend shut down successfully.");
  logger.Info("Phase 10 skeleton complete. Renderer and physics module loader contracts wired.");

  return 0;
}

} // namespace cookie::core

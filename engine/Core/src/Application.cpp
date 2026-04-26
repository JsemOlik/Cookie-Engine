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
#include "Cookie/Renderer/Primitives.h"
#include "Cookie/Renderer/SceneBuilder.h"
#include "Cookie/Renderer/Transform.h"

namespace cookie::core {

Application::Application(
    ApplicationConfig config,
    std::unique_ptr<cookie::core::IAudioBackend> audio_backend,
    std::unique_ptr<cookie::core::IPhysicsBackend> physics_backend,
    std::unique_ptr<cookie::renderer::IRendererBackend> renderer_backend)
    : config_(std::move(config)),
      audio_backend_(std::move(audio_backend)),
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
  logger.Info("Strict module mode: " +
              std::string(config_.strict_module_mode ? "true" : "false"));
  if (!config_.core_runtime_source.empty()) {
    logger.Info("Core runtime source: " + config_.core_runtime_source);
  }
  if (!config_.core_module_path.empty()) {
    logger.Info("Core module path: " + config_.core_module_path);
  }
  if (!config_.core_module_name.empty()) {
    logger.Info("Core module name: " + config_.core_module_name);
  }
  if (config_.core_module_api_version > 0) {
    logger.Info("Core module API version: " +
                std::to_string(config_.core_module_api_version));
  }
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
  if (!config_.audio_runtime_source.empty()) {
    logger.Info("Audio runtime source: " + config_.audio_runtime_source);
  }
  if (!config_.audio_module_path.empty()) {
    logger.Info("Audio module path: " + config_.audio_module_path);
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
  if (!config_.engine_config_path.empty() &&
      config_.engine_config_path != paths.engine_config.string()) {
    logger.Info("Engine config override in use: " + config_.engine_config_path);
  }

  if (config_.require_core_module && config_.core_runtime_source != "module") {
    logger.Error("Core module is required but runtime is in fallback mode.");
    return 2;
  }
  if (config_.require_renderer_module &&
      config_.renderer_runtime_source != "module") {
    logger.Error("Renderer module is required but runtime is in fallback mode.");
    return 3;
  }
  if (config_.require_physics_module && config_.physics_runtime_source != "module") {
    logger.Error("Physics module is required but runtime is in fallback mode.");
    return 4;
  }
  if (config_.require_audio_module && config_.audio_runtime_source != "module") {
    logger.Error("Audio module is required but runtime is in fallback mode.");
    return 5;
  }

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
    return 6;
  }
  if (!audio_backend_) {
    logger.Error("No audio backend instance is available.");
    return 7;
  }
  if (!physics_backend_) {
    logger.Error("No physics backend instance is available.");
    return 8;
  }

  logger.Info("Initializing audio backend.");
  if (!audio_backend_->Initialize()) {
    logger.Error("Audio backend failed to initialize.");
    return 9;
  }
  logger.Info("Audio backend initialized successfully.");

  logger.Info("Initializing physics backend.");
  if (!physics_backend_->Initialize()) {
    logger.Error("Physics backend failed to initialize.");
    audio_backend_->Shutdown();
    return 10;
  }
  logger.Info("Physics backend initialized successfully.");

  auto window = cookie::platform::CreatePlatformWindow({
      .title = config_.window_title,
      .width = config_.window_width,
      .height = config_.window_height,
  });
  if (!window) {
    logger.Error("Platform window creation failed.");
    renderer_backend_->Shutdown();
    physics_backend_->Shutdown();
    audio_backend_->Shutdown();
    return 12;
  }

  logger.Info("Platform window created successfully.");

  logger.Info("Initializing renderer backend: " +
              std::string(renderer_backend_->Name()));
  const cookie::renderer::RendererInitInfo renderer_init{
      .native_window_handle = window->GetNativeHandle(),
      .window_width = config_.window_width,
      .window_height = config_.window_height,
      .enable_vsync = true,
  };
  if (!renderer_backend_->Initialize(renderer_init)) {
    logger.Error("Renderer backend failed to initialize.");
    physics_backend_->Shutdown();
    audio_backend_->Shutdown();
    return 11;
  }
  logger.Info("Renderer backend initialized successfully.");

  logger.Info("Starting frame loop.");

  if (game_logic.IsLoaded()) {
    if (game_logic.Startup()) {
      logger.Info("Game logic startup completed.");
    } else {
      logger.Error("Game logic startup reported failure.");
    }
  }

  int frame_count = 0;
  cookie::renderer::SceneBuilder scene_builder;
  const auto cube_vertices = cookie::renderer::MakeColoredCubeVertices();
  const auto cube_indices = cookie::renderer::MakeCubeIndices();
  const float aspect_ratio =
      (config_.window_height > 0)
          ? static_cast<float>(config_.window_width) /
                static_cast<float>(config_.window_height)
          : 1.0f;
  const auto view = cookie::renderer::MakeLookAtView(
      4.0f, 3.0f, -7.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  const auto projection = cookie::renderer::MakeOrthographicProjection(
      5.0f * aspect_ratio, 5.0f, 0.01f, 100.0f);
  const auto cube_transform =
      cookie::renderer::MultiplyTransforms(view, projection);
  while (!window->ShouldClose()) {
    window->PollEvents();

    if (!renderer_backend_->BeginFrame()) {
      logger.Error("Renderer backend BeginFrame failed.");
      window->RequestClose();
      break;
    }

    renderer_backend_->Clear(config_.clear_color);
    scene_builder.Reset();
    scene_builder.AddIndexedMeshInstance(
        cube_vertices.data(), cube_vertices.size(),
        cube_indices.data(), cube_indices.size(), cube_transform);
    renderer_backend_->SubmitScene(scene_builder.Build());
    renderer_backend_->EndFrame();

    ++frame_count;
    const cookie::core::PhysicsStepStats step_stats =
        physics_backend_->StepSimulation({
            .delta_time_seconds = 1.0f / 60.0f,
        });
    audio_backend_->Update({
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
  audio_backend_->Shutdown();
  logger.Info("Audio backend shut down successfully.");
  logger.Info("Phase 24 complete. Static indexed cube render path wired.");

  return 0;
}

} // namespace cookie::core

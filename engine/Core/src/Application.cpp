#include "Cookie/Core/Application.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/GameLogicModule.h"
#include "Cookie/Core/Logger.h"
#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Assets/GlbMeshLoader.h"
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
    logger.Info(
        "Core module API version: " + std::to_string(config_.core_module_api_version));
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
  logger.Info("Camera mode: " + config_.camera_mode);
  logger.Info("Camera ortho height: " + std::to_string(config_.camera_ortho_height));
  logger.Info("Camera perspective FOV degrees: " +
              std::to_string(config_.camera_perspective_fov_degrees));
  logger.Info("Camera near/far: " + std::to_string(config_.camera_near_plane) + "/" +
              std::to_string(config_.camera_far_plane));
  logger.Info("Camera orbit enabled: " +
              std::string(config_.camera_orbit_enabled ? "true" : "false"));
  logger.Info("Camera orbit radius/height/speed: " +
              std::to_string(config_.camera_orbit_radius) + "/" +
              std::to_string(config_.camera_orbit_height) + "/" +
              std::to_string(config_.camera_orbit_speed));
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

  if (config_.require_core_module &&
      config_.core_runtime_source != "module") {
    logger.Error(
        "Core module is required but runtime is in fallback mode.");
    return 2;
  }
  if (config_.require_renderer_module &&
      config_.renderer_runtime_source != "module") {
    logger.Error(
        "Renderer module is required but runtime is in fallback mode.");
    return 3;
  }
  if (config_.require_physics_module &&
      config_.physics_runtime_source != "module") {
    logger.Error(
        "Physics module is required but runtime is in fallback mode.");
    return 4;
  }
  if (config_.require_audio_module &&
      config_.audio_runtime_source != "module") {
    logger.Error(
        "Audio module is required but runtime is in fallback mode.");
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
  const auto cube_vertices = cookie::renderer::MakeColoredCubeVertices(0.5f);
  const auto cube_indices = cookie::renderer::MakeCubeTriangleIndices();
  std::vector<cookie::renderer::SceneVertex> loaded_vertices;
  std::vector<std::uint32_t> loaded_indices;
  bool using_loaded_glb = false;
  {
    const auto glb_path = paths.project_root / "content" / "models" / "test_mesh.glb";
    cookie::assets::GlbMeshData glb_mesh;
    std::string glb_error;
    if (cookie::assets::LoadFirstMeshFromGlb(glb_path, glb_mesh, &glb_error) &&
        !glb_mesh.positions_xyz.empty()) {
      const std::size_t vertex_count = glb_mesh.positions_xyz.size() / 3;
      loaded_vertices.resize(vertex_count);
      float min_x = std::numeric_limits<float>::max();
      float min_y = std::numeric_limits<float>::max();
      float min_z = std::numeric_limits<float>::max();
      float max_x = std::numeric_limits<float>::lowest();
      float max_y = std::numeric_limits<float>::lowest();
      float max_z = std::numeric_limits<float>::lowest();
      for (std::size_t i = 0; i < vertex_count; ++i) {
        const float x = glb_mesh.positions_xyz[i * 3 + 0];
        const float y = glb_mesh.positions_xyz[i * 3 + 1];
        const float z = glb_mesh.positions_xyz[i * 3 + 2];
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        min_z = std::min(min_z, z);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
        max_z = std::max(max_z, z);
      }
      const float center_x = (min_x + max_x) * 0.5f;
      const float center_y = (min_y + max_y) * 0.5f;
      const float center_z = (min_z + max_z) * 0.5f;
      const float extent_x = max_x - min_x;
      const float extent_y = max_y - min_y;
      const float extent_z = max_z - min_z;
      const float max_extent = std::max(extent_x, std::max(extent_y, extent_z));
      const float normalizer = (max_extent > 0.00001f) ? (1.6f / max_extent) : 1.0f;
      for (std::size_t i = 0; i < vertex_count; ++i) {
        const float x = (glb_mesh.positions_xyz[i * 3 + 0] - center_x) * normalizer;
        const float y = (glb_mesh.positions_xyz[i * 3 + 1] - center_y) * normalizer;
        const float z = (glb_mesh.positions_xyz[i * 3 + 2] - center_z) * normalizer;
        loaded_vertices[i].position[0] = x;
        loaded_vertices[i].position[1] = y;
        loaded_vertices[i].position[2] = z;
        loaded_vertices[i].color[0] = 0.35f + std::abs(x) * 0.65f;
        loaded_vertices[i].color[1] = 0.35f + std::abs(y) * 0.65f;
        loaded_vertices[i].color[2] = 0.35f + std::abs(z) * 0.65f;
        loaded_vertices[i].color[3] = 1.0f;
      }

      loaded_indices = glb_mesh.indices;
      if (loaded_indices.empty()) {
        loaded_indices.resize(vertex_count);
        for (std::size_t i = 0; i < vertex_count; ++i) {
          loaded_indices[i] = static_cast<std::uint32_t>(i);
        }
      }

      using_loaded_glb = true;
      logger.Info("Loaded GLB mesh: " + glb_path.string());
      logger.Info("GLB mesh stats - vertices: " +
                  std::to_string(loaded_vertices.size()) +
                  ", indices: " + std::to_string(loaded_indices.size()));
    } else {
      logger.Info("GLB mesh not loaded: " + glb_path.string());
      if (!glb_error.empty()) {
        logger.Info("GLB load reason: " + glb_error);
      }
    }
  }
  const float aspect_ratio =
      (config_.window_height > 0)
          ? static_cast<float>(config_.window_width) /
                static_cast<float>(config_.window_height)
          : 1.0f;
  const bool use_perspective = config_.camera_mode == "perspective";
  const float near_plane = config_.camera_near_plane;
  const float far_plane = config_.camera_far_plane;
  const float ortho_height =
      (config_.camera_ortho_height > 0.0f) ? config_.camera_ortho_height : 2.8f;
  const float perspective_fov_radians =
      config_.camera_perspective_fov_degrees * (3.1415926535f / 180.0f);
  const bool perspective_params_valid =
      perspective_fov_radians > 0.0f && near_plane > 0.0f && far_plane != near_plane;
  const auto view_projection =
      (use_perspective && perspective_params_valid)
          ? cookie::renderer::MakePerspectiveProjection(
                perspective_fov_radians, aspect_ratio, near_plane, far_plane)
          : cookie::renderer::MakeOrthographicProjection(
                ortho_height * aspect_ratio, ortho_height, near_plane, far_plane);
  while (!window->ShouldClose()) {
    window->PollEvents();

    if (!renderer_backend_->BeginFrame()) {
      logger.Error("Renderer backend BeginFrame failed.");
      window->RequestClose();
      break;
    }

    renderer_backend_->Clear(config_.clear_color);
    const float orbit_angle = static_cast<float>(frame_count) *
        (config_.camera_orbit_speed * (1.0f / 60.0f));
    const float camera_x = config_.camera_orbit_enabled
                               ? std::cos(orbit_angle) * config_.camera_orbit_radius
                               : 0.0f;
    const float camera_y = config_.camera_orbit_enabled
                               ? config_.camera_orbit_height
                               : 0.0f;
    const float camera_z = config_.camera_orbit_enabled
                               ? std::sin(orbit_angle) * config_.camera_orbit_radius
                               : -config_.camera_orbit_radius;
    const auto view = cookie::renderer::MakeLookAtView(
        camera_x, camera_y, camera_z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    scene_builder.Reset(cookie::renderer::MultiplyTransforms(view, view_projection));
    if (using_loaded_glb) {
      const auto glb_model =
          cookie::renderer::MultiplyTransforms(
              cookie::renderer::MakeTranslationTransform(0.0f, 0.0f, 0.0f),
              cookie::renderer::MultiplyTransforms(
                  cookie::renderer::MakeZRotationTransform(
                      static_cast<float>(frame_count) * 0.0125f),
                  cookie::renderer::MakeScaleTransform(1.0f, 1.0f, 1.0f)));
      scene_builder.AddIndexedMeshInstance(
          loaded_vertices.data(), loaded_vertices.size(), loaded_indices.data(),
          loaded_indices.size(), glb_model);
    } else {
      const auto left_cube_model =
          cookie::renderer::MultiplyTransforms(
              cookie::renderer::MakeTranslationTransform(-0.7f, 0.0f, -0.25f),
              cookie::renderer::MultiplyTransforms(
                  cookie::renderer::MakeZRotationTransform(
                      static_cast<float>(frame_count) * 0.01f),
                  cookie::renderer::MakeScaleTransform(0.8f, 0.8f, 1.0f)));
      const auto right_cube_model =
          cookie::renderer::MultiplyTransforms(
              cookie::renderer::MakeTranslationTransform(0.75f, 0.0f, 0.35f),
              cookie::renderer::MultiplyTransforms(
                  cookie::renderer::MakeZRotationTransform(
                      static_cast<float>(frame_count) * -0.015f),
                  cookie::renderer::MakeScaleTransform(0.5f, 0.5f, 1.0f)));
      scene_builder.AddIndexedMeshInstance(
          cube_vertices.data(), cube_vertices.size(),
          cube_indices.data(), cube_indices.size(), left_cube_model);
      scene_builder.AddIndexedMeshInstance(
          cube_vertices.data(), cube_vertices.size(),
          cube_indices.data(), cube_indices.size(), right_cube_model);
    }
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
  logger.Info("Phase 23 complete. Indexed cube render path wired.");

  return 0;
}

} // namespace cookie::core

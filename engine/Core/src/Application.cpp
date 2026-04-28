#include "Cookie/Core/Application.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>
#include <utility>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/GameLogicModule.h"
#include "Cookie/Core/Logger.h"
#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Platform/PlatformPaths.h"
#include "Cookie/Platform/PlatformWindow.h"
#include "Cookie/Renderer/MeshAsset.h"
#include "Cookie/Renderer/Primitives.h"
#include "Cookie/Renderer/SceneBuilder.h"
#include "Cookie/Renderer/Transform.h"

namespace cookie::core {
namespace {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

Vec3 Add(const Vec3& a, const Vec3& b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 Subtract(const Vec3& a, const Vec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 Scale(const Vec3& value, float scale) {
  return {value.x * scale, value.y * scale, value.z * scale};
}

float Length(const Vec3& value) {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 Normalize(const Vec3& value) {
  const float length = Length(value);
  if (length <= 0.000001f) {
    return {};
  }
  return Scale(value, 1.0f / length);
}

Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

} // namespace

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
  logger.Info("Camera mode: " + config_.camera_mode);
  logger.Info("Demo albedo asset id: " + config_.demo_albedo_asset_id);
  logger.Info("Fly camera controls: mouse look, WASD move, Q/E vertical, Shift speed.");
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
  const auto cooked_registry = paths.project_root / "content" / "cooked_assets.pakreg";
  if (assets.LoadCookedRegistry(cooked_registry)) {
    logger.Info("Loaded cooked asset registry: " + cooked_registry.string());
  } else {
    logger.Info("Cooked asset registry not found or unreadable: " +
                cooked_registry.string());
  }
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
  const auto frame_start_time = std::chrono::steady_clock::now();
  auto last_frame_time = frame_start_time;
  std::filesystem::path resolved_demo_albedo_texture_path;
  if (const auto cooked_record =
          assets.ResolveCookedRecord(config_.demo_albedo_asset_id)) {
    resolved_demo_albedo_texture_path =
        paths.project_root / "content" / cooked_record->runtime_relative_path;
    logger.Info("Resolved demo albedo from cooked registry asset id '" +
                config_.demo_albedo_asset_id + "' to '" +
                resolved_demo_albedo_texture_path.string() + "'.");
  } else {
    resolved_demo_albedo_texture_path =
        assets.ResolveAssetPath(config_.demo_albedo_asset_id);
    if (resolved_demo_albedo_texture_path.empty()) {
      // Transitional fallback while phase 24A cooks registry artifacts.
      resolved_demo_albedo_texture_path = config_.demo_albedo_asset_id;
      if (!resolved_demo_albedo_texture_path.empty() &&
          resolved_demo_albedo_texture_path.is_relative()) {
        resolved_demo_albedo_texture_path =
            paths.project_root / resolved_demo_albedo_texture_path;
      }
      logger.Info("Cooked registry missing asset id; fallback path used: " +
                  resolved_demo_albedo_texture_path.string());
    } else {
      logger.Info("Resolved demo albedo asset id via mounted package/source: '" +
                  config_.demo_albedo_asset_id + "' -> '" +
                  resolved_demo_albedo_texture_path.string() + "'.");
    }
  }
  const std::string resolved_demo_albedo_texture =
      resolved_demo_albedo_texture_path.string();
  cookie::renderer::SceneBuilder scene_builder;
  cookie::renderer::ImportedMesh imported_mesh;
  bool has_imported_mesh = false;
  {
    const auto imported_mesh_path = paths.project_root / "content" / "models" /
                                    "test_mesh.glb";
    std::string imported_mesh_error;
    has_imported_mesh = cookie::renderer::LoadMeshFromPath(
        imported_mesh_path, imported_mesh, &imported_mesh_error);
    if (has_imported_mesh) {
      logger.Info("Loaded imported mesh: " + imported_mesh_path.string() +
                  " (primitives: " +
                  std::to_string(imported_mesh.primitives.size()) + ")");
    } else {
      logger.Info("Imported mesh load skipped/fallback in use: " +
                  imported_mesh_path.string() + " (" + imported_mesh_error + ")");
    }
  }
  const auto cube_vertices = cookie::renderer::MakeColoredCubeVertices();
  const auto cube_indices = cookie::renderer::MakeCubeIndices();
  const float aspect_ratio =
      (config_.window_height > 0)
          ? static_cast<float>(config_.window_width) /
                static_cast<float>(config_.window_height)
          : 1.0f;
  const bool use_orthographic = (config_.camera_mode == "orthographic");
  const auto projection = use_orthographic
                              ? cookie::renderer::MakeOrthographicProjection(
                                    5.0f * aspect_ratio, 5.0f, 0.01f, 100.0f)
                              : cookie::renderer::MakePerspectiveProjection(
                                    1.04719755f, aspect_ratio, 0.01f, 100.0f);

  Vec3 camera_position{4.0f, 3.0f, -7.0f};
  const Vec3 world_up{0.0f, 1.0f, 0.0f};
  const Vec3 initial_target{0.0f, 0.0f, 0.0f};
  const Vec3 initial_forward = Normalize(Subtract(initial_target, camera_position));
  float yaw_radians = std::atan2(initial_forward.x, initial_forward.z);
  float pitch_radians = std::asin(initial_forward.y);
  constexpr float kMouseSensitivity = 0.0025f;
  constexpr float kMoveSpeed = 4.5f;
  constexpr float kFastMoveSpeed = 11.0f;

  while (!window->ShouldClose()) {
    window->PollEvents();
    if (window->IsKeyDown(cookie::platform::KeyCode::Escape)) {
      window->RequestClose();
      continue;
    }

    const auto now = std::chrono::steady_clock::now();
    float delta_time_seconds =
        std::chrono::duration<float>(now - last_frame_time).count();
    last_frame_time = now;
    if (delta_time_seconds <= 0.0f) {
      delta_time_seconds = 1.0f / 60.0f;
    }
    delta_time_seconds = std::min(delta_time_seconds, 0.1f);

    float mouse_delta_x = 0.0f;
    float mouse_delta_y = 0.0f;
    window->ConsumeMouseDelta(mouse_delta_x, mouse_delta_y);
    yaw_radians += mouse_delta_x * kMouseSensitivity;
    pitch_radians -= mouse_delta_y * kMouseSensitivity;
    pitch_radians = std::clamp(pitch_radians, -1.54f, 1.54f);

    const Vec3 forward = Normalize({
        std::cos(pitch_radians) * std::sin(yaw_radians),
        std::sin(pitch_radians),
        std::cos(pitch_radians) * std::cos(yaw_radians),
    });
    const Vec3 right = Normalize(Cross(world_up, forward));

    Vec3 movement{};
    if (window->IsKeyDown(cookie::platform::KeyCode::W)) {
      movement = Add(movement, forward);
    }
    if (window->IsKeyDown(cookie::platform::KeyCode::S)) {
      movement = Subtract(movement, forward);
    }
    if (window->IsKeyDown(cookie::platform::KeyCode::D)) {
      movement = Add(movement, right);
    }
    if (window->IsKeyDown(cookie::platform::KeyCode::A)) {
      movement = Subtract(movement, right);
    }
    if (window->IsKeyDown(cookie::platform::KeyCode::E)) {
      movement = Add(movement, world_up);
    }
    if (window->IsKeyDown(cookie::platform::KeyCode::Q)) {
      movement = Subtract(movement, world_up);
    }

    const float movement_length = Length(movement);
    if (movement_length > 0.0001f) {
      const float speed = window->IsKeyDown(cookie::platform::KeyCode::Shift)
                              ? kFastMoveSpeed
                              : kMoveSpeed;
      const Vec3 direction = Scale(movement, 1.0f / movement_length);
      camera_position = Add(
          camera_position, Scale(direction, speed * delta_time_seconds));
    }

    const Vec3 camera_target = Add(camera_position, forward);
    const auto view = cookie::renderer::MakeLookAtView(
        camera_position.x, camera_position.y, camera_position.z,
        camera_target.x, camera_target.y, camera_target.z,
        world_up.x, world_up.y, world_up.z);
    const auto view_projection =
        cookie::renderer::MultiplyTransforms(view, projection);

    if (!renderer_backend_->BeginFrame()) {
      logger.Error("Renderer backend BeginFrame failed.");
      window->RequestClose();
      break;
    }

    renderer_backend_->Clear(config_.clear_color);
    scene_builder.Reset();
    cookie::renderer::RenderMaterial demo_material{};
    demo_material.albedo_texture_path = resolved_demo_albedo_texture.c_str();
    demo_material.tint[0] = 1.0f;
    demo_material.tint[1] = 1.0f;
    demo_material.tint[2] = 1.0f;
    demo_material.tint[3] = 1.0f;
    demo_material.use_albedo = true;
    const std::size_t demo_material_index =
        scene_builder.AddMaterial(demo_material);
    const float elapsed_seconds =
        std::chrono::duration<float>(now - frame_start_time).count();
    const float angle = elapsed_seconds;
    const auto world =
        cookie::renderer::MultiplyTransforms(
            cookie::renderer::MakeXRotationTransform(angle * 0.9f),
            cookie::renderer::MultiplyTransforms(
                cookie::renderer::MakeYRotationTransform(angle * 0.65f),
                cookie::renderer::MakeZRotationTransform(angle * 0.45f)));
    const auto mesh_transform =
        cookie::renderer::MultiplyTransforms(world, view_projection);
    if (has_imported_mesh) {
      for (const auto& primitive : imported_mesh.primitives) {
        if (!primitive.indices.empty()) {
          scene_builder.AddIndexedMeshInstance(
              primitive.vertices.data(), primitive.vertices.size(),
              primitive.indices.data(), primitive.indices.size(),
              demo_material_index, mesh_transform);
        } else {
          scene_builder.AddMeshInstance(
              primitive.vertices.data(), primitive.vertices.size(),
              demo_material_index, mesh_transform);
        }
      }
    } else {
      scene_builder.AddIndexedMeshInstance(
          cube_vertices.data(), cube_vertices.size(), cube_indices.data(),
          cube_indices.size(), demo_material_index, mesh_transform);
    }
    renderer_backend_->SubmitScene(scene_builder.Build());
    renderer_backend_->EndFrame();

    ++frame_count;
    const cookie::core::PhysicsStepStats step_stats =
        physics_backend_->StepSimulation({
            .delta_time_seconds = delta_time_seconds,
        });
    audio_backend_->Update({
        .delta_time_seconds = delta_time_seconds,
    });

    if (game_logic.IsLoaded()) {
      game_logic.Update(delta_time_seconds);
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
  logger.Info("Phase 25 complete. Materials + textures baseline wired.");

  return 0;
}

} // namespace cookie::core

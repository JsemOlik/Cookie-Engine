#include "Cookie/Core/Application.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>

#include "Cookie/Core/ConfigPaths.h"
#include "Cookie/Core/GameLogicModule.h"
#include "Cookie/Core/Logger.h"
#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Assets/MaterialAsset.h"
#include "Cookie/Assets/SceneAsset.h"
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

std::filesystem::path ResolveAssetPathFromId(
    const cookie::assets::AssetRegistry& assets,
    const StartupPaths& paths, std::string_view asset_id) {
  if (const auto cooked_record = assets.ResolveCookedRecord(asset_id)) {
    return paths.project_root / "content" / cooked_record->runtime_relative_path;
  }
  return assets.ResolveAssetPath(asset_id);
}

bool IsTypeOneOf(const std::string& type,
                 std::initializer_list<std::string_view> allowed) {
  for (const auto allowed_type : allowed) {
    if (type == allowed_type) {
      return true;
    }
  }
  return false;
}

bool ValidateAssetType(
    const cookie::assets::AssetRegistry& assets, std::string_view asset_id,
    std::initializer_list<std::string_view> expected_types, Logger& logger,
    const std::string& context) {
  if (asset_id.empty()) {
    logger.Error("Missing AssetId for " + context + ".");
    return false;
  }
  const auto record = assets.ResolveCookedRecord(asset_id);
  if (!record.has_value()) {
    logger.Error("Missing cooked asset for " + context + " AssetId '" +
                 std::string(asset_id) + "'.");
    return false;
  }
  if (!IsTypeOneOf(record->type, expected_types)) {
    logger.Error("Asset type mismatch for " + context + " AssetId '" +
                 std::string(asset_id) + "': got '" + record->type + "'.");
    return false;
  }
  return true;
}

void AddSceneAndNestedSceneObjects(
    const cookie::assets::AssetRegistry& assets, const StartupPaths& paths,
    std::string_view root_scene_asset_id,
    std::vector<cookie::assets::SceneAssetObject>& out_objects,
    Logger& logger, bool& out_had_errors) {
  std::deque<std::string> pending_scene_ids;
  std::set<std::string> visited_scene_ids;
  pending_scene_ids.push_back(std::string(root_scene_asset_id));

  while (!pending_scene_ids.empty()) {
    const std::string scene_asset_id = pending_scene_ids.front();
    pending_scene_ids.pop_front();
    if (scene_asset_id.empty() ||
        visited_scene_ids.find(scene_asset_id) != visited_scene_ids.end()) {
      continue;
    }
    visited_scene_ids.insert(scene_asset_id);
    if (!ValidateAssetType(
            assets, scene_asset_id, {"Scene", "SceneAsset"}, logger,
            "scene")) {
      out_had_errors = true;
      continue;
    }

    const auto scene_path = ResolveAssetPathFromId(assets, paths, scene_asset_id);
    if (scene_path.empty()) {
      logger.Error("Failed to resolve scene asset id: " + scene_asset_id);
      out_had_errors = true;
      continue;
    }

    cookie::assets::SceneAsset scene_asset;
    std::string scene_error;
    if (!cookie::assets::LoadSceneAsset(scene_path, scene_asset, &scene_error)) {
      logger.Error(scene_error);
      out_had_errors = true;
      continue;
    }

    logger.Info("Loaded scene asset '" + scene_asset_id + "' from '" +
                scene_path.string() + "' with objects: " +
                std::to_string(scene_asset.objects.size()));
    out_objects.insert(
        out_objects.end(), scene_asset.objects.begin(), scene_asset.objects.end());
    for (const std::string& nested_scene_id : scene_asset.nested_scene_asset_ids) {
      if (!nested_scene_id.empty() &&
          !ValidateAssetType(
              assets, nested_scene_id, {"Scene", "SceneAsset"}, logger,
              "nested_scene")) {
        out_had_errors = true;
      }
      pending_scene_ids.push_back(nested_scene_id);
    }
  }
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
  logger.Info("Startup scene asset id: " + config_.startup_scene_asset_id);
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
        assets.ResolveAssetPath(config_.demo_albedo_asset_id);
    if (!resolved_demo_albedo_texture_path.empty()) {
      logger.Info("Resolved demo albedo from cooked registry asset id '" +
                  config_.demo_albedo_asset_id + "' to '" +
                  resolved_demo_albedo_texture_path.string() + "'.");
    } else {
      logger.Info("Cooked record exists but payload resolve failed for asset id '" +
                  config_.demo_albedo_asset_id + "'.");
    }
  }
  if (resolved_demo_albedo_texture_path.empty()) {
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
    } else if (assets.ResolveCookedRecord(config_.demo_albedo_asset_id).has_value()) {
      logger.Info("Resolved demo albedo asset id via archive extraction fallback: '" +
                  config_.demo_albedo_asset_id + "' -> '" +
                  resolved_demo_albedo_texture_path.string() + "'.");
    } else {
      logger.Info("Resolved demo albedo asset id via mounted package/source: '" +
                  config_.demo_albedo_asset_id + "' -> '" +
                  resolved_demo_albedo_texture_path.string() + "'.");
    }
  }
  const std::string resolved_demo_albedo_texture =
      resolved_demo_albedo_texture_path.string();
  cookie::renderer::SceneBuilder scene_builder;
  std::vector<cookie::assets::SceneAssetObject> startup_scene_objects;
  bool startup_scene_errors = false;
  AddSceneAndNestedSceneObjects(
      assets, paths, config_.startup_scene_asset_id, startup_scene_objects, logger,
      startup_scene_errors);
  logger.Info("Startup scene resolved total objects: " +
              std::to_string(startup_scene_objects.size()));
  if (!config_.startup_scene_asset_id.empty() && startup_scene_objects.empty()) {
    logger.Error("Startup scene resolved zero objects; startup scene is authoritative.");
    return 13;
  }
  if (startup_scene_errors) {
    logger.Error("Startup scene validation failed.");
    return 14;
  }

  struct LoadedScenePrimitive {
    std::vector<cookie::renderer::SceneVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::string albedo_texture_path;
    cookie::renderer::Float4x4 object_transform =
        cookie::renderer::MakeIdentityTransform();
  };
  std::vector<LoadedScenePrimitive> loaded_scene_primitives;
  for (const auto& object : startup_scene_objects) {
    if (!ValidateAssetType(
            assets, object.mesh_renderer.mesh_asset_id, {"Mesh", "Model"}, logger,
            "scene mesh")) {
      startup_scene_errors = true;
      continue;
    }
    if (!ValidateAssetType(
            assets, object.mesh_renderer.material_asset_id,
            {"Material", "MaterialAsset"}, logger, "scene material")) {
      startup_scene_errors = true;
      continue;
    }

    const auto mesh_path = ResolveAssetPathFromId(
        assets, paths, object.mesh_renderer.mesh_asset_id);
    if (mesh_path.empty()) {
      logger.Error("Scene object '" + object.name +
                   "' failed mesh resolve for asset id: " +
                   object.mesh_renderer.mesh_asset_id);
      startup_scene_errors = true;
      continue;
    }

    cookie::renderer::ImportedMesh imported_mesh;
    std::string imported_mesh_error;
    if (!cookie::renderer::LoadMeshFromPath(
            mesh_path, imported_mesh, &imported_mesh_error)) {
      logger.Error("Scene object '" + object.name + "' failed mesh load: " +
                   imported_mesh_error);
      startup_scene_errors = true;
      continue;
    }

    std::string resolved_albedo_texture = resolved_demo_albedo_texture;
    const auto material_path =
        ResolveAssetPathFromId(
            assets, paths, object.mesh_renderer.material_asset_id);
    if (!material_path.empty()) {
      cookie::assets::MaterialAsset material_asset;
      std::string material_error;
      if (cookie::assets::LoadMaterialAsset(
              material_path, material_asset, &material_error) &&
          !material_asset.albedo_asset_id.empty()) {
        const auto texture_path =
            ResolveAssetPathFromId(assets, paths, material_asset.albedo_asset_id);
        if (!texture_path.empty()) {
          resolved_albedo_texture = texture_path.string();
        }
      } else if (!material_error.empty()) {
        logger.Info(material_error);
      }
    }

    for (const auto& primitive : imported_mesh.primitives) {
      LoadedScenePrimitive loaded{};
      loaded.vertices = primitive.vertices;
      loaded.indices = primitive.indices;
      loaded.albedo_texture_path = resolved_albedo_texture;
      loaded.object_transform =
          cookie::renderer::MultiplyTransforms(
              cookie::renderer::MakeScaleTransform(
                  object.transform.scale[0], object.transform.scale[1],
                  object.transform.scale[2]),
              cookie::renderer::MakeTranslationTransform(
                  object.transform.position[0], object.transform.position[1],
                  object.transform.position[2]));
      loaded_scene_primitives.push_back(std::move(loaded));
    }
  }
  if (startup_scene_errors) {
    logger.Error("Scene object reference validation failed.");
    return 15;
  }
  logger.Info("Loaded scene primitives from startup scenes: " +
              std::to_string(loaded_scene_primitives.size()));
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
    if (!loaded_scene_primitives.empty()) {
      for (const auto& primitive : loaded_scene_primitives) {
        cookie::renderer::RenderMaterial material{};
        material.albedo_texture_path = primitive.albedo_texture_path.c_str();
        material.tint[0] = 1.0f;
        material.tint[1] = 1.0f;
        material.tint[2] = 1.0f;
        material.tint[3] = 1.0f;
        material.use_albedo = true;
        const std::size_t material_index = scene_builder.AddMaterial(material);
        if (!primitive.indices.empty()) {
          scene_builder.AddIndexedMeshInstance(
              primitive.vertices.data(), primitive.vertices.size(),
              primitive.indices.data(), primitive.indices.size(), material_index,
              cookie::renderer::MultiplyTransforms(
                  primitive.object_transform, mesh_transform));
        } else {
          scene_builder.AddMeshInstance(
              primitive.vertices.data(), primitive.vertices.size(), material_index,
              cookie::renderer::MultiplyTransforms(
                  primitive.object_transform, mesh_transform));
        }
      }
    } else if (config_.startup_scene_asset_id.empty()) {
      cookie::renderer::RenderMaterial demo_material{};
      demo_material.albedo_texture_path = resolved_demo_albedo_texture.c_str();
      demo_material.tint[0] = 1.0f;
      demo_material.tint[1] = 1.0f;
      demo_material.tint[2] = 1.0f;
      demo_material.tint[3] = 1.0f;
      demo_material.use_albedo = true;
      const std::size_t demo_material_index =
          scene_builder.AddMaterial(demo_material);
      scene_builder.AddIndexedMeshInstance(
          cube_vertices.data(), cube_vertices.size(), cube_indices.data(),
          cube_indices.size(), demo_material_index, mesh_transform);
    } else {
      logger.Error(
          "Startup scene was configured but produced no renderable primitives.");
      window->RequestClose();
      break;
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

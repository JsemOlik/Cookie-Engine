#include "Cookie/Tools/BuildPipeline.h"

#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Assets/CookedAssetRegistry.h"
#include "Cookie/Assets/MaterialAsset.h"
#include "Cookie/Assets/SceneAsset.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace cookie::tools {
namespace {

void AddWarning(ExportResult& result, const std::string& message) {
  result.warnings.push_back(ExportWarning{message});
}

bool CopyFileIfExists(
    const fs::path& source, const fs::path& destination, ExportResult& result) {
  if (!fs::exists(source)) {
    AddWarning(result, "Missing file: " + source.string());
    return false;
  }

  std::error_code error;
  fs::create_directories(destination.parent_path(), error);
  if (error) {
    AddWarning(
        result,
        "Failed to create destination directory: " +
            destination.parent_path().string());
    return false;
  }

  fs::copy_file(source, destination, fs::copy_options::overwrite_existing, error);
  if (error) {
    AddWarning(
        result,
        "Failed to copy file: " + source.string() + " -> " + destination.string());
    return false;
  }
  return true;
}

void WarnIfMissing(const fs::path& source, ExportResult& result, const char* label) {
  if (!fs::exists(source)) {
    AddWarning(result, std::string(label) + " missing: " + source.string());
  }
}

void CopyDirectoryContents(
    const fs::path& source_dir, const fs::path& destination_dir,
    ExportResult& result) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    AddWarning(result, "Missing directory: " + source_dir.string());
    return;
  }

  std::error_code error;
  fs::create_directories(destination_dir, error);
  if (error) {
    AddWarning(
        result, "Failed to create export directory: " + destination_dir.string());
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
    const auto relative_path = fs::relative(entry.path(), source_dir, error);
    if (error) {
      AddWarning(
          result,
          "Failed to compute relative path for: " + entry.path().string());
      error.clear();
      continue;
    }

    const fs::path destination = destination_dir / relative_path;
    if (entry.is_directory()) {
      fs::create_directories(destination, error);
      if (error) {
        AddWarning(
            result,
            "Failed to create export subdirectory: " + destination.string());
        error.clear();
      }
      continue;
    }

    if (!entry.is_regular_file()) {
      continue;
    }

    fs::create_directories(destination.parent_path(), error);
    if (error) {
      AddWarning(
          result,
          "Failed to create parent directory for: " + destination.string());
      error.clear();
      continue;
    }

    fs::copy_file(
        entry.path(), destination, fs::copy_options::overwrite_existing, error);
    if (error) {
      AddWarning(
          result,
          "Failed to copy file: " + entry.path().string() + " -> " +
              destination.string());
      error.clear();
    }
  }
}

void CopyDirectoryDlls(
    const fs::path& source_dir, const fs::path& destination_dir,
    ExportResult& result) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    AddWarning(result, "Missing directory: " + source_dir.string());
    return;
  }

  std::error_code error;
  fs::create_directories(destination_dir, error);
  if (error) {
    AddWarning(
        result,
        "Failed to create DLL destination directory: " + destination_dir.string());
    return;
  }

  for (const auto& entry : fs::directory_iterator(source_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const fs::path source = entry.path();
    if (source.extension() != ".dll") {
      continue;
    }

    const fs::path destination = destination_dir / source.filename();
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, error);
    if (error) {
      AddWarning(
          result,
          "Failed to copy DLL: " + source.string() + " -> " +
              destination.string());
      error.clear();
    }
  }
}

void ApplyEngineProfile(
    const fs::path& source_config_dir, const fs::path& export_config_dir,
    BuildProfile profile, ExportResult& result) {
  const fs::path profile_file =
      source_config_dir / ("engine." + std::string(ToString(profile)) + ".json");
  const fs::path export_engine = export_config_dir / "engine.json";
  if (!CopyFileIfExists(profile_file, export_engine, result)) {
    AddWarning(
        result,
        "Failed to apply engine profile: " + std::string(ToString(profile)) +
            " (falling back to copied config/engine.json if present)");
  }
}

void RemoveProfileConfigsFromExport(
    const fs::path& export_config_dir, ExportResult& result) {
  const fs::path dev_profile = export_config_dir / "engine.dev.json";
  const fs::path release_profile = export_config_dir / "engine.release.json";

  std::error_code error;
  if (fs::exists(dev_profile) && !fs::remove(dev_profile, error)) {
    AddWarning(
        result, "Failed to remove exported profile file: " + dev_profile.string());
  }
  error.clear();
  if (fs::exists(release_profile) && !fs::remove(release_profile, error)) {
    AddWarning(
        result,
        "Failed to remove exported profile file: " + release_profile.string());
  }
}

void WriteExportReport(
    const fs::path& report_path, const BuildCookPackageOptions& options,
    const ExportResult& result) {
  std::ofstream report(report_path, std::ios::trunc);
  if (!report.is_open()) {
    return;
  }

  report << "Cookie Engine Export Report\n";
  report << "Game: " << options.game_name << "\n";
  report << "Engine profile: " << ToString(options.profile) << "\n";
  report << "Debug logging: " << (options.debug_logging ? "true" : "false")
         << "\n\n";
  report << "Status: " << (result.success ? "success" : "failed") << "\n\n";
  report << "BuildCookPackage API path: active\n\n";

  if (!result.warnings.empty()) {
    report << "Warnings:\n";
    for (const auto& warning : result.warnings) {
      report << " - " << warning.message << "\n";
    }
  }
}

void BuildCookedRegistryArtifact(
    const fs::path& project_root, const fs::path& export_content_dir,
    ExportResult& result) {
  cookie::assets::CookedAssetRegistry registry;
  const fs::path content_root = project_root / "content";

  if (!fs::exists(content_root)) {
    AddWarning(result, "Cannot build cooked registry; missing content directory.");
    return;
  }

  std::error_code error;
  for (const auto& entry : fs::recursive_directory_iterator(content_root, error)) {
    if (error || !entry.is_regular_file()) {
      continue;
    }

    const fs::path meta_path = entry.path();
    const std::string meta_filename = meta_path.filename().string();
    if (meta_filename.size() <= 5 ||
        meta_filename.substr(meta_filename.size() - 5) != ".meta") {
      continue;
    }

    cookie::assets::AssetMeta meta;
    std::string parse_error;
    if (!cookie::assets::LoadAssetMeta(meta_path, meta, &parse_error)) {
      AddWarning(result, parse_error);
      continue;
    }

    const fs::path source_path =
        meta_path.parent_path() / meta_filename.substr(0, meta_filename.size() - 5);
    const fs::path relative_content_path =
        fs::relative(source_path, content_root, error);
    if (error) {
      AddWarning(
          result,
          "Failed to compute content relative path for: " + source_path.string());
      error.clear();
      continue;
    }

    cookie::assets::CookedAssetRecord record{};
    record.asset_id = meta.asset_id;
    record.type = meta.type;
    record.runtime_relative_path = relative_content_path.string();
    record.source_relative_path =
        fs::relative(source_path, project_root, error).string();
    if (error) {
      record.source_relative_path = source_path.string();
      error.clear();
    }
    record.dependencies = meta.dependencies;
    if (record.dependencies.empty()) {
      if (meta.type == "Scene" || meta.type == "SceneAsset") {
        cookie::assets::SceneAsset scene_asset;
        if (cookie::assets::LoadSceneAsset(source_path, scene_asset, nullptr)) {
          for (const auto& nested : scene_asset.nested_scene_asset_ids) {
            if (!nested.empty()) {
              record.dependencies.push_back(nested);
            }
          }
          for (const auto& object : scene_asset.objects) {
            if (!object.mesh_asset_id.empty()) {
              record.dependencies.push_back(object.mesh_asset_id);
            }
            if (!object.material_asset_id.empty()) {
              record.dependencies.push_back(object.material_asset_id);
            }
          }
        }
      } else if (meta.type == "Material" || meta.type == "MaterialAsset") {
        cookie::assets::MaterialAsset material_asset;
        if (cookie::assets::LoadMaterialAsset(
                source_path, material_asset, nullptr) &&
            !material_asset.albedo_asset_id.empty()) {
          record.dependencies.push_back(material_asset.albedo_asset_id);
        }
      }
    }
    registry.AddOrReplace(std::move(record));
  }

  const fs::path registry_path = export_content_dir / "cooked_assets.pakreg";
  if (!registry.SaveToFile(registry_path)) {
    AddWarning(result, "Failed to write cooked registry: " + registry_path.string());
  }
}

}  // namespace

BuildProfile ParseBuildProfile(const std::string& value) {
  std::string normalized = value;
  std::transform(
      normalized.begin(), normalized.end(), normalized.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  if (normalized == "dev") {
    return BuildProfile::kDev;
  }
  return BuildProfile::kRelease;
}

const char* ToString(BuildProfile profile) {
  switch (profile) {
    case BuildProfile::kDev:
      return "dev";
    case BuildProfile::kRelease:
    default:
      return "release";
  }
}

ExportResult BuildCookPackage(const BuildCookPackageOptions& options) {
  ExportResult result;
  result.export_root = fs::absolute(options.export_parent_dir) / options.game_name;

  const fs::path project_root = fs::absolute(options.project_root);
  const fs::path runtime_build_dir = fs::absolute(options.runtime_build_dir);
  const fs::path export_root = result.export_root;
  const fs::path export_bin = export_root / "bin";
  const fs::path export_content = export_root / "content";
  const fs::path export_config = export_root / "config";
  const fs::path export_logs = export_root / "logs";

  try {
    fs::remove_all(export_root);

    fs::create_directories(export_bin);
    fs::create_directories(export_content);
    fs::create_directories(export_config);
    fs::create_directories(export_logs);

    const fs::path runtime_exe_source = runtime_build_dir / "CookieRuntime.exe";
    const fs::path runtime_exe_destination = export_root / (options.game_name + ".exe");
    if (!CopyFileIfExists(runtime_exe_source, runtime_exe_destination, result)) {
      result.success = false;
    }

    CopyFileIfExists(runtime_build_dir / "GameLogic.dll", export_bin / "GameLogic.dll", result);
    CopyDirectoryDlls(runtime_build_dir / "bin", export_bin, result);
    WarnIfMissing(runtime_build_dir / "bin" / "Core.dll", result, "Expected core module");
    WarnIfMissing(runtime_build_dir / "bin" / "RendererDX11.dll", result, "Expected renderer module");
    WarnIfMissing(runtime_build_dir / "bin" / "Physics.dll", result, "Expected physics module");
    WarnIfMissing(runtime_build_dir / "bin" / "Audio.dll", result, "Expected audio module");

    CopyDirectoryContents(project_root / "content", export_content, result);
    CopyDirectoryContents(project_root / "config", export_config, result);
    BuildCookedRegistryArtifact(project_root, export_content, result);
    ApplyEngineProfile(project_root / "config", export_config, options.profile, result);
    RemoveProfileConfigsFromExport(export_config, result);

    WriteExportReport(export_root / "export_report.txt", options, result);
  } catch (const std::exception& exception) {
    result.success = false;
    AddWarning(result, std::string("Export failed: ") + exception.what());
  }

  return result;
}

}  // namespace cookie::tools

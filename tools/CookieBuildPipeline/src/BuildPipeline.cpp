#include "Cookie/Tools/BuildPipeline.h"

#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Assets/AssetPackage.h"
#include "Cookie/Assets/CookedAssetRegistry.h"
#include "Cookie/Assets/MaterialAsset.h"
#include "Cookie/Assets/SceneAsset.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <fstream>
#include <map>
#include <set>
#include <system_error>

namespace fs = std::filesystem;

namespace cookie::tools {
namespace {

void AddWarning(ExportResult& result, const std::string& message) {
  result.warnings.push_back(ExportWarning{message});
}

void AddError(ExportResult& result, const std::string& message) {
  result.success = false;
  AddWarning(result, message);
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

bool BuildCookedRegistryArtifact(
    const fs::path& project_root, const fs::path& export_content_dir,
    ExportResult& result) {
  cookie::assets::CookedAssetRegistry registry;
  const fs::path content_root = project_root / "content";

  if (!fs::exists(content_root)) {
    AddError(result, "Cannot build cooked registry; missing content directory.");
    return false;
  }

  std::error_code error;
  std::map<std::string, std::vector<cookie::assets::PakWriteEntry>> package_entries;
  auto package_for_type = [](std::string_view type) -> std::string {
    if (type == "Texture" || type == "Texture2D") {
      return "textures.pak";
    }
    if (type == "Mesh" || type == "Model") {
      return "models.pak";
    }
    if (type == "Material" || type == "MaterialAsset") {
      return "materials.pak";
    }
    if (type == "Scene" || type == "SceneAsset") {
      return "scenes.pak";
    }
    return "misc.pak";
  };

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
      AddError(result, "Invalid .meta file: " + parse_error);
      continue;
    }

    const fs::path source_path =
        meta_path.parent_path() / meta_filename.substr(0, meta_filename.size() - 5);
    if (!fs::exists(source_path)) {
      AddError(result, "Missing source payload for meta: " + source_path.string());
      continue;
    }

    const auto archive_name = package_for_type(meta.type);
    const std::string archive_entry_name =
        meta.asset_id + source_path.extension().string();

    cookie::assets::CookedAssetRecord record{};
    record.asset_id = meta.asset_id;
    record.type = meta.type;
    record.runtime_relative_path = "pak://" + archive_name + "#" + archive_entry_name;
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
          record.dependencies = cookie::assets::ExtractSceneDependencies(scene_asset);
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
    package_entries[archive_name].push_back(cookie::assets::PakWriteEntry{
        .name = archive_entry_name,
        .source_path = source_path,
    });
  }

  const fs::path registry_path = export_content_dir / "cooked_assets.pakreg";
  if (!registry.SaveToFile(registry_path)) {
    AddError(result, "Failed to write cooked registry: " + registry_path.string());
  }

  for (auto& [package_name, entries] : package_entries) {
    const fs::path pak_path = export_content_dir / package_name;
    std::string write_error;
    if (!cookie::assets::WritePakArchive(pak_path, entries, &write_error)) {
      AddError(
          result, "Failed to write package archive: " + pak_path.string() +
                      " (" + write_error + ")");
      continue;
    }
  }

  std::vector<cookie::assets::PakWriteEntry> base_entries;
  base_entries.reserve(registry.Entries().size() + 1);
  std::string asset_list;
  for (const auto& record : registry.Entries()) {
    base_entries.push_back(cookie::assets::PakWriteEntry{
        .name = record.asset_id,
        .source_path = {},
        .inline_bytes = {},
    });
    asset_list += record.asset_id + "\n";
  }
  base_entries.push_back(cookie::assets::PakWriteEntry{
      .name = "assets.list",
      .source_path = {},
      .inline_bytes = std::vector<std::uint8_t>(asset_list.begin(), asset_list.end()),
  });

  const fs::path base_pak_path = export_content_dir / "base.pak";
  std::string base_error;
  if (!cookie::assets::WritePakArchive(base_pak_path, base_entries, &base_error)) {
    AddError(
        result, "Failed to write base package archive: " + base_pak_path.string() +
                    " (" + base_error + ")");
  }

  if (registry.Entries().empty()) {
    AddError(result, "Cooked registry produced zero assets; refusing export.");
    return false;
  }

  std::set<std::string> registry_ids;
  for (const auto& record : registry.Entries()) {
    registry_ids.insert(record.asset_id);
  }
  for (const auto& record : registry.Entries()) {
    for (const auto& dependency : record.dependencies) {
      if (dependency.empty()) {
        continue;
      }
      if (registry_ids.find(dependency) == registry_ids.end()) {
        AddError(
            result,
            "Missing dependency asset in cooked registry: " + record.asset_id +
                " -> " + dependency);
      }
    }
  }

  std::map<std::string, cookie::assets::CookedAssetRecord> records_by_id;
  for (const auto& record : registry.Entries()) {
    records_by_id.insert({record.asset_id, record});
  }
  for (const auto& record : registry.Entries()) {
    if (!IsTypeOneOf(record.type, {"Scene", "SceneAsset"})) {
      continue;
    }
    const auto source_path = project_root / record.source_relative_path;
    cookie::assets::SceneAsset scene_asset;
    if (!cookie::assets::LoadSceneAsset(source_path, scene_asset, nullptr)) {
      continue;
    }
    for (const auto& nested_scene_id : scene_asset.nested_scene_asset_ids) {
      if (nested_scene_id.empty()) {
        continue;
      }
      const auto it = records_by_id.find(nested_scene_id);
      if (it == records_by_id.end()) {
        AddError(result, "Scene reference missing: " + record.asset_id + " -> " +
                             nested_scene_id);
        continue;
      }
      if (!IsTypeOneOf(it->second.type, {"Scene", "SceneAsset"})) {
        AddError(result, "Scene reference type mismatch: " + record.asset_id +
                             " nested_scene " + nested_scene_id + " type=" +
                             it->second.type);
      }
    }
    for (const auto& object : scene_asset.objects) {
      if (object.has_mesh_renderer &&
          object.mesh_renderer.mesh_asset_id.empty()) {
        AddError(result, "Scene mesh reference missing: " + record.asset_id +
                             " object=" + object.name + " mesh=<empty>");
      }
      if (object.has_mesh_renderer &&
          object.mesh_renderer.material_asset_id.empty()) {
        AddError(result, "Scene material reference missing: " + record.asset_id +
                             " object=" + object.name + " material=<empty>");
      }
      if (object.has_mesh_renderer && !object.mesh_renderer.mesh_asset_id.empty()) {
        const auto it = records_by_id.find(object.mesh_renderer.mesh_asset_id);
        if (it == records_by_id.end()) {
          AddError(result, "Scene mesh reference missing: " + record.asset_id +
                               " object=" + object.name + " mesh=" +
                               object.mesh_renderer.mesh_asset_id);
        } else if (!IsTypeOneOf(it->second.type, {"Mesh", "Model"})) {
          AddError(result, "Scene mesh reference type mismatch: " + record.asset_id +
                               " object=" + object.name + " mesh=" +
                               object.mesh_renderer.mesh_asset_id + " type=" +
                               it->second.type);
        }
      }
      if (object.has_mesh_renderer &&
          !object.mesh_renderer.material_asset_id.empty()) {
        const auto it = records_by_id.find(object.mesh_renderer.material_asset_id);
        if (it == records_by_id.end()) {
          AddError(result, "Scene material reference missing: " + record.asset_id +
                               " object=" + object.name + " material=" +
                               object.mesh_renderer.material_asset_id);
        } else if (!IsTypeOneOf(it->second.type, {"Material", "MaterialAsset"})) {
          AddError(result,
                   "Scene material reference type mismatch: " + record.asset_id +
                       " object=" + object.name + " material=" +
                       object.mesh_renderer.material_asset_id + " type=" +
                       it->second.type);
        }
      }
    }
  }
  return result.success;
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

    if (options.project_root.empty() || !fs::exists(project_root / "content") ||
        !fs::exists(project_root / "config")) {
      AddError(
          result,
          "Invalid project root for export: expected config/ and content/ at " +
              project_root.string());
      WriteExportReport(export_root / "export_report.txt", options, result);
      return result;
    }

    if (options.runtime_build_dir.empty() ||
        !fs::exists(runtime_build_dir / "CookieRuntime.exe")) {
      AddError(
          result,
          "Invalid runtime build dir: missing CookieRuntime.exe at " +
              (runtime_build_dir / "CookieRuntime.exe").string());
      WriteExportReport(export_root / "export_report.txt", options, result);
      return result;
    }

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

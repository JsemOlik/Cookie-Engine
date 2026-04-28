#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cookie::tools {

enum class BuildProfile {
  kDev,
  kRelease,
};

struct ExportWarning {
  std::string message;
};

struct ExportResult {
  bool success = true;
  std::vector<ExportWarning> warnings;
  std::filesystem::path export_root;
};

struct BuildCookPackageOptions {
  std::filesystem::path project_root;
  std::filesystem::path runtime_build_dir;
  std::filesystem::path export_parent_dir;
  std::string game_name = "MyGame";
  BuildProfile profile = BuildProfile::kRelease;
  bool debug_logging = false;
};

BuildProfile ParseBuildProfile(const std::string& value);
const char* ToString(BuildProfile profile);

// Future editor path should call this API directly. This produces the shipped
// runtime layout with cooked payloads + package manifests (no raw .meta/.cookieasset).
ExportResult BuildCookPackage(const BuildCookPackageOptions& options);

}  // namespace cookie::tools

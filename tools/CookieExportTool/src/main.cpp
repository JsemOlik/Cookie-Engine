#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct ExportResult {
  bool success = true;
  std::vector<std::string> warnings;
};

bool CopyFileIfExists(
    const fs::path& source, const fs::path& destination,
    ExportResult& result);

void PrintUsage() {
  std::cout
      << "Usage: CookieExportTool <project-root> <runtime-build-dir> "
         "<export-parent-dir> [game-name] [profile]\n"
      << "Example: CookieExportTool . out/build/x64-debug/apps/CookieRuntime "
         "out/export MyGame release\n";
}

std::string NormalizeProfileName(std::string value) {
  for (char& ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  if (value != "dev" && value != "release") {
    return "release";
  }
  return value;
}

void ApplyEngineProfile(
    const fs::path& source_config_dir, const fs::path& export_config_dir,
    const std::string& profile_name, ExportResult& result) {
  const fs::path profile_file =
      source_config_dir / ("engine." + profile_name + ".json");
  const fs::path export_engine = export_config_dir / "engine.json";
  if (!CopyFileIfExists(profile_file, export_engine, result)) {
    result.warnings.push_back(
        "Failed to apply engine profile: " + profile_name +
        " (falling back to copied config/engine.json if present)");
  }
}

void RemoveProfileConfigsFromExport(
    const fs::path& export_config_dir, ExportResult& result) {
  const fs::path dev_profile = export_config_dir / "engine.dev.json";
  const fs::path release_profile = export_config_dir / "engine.release.json";

  std::error_code error;
  if (fs::exists(dev_profile) && !fs::remove(dev_profile, error)) {
    result.warnings.push_back(
        "Failed to remove exported profile file: " + dev_profile.string());
  }
  error.clear();
  if (fs::exists(release_profile) && !fs::remove(release_profile, error)) {
    result.warnings.push_back(
        "Failed to remove exported profile file: " + release_profile.string());
  }
}

bool CopyFileIfExists(
    const fs::path& source, const fs::path& destination,
    ExportResult& result) {
  if (!fs::exists(source)) {
    result.warnings.push_back("Missing file: " + source.string());
    return false;
  }

  fs::create_directories(destination.parent_path());
  fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
  return true;
}

void WarnIfMissing(const fs::path& source, ExportResult& result, const char* label) {
  if (!fs::exists(source)) {
    result.warnings.push_back(
        std::string(label) + " missing: " + source.string());
  }
}

void CopyDirectoryContents(
    const fs::path& source_dir, const fs::path& destination_dir,
    ExportResult& result) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    result.warnings.push_back("Missing directory: " + source_dir.string());
    return;
  }

  std::error_code error;
  fs::create_directories(destination_dir, error);
  if (error) {
    result.warnings.push_back(
        "Failed to create export directory: " + destination_dir.string());
    return;
  }

  for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
    const auto relative_path = fs::relative(entry.path(), source_dir, error);
    if (error) {
      result.warnings.push_back(
          "Failed to compute relative path for: " + entry.path().string());
      error.clear();
      continue;
    }

    const fs::path destination = destination_dir / relative_path;
    if (entry.is_directory()) {
      fs::create_directories(destination, error);
      if (error) {
        result.warnings.push_back(
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
      result.warnings.push_back(
          "Failed to create parent directory for: " + destination.string());
      error.clear();
      continue;
    }

    fs::copy_file(
        entry.path(), destination, fs::copy_options::overwrite_existing, error);
    if (error) {
      result.warnings.push_back(
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
    result.warnings.push_back("Missing directory: " + source_dir.string());
    return;
  }

  fs::create_directories(destination_dir);
  for (const auto& entry : fs::directory_iterator(source_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const fs::path source = entry.path();
    if (source.extension() != ".dll") {
      continue;
    }

    const fs::path destination = destination_dir / source.filename();
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
  }
}

void WriteExportReport(
    const fs::path& report_path, const std::string& game_name,
    const std::string& profile_name,
    const ExportResult& result) {
  std::ofstream report(report_path, std::ios::trunc);
  if (!report.is_open()) {
    return;
  }

  report << "Cookie Engine Export Report\n";
  report << "Game: " << game_name << "\n";
  report << "Engine profile: " << profile_name << "\n\n";
  report << "Status: " << (result.success ? "success" : "failed") << "\n\n";

  report << "Future module placeholders (expected in later phases):\n";
  report << " - (none currently)\n";
  report << "\n";

  if (!result.warnings.empty()) {
    report << "Warnings:\n";
    for (const auto& warning : result.warnings) {
      report << " - " << warning << "\n";
    }
  }
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    PrintUsage();
    return 1;
  }

  const fs::path project_root = fs::absolute(argv[1]);
  const fs::path runtime_build_dir = fs::absolute(argv[2]);
  const fs::path export_parent_dir = fs::absolute(argv[3]);
  const std::string game_name = argc >= 5 ? argv[4] : "MyGame";
  const std::string profile_name =
      argc >= 6 ? NormalizeProfileName(argv[5]) : "release";

  const fs::path export_root = export_parent_dir / game_name;
  const fs::path export_bin = export_root / "bin";
  const fs::path export_content = export_root / "content";
  const fs::path export_config = export_root / "config";
  const fs::path export_logs = export_root / "logs";

  ExportResult result;

  try {
    fs::remove_all(export_root);

    fs::create_directories(export_bin);
    fs::create_directories(export_content);
    fs::create_directories(export_config);
    fs::create_directories(export_logs);

    const fs::path runtime_exe_source = runtime_build_dir / "CookieRuntime.exe";
    const fs::path runtime_exe_destination = export_root / (game_name + ".exe");
    if (!CopyFileIfExists(runtime_exe_source, runtime_exe_destination, result)) {
      result.success = false;
    }

    CopyFileIfExists(
        runtime_build_dir / "GameLogic.dll", export_bin / "GameLogic.dll", result);
    CopyDirectoryDlls(runtime_build_dir / "bin", export_bin, result);
    WarnIfMissing(
        runtime_build_dir / "bin" / "Core.dll", result,
        "Expected core module");
    WarnIfMissing(
        runtime_build_dir / "bin" / "RendererDX11.dll", result,
        "Expected renderer module");
    WarnIfMissing(
        runtime_build_dir / "bin" / "Physics.dll", result,
        "Expected physics module");
    WarnIfMissing(
        runtime_build_dir / "bin" / "Audio.dll", result,
        "Expected audio module");

    CopyDirectoryContents(project_root / "content", export_content, result);
    CopyDirectoryContents(project_root / "config", export_config, result);
    ApplyEngineProfile(project_root / "config", export_config, profile_name, result);
    RemoveProfileConfigsFromExport(export_config, result);

    WriteExportReport(
        export_root / "export_report.txt", game_name, profile_name, result);
  } catch (const std::exception& exception) {
    std::cout << "Export failed: " << exception.what() << '\n';
    return 2;
  }

  std::cout << "Export root: " << export_root.string() << '\n';
  if (!result.warnings.empty()) {
    std::cout << "Warnings:\n";
    for (const auto& warning : result.warnings) {
      std::cout << " - " << warning << '\n';
    }
  }

  return result.success ? 0 : 3;
}

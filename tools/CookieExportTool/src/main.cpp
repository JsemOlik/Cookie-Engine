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

void PrintUsage() {
  std::cout
      << "Usage: CookieExportTool <project-root> <runtime-build-dir> "
         "<export-parent-dir> [game-name]\n"
      << "Example: CookieExportTool . out/build/x64-debug/apps/CookieRuntime "
         "out/export MyGame\n";
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

void CopyDirectoryContents(
    const fs::path& source_dir, const fs::path& destination_dir,
    ExportResult& result) {
  if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
    result.warnings.push_back("Missing directory: " + source_dir.string());
    return;
  }

  fs::create_directories(destination_dir);
  for (const auto& entry : fs::directory_iterator(source_dir)) {
    const fs::path destination = destination_dir / entry.path().filename();
    if (entry.is_regular_file()) {
      fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing);
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
    const ExportResult& result) {
  std::ofstream report(report_path, std::ios::trunc);
  if (!report.is_open()) {
    return;
  }

  report << "Cookie Engine Export Report\n";
  report << "Game: " << game_name << "\n\n";
  report << "Status: " << (result.success ? "success" : "failed") << "\n\n";

  report << "Future module placeholders (expected in later phases):\n";
  report << " - Core.dll\n";
  report << " - Physics.dll\n";
  report << " - Audio.dll\n\n";

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

    CopyDirectoryContents(project_root / "content", export_content, result);
    CopyDirectoryContents(project_root / "config", export_config, result);

    WriteExportReport(export_root / "export_report.txt", game_name, result);
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

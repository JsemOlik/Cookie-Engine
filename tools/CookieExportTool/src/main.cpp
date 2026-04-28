#include <filesystem>
#include <iostream>
#include <string>

#include "Cookie/Tools/BuildPipeline.h"

namespace fs = std::filesystem;

namespace {

void PrintUsage() {
  std::cout
      << "Usage: CookieExportTool <project-root> <runtime-build-dir> "
         "<export-parent-dir> [game-name] [profile]\n"
      << "Example: CookieExportTool . out/build/x64-debug/apps/CookieRuntime "
         "out/export MyGame release\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    PrintUsage();
    return 1;
  }

  cookie::tools::BuildCookPackageOptions options;
  options.project_root = fs::absolute(argv[1]);
  options.runtime_build_dir = fs::absolute(argv[2]);
  options.export_parent_dir = fs::absolute(argv[3]);
  options.game_name = argc >= 5 ? argv[4] : "MyGame";
  options.profile =
      argc >= 6 ? cookie::tools::ParseBuildProfile(argv[5])
                : cookie::tools::BuildProfile::kRelease;

  const auto result = cookie::tools::BuildCookPackage(options);
  std::cout << "Export root: " << result.export_root.string() << '\n';
  if (!result.warnings.empty()) {
    std::cout << "Warnings:\n";
    for (const auto& warning : result.warnings) {
      std::cout << " - " << warning.message << '\n';
    }
  }

  return result.success ? 0 : 3;
}

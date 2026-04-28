#include <QApplication>

#include <filesystem>
#include <string>

#include "Cookie/Editor/EditorMainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  std::filesystem::path project_root_override;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--project-root=", 0) == 0) {
      project_root_override = arg.substr(std::string("--project-root=").size());
      continue;
    }
    if (arg == "--project-root" && (i + 1) < argc) {
      project_root_override = argv[++i];
      continue;
    }
  }

  EditorMainWindow window(project_root_override);
  window.show();
  return app.exec();
}

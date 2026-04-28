#include "Cookie/Editor/EditorMainWindow.h"

#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Tools/BuildPipeline.h"

namespace {

QDockWidget* CreateDock(
    const QString& title, QWidget* content, QWidget* parent) {
  auto* dock = new QDockWidget(title, parent);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  dock->setWidget(content);
  return dock;
}

QWidget* CreatePlaceholderPanel(const QString& label_text, QWidget* parent) {
  auto* label = new QLabel(label_text, parent);
  label->setAlignment(Qt::AlignCenter);
  return label;
}

QString BuildMetaSummary(const cookie::assets::DiscoveredAsset& asset) {
  QString summary;
  summary += "AssetId: " + QString::fromStdString(asset.meta.asset_id) + "\n";
  summary += "Type: " + QString::fromStdString(asset.meta.type) + "\n";
  summary += "Importer: " + QString::fromStdString(asset.meta.importer) + "\n";
  summary +=
      "Importer Version: " + QString::number(asset.meta.importer_version) + "\n";
  summary += "Source: " + QString::fromStdString(asset.source_relative_path) + "\n";
  summary += "Meta: " + QString::fromStdString(asset.meta_path.string()) + "\n";
  summary += "Fingerprint: " +
             QString::fromStdString(asset.meta.source_fingerprint) + "\n";
  summary += "\nSettings:\n";
  for (const auto& setting : asset.meta.settings) {
    summary += " - " + QString::fromStdString(setting.key) + " = " +
               QString::fromStdString(setting.value) + "\n";
  }
  summary += "\nDependencies:\n";
  for (const std::string& dependency : asset.meta.dependencies) {
    summary += " - " + QString::fromStdString(dependency) + "\n";
  }
  return summary;
}

} // namespace

EditorMainWindow::EditorMainWindow(QWidget* parent) : QMainWindow(parent) {
  project_root_ = std::filesystem::current_path();
  setWindowTitle("Cookie Editor");
  resize(1600, 900);

  auto* scene_viewport =
      new QTextEdit("Scene Viewport Placeholder\n\n(Phase 6 skeleton)");
  scene_viewport->setReadOnly(true);
  setCentralWidget(scene_viewport);

  assets_list_ = new QListWidget(this);
  meta_inspector_ = new QTextEdit(this);
  meta_inspector_->setReadOnly(true);

  connect(assets_list_, &QListWidget::currentRowChanged, this,
          [this](const int index) { UpdateMetaInspector(index); });

  auto* build_panel = new QWidget(this);
  auto* build_layout = new QVBoxLayout(build_panel);
  auto* form_layout = new QFormLayout();
  game_name_input_ = new QLineEdit("MyGame", build_panel);
  runtime_build_dir_input_ =
      new QLineEdit(QString::fromStdString(
          (project_root_ / ".." / ".." / "ce-build" / "x64-release-vcpkg" /
           "apps" / "CookieRuntime")
              .lexically_normal()
              .string()),
                    build_panel);
  export_parent_input_ = new QLineEdit(
      QString::fromStdString((project_root_ / "out" / "ship").string()),
      build_panel);

  auto* profile_combo = new QComboBox(build_panel);
  profile_combo->addItem("release");
  profile_combo->addItem("dev");
  auto* debug_logging_combo = new QComboBox(build_panel);
  debug_logging_combo->addItem("false");
  debug_logging_combo->addItem("true");

  form_layout->addRow("Game Name", game_name_input_);
  form_layout->addRow("Runtime Build Dir", runtime_build_dir_input_);
  form_layout->addRow("Export Parent Dir", export_parent_input_);
  form_layout->addRow("Profile", profile_combo);
  form_layout->addRow("Debug Logging", debug_logging_combo);
  build_layout->addLayout(form_layout);

  auto* run_export_button = new QPushButton("Build/Export", build_panel);
  build_layout->addWidget(run_export_button);
  build_output_ = new QTextEdit(build_panel);
  build_output_->setReadOnly(true);
  build_layout->addWidget(build_output_);

  connect(run_export_button, &QPushButton::clicked, this, [this, profile_combo,
                                                            debug_logging_combo]() {
    cookie::tools::BuildCookPackageOptions options;
    options.project_root = project_root_;
    options.runtime_build_dir =
        runtime_build_dir_input_->text().trimmed().toStdString();
    options.export_parent_dir = export_parent_input_->text().trimmed().toStdString();
    options.game_name = game_name_input_->text().trimmed().toStdString();
    options.profile = cookie::tools::ParseBuildProfile(
        profile_combo->currentText().toStdString());
    options.debug_logging = (debug_logging_combo->currentText() == "true");

    const auto result = cookie::tools::BuildCookPackage(options);
    QString output = "Export status: ";
    output += result.success ? "success\n" : "failed\n";
    output += "Export root: " + QString::fromStdString(result.export_root.string()) +
              "\n";
    if (result.warnings.empty()) {
      output += "Warnings: none\n";
    } else {
      output += "Warnings:\n";
      for (const auto& warning : result.warnings) {
        output += " - " + QString::fromStdString(warning.message) + "\n";
      }
    }
    build_output_->setPlainText(output);
    statusBar()->showMessage(result.success ? "Export succeeded"
                                            : "Export finished with failures");
  });

  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock(
                    "Hierarchy",
                    CreatePlaceholderPanel("Hierarchy Placeholder", this), this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Meta Inspector", meta_inspector_, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Assets", assets_list_, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Build/Export", build_panel, this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock(
                    "Game Viewport",
                    CreatePlaceholderPanel("Game Viewport Placeholder", this), this));

  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("E&xit", this, &QWidget::close);

  RefreshAssets();
  statusBar()->showMessage("Cookie Editor Phase 24A shell loaded");
}

void EditorMainWindow::RefreshAssets() {
  assets_list_->clear();
  discovered_assets_.clear();

  asset_registry_.DiscoverProjectAssets(project_root_);
  discovered_assets_ = asset_registry_.GetDiscoveredAssets();
  for (const auto& asset : discovered_assets_) {
    const QString label =
        QString::fromStdString(asset.source_relative_path) + " [" +
        QString::fromStdString(asset.meta.asset_id) + "]";
    assets_list_->addItem(label);
  }

  if (!discovered_assets_.empty()) {
    assets_list_->setCurrentRow(0);
  } else {
    meta_inspector_->setPlainText(
        "No .meta-backed assets found under project Assets/ or content/.");
  }
}

void EditorMainWindow::UpdateMetaInspector(const int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= discovered_assets_.size()) {
    meta_inspector_->setPlainText("Select an asset to inspect metadata.");
    return;
  }

  meta_inspector_->setPlainText(BuildMetaSummary(discovered_assets_[index]));
}

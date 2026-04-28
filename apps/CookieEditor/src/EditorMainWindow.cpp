#include "Cookie/Editor/EditorMainWindow.h"

#include <QComboBox>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <system_error>

#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Assets/SceneAsset.h"
#include "Cookie/Core/GameConfig.h"
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

std::filesystem::path DetectProjectRoot(std::filesystem::path start) {
  std::error_code error;
  start = std::filesystem::absolute(start, error);
  if (error) {
    return std::filesystem::current_path();
  }

  auto candidate = start;
  while (!candidate.empty()) {
    if (std::filesystem::exists(candidate / "config" / "game.json") &&
        std::filesystem::exists(candidate / "content")) {
      return candidate;
    }
    const auto parent = candidate.parent_path();
    if (parent == candidate) {
      break;
    }
    candidate = parent;
  }
  return std::filesystem::current_path();
}

bool IsProjectRoot(const std::filesystem::path& candidate) {
  return std::filesystem::exists(candidate / "config" / "game.json") &&
         std::filesystem::exists(candidate / "content");
}

bool HasRuntimeArtifacts(const std::filesystem::path& runtime_build_dir) {
  return std::filesystem::exists(runtime_build_dir / "CookieRuntime.exe") &&
         std::filesystem::exists(runtime_build_dir / "bin" / "Core.dll") &&
         std::filesystem::exists(runtime_build_dir / "bin" / "RendererDX11.dll");
}

std::filesystem::path DetectDefaultRuntimeBuildDir(
    const std::filesystem::path& project_root) {
  const std::vector<std::filesystem::path> candidates = {
      std::filesystem::path("C:/ce-build/x64-debug-vcpkg/apps/CookieRuntime"),
      std::filesystem::path("C:/ce-build/x64-release-vcpkg/apps/CookieRuntime"),
      (project_root / ".." / ".." / "ce-build" / "x64-debug-vcpkg" / "apps" /
       "CookieRuntime")
          .lexically_normal(),
      (project_root / ".." / ".." / "ce-build" / "x64-release-vcpkg" / "apps" /
       "CookieRuntime")
          .lexically_normal(),
  };
  for (const auto& candidate : candidates) {
    if (HasRuntimeArtifacts(candidate)) {
      return candidate;
    }
  }
  return candidates.front();
}

bool TryParseFloat(const QString& text, float& out_value) {
  const std::string raw = text.trimmed().toStdString();
  if (raw.empty()) {
    return false;
  }
  const char* begin = raw.data();
  const char* end = raw.data() + raw.size();
  auto parsed = std::from_chars(begin, end, out_value);
  return parsed.ec == std::errc() && parsed.ptr == end;
}

std::filesystem::path ResolveEditorProjectRoot(
    const std::filesystem::path& project_root_override) {
  if (!project_root_override.empty()) {
    const auto detected = DetectProjectRoot(project_root_override);
    if (IsProjectRoot(detected)) {
      return detected;
    }
  }

  const QString env_root_qt = qEnvironmentVariable("COOKIE_PROJECT_ROOT");
  if (!env_root_qt.isEmpty()) {
    const auto detected =
        DetectProjectRoot(std::filesystem::path(env_root_qt.toStdString()));
    if (IsProjectRoot(detected)) {
      return detected;
    }
  }

  const auto from_cwd = DetectProjectRoot(std::filesystem::current_path());
  if (IsProjectRoot(from_cwd)) {
    return from_cwd;
  }

  const auto app_dir = std::filesystem::path(
      QCoreApplication::applicationDirPath().toStdString());
  const auto from_app_dir = DetectProjectRoot(app_dir);
  if (IsProjectRoot(from_app_dir)) {
    return from_app_dir;
  }

#ifdef COOKIE_ENGINE_SOURCE_ROOT
  const auto from_source_root =
      DetectProjectRoot(std::filesystem::path(COOKIE_ENGINE_SOURCE_ROOT));
  if (IsProjectRoot(from_source_root)) {
    return from_source_root;
  }
#endif

  return std::filesystem::current_path();
}

} // namespace

EditorMainWindow::EditorMainWindow(
    const std::filesystem::path& project_root_override, QWidget* parent)
    : QMainWindow(parent) {
  project_root_ = ResolveEditorProjectRoot(project_root_override);
  setWindowTitle("Cookie Editor");
  resize(1600, 900);

  auto* scene_viewport =
      new QTextEdit("Scene Viewport Placeholder\n\n(Phase 6 skeleton)");
  scene_viewport->setReadOnly(true);
  setCentralWidget(scene_viewport);

  assets_list_ = new QListWidget(this);
  scene_outliner_list_ = new QListWidget(this);
  meta_inspector_ = new QTextEdit(this);
  meta_inspector_->setReadOnly(true);
  startup_scene_selector_ = new QComboBox(this);
  object_name_input_ = new QLineEdit(this);
  pos_x_input_ = new QLineEdit(this);
  pos_y_input_ = new QLineEdit(this);
  pos_z_input_ = new QLineEdit(this);
  scale_x_input_ = new QLineEdit(this);
  scale_y_input_ = new QLineEdit(this);
  scale_z_input_ = new QLineEdit(this);
  mesh_asset_id_input_ = new QLineEdit(this);
  material_asset_id_input_ = new QLineEdit(this);
  rigidbody_enabled_input_ = new QCheckBox(this);
  rigidbody_type_input_ = new QLineEdit(this);
  rigidbody_mass_input_ = new QLineEdit(this);
  apply_object_button_ = new QPushButton("Apply Object", this);
  save_scene_button_ = new QPushButton("Save Scene", this);

  connect(assets_list_, &QListWidget::currentRowChanged, this,
          [this](const int index) { UpdateMetaInspector(index); });
  connect(scene_outliner_list_, &QListWidget::currentRowChanged, this,
          [this](const int) { RefreshInspector(); });
  connect(startup_scene_selector_, &QComboBox::currentTextChanged, this,
          [this](const QString& text) {
            const QString selected_asset_id = text.section(' ', 0, 0);
            if (selected_asset_id.isEmpty()) {
              return;
            }
            auto game_config =
                cookie::core::LoadGameConfig(project_root_ / "config" / "game.json");
            game_config.startup_scene_asset_id = selected_asset_id.toStdString();
            cookie::core::SaveGameConfig(
                project_root_ / "config" / "game.json", game_config);
            statusBar()->showMessage("Updated startup scene AssetId in config/game.json");
            LoadActiveScene();
            RefreshSceneOutliner();
          });
  connect(apply_object_button_, &QPushButton::clicked, this,
          [this]() { ApplyInspectorToSelectedObject(); });
  connect(save_scene_button_, &QPushButton::clicked, this,
          [this]() { SaveActiveScene(); });

  auto* build_panel = new QWidget(this);
  auto* build_layout = new QVBoxLayout(build_panel);
  auto* form_layout = new QFormLayout();
  game_name_input_ = new QLineEdit("MyGame", build_panel);
  const auto detected_runtime_build_dir = DetectDefaultRuntimeBuildDir(project_root_);
  runtime_build_dir_input_ =
      new QLineEdit(QString::fromStdString(
          detected_runtime_build_dir.string()),
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
  form_layout->addRow("Startup Scene", startup_scene_selector_);
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
    const auto runtime_build_dir = std::filesystem::path(
        runtime_build_dir_input_->text().trimmed().toStdString());
    if (!HasRuntimeArtifacts(runtime_build_dir)) {
      build_output_->setPlainText(
          "Export status: failed\n"
          "Runtime Build Dir is invalid or incomplete.\n"
          "Expected at least:\n"
          " - CookieRuntime.exe\n"
          " - bin/Core.dll\n"
          " - bin/RendererDX11.dll\n"
          "Check Runtime Build Dir in Build/Export panel.");
      statusBar()->showMessage("Export failed: invalid runtime build dir");
      return;
    }

    cookie::tools::BuildCookPackageOptions options;
    options.project_root = project_root_;
    options.runtime_build_dir = runtime_build_dir;
    options.export_parent_dir = export_parent_input_->text().trimmed().toStdString();
    options.game_name = game_name_input_->text().trimmed().toStdString();
    options.profile = cookie::tools::ParseBuildProfile(
        profile_combo->currentText().toStdString());
    options.debug_logging = (debug_logging_combo->currentText() == "true");

    std::string runtime_dir_lower = options.runtime_build_dir.string();
    std::transform(
        runtime_dir_lower.begin(), runtime_dir_lower.end(),
        runtime_dir_lower.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const bool runtime_is_debug =
        runtime_dir_lower.find("x64-debug-vcpkg") != std::string::npos;
    const bool runtime_is_release =
        runtime_dir_lower.find("x64-release-vcpkg") != std::string::npos;
    if (options.profile == cookie::tools::BuildProfile::kRelease &&
        runtime_is_debug) {
      build_output_->setPlainText(
          "Export status: failed\n"
          "Profile/runtime mismatch: selected profile is 'release' but Runtime Build Dir points to debug build.\n"
          "Use C:/ce-build/x64-release-vcpkg/apps/CookieRuntime for release export.");
      statusBar()->showMessage("Export failed: release profile requires release runtime dir");
      return;
    }
    if (options.profile == cookie::tools::BuildProfile::kDev &&
        runtime_is_release) {
      build_output_->setPlainText(
          "Export status: failed\n"
          "Profile/runtime mismatch: selected profile is 'dev' but Runtime Build Dir points to release build.\n"
          "Use C:/ce-build/x64-debug-vcpkg/apps/CookieRuntime for dev export.");
      statusBar()->showMessage("Export failed: dev profile requires debug runtime dir");
      return;
    }

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

  auto* scene_outliner_panel = new QWidget(this);
  auto* scene_outliner_layout = new QVBoxLayout(scene_outliner_panel);
  scene_outliner_layout->addWidget(scene_outliner_list_);
  auto* outliner_actions = new QHBoxLayout();
  auto* add_object_button = new QPushButton("Add", scene_outliner_panel);
  auto* rename_object_button = new QPushButton("Rename", scene_outliner_panel);
  auto* remove_object_button = new QPushButton("Remove", scene_outliner_panel);
  outliner_actions->addWidget(add_object_button);
  outliner_actions->addWidget(rename_object_button);
  outliner_actions->addWidget(remove_object_button);
  scene_outliner_layout->addLayout(outliner_actions);
  scene_outliner_layout->addWidget(save_scene_button_);

  connect(add_object_button, &QPushButton::clicked, this, [this]() {
    cookie::assets::SceneAssetObject object{};
    object.name = "NewObject_" + std::to_string(active_scene_.objects.size() + 1);
    active_scene_.objects.push_back(object);
    RefreshSceneOutliner();
    scene_outliner_list_->setCurrentRow(
        static_cast<int>(active_scene_.objects.size() - 1));
    statusBar()->showMessage("Added scene object.");
  });
  connect(rename_object_button, &QPushButton::clicked, this, [this]() {
    const int index = CurrentSceneObjectIndex();
    if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
      return;
    }
    const QString current =
        QString::fromStdString(active_scene_.objects[static_cast<std::size_t>(index)].name);
    bool accepted = false;
    const QString renamed = QInputDialog::getText(
        this, "Rename Object", "Object Name:", QLineEdit::Normal, current,
        &accepted);
    if (!accepted || renamed.trimmed().isEmpty()) {
      return;
    }
    active_scene_.objects[static_cast<std::size_t>(index)].name =
        renamed.trimmed().toStdString();
    RefreshSceneOutliner();
    scene_outliner_list_->setCurrentRow(index);
    statusBar()->showMessage("Renamed scene object.");
  });
  connect(remove_object_button, &QPushButton::clicked, this, [this]() {
    const int index = CurrentSceneObjectIndex();
    if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
      return;
    }
    active_scene_.objects.erase(active_scene_.objects.begin() + index);
    RefreshSceneOutliner();
    const int next = std::min(index, static_cast<int>(active_scene_.objects.size()) - 1);
    if (next >= 0) {
      scene_outliner_list_->setCurrentRow(next);
    }
    statusBar()->showMessage("Removed scene object.");
  });

  auto* inspector_panel = new QWidget(this);
  auto* inspector_layout = new QFormLayout(inspector_panel);
  inspector_layout->addRow("Name", object_name_input_);
  inspector_layout->addRow("Position X", pos_x_input_);
  inspector_layout->addRow("Position Y", pos_y_input_);
  inspector_layout->addRow("Position Z", pos_z_input_);
  inspector_layout->addRow("Scale X", scale_x_input_);
  inspector_layout->addRow("Scale Y", scale_y_input_);
  inspector_layout->addRow("Scale Z", scale_z_input_);
  inspector_layout->addRow("Mesh AssetId", mesh_asset_id_input_);
  inspector_layout->addRow("Material AssetId", material_asset_id_input_);
  inspector_layout->addRow("RigidBody Enabled", rigidbody_enabled_input_);
  inspector_layout->addRow("RigidBody Type", rigidbody_type_input_);
  inspector_layout->addRow("RigidBody Mass", rigidbody_mass_input_);
  inspector_layout->addRow(apply_object_button_);

  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock(
                    "Hierarchy",
                    CreatePlaceholderPanel("Hierarchy Placeholder", this), this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Meta Inspector", meta_inspector_, this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Object Inspector", inspector_panel, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Assets", assets_list_, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Build/Export", build_panel, this));
  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock("Scene Outliner", scene_outliner_panel, this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock(
                    "Game Viewport",
                    CreatePlaceholderPanel("Game Viewport Placeholder", this), this));

  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("E&xit", this, &QWidget::close);

  const auto cooked_registry_path = project_root_ / "content" / "cooked_assets.pakreg";
  asset_registry_.LoadCookedRegistry(cooked_registry_path);
  RefreshAssets();
  RefreshStartupSceneSelector();
  LoadActiveScene();
  RefreshSceneOutliner();
  RefreshInspector();
  statusBar()->showMessage(
      "Cookie Editor Phase 24B shell loaded (project root: " +
      QString::fromStdString(project_root_.string()) + ")");
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

  RefreshStartupSceneSelector();
  LoadActiveScene();
  RefreshSceneOutliner();
  RefreshInspector();
}

void EditorMainWindow::UpdateMetaInspector(const int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= discovered_assets_.size()) {
    meta_inspector_->setPlainText("Select an asset to inspect metadata.");
    return;
  }

  meta_inspector_->setPlainText(BuildMetaSummary(discovered_assets_[index]));
}

void EditorMainWindow::RefreshStartupSceneSelector() {
  startup_scene_selector_->clear();
  auto game_config = cookie::core::LoadGameConfig(project_root_ / "config" / "game.json");

  int selected_index = -1;
  int index = 0;
  for (const auto& asset : discovered_assets_) {
    if (asset.meta.type != "Scene" && asset.meta.type != "SceneAsset") {
      continue;
    }
    const QString label = QString::fromStdString(asset.meta.asset_id) + " (" +
                          QString::fromStdString(asset.source_relative_path) + ")";
    startup_scene_selector_->addItem(label);
    if (asset.meta.asset_id == game_config.startup_scene_asset_id) {
      selected_index = index;
    }
    ++index;
  }

  if (selected_index >= 0) {
    startup_scene_selector_->setCurrentIndex(selected_index);
  } else if (startup_scene_selector_->count() > 0) {
    startup_scene_selector_->setCurrentIndex(0);
  }
}

void EditorMainWindow::RefreshSceneOutliner() {
  scene_outliner_list_->clear();
  if (active_scene_path_.empty()) {
    scene_outliner_list_->addItem("No startup scene AssetId configured.");
    return;
  }

  for (const auto& object : active_scene_.objects) {
    scene_outliner_list_->addItem(QString::fromStdString(object.name));
  }
}

int EditorMainWindow::CurrentSceneObjectIndex() const {
  return scene_outliner_list_->currentRow();
}

void EditorMainWindow::LoadActiveScene() {
  active_scene_ = {};
  active_scene_path_.clear();
  const auto game_config =
      cookie::core::LoadGameConfig(project_root_ / "config" / "game.json");
  if (game_config.startup_scene_asset_id.empty()) {
    return;
  }

  const auto scene_path = asset_registry_.ResolveAssetPath(
      game_config.startup_scene_asset_id);
  if (scene_path.empty()) {
    statusBar()->showMessage(
        "Startup scene not resolved: " +
        QString::fromStdString(game_config.startup_scene_asset_id));
    return;
  }

  std::string scene_error;
  if (!cookie::assets::LoadSceneAsset(scene_path, active_scene_, &scene_error)) {
    statusBar()->showMessage(
        "Failed to load startup scene: " + QString::fromStdString(scene_error));
    return;
  }
  active_scene_path_ = scene_path;
}

void EditorMainWindow::RefreshInspector() {
  const int index = CurrentSceneObjectIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
    object_name_input_->setText("");
    pos_x_input_->setText("");
    pos_y_input_->setText("");
    pos_z_input_->setText("");
    scale_x_input_->setText("");
    scale_y_input_->setText("");
    scale_z_input_->setText("");
    mesh_asset_id_input_->setText("");
    material_asset_id_input_->setText("");
    rigidbody_enabled_input_->setChecked(false);
    rigidbody_type_input_->setText("");
    rigidbody_mass_input_->setText("");
    return;
  }
  const auto& object = active_scene_.objects[static_cast<std::size_t>(index)];
  object_name_input_->setText(QString::fromStdString(object.name));
  pos_x_input_->setText(QString::number(object.transform.position[0]));
  pos_y_input_->setText(QString::number(object.transform.position[1]));
  pos_z_input_->setText(QString::number(object.transform.position[2]));
  scale_x_input_->setText(QString::number(object.transform.scale[0]));
  scale_y_input_->setText(QString::number(object.transform.scale[1]));
  scale_z_input_->setText(QString::number(object.transform.scale[2]));
  mesh_asset_id_input_->setText(
      QString::fromStdString(object.mesh_renderer.mesh_asset_id));
  material_asset_id_input_->setText(
      QString::fromStdString(object.mesh_renderer.material_asset_id));
  rigidbody_enabled_input_->setChecked(object.has_rigidbody && object.rigidbody.enabled);
  rigidbody_type_input_->setText(QString::fromStdString(object.rigidbody.body_type));
  rigidbody_mass_input_->setText(QString::number(object.rigidbody.mass));
}

void EditorMainWindow::ApplyInspectorToSelectedObject() {
  const int index = CurrentSceneObjectIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
    return;
  }
  auto& object = active_scene_.objects[static_cast<std::size_t>(index)];
  object.name = object_name_input_->text().trimmed().toStdString();
  if (object.name.empty()) {
    object.name = "Object_" + std::to_string(index + 1);
  }

  TryParseFloat(pos_x_input_->text(), object.transform.position[0]);
  TryParseFloat(pos_y_input_->text(), object.transform.position[1]);
  TryParseFloat(pos_z_input_->text(), object.transform.position[2]);
  TryParseFloat(scale_x_input_->text(), object.transform.scale[0]);
  TryParseFloat(scale_y_input_->text(), object.transform.scale[1]);
  TryParseFloat(scale_z_input_->text(), object.transform.scale[2]);

  object.mesh_renderer.mesh_asset_id =
      mesh_asset_id_input_->text().trimmed().toStdString();
  object.mesh_renderer.material_asset_id =
      material_asset_id_input_->text().trimmed().toStdString();
  object.has_rigidbody = rigidbody_enabled_input_->isChecked();
  object.rigidbody.enabled = rigidbody_enabled_input_->isChecked();
  object.rigidbody.body_type = rigidbody_type_input_->text().trimmed().toStdString();
  TryParseFloat(rigidbody_mass_input_->text(), object.rigidbody.mass);

  RefreshSceneOutliner();
  scene_outliner_list_->setCurrentRow(index);
  statusBar()->showMessage("Applied object edits.");
}

void EditorMainWindow::SaveActiveScene() {
  if (active_scene_path_.empty()) {
    QMessageBox::warning(this, "Save Scene", "No active startup scene loaded.");
    return;
  }
  std::string save_error;
  if (!cookie::assets::SaveSceneAsset(active_scene_path_, active_scene_, &save_error)) {
    QMessageBox::critical(
        this, "Save Scene",
        QString("Failed to save scene:\n") + QString::fromStdString(save_error));
    return;
  }
  statusBar()->showMessage(
      "Saved scene: " + QString::fromStdString(active_scene_path_.string()));
}

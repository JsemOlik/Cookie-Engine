#include "Cookie/Editor/EditorMainWindow.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QCursor>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QMenuBar>
#include <QMap>
#include <QMessageBox>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "Cookie/Assets/AssetMeta.h"
#include "Cookie/Assets/MaterialAsset.h"
#include "Cookie/Assets/SceneAsset.h"
#include "Cookie/Core/GameConfig.h"
#include "Cookie/Renderer/SceneBuilder.h"
#include "Cookie/Renderer/Transform.h"
#include "Cookie/RendererDX11/RendererDX11Backend.h"
#include "Cookie/Tools/BuildPipeline.h"

namespace {

constexpr float kDegreesToRadians = 0.017453292519943295769f;

struct EditorVec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

EditorVec3 AddVec3(const EditorVec3& a, const EditorVec3& b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

EditorVec3 SubtractVec3(const EditorVec3& a, const EditorVec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

EditorVec3 ScaleVec3(const EditorVec3& value, float scalar) {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

float LengthVec3(const EditorVec3& value) {
  return std::sqrt(
      value.x * value.x + value.y * value.y + value.z * value.z);
}

EditorVec3 NormalizeVec3(const EditorVec3& value) {
  const float length = LengthVec3(value);
  if (length <= 0.000001f) {
    return {};
  }
  return ScaleVec3(value, 1.0f / length);
}

EditorVec3 CrossVec3(const EditorVec3& a, const EditorVec3& b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

QDockWidget* CreateDock(
    const QString& title, QWidget* content, QWidget* parent) {
  auto* dock = new QDockWidget(title, parent);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  dock->setWidget(content);
  return dock;
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

bool IsTypeOneOf(const std::string& type,
                 std::initializer_list<std::string_view> allowed_types) {
  for (const auto allowed : allowed_types) {
    if (type == allowed) {
      return true;
    }
  }
  return false;
}

cookie::renderer::Float4x4 BuildObjectTransform(
    const cookie::assets::SceneAssetObject& object) {
  const auto rotation =
      cookie::renderer::MultiplyTransforms(
          cookie::renderer::MakeXRotationTransform(
              object.transform.rotation_euler_degrees[0] * kDegreesToRadians),
          cookie::renderer::MultiplyTransforms(
              cookie::renderer::MakeYRotationTransform(
                  object.transform.rotation_euler_degrees[1] * kDegreesToRadians),
              cookie::renderer::MakeZRotationTransform(
                  object.transform.rotation_euler_degrees[2] *
                  kDegreesToRadians)));
  return cookie::renderer::MultiplyTransforms(
      cookie::renderer::MakeScaleTransform(
          object.transform.scale[0], object.transform.scale[1],
          object.transform.scale[2]),
      cookie::renderer::MultiplyTransforms(
          rotation,
          cookie::renderer::MakeTranslationTransform(
              object.transform.position[0], object.transform.position[1],
              object.transform.position[2])));
}

} // namespace

EditorMainWindow::EditorMainWindow(
    const std::filesystem::path& project_root_override, QWidget* parent)
    : QMainWindow(parent) {
  project_root_ = ResolveEditorProjectRoot(project_root_override);
  setWindowTitle("Cookie Editor");
  resize(1600, 900);

  viewport_widget_ = new QWidget(this);
  viewport_widget_->setAttribute(Qt::WA_NativeWindow, true);
  viewport_widget_->setAutoFillBackground(true);
  viewport_widget_->setStyleSheet("background-color: #11142A;");
  viewport_widget_->setFocusPolicy(Qt::StrongFocus);
  viewport_widget_->setMouseTracking(true);
  viewport_widget_->installEventFilter(this);
  setCentralWidget(viewport_widget_);

  const EditorVec3 initial_camera_position{
      viewport_camera_position_[0],
      viewport_camera_position_[1],
      viewport_camera_position_[2],
  };
  const EditorVec3 initial_target{0.0f, 0.0f, 0.0f};
  const EditorVec3 initial_forward =
      NormalizeVec3(SubtractVec3(initial_target, initial_camera_position));
  viewport_camera_yaw_radians_ =
      std::atan2(initial_forward.x, initial_forward.z);
  viewport_camera_pitch_radians_ = std::asin(initial_forward.y);
  viewport_last_frame_time_ = std::chrono::steady_clock::now();

  assets_tree_ = new QTreeWidget(this);
  assets_tree_->setColumnCount(1);
  assets_tree_->setHeaderHidden(true);
  scene_outliner_list_ = new QListWidget(this);
  meta_inspector_ = new QTextEdit(this);
  meta_inspector_->setReadOnly(true);
  startup_scene_selector_ = new QComboBox(this);
  component_type_selector_ = new QComboBox(this);
  component_type_selector_->addItem("MeshRenderer");
  component_type_selector_->addItem("RigidBody");
  add_component_button_ = new QPushButton("Add Component", this);
  remove_component_button_ = new QPushButton("Remove Component", this);

  object_name_input_ = new QLineEdit(this);
  pos_x_input_ = new QLineEdit(this);
  pos_y_input_ = new QLineEdit(this);
  pos_z_input_ = new QLineEdit(this);
  rot_x_input_ = new QLineEdit(this);
  rot_y_input_ = new QLineEdit(this);
  rot_z_input_ = new QLineEdit(this);
  scale_x_input_ = new QLineEdit(this);
  scale_y_input_ = new QLineEdit(this);
  scale_z_input_ = new QLineEdit(this);
  mesh_asset_picker_ = new QComboBox(this);
  material_asset_picker_ = new QComboBox(this);
  rigidbody_type_input_ = new QLineEdit(this);
  rigidbody_mass_input_ = new QLineEdit(this);
  apply_object_button_ = new QPushButton("Apply Object", this);
  save_scene_button_ = new QPushButton("Save Scene", this);

  mesh_renderer_status_label_ = new QLabel("MeshRenderer: not added", this);
  rigidbody_status_label_ = new QLabel("RigidBody: not added", this);

  connect(assets_tree_, &QTreeWidget::currentItemChanged, this,
          [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
            UpdateMetaInspector(ResolveAssetBrowserIndex(current));
          });
  connect(scene_outliner_list_, &QListWidget::currentItemChanged, this,
          [this](QListWidgetItem* current, QListWidgetItem* previous) {
            if (previous != nullptr) {
              const int previous_index = scene_outliner_list_->row(previous);
              ApplyInspectorToObjectIndex(previous_index, false);
            }
            (void)current;
            RefreshInspector();
          });
  connect(startup_scene_selector_, &QComboBox::currentTextChanged, this,
          [this](const QString& text) {
            ApplyInspectorToSelectedObject();
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
  connect(add_component_button_, &QPushButton::clicked, this,
          [this]() { AddComponentToSelectedObject(); });
  connect(remove_component_button_, &QPushButton::clicked, this,
          [this]() { RemoveComponentFromSelectedObject(); });
  auto connect_auto_apply_line_edit = [this](QLineEdit* input) {
    connect(input, &QLineEdit::editingFinished, this,
            [this]() { ApplyInspectorToSelectedObject(); });
  };
  connect_auto_apply_line_edit(object_name_input_);
  connect_auto_apply_line_edit(pos_x_input_);
  connect_auto_apply_line_edit(pos_y_input_);
  connect_auto_apply_line_edit(pos_z_input_);
  connect_auto_apply_line_edit(rot_x_input_);
  connect_auto_apply_line_edit(rot_y_input_);
  connect_auto_apply_line_edit(rot_z_input_);
  connect_auto_apply_line_edit(scale_x_input_);
  connect_auto_apply_line_edit(scale_y_input_);
  connect_auto_apply_line_edit(scale_z_input_);
  connect_auto_apply_line_edit(rigidbody_type_input_);
  connect_auto_apply_line_edit(rigidbody_mass_input_);
  connect(mesh_asset_picker_, &QComboBox::currentTextChanged, this,
          [this](const QString&) { ApplyInspectorToSelectedObject(); });
  connect(material_asset_picker_, &QComboBox::currentTextChanged, this,
          [this](const QString&) { ApplyInspectorToSelectedObject(); });

  auto* build_panel = new QWidget(this);
  auto* build_layout = new QVBoxLayout(build_panel);
  auto* form_layout = new QFormLayout();
  game_name_input_ = new QLineEdit("MyGame", build_panel);
  const auto detected_runtime_build_dir = DetectDefaultRuntimeBuildDir(project_root_);
  runtime_build_dir_input_ =
      new QLineEdit(QString::fromStdString(detected_runtime_build_dir.string()),
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
    RebuildViewportSceneCache();
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
    RebuildViewportSceneCache();
    statusBar()->showMessage("Removed scene object.");
  });

  auto* inspector_panel = new QWidget(this);
  auto* inspector_layout = new QVBoxLayout(inspector_panel);

  auto* object_fields = new QFormLayout();
  object_fields->addRow("Name", object_name_input_);
  object_fields->addRow("Position X", pos_x_input_);
  object_fields->addRow("Position Y", pos_y_input_);
  object_fields->addRow("Position Z", pos_z_input_);
  object_fields->addRow("Rotation X", rot_x_input_);
  object_fields->addRow("Rotation Y", rot_y_input_);
  object_fields->addRow("Rotation Z", rot_z_input_);
  object_fields->addRow("Scale X", scale_x_input_);
  object_fields->addRow("Scale Y", scale_y_input_);
  object_fields->addRow("Scale Z", scale_z_input_);
  inspector_layout->addLayout(object_fields);

  auto* component_actions = new QHBoxLayout();
  component_actions->addWidget(component_type_selector_);
  component_actions->addWidget(add_component_button_);
  component_actions->addWidget(remove_component_button_);
  inspector_layout->addLayout(component_actions);

  mesh_renderer_group_ = new QGroupBox("MeshRenderer", inspector_panel);
  auto* mesh_layout = new QFormLayout(mesh_renderer_group_);
  mesh_layout->addRow("Mesh Asset", mesh_asset_picker_);
  mesh_layout->addRow("Material Asset", material_asset_picker_);

  rigidbody_group_ = new QGroupBox("RigidBody", inspector_panel);
  auto* rigid_layout = new QFormLayout(rigidbody_group_);
  rigid_layout->addRow("RigidBody Type", rigidbody_type_input_);
  rigid_layout->addRow("RigidBody Mass", rigidbody_mass_input_);

  inspector_layout->addWidget(mesh_renderer_status_label_);
  inspector_layout->addWidget(mesh_renderer_group_);
  inspector_layout->addWidget(rigidbody_status_label_);
  inspector_layout->addWidget(rigidbody_group_);

  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock(
                    "Hierarchy",
                    new QLabel("Hierarchy Placeholder", this), this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Meta Inspector", meta_inspector_, this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Object Inspector", inspector_panel, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Assets", assets_tree_, this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Build/Export", build_panel, this));
  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock("Scene Outliner", scene_outliner_panel, this));

  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("E&xit", this, &QWidget::close);

  const auto cooked_registry_path = project_root_ / "content" / "cooked_assets.pakreg";
  asset_registry_.LoadCookedRegistry(cooked_registry_path);
  RefreshAssets();
  RefreshStartupSceneSelector();
  LoadActiveScene();
  RefreshSceneOutliner();
  RefreshInspector();

  viewport_timer_ = new QTimer(this);
  connect(viewport_timer_, &QTimer::timeout, this,
          [this]() { RenderViewportFrame(); });
  viewport_timer_->start(16);
  connect(qApp, &QCoreApplication::aboutToQuit, this,
          [this]() { ShutdownViewportRenderer(); });

  statusBar()->showMessage(
      "Cookie Editor Phase 24D loaded (project root: " +
      QString::fromStdString(project_root_.string()) + ")");
}

void EditorMainWindow::RefreshAssets() {
  assets_tree_->clear();
  discovered_assets_.clear();

  asset_registry_.DiscoverProjectAssets(project_root_);
  discovered_assets_ = asset_registry_.GetDiscoveredAssets();
  RebuildAssetBrowserTree();
  RebuildMeshRendererAssetPickers();

  if (discovered_assets_.empty()) {
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

int EditorMainWindow::ResolveAssetBrowserIndex(QTreeWidgetItem* item) const {
  if (item == nullptr) {
    return -1;
  }
  bool ok = false;
  const int index = item->data(0, Qt::UserRole).toInt(&ok);
  if (!ok) {
    return -1;
  }
  if (index < 0 || static_cast<std::size_t>(index) >= discovered_assets_.size()) {
    return -1;
  }
  return index;
}

void EditorMainWindow::RebuildAssetBrowserTree() {
  assets_tree_->clear();

  QMap<QString, QTreeWidgetItem*> folder_nodes;
  std::function<QTreeWidgetItem*(const QString&)> ensure_folder_path =
      [&](const QString& folder_path) -> QTreeWidgetItem* {
    if (folder_path.isEmpty()) {
      return nullptr;
    }
    if (folder_nodes.contains(folder_path)) {
      return folder_nodes[folder_path];
    }

    const int sep = folder_path.lastIndexOf('/');
    QTreeWidgetItem* parent = nullptr;
    if (sep > 0) {
      parent = ensure_folder_path(folder_path.left(sep));
    }
    auto* node = new QTreeWidgetItem();
    node->setText(0, folder_path.mid(sep + 1));
    node->setData(0, Qt::UserRole, -1);
    if (parent != nullptr) {
      parent->addChild(node);
    } else {
      assets_tree_->addTopLevelItem(node);
    }
    folder_nodes.insert(folder_path, node);
    return node;
  };

  for (std::size_t i = 0; i < discovered_assets_.size(); ++i) {
    const auto& asset = discovered_assets_[i];
    QString rel = QString::fromStdString(asset.source_relative_path);
    rel.replace('\\', '/');
    const int sep = rel.lastIndexOf('/');
    QTreeWidgetItem* parent = nullptr;
    if (sep > 0) {
      parent = ensure_folder_path(rel.left(sep));
    }

    auto* file_item = new QTreeWidgetItem();
    file_item->setText(0, rel.mid(sep + 1));
    file_item->setData(0, Qt::UserRole, static_cast<int>(i));
    file_item->setToolTip(
        0, rel + "\nAssetId: " + QString::fromStdString(asset.meta.asset_id));
    if (parent != nullptr) {
      parent->addChild(file_item);
    } else {
      assets_tree_->addTopLevelItem(file_item);
    }
  }

  assets_tree_->expandToDepth(1);
  if (!discovered_assets_.empty()) {
    QTreeWidgetItemIterator it(assets_tree_);
    while (*it != nullptr) {
      const int idx = ResolveAssetBrowserIndex(*it);
      if (idx >= 0) {
        assets_tree_->setCurrentItem(*it);
        break;
      }
      ++it;
    }
  }
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
    QString label = QString::fromStdString(object.name);
    if (object.has_mesh_renderer) {
      label += " [MeshRenderer]";
    }
    if (object.has_rigidbody) {
      label += " [RigidBody]";
    }
    scene_outliner_list_->addItem(label);
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
  RebuildViewportSceneCache();
}

void EditorMainWindow::RefreshInspector() {
  inspector_updating_ = true;
  const int index = CurrentSceneObjectIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
    object_name_input_->setText("");
    pos_x_input_->setText("");
    pos_y_input_->setText("");
    pos_z_input_->setText("");
    rot_x_input_->setText("");
    rot_y_input_->setText("");
    rot_z_input_->setText("");
    scale_x_input_->setText("");
    scale_y_input_->setText("");
    scale_z_input_->setText("");
    mesh_asset_picker_->setCurrentIndex(-1);
    material_asset_picker_->setCurrentIndex(-1);
    rigidbody_type_input_->setText("");
    rigidbody_mass_input_->setText("");
    mesh_renderer_status_label_->setText("MeshRenderer: not added");
    rigidbody_status_label_->setText("RigidBody: not added");
    mesh_renderer_group_->setVisible(false);
    rigidbody_group_->setVisible(false);
    inspector_updating_ = false;
    return;
  }

  const auto& object = active_scene_.objects[static_cast<std::size_t>(index)];
  object_name_input_->setText(QString::fromStdString(object.name));
  pos_x_input_->setText(QString::number(object.transform.position[0]));
  pos_y_input_->setText(QString::number(object.transform.position[1]));
  pos_z_input_->setText(QString::number(object.transform.position[2]));
  rot_x_input_->setText(QString::number(object.transform.rotation_euler_degrees[0]));
  rot_y_input_->setText(QString::number(object.transform.rotation_euler_degrees[1]));
  rot_z_input_->setText(QString::number(object.transform.rotation_euler_degrees[2]));
  scale_x_input_->setText(QString::number(object.transform.scale[0]));
  scale_y_input_->setText(QString::number(object.transform.scale[1]));
  scale_z_input_->setText(QString::number(object.transform.scale[2]));

  if (object.has_mesh_renderer) {
    mesh_renderer_group_->setVisible(true);
    mesh_renderer_status_label_->setText("MeshRenderer: added");
    int mesh_index = mesh_asset_picker_->findData(
        QString::fromStdString(object.mesh_renderer.mesh_asset_id));
    int material_index = material_asset_picker_->findData(
        QString::fromStdString(object.mesh_renderer.material_asset_id));
    mesh_asset_picker_->setCurrentIndex(mesh_index);
    material_asset_picker_->setCurrentIndex(material_index);
  } else {
    mesh_renderer_group_->setVisible(false);
    mesh_renderer_status_label_->setText("MeshRenderer: not added");
    mesh_asset_picker_->setCurrentIndex(-1);
    material_asset_picker_->setCurrentIndex(-1);
  }

  if (object.has_rigidbody) {
    rigidbody_group_->setVisible(true);
    rigidbody_status_label_->setText("RigidBody: added");
    rigidbody_type_input_->setText(QString::fromStdString(object.rigidbody.body_type));
    rigidbody_mass_input_->setText(QString::number(object.rigidbody.mass));
  } else {
    rigidbody_group_->setVisible(false);
    rigidbody_status_label_->setText("RigidBody: not added");
    rigidbody_type_input_->setText("");
    rigidbody_mass_input_->setText("");
  }
  inspector_updating_ = false;
}

void EditorMainWindow::ApplyInspectorToObjectIndex(int index, bool show_status) {
  if (inspector_updating_) {
    return;
  }
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
  TryParseFloat(rot_x_input_->text(), object.transform.rotation_euler_degrees[0]);
  TryParseFloat(rot_y_input_->text(), object.transform.rotation_euler_degrees[1]);
  TryParseFloat(rot_z_input_->text(), object.transform.rotation_euler_degrees[2]);
  TryParseFloat(scale_x_input_->text(), object.transform.scale[0]);
  TryParseFloat(scale_y_input_->text(), object.transform.scale[1]);
  TryParseFloat(scale_z_input_->text(), object.transform.scale[2]);

  if (object.has_mesh_renderer) {
    object.mesh_renderer.mesh_asset_id =
        mesh_asset_picker_->currentData().toString().trimmed().toStdString();
    object.mesh_renderer.material_asset_id =
        material_asset_picker_->currentData().toString().trimmed().toStdString();
  }

  if (object.has_rigidbody) {
    object.rigidbody.body_type = rigidbody_type_input_->text().trimmed().toStdString();
    if (object.rigidbody.body_type.empty()) {
      object.rigidbody.body_type = "dynamic";
    }
    TryParseFloat(rigidbody_mass_input_->text(), object.rigidbody.mass);
  }

  RefreshSceneOutliner();
  scene_outliner_list_->setCurrentRow(index);
  RebuildViewportSceneCache();
  if (show_status) {
    statusBar()->showMessage("Applied object edits.");
  }
}

void EditorMainWindow::ApplyInspectorToSelectedObject() {
  ApplyInspectorToObjectIndex(CurrentSceneObjectIndex(), true);
}

void EditorMainWindow::RebuildMeshRendererAssetPickers() {
  const int current_object_index = CurrentSceneObjectIndex();
  std::string mesh_selected_id;
  std::string material_selected_id;
  if (current_object_index >= 0 &&
      static_cast<std::size_t>(current_object_index) < active_scene_.objects.size()) {
    const auto& object = active_scene_.objects[static_cast<std::size_t>(current_object_index)];
    mesh_selected_id = object.mesh_renderer.mesh_asset_id;
    material_selected_id = object.mesh_renderer.material_asset_id;
  }

  inspector_updating_ = true;
  mesh_asset_picker_->clear();
  material_asset_picker_->clear();
  mesh_asset_picker_->addItem("(None)", QString{});
  material_asset_picker_->addItem("(None)", QString{});

  for (const auto& asset : discovered_assets_) {
    const QString display = QString::fromStdString(asset.source_relative_path);
    const QString asset_id = QString::fromStdString(asset.meta.asset_id);
    if (IsTypeOneOf(asset.meta.type, {"Mesh", "Model"})) {
      mesh_asset_picker_->addItem(display, asset_id);
    }
    if (IsTypeOneOf(asset.meta.type, {"Material", "MaterialAsset"})) {
      material_asset_picker_->addItem(display, asset_id);
    }
  }

  int mesh_index = mesh_asset_picker_->findData(QString::fromStdString(mesh_selected_id));
  int material_index =
      material_asset_picker_->findData(QString::fromStdString(material_selected_id));
  if (mesh_index < 0) {
    mesh_index = 0;
  }
  if (material_index < 0) {
    material_index = 0;
  }
  mesh_asset_picker_->setCurrentIndex(mesh_index);
  material_asset_picker_->setCurrentIndex(material_index);
  inspector_updating_ = false;
}

void EditorMainWindow::AddComponentToSelectedObject() {
  const int index = CurrentSceneObjectIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
    return;
  }

  auto& object = active_scene_.objects[static_cast<std::size_t>(index)];
  const std::string component = component_type_selector_->currentText().toStdString();
  if (component == "MeshRenderer") {
    object.has_mesh_renderer = true;
  } else if (component == "RigidBody") {
    object.has_rigidbody = true;
    if (object.rigidbody.body_type.empty()) {
      object.rigidbody.body_type = "dynamic";
    }
  }

  RefreshSceneOutliner();
  scene_outliner_list_->setCurrentRow(index);
  RefreshInspector();
  RebuildViewportSceneCache();
  statusBar()->showMessage("Added component: " + component_type_selector_->currentText());
}

void EditorMainWindow::RemoveComponentFromSelectedObject() {
  const int index = CurrentSceneObjectIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= active_scene_.objects.size()) {
    return;
  }

  auto& object = active_scene_.objects[static_cast<std::size_t>(index)];
  const std::string component = component_type_selector_->currentText().toStdString();
  if (component == "MeshRenderer") {
    object.has_mesh_renderer = false;
    object.mesh_renderer = {};
  } else if (component == "RigidBody") {
    object.has_rigidbody = false;
    object.rigidbody = {};
  }

  RefreshSceneOutliner();
  scene_outliner_list_->setCurrentRow(index);
  RefreshInspector();
  RebuildViewportSceneCache();
  statusBar()->showMessage("Removed component: " + component_type_selector_->currentText());
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
  RebuildViewportSceneCache();
  statusBar()->showMessage(
      "Saved scene: " + QString::fromStdString(active_scene_path_.string()));
}

void EditorMainWindow::EnsureViewportRendererInitialized() {
  if (viewport_renderer_initialized_) {
    return;
  }
  if (viewport_widget_ == nullptr) {
    return;
  }

  viewport_renderer_ = cookie::renderer::dx11::CreateRendererDX11Backend();
  if (!viewport_renderer_) {
    statusBar()->showMessage("Viewport renderer creation failed.");
    return;
  }

  const WId window_id = viewport_widget_->winId();
  cookie::renderer::RendererInitInfo init{};
  init.native_window_handle = reinterpret_cast<void*>(window_id);
  init.window_width = std::max(1, viewport_widget_->width());
  init.window_height = std::max(1, viewport_widget_->height());
  init.enable_vsync = true;
  if (!viewport_renderer_->Initialize(init)) {
    viewport_renderer_.reset();
    statusBar()->showMessage("Viewport renderer initialization failed.");
    return;
  }

  viewport_renderer_initialized_ = true;
  statusBar()->showMessage("Viewport renderer initialized.");
}

void EditorMainWindow::ShutdownViewportRenderer() {
  if (viewport_renderer_) {
    viewport_renderer_->Shutdown();
    viewport_renderer_.reset();
  }
  viewport_renderer_initialized_ = false;
}

void EditorMainWindow::RebuildViewportSceneCache() {
  viewport_mesh_cache_.clear();
  viewport_mesh_textures_.clear();
  viewport_mesh_transforms_.clear();

  for (const auto& object : active_scene_.objects) {
    if (!object.has_mesh_renderer) {
      continue;
    }

    const auto mesh_record = asset_registry_.ResolveCookedRecord(
        object.mesh_renderer.mesh_asset_id);
    if (!mesh_record.has_value() ||
        !IsTypeOneOf(mesh_record->type, {"Mesh", "Model"})) {
      continue;
    }
    const auto material_record = asset_registry_.ResolveCookedRecord(
        object.mesh_renderer.material_asset_id);
    if (!material_record.has_value() ||
        !IsTypeOneOf(material_record->type, {"Material", "MaterialAsset"})) {
      continue;
    }

    const auto mesh_path =
        asset_registry_.ResolveAssetPath(object.mesh_renderer.mesh_asset_id);
    const auto material_path =
        asset_registry_.ResolveAssetPath(object.mesh_renderer.material_asset_id);
    if (mesh_path.empty() || material_path.empty()) {
      continue;
    }

    cookie::assets::MaterialAsset material_asset;
    if (!cookie::assets::LoadMaterialAsset(material_path, material_asset, nullptr)) {
      continue;
    }

    std::string albedo_texture;
    if (!material_asset.albedo_asset_id.empty()) {
      const auto texture_path =
          asset_registry_.ResolveAssetPath(material_asset.albedo_asset_id);
      if (!texture_path.empty()) {
        albedo_texture = texture_path.string();
      }
    }

    cookie::renderer::ImportedMesh imported_mesh;
    if (!cookie::renderer::LoadMeshFromPath(mesh_path, imported_mesh, nullptr)) {
      continue;
    }

    for (const auto& primitive : imported_mesh.primitives) {
      viewport_mesh_cache_.push_back(primitive);
      viewport_mesh_textures_.push_back(albedo_texture);
      viewport_mesh_transforms_.push_back(BuildObjectTransform(object));
    }
  }
}

void EditorMainWindow::RenderViewportFrame() {
  EnsureViewportRendererInitialized();
  if (!viewport_renderer_initialized_ || !viewport_renderer_) {
    return;
  }

  if (!viewport_renderer_->BeginFrame()) {
    return;
  }

  viewport_renderer_->Clear({0.07f, 0.08f, 0.16f, 1.0f});

  cookie::renderer::SceneBuilder scene_builder;
  const auto now = std::chrono::steady_clock::now();
  float delta_time_seconds =
      std::chrono::duration<float>(now - viewport_last_frame_time_).count();
  viewport_last_frame_time_ = now;
  if (delta_time_seconds <= 0.0f) {
    delta_time_seconds = 1.0f / 60.0f;
  }
  delta_time_seconds = std::min(delta_time_seconds, 0.1f);
  UpdateViewportCamera(delta_time_seconds);

  const float aspect_ratio =
      static_cast<float>(std::max(1, viewport_widget_->width())) /
      static_cast<float>(std::max(1, viewport_widget_->height()));
  const auto projection = cookie::renderer::MakePerspectiveProjection(
      1.04719755f, aspect_ratio, 0.01f, 100.0f);
  const EditorVec3 world_up{0.0f, 1.0f, 0.0f};
  const EditorVec3 forward = NormalizeVec3({
      std::cos(viewport_camera_pitch_radians_) * std::sin(viewport_camera_yaw_radians_),
      std::sin(viewport_camera_pitch_radians_),
      std::cos(viewport_camera_pitch_radians_) * std::cos(viewport_camera_yaw_radians_),
  });
  const EditorVec3 camera_position{
      viewport_camera_position_[0],
      viewport_camera_position_[1],
      viewport_camera_position_[2],
  };
  const EditorVec3 camera_target = AddVec3(camera_position, forward);
  const auto view = cookie::renderer::MakeLookAtView(
      camera_position.x, camera_position.y, camera_position.z,
      camera_target.x, camera_target.y, camera_target.z,
      world_up.x, world_up.y, world_up.z);
  const auto view_projection = cookie::renderer::MultiplyTransforms(view, projection);

  for (std::size_t i = 0; i < viewport_mesh_cache_.size(); ++i) {
    cookie::renderer::RenderMaterial material{};
    const auto& albedo = viewport_mesh_textures_[i];
    material.albedo_texture_path = albedo.empty() ? nullptr : albedo.c_str();
    material.tint[0] = 1.0f;
    material.tint[1] = 1.0f;
    material.tint[2] = 1.0f;
    material.tint[3] = 1.0f;
    material.use_albedo = !albedo.empty();

    const std::size_t material_index = scene_builder.AddMaterial(material);
    const auto model_transform = cookie::renderer::MultiplyTransforms(
        viewport_mesh_transforms_[i], view_projection);

    const auto& primitive = viewport_mesh_cache_[i];
    if (!primitive.indices.empty()) {
      scene_builder.AddIndexedMeshInstance(
          primitive.vertices.data(), primitive.vertices.size(),
          primitive.indices.data(), primitive.indices.size(), material_index,
          model_transform);
    } else {
      scene_builder.AddMeshInstance(
          primitive.vertices.data(), primitive.vertices.size(), material_index,
          model_transform);
    }
  }

  viewport_renderer_->SubmitScene(scene_builder.Build());
  viewport_renderer_->EndFrame();
}

bool EditorMainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched != viewport_widget_ || viewport_widget_ == nullptr || event == nullptr) {
    return QMainWindow::eventFilter(watched, event);
  }

  switch (event->type()) {
    case QEvent::Enter:
      viewport_is_hovered_ = true;
      break;
    case QEvent::Leave:
      viewport_is_hovered_ = false;
      viewport_mouse_look_active_ = false;
      if (viewport_cursor_captured_) {
        viewport_widget_->unsetCursor();
        viewport_cursor_captured_ = false;
      }
      viewport_pending_mouse_delta_x_ = 0.0f;
      viewport_pending_mouse_delta_y_ = 0.0f;
      break;
    case QEvent::MouseButtonPress: {
      const auto* mouse_event = static_cast<QMouseEvent*>(event);
      if (mouse_event->button() == Qt::RightButton) {
        viewport_mouse_look_active_ = viewport_is_hovered_ || viewport_widget_->hasFocus();
        viewport_widget_->setFocus();
        if (viewport_mouse_look_active_) {
          viewport_widget_->setCursor(Qt::BlankCursor);
          viewport_cursor_captured_ = true;
          const QPoint local_center(
              viewport_widget_->width() / 2, viewport_widget_->height() / 2);
          QCursor::setPos(viewport_widget_->mapToGlobal(local_center));
        }
      }
      break;
    }
    case QEvent::MouseButtonRelease: {
      const auto* mouse_event = static_cast<QMouseEvent*>(event);
      if (mouse_event->button() == Qt::RightButton) {
        viewport_mouse_look_active_ = false;
        if (viewport_cursor_captured_) {
          viewport_widget_->unsetCursor();
          viewport_cursor_captured_ = false;
        }
      }
      break;
    }
    case QEvent::MouseMove: {
      const auto* mouse_event = static_cast<QMouseEvent*>(event);
      if (viewport_mouse_look_active_ && (viewport_is_hovered_ || viewport_widget_->hasFocus())) {
        const QPoint local_center(
            viewport_widget_->width() / 2, viewport_widget_->height() / 2);
        const QPointF current_local = mouse_event->position();
        viewport_pending_mouse_delta_x_ +=
            static_cast<float>(current_local.x() - local_center.x());
        viewport_pending_mouse_delta_y_ +=
            static_cast<float>(current_local.y() - local_center.y());
        QCursor::setPos(viewport_widget_->mapToGlobal(local_center));
      }
      break;
    }
    case QEvent::FocusOut:
      viewport_mouse_look_active_ = false;
      if (viewport_cursor_captured_) {
        viewport_widget_->unsetCursor();
        viewport_cursor_captured_ = false;
      }
      viewport_key_w_down_ = false;
      viewport_key_a_down_ = false;
      viewport_key_s_down_ = false;
      viewport_key_d_down_ = false;
      viewport_key_q_down_ = false;
      viewport_key_e_down_ = false;
      viewport_key_shift_down_ = false;
      break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
      const auto* key_event = static_cast<QKeyEvent*>(event);
      const bool is_down = (event->type() == QEvent::KeyPress);
      switch (key_event->key()) {
        case Qt::Key_W:
          viewport_key_w_down_ = is_down;
          break;
        case Qt::Key_A:
          viewport_key_a_down_ = is_down;
          break;
        case Qt::Key_S:
          viewport_key_s_down_ = is_down;
          break;
        case Qt::Key_D:
          viewport_key_d_down_ = is_down;
          break;
        case Qt::Key_Q:
          viewport_key_q_down_ = is_down;
          break;
        case Qt::Key_E:
          viewport_key_e_down_ = is_down;
          break;
        case Qt::Key_Shift:
          viewport_key_shift_down_ = is_down;
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return QMainWindow::eventFilter(watched, event);
}

void EditorMainWindow::UpdateViewportCamera(float delta_time_seconds) {
  if (viewport_mouse_look_active_ && (viewport_is_hovered_ || viewport_widget_->hasFocus())) {
    constexpr float kMouseSensitivity = 0.0025f;
    viewport_camera_yaw_radians_ += viewport_pending_mouse_delta_x_ * kMouseSensitivity;
    viewport_camera_pitch_radians_ -= viewport_pending_mouse_delta_y_ * kMouseSensitivity;
    viewport_camera_pitch_radians_ =
        std::clamp(viewport_camera_pitch_radians_, -1.54f, 1.54f);
  }
  viewport_pending_mouse_delta_x_ = 0.0f;
  viewport_pending_mouse_delta_y_ = 0.0f;

  if (!viewport_mouse_look_active_ || !(viewport_is_hovered_ || viewport_widget_->hasFocus())) {
    return;
  }

  constexpr float kMoveSpeed = 4.5f;
  constexpr float kFastMoveSpeed = 11.0f;
  const EditorVec3 forward = NormalizeVec3({
      std::cos(viewport_camera_pitch_radians_) * std::sin(viewport_camera_yaw_radians_),
      std::sin(viewport_camera_pitch_radians_),
      std::cos(viewport_camera_pitch_radians_) * std::cos(viewport_camera_yaw_radians_),
  });
  const EditorVec3 world_up{0.0f, 1.0f, 0.0f};
  const EditorVec3 right = NormalizeVec3(CrossVec3(world_up, forward));

  EditorVec3 movement{};
  if (viewport_key_w_down_) {
    movement = AddVec3(movement, forward);
  }
  if (viewport_key_s_down_) {
    movement = SubtractVec3(movement, forward);
  }
  if (viewport_key_d_down_) {
    movement = AddVec3(movement, right);
  }
  if (viewport_key_a_down_) {
    movement = SubtractVec3(movement, right);
  }
  if (viewport_key_e_down_) {
    movement = AddVec3(movement, world_up);
  }
  if (viewport_key_q_down_) {
    movement = SubtractVec3(movement, world_up);
  }

  const float movement_length = LengthVec3(movement);
  if (movement_length <= 0.0001f) {
    return;
  }
  const float speed = viewport_key_shift_down_ ? kFastMoveSpeed : kMoveSpeed;
  const EditorVec3 direction = ScaleVec3(movement, 1.0f / movement_length);
  const EditorVec3 delta = ScaleVec3(direction, speed * delta_time_seconds);
  viewport_camera_position_[0] += delta.x;
  viewport_camera_position_[1] += delta.y;
  viewport_camera_position_[2] += delta.z;
}

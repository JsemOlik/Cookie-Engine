#pragma once

#include <QMainWindow>

#include <filesystem>
#include <chrono>
#include <memory>
#include <vector>

#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Assets/SceneAsset.h"
#include "Cookie/Renderer/MeshAsset.h"
#include "Cookie/Renderer/RendererBackend.h"

namespace cookie::renderer {
class SceneBuilder;
}

class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QWidget;
class QTimer;
class QLabel;
class QGroupBox;

class EditorMainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit EditorMainWindow(
      const std::filesystem::path& project_root_override = {},
      QWidget* parent = nullptr);

 private:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void RefreshAssets();
  void RefreshStartupSceneSelector();
  void RefreshSceneOutliner();
  void UpdateMetaInspector(int index);
  void LoadActiveScene();
  void RefreshInspector();
  void SaveActiveScene();
  void RebuildAssetBrowserTree();
  void RebuildMeshRendererAssetPickers();
  void ApplyInspectorToObjectIndex(int index, bool show_status);
  int ResolveAssetBrowserIndex(QTreeWidgetItem* item) const;
  void ApplyInspectorToSelectedObject();
  int CurrentSceneObjectIndex() const;
  void AddComponentToSelectedObject();
  void RemoveComponentFromSelectedObject();

  void EnsureViewportRendererInitialized();
  void ShutdownViewportRenderer();
  void RebuildViewportSceneCache();
  void RenderViewportFrame();
  void UpdateViewportCamera(float delta_time_seconds);
  void SetSelectedObjectIndexFromViewport(int index);
  bool TryPickSceneObjectFromViewport(float mouse_x, float mouse_y, int& out_index) const;
  void RenderSelectedObjectGizmo(cookie::renderer::SceneBuilder& scene_builder,
                                 const cookie::renderer::Float4x4& view_projection);
  bool BeginGizmoDrag(float mouse_x, float mouse_y);
  void UpdateGizmoDrag(float mouse_x, float mouse_y);
  void EndGizmoDrag();

  std::filesystem::path project_root_;
  cookie::assets::AssetRegistry asset_registry_;
  std::vector<cookie::assets::DiscoveredAsset> discovered_assets_;
  cookie::assets::SceneAsset active_scene_;
  std::filesystem::path active_scene_path_;
  QTreeWidget* assets_tree_ = nullptr;
  QListWidget* scene_outliner_list_ = nullptr;
  QTextEdit* meta_inspector_ = nullptr;
  QComboBox* startup_scene_selector_ = nullptr;
  QLineEdit* object_name_input_ = nullptr;
  QLineEdit* pos_x_input_ = nullptr;
  QLineEdit* pos_y_input_ = nullptr;
  QLineEdit* pos_z_input_ = nullptr;
  QLineEdit* scale_x_input_ = nullptr;
  QLineEdit* scale_y_input_ = nullptr;
  QLineEdit* scale_z_input_ = nullptr;
  QLineEdit* rot_x_input_ = nullptr;
  QLineEdit* rot_y_input_ = nullptr;
  QLineEdit* rot_z_input_ = nullptr;
  QComboBox* component_type_selector_ = nullptr;
  QPushButton* add_component_button_ = nullptr;
  QPushButton* remove_component_button_ = nullptr;
  QLabel* mesh_renderer_status_label_ = nullptr;
  QLabel* rigidbody_status_label_ = nullptr;
  QGroupBox* mesh_renderer_group_ = nullptr;
  QGroupBox* rigidbody_group_ = nullptr;
  QComboBox* mesh_asset_picker_ = nullptr;
  QComboBox* material_asset_picker_ = nullptr;
  QLineEdit* rigidbody_type_input_ = nullptr;
  QLineEdit* rigidbody_mass_input_ = nullptr;
  QPushButton* apply_object_button_ = nullptr;
  QPushButton* save_scene_button_ = nullptr;
  QLineEdit* game_name_input_ = nullptr;
  QLineEdit* runtime_build_dir_input_ = nullptr;
  QLineEdit* export_parent_input_ = nullptr;
  QTextEdit* build_output_ = nullptr;

  QWidget* viewport_widget_ = nullptr;
  QTimer* viewport_timer_ = nullptr;
  std::unique_ptr<cookie::renderer::IRendererBackend> viewport_renderer_;
  bool viewport_renderer_initialized_ = false;
  std::vector<cookie::renderer::ImportedPrimitive> viewport_mesh_cache_;
  std::vector<std::string> viewport_mesh_textures_;
  std::vector<cookie::renderer::Float4x4> viewport_mesh_transforms_;
  bool inspector_updating_ = false;
  enum class GizmoMode {
    kTranslate = 0,
    kRotate = 1,
    kScale = 2,
  };
  QComboBox* gizmo_mode_selector_ = nullptr;
  GizmoMode gizmo_mode_ = GizmoMode::kTranslate;
  int selected_object_index_ = -1;
  bool gizmo_drag_active_ = false;
  int gizmo_drag_axis_ = -1;
  float gizmo_drag_start_mouse_x_ = 0.0f;
  float gizmo_drag_start_mouse_y_ = 0.0f;
  float gizmo_drag_start_position_[3] = {0.0f, 0.0f, 0.0f};
  float gizmo_drag_start_rotation_[3] = {0.0f, 0.0f, 0.0f};
  float gizmo_drag_start_scale_[3] = {1.0f, 1.0f, 1.0f};

  float viewport_camera_position_[3] = {4.0f, 3.0f, -7.0f};
  float viewport_camera_yaw_radians_ = 0.0f;
  float viewport_camera_pitch_radians_ = 0.0f;
  bool viewport_is_hovered_ = false;
  bool viewport_mouse_look_active_ = false;
  bool viewport_cursor_captured_ = false;
  float viewport_pending_mouse_delta_x_ = 0.0f;
  float viewport_pending_mouse_delta_y_ = 0.0f;
  bool viewport_key_w_down_ = false;
  bool viewport_key_a_down_ = false;
  bool viewport_key_s_down_ = false;
  bool viewport_key_d_down_ = false;
  bool viewport_key_q_down_ = false;
  bool viewport_key_e_down_ = false;
  bool viewport_key_shift_down_ = false;
  std::chrono::steady_clock::time_point viewport_last_frame_time_{};
};

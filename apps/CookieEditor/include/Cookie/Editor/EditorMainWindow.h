#pragma once

#include <QMainWindow>

#include <filesystem>
#include <memory>
#include <vector>

#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Assets/SceneAsset.h"
#include "Cookie/Renderer/MeshAsset.h"
#include "Cookie/Renderer/RendererBackend.h"

class QListWidget;
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
  void RefreshAssets();
  void RefreshStartupSceneSelector();
  void RefreshSceneOutliner();
  void UpdateMetaInspector(int index);
  void LoadActiveScene();
  void RefreshInspector();
  void SaveActiveScene();
  void ApplyInspectorToSelectedObject();
  int CurrentSceneObjectIndex() const;
  void AddComponentToSelectedObject();
  void RemoveComponentFromSelectedObject();

  void EnsureViewportRendererInitialized();
  void ShutdownViewportRenderer();
  void RebuildViewportSceneCache();
  void RenderViewportFrame();

  std::filesystem::path project_root_;
  cookie::assets::AssetRegistry asset_registry_;
  std::vector<cookie::assets::DiscoveredAsset> discovered_assets_;
  cookie::assets::SceneAsset active_scene_;
  std::filesystem::path active_scene_path_;
  QListWidget* assets_list_ = nullptr;
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
  QLineEdit* mesh_asset_id_input_ = nullptr;
  QLineEdit* material_asset_id_input_ = nullptr;
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
};

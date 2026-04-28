#pragma once

#include <QMainWindow>

#include <filesystem>
#include <vector>

#include "Cookie/Assets/AssetRegistry.h"
#include "Cookie/Assets/SceneAsset.h"

class QListWidget;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QPushButton;

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
  QLineEdit* mesh_asset_id_input_ = nullptr;
  QLineEdit* material_asset_id_input_ = nullptr;
  QCheckBox* rigidbody_enabled_input_ = nullptr;
  QLineEdit* rigidbody_type_input_ = nullptr;
  QLineEdit* rigidbody_mass_input_ = nullptr;
  QPushButton* apply_object_button_ = nullptr;
  QPushButton* save_scene_button_ = nullptr;
  QLineEdit* game_name_input_ = nullptr;
  QLineEdit* runtime_build_dir_input_ = nullptr;
  QLineEdit* export_parent_input_ = nullptr;
  QTextEdit* build_output_ = nullptr;
};

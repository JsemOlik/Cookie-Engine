#pragma once

#include <QMainWindow>

#include <filesystem>
#include <vector>

#include "Cookie/Assets/AssetRegistry.h"

class QListWidget;
class QLineEdit;
class QTextEdit;

class EditorMainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit EditorMainWindow(QWidget* parent = nullptr);

 private:
  void RefreshAssets();
  void UpdateMetaInspector(int index);

  std::filesystem::path project_root_;
  cookie::assets::AssetRegistry asset_registry_;
  std::vector<cookie::assets::DiscoveredAsset> discovered_assets_;
  QListWidget* assets_list_ = nullptr;
  QTextEdit* meta_inspector_ = nullptr;
  QLineEdit* game_name_input_ = nullptr;
  QLineEdit* runtime_build_dir_input_ = nullptr;
  QLineEdit* export_parent_input_ = nullptr;
  QTextEdit* build_output_ = nullptr;
};

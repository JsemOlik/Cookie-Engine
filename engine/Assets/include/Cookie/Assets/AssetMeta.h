#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cookie::assets {

struct AssetMetaSetting {
  std::string key;
  std::string value;
};

struct AssetMeta {
  std::string asset_id;
  std::string type;
  std::string importer;
  int importer_version = 1;
  std::string source_fingerprint;
  std::vector<AssetMetaSetting> settings;
  std::vector<std::string> dependencies;
  std::vector<std::string> labels;
};

bool LoadAssetMeta(const std::filesystem::path& meta_path, AssetMeta& out_meta,
                   std::string* error_message = nullptr);
bool SaveAssetMeta(const std::filesystem::path& meta_path, const AssetMeta& meta,
                   std::string* error_message = nullptr);

}  // namespace cookie::assets

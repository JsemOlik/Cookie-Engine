#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cookie::assets {

struct CookedAssetRecord {
  std::string asset_id;
  std::string type;
  std::string runtime_relative_path;
  std::string source_relative_path;
  std::vector<std::string> dependencies;
};

class CookedAssetRegistry {
 public:
  bool LoadFromFile(const std::filesystem::path& registry_path);
  bool SaveToFile(const std::filesystem::path& registry_path) const;

  const CookedAssetRecord* FindByAssetId(std::string_view asset_id) const;
  const std::vector<CookedAssetRecord>& Entries() const;

  void Clear();
  void AddOrReplace(CookedAssetRecord record);

 private:
  std::vector<CookedAssetRecord> records_;
};

}  // namespace cookie::assets

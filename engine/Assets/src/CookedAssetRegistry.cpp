#include "Cookie/Assets/CookedAssetRegistry.h"

#include <algorithm>
#include <fstream>

namespace cookie::assets {
namespace {

std::string Trim(std::string value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

}  // namespace

bool CookedAssetRegistry::LoadFromFile(const std::filesystem::path& registry_path) {
  std::ifstream file(registry_path);
  if (!file.is_open()) {
    return false;
  }

  std::vector<CookedAssetRecord> parsed_records;
  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }

    std::size_t cursor = 0;
    std::vector<std::string> fields;
    while (cursor <= trimmed.size()) {
      const std::size_t next = trimmed.find('|', cursor);
      if (next == std::string::npos) {
        fields.push_back(trimmed.substr(cursor));
        break;
      }
      fields.push_back(trimmed.substr(cursor, next - cursor));
      cursor = next + 1;
    }

    if (fields.size() < 4) {
      continue;
    }

    CookedAssetRecord record{};
    record.asset_id = fields[0];
    record.type = fields[1];
    record.runtime_relative_path = fields[2];
    record.source_relative_path = fields[3];
    if (fields.size() >= 5 && !fields[4].empty()) {
      std::size_t dep_cursor = 0;
      while (dep_cursor <= fields[4].size()) {
        const std::size_t dep_next = fields[4].find(',', dep_cursor);
        if (dep_next == std::string::npos) {
          record.dependencies.push_back(fields[4].substr(dep_cursor));
          break;
        }
        record.dependencies.push_back(
            fields[4].substr(dep_cursor, dep_next - dep_cursor));
        dep_cursor = dep_next + 1;
      }
    }
    parsed_records.push_back(std::move(record));
  }

  records_ = std::move(parsed_records);
  return true;
}

bool CookedAssetRegistry::SaveToFile(
    const std::filesystem::path& registry_path) const {
  std::error_code error;
  std::filesystem::create_directories(registry_path.parent_path(), error);
  if (error) {
    return false;
  }

  std::ofstream file(registry_path, std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }

  file << "# asset_id|type|runtime_relative_path|source_relative_path|deps\n";
  for (const auto& record : records_) {
    file << record.asset_id << "|" << record.type << "|"
         << record.runtime_relative_path << "|" << record.source_relative_path
         << "|";
    for (std::size_t i = 0; i < record.dependencies.size(); ++i) {
      file << record.dependencies[i];
      if (i + 1 < record.dependencies.size()) {
        file << ",";
      }
    }
    file << "\n";
  }

  return true;
}

const CookedAssetRecord* CookedAssetRegistry::FindByAssetId(
    std::string_view asset_id) const {
  const auto iter = std::find_if(
      records_.begin(), records_.end(),
      [asset_id](const CookedAssetRecord& record) {
        return record.asset_id == asset_id;
      });
  if (iter == records_.end()) {
    return nullptr;
  }
  return &(*iter);
}

const std::vector<CookedAssetRecord>& CookedAssetRegistry::Entries() const {
  return records_;
}

void CookedAssetRegistry::Clear() { records_.clear(); }

void CookedAssetRegistry::AddOrReplace(CookedAssetRecord record) {
  const auto iter = std::find_if(
      records_.begin(), records_.end(),
      [&record](const CookedAssetRecord& existing) {
        return existing.asset_id == record.asset_id;
      });
  if (iter != records_.end()) {
    *iter = std::move(record);
    return;
  }
  records_.push_back(std::move(record));
}

}  // namespace cookie::assets

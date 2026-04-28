#include "Cookie/Assets/AssetMeta.h"

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

bool ParseKeyValueLine(const std::string& line, std::string& out_key,
                       std::string& out_value) {
  const std::size_t separator = line.find(':');
  if (separator == std::string::npos) {
    return false;
  }
  out_key = Trim(line.substr(0, separator));
  out_value = Trim(line.substr(separator + 1));
  return !out_key.empty();
}

}  // namespace

bool LoadAssetMeta(const std::filesystem::path& meta_path, AssetMeta& out_meta,
                   std::string* error_message) {
  std::ifstream file(meta_path);
  if (!file.is_open()) {
    if (error_message) {
      *error_message = "Failed to open meta file: " + meta_path.string();
    }
    return false;
  }

  AssetMeta parsed{};
  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }

    std::string key;
    std::string value;
    if (!ParseKeyValueLine(trimmed, key, value)) {
      continue;
    }

    if (key == "asset_id" || key == "guid") {
      parsed.asset_id = value;
    } else if (key == "type") {
      parsed.type = value;
    } else if (key == "importer") {
      parsed.importer = value;
    } else if (key == "importer_version") {
      try {
        parsed.importer_version = std::stoi(value);
      } catch (...) {
        parsed.importer_version = 1;
      }
    } else if (key == "source_fingerprint") {
      parsed.source_fingerprint = value;
    } else if (key == "dep") {
      if (!value.empty()) {
        parsed.dependencies.push_back(value);
      }
    } else if (key == "label") {
      if (!value.empty()) {
        parsed.labels.push_back(value);
      }
    } else if (key.rfind("setting.", 0) == 0) {
      parsed.settings.push_back({key.substr(8), value});
    }
  }

  if (parsed.asset_id.empty() || parsed.type.empty()) {
    if (error_message) {
      *error_message = "Meta file missing required fields (asset_id/type): " +
                       meta_path.string();
    }
    return false;
  }

  out_meta = std::move(parsed);
  return true;
}

bool SaveAssetMeta(const std::filesystem::path& meta_path, const AssetMeta& meta,
                   std::string* error_message) {
  std::error_code error;
  std::filesystem::create_directories(meta_path.parent_path(), error);
  if (error) {
    if (error_message) {
      *error_message = "Failed to create directory for meta file: " +
                       meta_path.parent_path().string();
    }
    return false;
  }

  std::ofstream file(meta_path, std::ios::trunc);
  if (!file.is_open()) {
    if (error_message) {
      *error_message = "Failed to write meta file: " + meta_path.string();
    }
    return false;
  }

  file << "asset_id: " << meta.asset_id << "\n";
  file << "type: " << meta.type << "\n";
  file << "importer: " << meta.importer << "\n";
  file << "importer_version: " << meta.importer_version << "\n";
  file << "source_fingerprint: " << meta.source_fingerprint << "\n";
  for (const auto& setting : meta.settings) {
    file << "setting." << setting.key << ": " << setting.value << "\n";
  }
  for (const std::string& dependency : meta.dependencies) {
    file << "dep: " << dependency << "\n";
  }
  for (const std::string& label : meta.labels) {
    file << "label: " << label << "\n";
  }
  return true;
}

}  // namespace cookie::assets

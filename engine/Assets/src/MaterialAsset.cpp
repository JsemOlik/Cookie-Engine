#include "Cookie/Assets/MaterialAsset.h"

#include <fstream>
#include <sstream>

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

bool ParseKeyValue(const std::string& line, std::string& out_key,
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

bool LoadMaterialAsset(const std::filesystem::path& material_path,
                       MaterialAsset& out_material, std::string* error_message) {
  std::ifstream file(material_path);
  if (!file.is_open()) {
    if (error_message) {
      *error_message = "Failed to open material asset: " + material_path.string();
    }
    return false;
  }

  MaterialAsset parsed{};
  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }

    std::string key;
    std::string value;
    if (!ParseKeyValue(trimmed, key, value)) {
      continue;
    }

    if (key == "albedo_asset_id") {
      parsed.albedo_asset_id = value;
    } else if (key == "tint") {
      std::istringstream stream(value);
      stream >> parsed.tint[0] >> parsed.tint[1] >> parsed.tint[2] >> parsed.tint[3];
    }
  }

  out_material = parsed;
  return true;
}

}  // namespace cookie::assets

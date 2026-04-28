#include "Cookie/Assets/AssetId.h"

#include <array>
#include <cstdint>
#include <random>

namespace cookie::assets {
namespace {

char ToHexNibble(std::uint8_t value) {
  if (value < 10) {
    return static_cast<char>('0' + value);
  }
  return static_cast<char>('a' + (value - 10));
}

}  // namespace

std::string GenerateAssetId() {
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<int> distribution(0, 255);

  std::array<std::uint8_t, 16> bytes{};
  for (std::uint8_t& value : bytes) {
    value = static_cast<std::uint8_t>(distribution(generator));
  }

  std::string asset_id;
  asset_id.reserve(32);
  for (const std::uint8_t value : bytes) {
    asset_id.push_back(ToHexNibble(static_cast<std::uint8_t>((value >> 4) & 0x0F)));
    asset_id.push_back(ToHexNibble(static_cast<std::uint8_t>(value & 0x0F)));
  }
  return asset_id;
}

bool IsValidAssetId(std::string_view asset_id) {
  if (asset_id.size() != 32) {
    return false;
  }
  for (const char value : asset_id) {
    const bool is_digit = (value >= '0' && value <= '9');
    const bool is_lower_hex = (value >= 'a' && value <= 'f');
    if (!is_digit && !is_lower_hex) {
      return false;
    }
  }
  return true;
}

}  // namespace cookie::assets

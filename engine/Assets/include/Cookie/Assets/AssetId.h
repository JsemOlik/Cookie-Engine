#pragma once

#include <string>
#include <string_view>

namespace cookie::assets {

std::string GenerateAssetId();
bool IsValidAssetId(std::string_view asset_id);

}  // namespace cookie::assets

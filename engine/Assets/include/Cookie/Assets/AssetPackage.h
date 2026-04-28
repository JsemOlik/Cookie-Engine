#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cookie::assets {

struct MountedPackage {
  std::filesystem::path pak_path;
  std::vector<std::string> asset_ids;
};

struct PakEntryInfo {
  std::string name;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
};

struct PakWriteEntry {
  std::string name;
  std::filesystem::path source_path;
  std::vector<std::uint8_t> inline_bytes;
};

class PakArchive {
 public:
  bool Open(const std::filesystem::path& pak_path);
  const std::vector<PakEntryInfo>& Entries() const;
  bool ReadEntry(std::string_view entry_name, std::vector<std::uint8_t>& out_bytes) const;
  bool ReadEntryToFile(std::string_view entry_name, const std::filesystem::path& destination) const;

 private:
  std::filesystem::path pak_path_;
  std::vector<PakEntryInfo> entries_;
};

bool WritePakArchive(const std::filesystem::path& pak_path,
                     const std::vector<PakWriteEntry>& entries,
                     std::string* error_message);

bool LoadPackageFromTextPak(const std::filesystem::path& pak_path,
                            MountedPackage& out_package);

} // namespace cookie::assets

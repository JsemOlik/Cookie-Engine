#include "Cookie/Assets/AssetPackage.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace cookie::assets {
namespace {

constexpr std::array<char, 4> kPakMagic = {'C', 'P', 'A', 'K'};
constexpr std::uint32_t kPakVersion = 1;

struct PakHeader {
  char magic[4];
  std::uint32_t version = 0;
  std::uint32_t entry_count = 0;
  std::uint64_t index_offset = 0;
};

std::string Trim(const std::string& value) {
  const std::size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }

  const std::size_t last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

template <typename T>
bool ReadValue(std::ifstream& stream, T& value) {
  stream.read(reinterpret_cast<char*>(&value), sizeof(T));
  return stream.good();
}

template <typename T>
bool WriteValue(std::ofstream& stream, const T& value) {
  stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
  return stream.good();
}

bool TryLoadBinaryPackage(const std::filesystem::path& pak_path,
                          MountedPackage& out_package) {
  std::ifstream file(pak_path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  PakHeader header{};
  if (!ReadValue(file, header)) {
    return false;
  }
  if (!std::equal(kPakMagic.begin(), kPakMagic.end(), header.magic)) {
    return false;
  }
  if (header.version != kPakVersion) {
    return false;
  }

  file.seekg(static_cast<std::streamoff>(header.index_offset), std::ios::beg);
  if (!file.good()) {
    return false;
  }

  MountedPackage parsed{};
  parsed.pak_path = pak_path;
  for (std::uint32_t i = 0; i < header.entry_count; ++i) {
    std::uint16_t name_size = 0;
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    if (!ReadValue(file, name_size) || !ReadValue(file, offset) ||
        !ReadValue(file, size)) {
      return false;
    }
    std::string entry_name(name_size, '\0');
    file.read(entry_name.data(), static_cast<std::streamsize>(name_size));
    if (!file.good()) {
      return false;
    }
    parsed.asset_ids.push_back(std::move(entry_name));
  }

  out_package = std::move(parsed);
  return true;
}

} // namespace

bool PakArchive::Open(const std::filesystem::path& pak_path) {
  pak_path_.clear();
  entries_.clear();

  std::ifstream file(pak_path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  PakHeader header{};
  if (!ReadValue(file, header)) {
    return false;
  }
  if (!std::equal(kPakMagic.begin(), kPakMagic.end(), header.magic) ||
      header.version != kPakVersion) {
    return false;
  }

  file.seekg(static_cast<std::streamoff>(header.index_offset), std::ios::beg);
  if (!file.good()) {
    return false;
  }

  std::vector<PakEntryInfo> parsed_entries;
  parsed_entries.reserve(header.entry_count);
  for (std::uint32_t i = 0; i < header.entry_count; ++i) {
    std::uint16_t name_size = 0;
    PakEntryInfo entry{};
    if (!ReadValue(file, name_size) || !ReadValue(file, entry.offset) ||
        !ReadValue(file, entry.size)) {
      return false;
    }
    entry.name.resize(name_size);
    file.read(entry.name.data(), static_cast<std::streamsize>(name_size));
    if (!file.good()) {
      return false;
    }
    parsed_entries.push_back(std::move(entry));
  }

  pak_path_ = pak_path;
  entries_ = std::move(parsed_entries);
  return true;
}

const std::vector<PakEntryInfo>& PakArchive::Entries() const { return entries_; }

bool PakArchive::ReadEntry(std::string_view entry_name,
                           std::vector<std::uint8_t>& out_bytes) const {
  const auto iter = std::find_if(
      entries_.begin(), entries_.end(),
      [entry_name](const PakEntryInfo& entry) { return entry.name == entry_name; });
  if (iter == entries_.end()) {
    return false;
  }

  std::ifstream file(pak_path_, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.seekg(static_cast<std::streamoff>(iter->offset), std::ios::beg);
  if (!file.good()) {
    return false;
  }
  out_bytes.resize(static_cast<std::size_t>(iter->size));
  if (!out_bytes.empty()) {
    file.read(reinterpret_cast<char*>(out_bytes.data()),
              static_cast<std::streamsize>(out_bytes.size()));
    if (file.gcount() != static_cast<std::streamsize>(out_bytes.size())) {
      out_bytes.clear();
      return false;
    }
  }
  return true;
}

bool PakArchive::ReadEntryToFile(std::string_view entry_name,
                                 const std::filesystem::path& destination) const {
  std::vector<std::uint8_t> bytes;
  if (!ReadEntry(entry_name, bytes)) {
    return false;
  }
  std::error_code error;
  std::filesystem::create_directories(destination.parent_path(), error);
  if (error) {
    return false;
  }

  std::ofstream file(destination, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }
  if (!bytes.empty()) {
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  }
  return file.good();
}

bool WritePakArchive(const std::filesystem::path& pak_path,
                     const std::vector<PakWriteEntry>& entries,
                     std::string* error_message) {
  std::error_code error;
  std::filesystem::create_directories(pak_path.parent_path(), error);
  if (error) {
    if (error_message != nullptr) {
      *error_message = "Failed to create pak directory: " + pak_path.parent_path().string();
    }
    return false;
  }

  std::ofstream file(pak_path, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    if (error_message != nullptr) {
      *error_message = "Failed to open pak for writing: " + pak_path.string();
    }
    return false;
  }

  PakHeader header{};
  std::memcpy(header.magic, kPakMagic.data(), kPakMagic.size());
  header.version = kPakVersion;
  header.entry_count = static_cast<std::uint32_t>(entries.size());
  header.index_offset = 0;
  if (!WriteValue(file, header)) {
    if (error_message != nullptr) {
      *error_message = "Failed to write pak header: " + pak_path.string();
    }
    return false;
  }

  std::vector<PakEntryInfo> indexed_entries;
  indexed_entries.reserve(entries.size());
  for (const auto& entry : entries) {
    const std::uint64_t blob_offset = static_cast<std::uint64_t>(file.tellp());
    std::vector<std::uint8_t> bytes;
    if (!entry.source_path.empty()) {
      std::ifstream source(entry.source_path, std::ios::binary);
      if (!source.is_open()) {
        if (error_message != nullptr) {
          *error_message = "Failed to open source payload: " + entry.source_path.string();
        }
        return false;
      }
      bytes.assign(std::istreambuf_iterator<char>(source),
                   std::istreambuf_iterator<char>());
      if (!source.eof() && source.fail()) {
        if (error_message != nullptr) {
          *error_message = "Failed reading source payload: " + entry.source_path.string();
        }
        return false;
      }
    } else {
      bytes = entry.inline_bytes;
    }

    if (!bytes.empty()) {
      file.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
      if (!file.good()) {
        if (error_message != nullptr) {
          *error_message = "Failed writing payload to pak: " + pak_path.string();
        }
        return false;
      }
    }
    indexed_entries.push_back(PakEntryInfo{
        .name = entry.name,
        .offset = blob_offset,
        .size = static_cast<std::uint64_t>(bytes.size()),
    });
  }

  const std::uint64_t index_offset = static_cast<std::uint64_t>(file.tellp());
  for (const auto& entry : indexed_entries) {
    const auto name_size = static_cast<std::uint16_t>(entry.name.size());
    if (!WriteValue(file, name_size) || !WriteValue(file, entry.offset) ||
        !WriteValue(file, entry.size)) {
      if (error_message != nullptr) {
        *error_message = "Failed writing pak index: " + pak_path.string();
      }
      return false;
    }
    file.write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
    if (!file.good()) {
      if (error_message != nullptr) {
        *error_message = "Failed writing pak entry name: " + pak_path.string();
      }
      return false;
    }
  }

  header.index_offset = index_offset;
  file.seekp(0, std::ios::beg);
  if (!WriteValue(file, header)) {
    if (error_message != nullptr) {
      *error_message = "Failed finalizing pak header: " + pak_path.string();
    }
    return false;
  }
  return true;
}

bool LoadPackageFromTextPak(const std::filesystem::path& pak_path,
                            MountedPackage& out_package) {
  if (TryLoadBinaryPackage(pak_path, out_package)) {
    return true;
  }

  std::ifstream file(pak_path);
  if (!file.is_open()) {
    return false;
  }

  MountedPackage parsed{};
  parsed.pak_path = pak_path;

  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed.rfind('#', 0) == 0) {
      continue;
    }
    parsed.asset_ids.push_back(trimmed);
  }

  out_package = std::move(parsed);
  return true;
}

} // namespace cookie::assets

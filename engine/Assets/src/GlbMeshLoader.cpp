#include "Cookie/Assets/GlbMeshLoader.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cookie::assets {
namespace {

constexpr std::uint32_t kGlbMagic = 0x46546C67;
constexpr std::uint32_t kJsonChunkType = 0x4E4F534A;
constexpr std::uint32_t kBinChunkType = 0x004E4942;

enum class JsonType {
  kNull,
  kBool,
  kNumber,
  kString,
  kArray,
  kObject,
};

struct JsonValue {
  JsonType type = JsonType::kNull;
  bool bool_value = false;
  double number_value = 0.0;
  std::string string_value;
  std::vector<JsonValue> array_values;
  std::vector<std::pair<std::string, JsonValue>> object_values;
};

class JsonParser {
 public:
  explicit JsonParser(std::string_view text) : text_(text) {}

  bool Parse(JsonValue& out_value) {
    SkipWhitespace();
    if (!ParseValue(out_value)) {
      return false;
    }
    SkipWhitespace();
    return position_ == text_.size();
  }

 private:
  bool ParseValue(JsonValue& out_value) {
    if (position_ >= text_.size()) {
      return false;
    }
    const char c = text_[position_];
    if (c == '{') {
      return ParseObject(out_value);
    }
    if (c == '[') {
      return ParseArray(out_value);
    }
    if (c == '"') {
      out_value.type = JsonType::kString;
      return ParseString(out_value.string_value);
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c)) != 0) {
      out_value.type = JsonType::kNumber;
      return ParseNumber(out_value.number_value);
    }
    if (StartsWith("true")) {
      position_ += 4;
      out_value.type = JsonType::kBool;
      out_value.bool_value = true;
      return true;
    }
    if (StartsWith("false")) {
      position_ += 5;
      out_value.type = JsonType::kBool;
      out_value.bool_value = false;
      return true;
    }
    if (StartsWith("null")) {
      position_ += 4;
      out_value.type = JsonType::kNull;
      return true;
    }
    return false;
  }

  bool ParseObject(JsonValue& out_value) {
    if (!Consume('{')) {
      return false;
    }
    out_value.type = JsonType::kObject;
    out_value.object_values.clear();
    SkipWhitespace();
    if (Consume('}')) {
      return true;
    }
    while (true) {
      std::string key;
      if (!ParseString(key)) {
        return false;
      }
      SkipWhitespace();
      if (!Consume(':')) {
        return false;
      }
      SkipWhitespace();
      JsonValue value;
      if (!ParseValue(value)) {
        return false;
      }
      out_value.object_values.push_back({key, std::move(value)});
      SkipWhitespace();
      if (Consume('}')) {
        return true;
      }
      if (!Consume(',')) {
        return false;
      }
      SkipWhitespace();
    }
  }

  bool ParseArray(JsonValue& out_value) {
    if (!Consume('[')) {
      return false;
    }
    out_value.type = JsonType::kArray;
    out_value.array_values.clear();
    SkipWhitespace();
    if (Consume(']')) {
      return true;
    }
    while (true) {
      JsonValue item;
      if (!ParseValue(item)) {
        return false;
      }
      out_value.array_values.push_back(std::move(item));
      SkipWhitespace();
      if (Consume(']')) {
        return true;
      }
      if (!Consume(',')) {
        return false;
      }
      SkipWhitespace();
    }
  }

  bool ParseString(std::string& out) {
    if (!Consume('"')) {
      return false;
    }
    out.clear();
    while (position_ < text_.size()) {
      const char c = text_[position_++];
      if (c == '"') {
        return true;
      }
      if (c == '\\') {
        if (position_ >= text_.size()) {
          return false;
        }
        const char escaped = text_[position_++];
        switch (escaped) {
          case '"': out.push_back('"'); break;
          case '\\': out.push_back('\\'); break;
          case '/': out.push_back('/'); break;
          case 'b': out.push_back('\b'); break;
          case 'f': out.push_back('\f'); break;
          case 'n': out.push_back('\n'); break;
          case 'r': out.push_back('\r'); break;
          case 't': out.push_back('\t'); break;
          default: return false;
        }
        continue;
      }
      out.push_back(c);
    }
    return false;
  }

  bool ParseNumber(double& out) {
    const std::size_t start = position_;
    if (position_ < text_.size() && text_[position_] == '-') {
      ++position_;
    }
    while (position_ < text_.size() &&
           std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
      ++position_;
    }
    if (position_ < text_.size() && text_[position_] == '.') {
      ++position_;
      while (position_ < text_.size() &&
             std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
        ++position_;
      }
    }
    if (position_ < text_.size() &&
        (text_[position_] == 'e' || text_[position_] == 'E')) {
      ++position_;
      if (position_ < text_.size() &&
          (text_[position_] == '+' || text_[position_] == '-')) {
        ++position_;
      }
      while (position_ < text_.size() &&
             std::isdigit(static_cast<unsigned char>(text_[position_])) != 0) {
        ++position_;
      }
    }
    if (start == position_) {
      return false;
    }
    try {
      out = std::stod(std::string(text_.substr(start, position_ - start)));
      return true;
    } catch (...) {
      return false;
    }
  }

  void SkipWhitespace() {
    while (position_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
      ++position_;
    }
  }

  bool Consume(char c) {
    if (position_ < text_.size() && text_[position_] == c) {
      ++position_;
      return true;
    }
    return false;
  }

  bool StartsWith(const char* value) const {
    const std::size_t len = std::strlen(value);
    if (position_ + len > text_.size()) {
      return false;
    }
    return text_.substr(position_, len) == value;
  }

  std::string_view text_;
  std::size_t position_ = 0;
};

const JsonValue* FindObjectMember(const JsonValue& object, const char* key) {
  if (object.type != JsonType::kObject) {
    return nullptr;
  }
  for (const auto& [name, value] : object.object_values) {
    if (name == key) {
      return &value;
    }
  }
  return nullptr;
}

std::optional<std::size_t> GetIndex(const JsonValue* value) {
  if (value == nullptr || value->type != JsonType::kNumber) {
    return std::nullopt;
  }
  if (value->number_value < 0.0) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(value->number_value);
}

std::optional<std::size_t> GetUnsigned(const JsonValue* value) {
  return GetIndex(value);
}

template <typename T>
bool ReadLittleEndian(const std::vector<std::uint8_t>& data, std::size_t offset,
                      T& out_value) {
  if (offset + sizeof(T) > data.size()) {
    return false;
  }
  std::memcpy(&out_value, data.data() + offset, sizeof(T));
  return true;
}

struct AccessorLayout {
  std::size_t byte_offset = 0;
  std::size_t count = 0;
  std::size_t stride = 0;
  std::size_t component_size = 0;
  std::size_t components_per_element = 0;
  std::uint32_t component_type = 0;
};

bool BuildAccessorLayout(const JsonValue& root, std::size_t accessor_index,
                         AccessorLayout& out_layout) {
  const JsonValue* accessors = FindObjectMember(root, "accessors");
  const JsonValue* buffer_views = FindObjectMember(root, "bufferViews");
  if (accessors == nullptr || accessors->type != JsonType::kArray ||
      buffer_views == nullptr || buffer_views->type != JsonType::kArray ||
      accessor_index >= accessors->array_values.size()) {
    return false;
  }

  const JsonValue& accessor = accessors->array_values[accessor_index];
  if (accessor.type != JsonType::kObject) {
    return false;
  }

  const auto view_index_opt =
      GetIndex(FindObjectMember(accessor, "bufferView"));
  const auto count_opt = GetUnsigned(FindObjectMember(accessor, "count"));
  const auto component_type_opt =
      GetUnsigned(FindObjectMember(accessor, "componentType"));
  const JsonValue* type_value = FindObjectMember(accessor, "type");
  if (!view_index_opt || !count_opt || !component_type_opt ||
      type_value == nullptr || type_value->type != JsonType::kString) {
    return false;
  }

  const std::size_t view_index = *view_index_opt;
  if (view_index >= buffer_views->array_values.size()) {
    return false;
  }
  const JsonValue& view = buffer_views->array_values[view_index];
  if (view.type != JsonType::kObject) {
    return false;
  }

  const auto view_offset_opt =
      GetUnsigned(FindObjectMember(view, "byteOffset"));
  const auto view_length_opt =
      GetUnsigned(FindObjectMember(view, "byteLength"));
  (void)view_length_opt;
  const auto view_stride_opt =
      GetUnsigned(FindObjectMember(view, "byteStride"));
  const auto accessor_offset_opt =
      GetUnsigned(FindObjectMember(accessor, "byteOffset"));

  std::size_t components = 0;
  if (type_value->string_value == "SCALAR") {
    components = 1;
  } else if (type_value->string_value == "VEC2") {
    components = 2;
  } else if (type_value->string_value == "VEC3") {
    components = 3;
  } else if (type_value->string_value == "VEC4") {
    components = 4;
  } else {
    return false;
  }

  std::size_t component_size = 0;
  switch (*component_type_opt) {
    case 5121: component_size = 1; break; // UNSIGNED_BYTE
    case 5123: component_size = 2; break; // UNSIGNED_SHORT
    case 5125: component_size = 4; break; // UNSIGNED_INT
    case 5126: component_size = 4; break; // FLOAT
    default: return false;
  }

  out_layout.byte_offset = view_offset_opt.value_or(0) + accessor_offset_opt.value_or(0);
  out_layout.count = *count_opt;
  out_layout.component_type = static_cast<std::uint32_t>(*component_type_opt);
  out_layout.component_size = component_size;
  out_layout.components_per_element = components;
  out_layout.stride = view_stride_opt.value_or(component_size * components);
  return true;
}

bool ExtractPrimitiveAccessors(const JsonValue& root, std::size_t& out_position_accessor,
                               std::optional<std::size_t>& out_indices_accessor) {
  const JsonValue* meshes = FindObjectMember(root, "meshes");
  if (meshes == nullptr || meshes->type != JsonType::kArray ||
      meshes->array_values.empty()) {
    return false;
  }
  const JsonValue& mesh = meshes->array_values[0];
  const JsonValue* primitives = FindObjectMember(mesh, "primitives");
  if (primitives == nullptr || primitives->type != JsonType::kArray ||
      primitives->array_values.empty()) {
    return false;
  }
  const JsonValue& primitive = primitives->array_values[0];
  const JsonValue* attributes = FindObjectMember(primitive, "attributes");
  if (attributes == nullptr || attributes->type != JsonType::kObject) {
    return false;
  }

  const auto position_accessor =
      GetIndex(FindObjectMember(*attributes, "POSITION"));
  if (!position_accessor) {
    return false;
  }
  out_position_accessor = *position_accessor;
  out_indices_accessor = GetIndex(FindObjectMember(primitive, "indices"));
  return true;
}

bool LoadFileBytes(const std::filesystem::path& path,
                   std::vector<std::uint8_t>& out_data) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.seekg(0, std::ios::end);
  const std::streamoff size = file.tellg();
  if (size <= 0) {
    return false;
  }
  file.seekg(0, std::ios::beg);
  out_data.resize(static_cast<std::size_t>(size));
  file.read(reinterpret_cast<char*>(out_data.data()), size);
  return file.good();
}

void SetError(std::string* out_error, const std::string& message) {
  if (out_error != nullptr) {
    *out_error = message;
  }
}

} // namespace

bool LoadFirstMeshFromGlb(const std::filesystem::path& glb_path,
                          GlbMeshData& out_mesh, std::string* out_error) {
  out_mesh.positions_xyz.clear();
  out_mesh.indices.clear();

  std::vector<std::uint8_t> file_data;
  if (!LoadFileBytes(glb_path, file_data)) {
    SetError(out_error, "Could not read GLB file.");
    return false;
  }

  if (file_data.size() < 20) {
    SetError(out_error, "GLB file is too small.");
    return false;
  }

  std::uint32_t magic = 0;
  std::uint32_t version = 0;
  std::uint32_t length = 0;
  if (!ReadLittleEndian(file_data, 0, magic) ||
      !ReadLittleEndian(file_data, 4, version) ||
      !ReadLittleEndian(file_data, 8, length)) {
    SetError(out_error, "Invalid GLB header.");
    return false;
  }
  if (magic != kGlbMagic || version != 2 || length > file_data.size()) {
    SetError(out_error, "Unsupported GLB format.");
    return false;
  }

  std::size_t cursor = 12;
  std::string json_chunk;
  std::vector<std::uint8_t> bin_chunk;
  while (cursor + 8 <= length) {
    std::uint32_t chunk_length = 0;
    std::uint32_t chunk_type = 0;
    if (!ReadLittleEndian(file_data, cursor, chunk_length) ||
        !ReadLittleEndian(file_data, cursor + 4, chunk_type)) {
      SetError(out_error, "Invalid GLB chunk header.");
      return false;
    }
    cursor += 8;
    if (cursor + chunk_length > length) {
      SetError(out_error, "GLB chunk length exceeds file size.");
      return false;
    }

    if (chunk_type == kJsonChunkType) {
      json_chunk.assign(
          reinterpret_cast<const char*>(file_data.data() + cursor), chunk_length);
    } else if (chunk_type == kBinChunkType) {
      bin_chunk.assign(file_data.begin() + static_cast<std::ptrdiff_t>(cursor),
                       file_data.begin() +
                           static_cast<std::ptrdiff_t>(cursor + chunk_length));
    }
    cursor += chunk_length;
  }

  if (json_chunk.empty() || bin_chunk.empty()) {
    SetError(out_error, "GLB is missing JSON or BIN chunk.");
    return false;
  }

  JsonValue root;
  JsonParser parser(json_chunk);
  if (!parser.Parse(root)) {
    SetError(out_error, "Failed to parse GLB JSON chunk.");
    return false;
  }

  std::size_t position_accessor_index = 0;
  std::optional<std::size_t> indices_accessor_index;
  if (!ExtractPrimitiveAccessors(root, position_accessor_index, indices_accessor_index)) {
    SetError(out_error, "Could not resolve mesh POSITION accessor.");
    return false;
  }

  AccessorLayout position_layout{};
  if (!BuildAccessorLayout(root, position_accessor_index, position_layout) ||
      position_layout.component_type != 5126 ||
      position_layout.components_per_element < 3) {
    SetError(out_error, "Unsupported POSITION accessor layout.");
    return false;
  }

  out_mesh.positions_xyz.resize(position_layout.count * 3);
  for (std::size_t i = 0; i < position_layout.count; ++i) {
    const std::size_t element_offset = position_layout.byte_offset + i * position_layout.stride;
    std::array<float, 3> position{};
    if (element_offset + sizeof(float) * 3 > bin_chunk.size()) {
      SetError(out_error, "POSITION accessor out of bounds.");
      return false;
    }
    std::memcpy(position.data(), bin_chunk.data() + element_offset, sizeof(float) * 3);
    out_mesh.positions_xyz[i * 3 + 0] = position[0];
    out_mesh.positions_xyz[i * 3 + 1] = position[1];
    out_mesh.positions_xyz[i * 3 + 2] = position[2];
  }

  if (indices_accessor_index) {
    AccessorLayout index_layout{};
    if (!BuildAccessorLayout(root, *indices_accessor_index, index_layout) ||
        index_layout.components_per_element != 1) {
      SetError(out_error, "Unsupported indices accessor layout.");
      return false;
    }
    out_mesh.indices.resize(index_layout.count);
    for (std::size_t i = 0; i < index_layout.count; ++i) {
      const std::size_t element_offset = index_layout.byte_offset + i * index_layout.stride;
      if (element_offset + index_layout.component_size > bin_chunk.size()) {
        SetError(out_error, "Indices accessor out of bounds.");
        return false;
      }

      switch (index_layout.component_type) {
        case 5121: {
          const std::uint8_t value = bin_chunk[element_offset];
          out_mesh.indices[i] = value;
          break;
        }
        case 5123: {
          std::uint16_t value = 0;
          std::memcpy(&value, bin_chunk.data() + element_offset, sizeof(value));
          out_mesh.indices[i] = value;
          break;
        }
        case 5125: {
          std::uint32_t value = 0;
          std::memcpy(&value, bin_chunk.data() + element_offset, sizeof(value));
          out_mesh.indices[i] = value;
          break;
        }
        default:
          SetError(out_error, "Unsupported indices component type.");
          return false;
      }
    }
  }

  if (out_mesh.positions_xyz.empty()) {
    SetError(out_error, "GLB did not produce vertex data.");
    return false;
  }

  return true;
}

} // namespace cookie::assets

#include "Cookie/Renderer/MeshAsset.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace cookie::renderer {
namespace {

struct JsonValue;
using JsonObject = std::unordered_map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

struct JsonValue {
  using Storage = std::variant<std::nullptr_t, bool, double, std::string, JsonArray,
                               JsonObject>;
  Storage value;
};

class JsonParser {
 public:
  explicit JsonParser(std::string_view source) : source_(source) {}

  std::optional<JsonValue> Parse() {
    SkipWhitespace();
    auto value = ParseValue();
    if (!value.has_value()) {
      return std::nullopt;
    }
    SkipWhitespace();
    if (pos_ != source_.size()) {
      return std::nullopt;
    }
    return value;
  }

 private:
  void SkipWhitespace() {
    while (pos_ < source_.size() &&
           std::isspace(static_cast<unsigned char>(source_[pos_])) != 0) {
      ++pos_;
    }
  }

  bool Consume(char ch) {
    if (pos_ < source_.size() && source_[pos_] == ch) {
      ++pos_;
      return true;
    }
    return false;
  }

  std::optional<JsonValue> ParseValue() {
    SkipWhitespace();
    if (pos_ >= source_.size()) {
      return std::nullopt;
    }

    const char ch = source_[pos_];
    if (ch == '{') {
      return ParseObject();
    }
    if (ch == '[') {
      return ParseArray();
    }
    if (ch == '"') {
      auto str = ParseString();
      if (!str.has_value()) {
        return std::nullopt;
      }
      return JsonValue{*str};
    }
    if (ch == 't' && source_.substr(pos_, 4) == "true") {
      pos_ += 4;
      return JsonValue{true};
    }
    if (ch == 'f' && source_.substr(pos_, 5) == "false") {
      pos_ += 5;
      return JsonValue{false};
    }
    if (ch == 'n' && source_.substr(pos_, 4) == "null") {
      pos_ += 4;
      return JsonValue{nullptr};
    }
    return ParseNumber();
  }

  std::optional<JsonValue> ParseObject() {
    if (!Consume('{')) {
      return std::nullopt;
    }
    JsonObject object;
    SkipWhitespace();
    if (Consume('}')) {
      return JsonValue{std::move(object)};
    }

    while (true) {
      SkipWhitespace();
      auto key = ParseString();
      if (!key.has_value()) {
        return std::nullopt;
      }
      SkipWhitespace();
      if (!Consume(':')) {
        return std::nullopt;
      }
      auto value = ParseValue();
      if (!value.has_value()) {
        return std::nullopt;
      }
      object.emplace(*key, std::move(*value));
      SkipWhitespace();
      if (Consume('}')) {
        break;
      }
      if (!Consume(',')) {
        return std::nullopt;
      }
    }

    return JsonValue{std::move(object)};
  }

  std::optional<JsonValue> ParseArray() {
    if (!Consume('[')) {
      return std::nullopt;
    }
    JsonArray array;
    SkipWhitespace();
    if (Consume(']')) {
      return JsonValue{std::move(array)};
    }

    while (true) {
      auto value = ParseValue();
      if (!value.has_value()) {
        return std::nullopt;
      }
      array.push_back(std::move(*value));
      SkipWhitespace();
      if (Consume(']')) {
        break;
      }
      if (!Consume(',')) {
        return std::nullopt;
      }
    }

    return JsonValue{std::move(array)};
  }

  std::optional<std::string> ParseString() {
    if (!Consume('"')) {
      return std::nullopt;
    }

    std::string result;
    while (pos_ < source_.size()) {
      char ch = source_[pos_++];
      if (ch == '"') {
        return result;
      }
      if (ch == '\\') {
        if (pos_ >= source_.size()) {
          return std::nullopt;
        }
        const char esc = source_[pos_++];
        switch (esc) {
          case '"': result.push_back('"'); break;
          case '\\': result.push_back('\\'); break;
          case '/': result.push_back('/'); break;
          case 'b': result.push_back('\b'); break;
          case 'f': result.push_back('\f'); break;
          case 'n': result.push_back('\n'); break;
          case 'r': result.push_back('\r'); break;
          case 't': result.push_back('\t'); break;
          default: return std::nullopt;
        }
      } else {
        result.push_back(ch);
      }
    }

    return std::nullopt;
  }

  std::optional<JsonValue> ParseNumber() {
    const std::size_t begin = pos_;
    if (pos_ < source_.size() && (source_[pos_] == '-' || source_[pos_] == '+')) {
      ++pos_;
    }
    while (pos_ < source_.size() &&
           std::isdigit(static_cast<unsigned char>(source_[pos_])) != 0) {
      ++pos_;
    }
    if (pos_ < source_.size() && source_[pos_] == '.') {
      ++pos_;
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_[pos_])) != 0) {
        ++pos_;
      }
    }
    if (pos_ < source_.size() && (source_[pos_] == 'e' || source_[pos_] == 'E')) {
      ++pos_;
      if (pos_ < source_.size() && (source_[pos_] == '-' || source_[pos_] == '+')) {
        ++pos_;
      }
      while (pos_ < source_.size() &&
             std::isdigit(static_cast<unsigned char>(source_[pos_])) != 0) {
        ++pos_;
      }
    }

    if (begin == pos_) {
      return std::nullopt;
    }

    const std::string_view token = source_.substr(begin, pos_ - begin);
    double value = 0.0;
    const auto parse_result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parse_result.ec != std::errc()) {
      return std::nullopt;
    }
    return JsonValue{value};
  }

  std::string_view source_;
  std::size_t pos_ = 0;
};

const JsonObject* AsObject(const JsonValue& value) {
  return std::get_if<JsonObject>(&value.value);
}

const JsonArray* AsArray(const JsonValue& value) {
  return std::get_if<JsonArray>(&value.value);
}

const std::string* AsString(const JsonValue& value) {
  return std::get_if<std::string>(&value.value);
}

std::optional<std::size_t> AsIndex(const JsonValue& value) {
  const double* number = std::get_if<double>(&value.value);
  if (number == nullptr || *number < 0.0) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(*number);
}

std::optional<std::size_t> ReadIndex(const JsonObject& object, std::string_view key) {
  const auto it = object.find(std::string(key));
  if (it == object.end()) {
    return std::nullopt;
  }
  return AsIndex(it->second);
}

struct BufferViewInfo {
  std::size_t buffer = 0;
  std::size_t byte_offset = 0;
  std::size_t byte_length = 0;
  std::size_t byte_stride = 0;
};

struct AccessorInfo {
  std::size_t buffer_view = 0;
  std::size_t byte_offset = 0;
  std::size_t count = 0;
  std::size_t component_type = 0;
  std::string type;
};

std::size_t ComponentSize(std::size_t component_type) {
  switch (component_type) {
    case 5120: return 1;
    case 5121: return 1;
    case 5122: return 2;
    case 5123: return 2;
    case 5125: return 4;
    case 5126: return 4;
    default: return 0;
  }
}

std::size_t TypeWidth(const std::string& type) {
  if (type == "SCALAR") return 1;
  if (type == "VEC2") return 2;
  if (type == "VEC3") return 3;
  if (type == "VEC4") return 4;
  return 0;
}

template <typename T>
bool ReadElement(const std::vector<std::uint8_t>& bytes, std::size_t offset, T& out_value) {
  if (offset + sizeof(T) > bytes.size()) {
    return false;
  }
  std::memcpy(&out_value, bytes.data() + offset, sizeof(T));
  return true;
}

bool LoadAccessorBytes(const AccessorInfo& accessor, const BufferViewInfo& view,
                      const std::vector<std::uint8_t>& binary,
                      std::size_t expected_component_type,
                      std::size_t expected_width,
                      std::vector<float>& out_values) {
  if (accessor.component_type != expected_component_type) {
    return false;
  }
  const std::size_t width = TypeWidth(accessor.type);
  if (width != expected_width) {
    return false;
  }

  const std::size_t component_size = ComponentSize(accessor.component_type);
  const std::size_t element_size = component_size * width;
  const std::size_t stride = (view.byte_stride > 0) ? view.byte_stride : element_size;
  const std::size_t base = view.byte_offset + accessor.byte_offset;

  out_values.clear();
  out_values.reserve(accessor.count * width);
  for (std::size_t i = 0; i < accessor.count; ++i) {
    const std::size_t offset = base + i * stride;
    for (std::size_t c = 0; c < width; ++c) {
      float value = 0.0f;
      if (!ReadElement(binary, offset + c * component_size, value)) {
        return false;
      }
      out_values.push_back(value);
    }
  }

  return true;
}

bool LoadIndices(const AccessorInfo& accessor, const BufferViewInfo& view,
                 const std::vector<std::uint8_t>& binary,
                 std::vector<std::uint32_t>& out_indices) {
  if (accessor.type != "SCALAR") {
    return false;
  }
  const std::size_t component_size = ComponentSize(accessor.component_type);
  if (component_size == 0) {
    return false;
  }

  const std::size_t stride = (view.byte_stride > 0) ? view.byte_stride : component_size;
  const std::size_t base = view.byte_offset + accessor.byte_offset;

  out_indices.clear();
  out_indices.reserve(accessor.count);
  for (std::size_t i = 0; i < accessor.count; ++i) {
    const std::size_t offset = base + i * stride;
    std::uint32_t index = 0;
    switch (accessor.component_type) {
      case 5121: {
        std::uint8_t value = 0;
        if (!ReadElement(binary, offset, value)) return false;
        index = value;
        break;
      }
      case 5123: {
        std::uint16_t value = 0;
        if (!ReadElement(binary, offset, value)) return false;
        index = value;
        break;
      }
      case 5125: {
        if (!ReadElement(binary, offset, index)) return false;
        break;
      }
      default:
        return false;
    }
    out_indices.push_back(index);
  }

  return true;
}

bool ReadFileBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& out_bytes) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file.seekg(0, std::ios::end);
  const std::streamoff size = file.tellg();
  file.seekg(0, std::ios::beg);
  if (size <= 0) {
    return false;
  }

  out_bytes.resize(static_cast<std::size_t>(size));
  file.read(reinterpret_cast<char*>(out_bytes.data()), size);
  return file.good();
}

bool ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint32_t& out_value) {
  if (offset + sizeof(std::uint32_t) > bytes.size()) {
    return false;
  }
  std::memcpy(&out_value, bytes.data() + offset, sizeof(std::uint32_t));
  return true;
}

} // namespace

bool LoadMeshFromPath(const std::filesystem::path& path, ImportedMesh& out_mesh,
                      std::string* out_error) {
  auto set_error = [&](std::string message) {
    if (out_error != nullptr) {
      *out_error = std::move(message);
    }
  };

  out_mesh = {};

  if (path.extension() != ".glb") {
    set_error("Only .glb meshes are currently supported.");
    return false;
  }

  std::vector<std::uint8_t> file_bytes;
  if (!ReadFileBytes(path, file_bytes) || file_bytes.size() < 20) {
    set_error("Failed to read GLB file bytes.");
    return false;
  }

  std::uint32_t magic = 0;
  std::uint32_t version = 0;
  if (!ReadU32(file_bytes, 0, magic) || !ReadU32(file_bytes, 4, version)) {
    set_error("GLB header read failed.");
    return false;
  }
  if (magic != 0x46546C67 || version != 2) {
    set_error("GLB header is invalid or unsupported version.");
    return false;
  }

  std::size_t cursor = 12;
  std::string json_chunk;
  std::vector<std::uint8_t> binary_chunk;

  while (cursor + 8 <= file_bytes.size()) {
    std::uint32_t chunk_length = 0;
    std::uint32_t chunk_type = 0;
    if (!ReadU32(file_bytes, cursor, chunk_length) ||
        !ReadU32(file_bytes, cursor + 4, chunk_type)) {
      set_error("GLB chunk header read failed.");
      return false;
    }
    cursor += 8;
    if (cursor + chunk_length > file_bytes.size()) {
      set_error("GLB chunk exceeds file length.");
      return false;
    }

    if (chunk_type == 0x4E4F534A) {
      json_chunk.assign(reinterpret_cast<const char*>(file_bytes.data() + cursor), chunk_length);
    } else if (chunk_type == 0x004E4942) {
      binary_chunk.assign(file_bytes.begin() + static_cast<std::ptrdiff_t>(cursor),
                          file_bytes.begin() + static_cast<std::ptrdiff_t>(cursor + chunk_length));
    }

    cursor += chunk_length;
  }

  if (json_chunk.empty() || binary_chunk.empty()) {
    set_error("GLB is missing JSON or BIN chunk.");
    return false;
  }

  JsonParser parser(json_chunk);
  auto root_value = parser.Parse();
  if (!root_value.has_value()) {
    set_error("Failed to parse GLB JSON chunk.");
    return false;
  }

  const JsonObject* root = AsObject(*root_value);
  if (root == nullptr) {
    set_error("GLB JSON root is not an object.");
    return false;
  }

  const auto buffer_views_it = root->find("bufferViews");
  const auto accessors_it = root->find("accessors");
  const auto meshes_it = root->find("meshes");
  if (buffer_views_it == root->end() || accessors_it == root->end() ||
      meshes_it == root->end()) {
    set_error("GLB JSON missing required arrays.");
    return false;
  }

  const JsonArray* buffer_views_json = AsArray(buffer_views_it->second);
  const JsonArray* accessors_json = AsArray(accessors_it->second);
  const JsonArray* meshes_json = AsArray(meshes_it->second);
  if (buffer_views_json == nullptr || accessors_json == nullptr || meshes_json == nullptr) {
    set_error("GLB required fields are malformed.");
    return false;
  }

  std::vector<BufferViewInfo> buffer_views;
  buffer_views.reserve(buffer_views_json->size());
  for (const JsonValue& value : *buffer_views_json) {
    const JsonObject* object = AsObject(value);
    if (object == nullptr) {
      set_error("bufferViews entry is malformed.");
      return false;
    }

    BufferViewInfo view{};
    view.buffer = ReadIndex(*object, "buffer").value_or(0);
    view.byte_offset = ReadIndex(*object, "byteOffset").value_or(0);
    view.byte_length = ReadIndex(*object, "byteLength").value_or(0);
    view.byte_stride = ReadIndex(*object, "byteStride").value_or(0);
    buffer_views.push_back(view);
  }

  std::vector<AccessorInfo> accessors;
  accessors.reserve(accessors_json->size());
  for (const JsonValue& value : *accessors_json) {
    const JsonObject* object = AsObject(value);
    if (object == nullptr) {
      set_error("accessors entry is malformed.");
      return false;
    }

    AccessorInfo accessor{};
    accessor.buffer_view = ReadIndex(*object, "bufferView").value_or(0);
    accessor.byte_offset = ReadIndex(*object, "byteOffset").value_or(0);
    accessor.count = ReadIndex(*object, "count").value_or(0);
    accessor.component_type = ReadIndex(*object, "componentType").value_or(0);
    const auto type_it = object->find("type");
    if (type_it == object->end()) {
      set_error("accessor missing type.");
      return false;
    }
    const std::string* type_name = AsString(type_it->second);
    if (type_name == nullptr) {
      set_error("accessor type is malformed.");
      return false;
    }
    accessor.type = *type_name;
    accessors.push_back(std::move(accessor));
  }

  for (const JsonValue& mesh_value : *meshes_json) {
    const JsonObject* mesh_object = AsObject(mesh_value);
    if (mesh_object == nullptr) {
      continue;
    }

    const auto primitives_it = mesh_object->find("primitives");
    const JsonArray* primitives =
        (primitives_it == mesh_object->end()) ? nullptr : AsArray(primitives_it->second);
    if (primitives == nullptr) {
      continue;
    }

    for (const JsonValue& primitive_value : *primitives) {
      const JsonObject* primitive = AsObject(primitive_value);
      if (primitive == nullptr) {
        continue;
      }

      const auto attributes_it = primitive->find("attributes");
      const JsonObject* attributes =
          (attributes_it == primitive->end()) ? nullptr : AsObject(attributes_it->second);
      if (attributes == nullptr) {
        continue;
      }

      const auto position_it = attributes->find("POSITION");
      if (position_it == attributes->end()) {
        continue;
      }
      const std::optional<std::size_t> position_accessor_index = AsIndex(position_it->second);
      if (!position_accessor_index.has_value() ||
          *position_accessor_index >= accessors.size()) {
        continue;
      }

      const AccessorInfo& position_accessor = accessors[*position_accessor_index];
      if (position_accessor.buffer_view >= buffer_views.size()) {
        continue;
      }

      std::vector<float> positions;
      if (!LoadAccessorBytes(position_accessor,
                            buffer_views[position_accessor.buffer_view], binary_chunk,
                            5126, 3, positions)) {
        continue;
      }

      std::vector<float> texcoords;
      const auto texcoord_it = attributes->find("TEXCOORD_0");
      if (texcoord_it != attributes->end()) {
        const std::optional<std::size_t> texcoord_accessor_index = AsIndex(texcoord_it->second);
        if (texcoord_accessor_index.has_value() &&
            *texcoord_accessor_index < accessors.size()) {
          const AccessorInfo& texcoord_accessor = accessors[*texcoord_accessor_index];
          if (texcoord_accessor.buffer_view < buffer_views.size()) {
            LoadAccessorBytes(texcoord_accessor,
                              buffer_views[texcoord_accessor.buffer_view], binary_chunk,
                              5126, 2, texcoords);
          }
        }
      }

      std::vector<float> colors;
      const auto color_it = attributes->find("COLOR_0");
      if (color_it != attributes->end()) {
        const std::optional<std::size_t> color_accessor_index = AsIndex(color_it->second);
        if (color_accessor_index.has_value() && *color_accessor_index < accessors.size()) {
          const AccessorInfo& color_accessor = accessors[*color_accessor_index];
          if (color_accessor.buffer_view < buffer_views.size()) {
            const std::size_t width = TypeWidth(color_accessor.type);
            if (width == 3 || width == 4) {
              LoadAccessorBytes(color_accessor,
                                buffer_views[color_accessor.buffer_view], binary_chunk,
                                5126, width, colors);
            }
          }
        }
      }

      ImportedPrimitive imported{};
      imported.vertices.resize(position_accessor.count);
      for (std::size_t i = 0; i < position_accessor.count; ++i) {
        SceneVertex& vertex = imported.vertices[i];
        vertex.position[0] = positions[i * 3 + 0];
        vertex.position[1] = positions[i * 3 + 1];
        vertex.position[2] = positions[i * 3 + 2];

        if (texcoords.size() >= (i + 1) * 2) {
          vertex.uv[0] = texcoords[i * 2 + 0];
          vertex.uv[1] = texcoords[i * 2 + 1];
        }

        if (colors.size() >= (i + 1) * 4) {
          vertex.color[0] = colors[i * 4 + 0];
          vertex.color[1] = colors[i * 4 + 1];
          vertex.color[2] = colors[i * 4 + 2];
          vertex.color[3] = colors[i * 4 + 3];
        } else if (colors.size() >= (i + 1) * 3) {
          vertex.color[0] = colors[i * 3 + 0];
          vertex.color[1] = colors[i * 3 + 1];
          vertex.color[2] = colors[i * 3 + 2];
          vertex.color[3] = 1.0f;
        }
      }

      const auto index_it = primitive->find("indices");
      if (index_it != primitive->end()) {
        const std::optional<std::size_t> index_accessor_index = AsIndex(index_it->second);
        if (index_accessor_index.has_value() && *index_accessor_index < accessors.size()) {
          const AccessorInfo& index_accessor = accessors[*index_accessor_index];
          if (index_accessor.buffer_view < buffer_views.size()) {
            if (!LoadIndices(index_accessor, buffer_views[index_accessor.buffer_view],
                             binary_chunk, imported.indices)) {
              imported.indices.clear();
            }
          }
        }
      }

      const auto material_it = primitive->find("material");
      const std::optional<std::size_t> material_index =
          (material_it == primitive->end()) ? std::nullopt : AsIndex(material_it->second);
      if (material_index.has_value() &&
          *material_index <= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        imported.material_index = static_cast<int>(*material_index);
      }

      if (!imported.vertices.empty()) {
        out_mesh.primitives.push_back(std::move(imported));
      }
    }

    if (!out_mesh.primitives.empty()) {
      break;
    }
  }

  if (out_mesh.primitives.empty()) {
    set_error("No supported mesh primitives found in GLB.");
    return false;
  }

  return true;
}

} // namespace cookie::renderer

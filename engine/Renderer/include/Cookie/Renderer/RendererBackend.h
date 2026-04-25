#pragma once

#include <string_view>

namespace cookie::renderer {

class IRendererBackend {
 public:
  virtual ~IRendererBackend() = default;

  virtual bool Initialize() = 0;
  virtual void Shutdown() = 0;
  virtual std::string_view Name() const = 0;
};

} // namespace cookie::renderer

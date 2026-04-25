#include "Cookie/Platform/PlatformWindow.h"

#include <memory>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace cookie::platform {
namespace {

std::wstring ToWideString(const std::string& value) {
  if (value.empty()) {
    return {};
  }

  const int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
  if (size_needed <= 0) {
    return {};
  }

  std::wstring result(static_cast<std::size_t>(size_needed), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(),
                      size_needed);

  if (!result.empty() && result.back() == L'\0') {
    result.pop_back();
  }

  return result;
}

class Win32Window final : public IPlatformWindow {
 public:
  explicit Win32Window(HWND handle) : handle_(handle) {}

  ~Win32Window() override {
    if (handle_ != nullptr) {
      DestroyWindow(handle_);
      handle_ = nullptr;
    }
  }

  void PollEvents() override {
    MSG message{};
    while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
      if (message.message == WM_QUIT) {
        should_close_ = true;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
  }

  bool ShouldClose() const override {
    return should_close_;
  }

  void RequestClose() override {
    should_close_ = true;
    if (handle_ != nullptr) {
      PostMessage(handle_, WM_CLOSE, 0, 0);
    }
  }

  void* GetNativeHandle() const override {
    return handle_;
  }

  void OnCloseMessage() {
    should_close_ = true;
  }

  void SetHandle(HWND handle) {
    handle_ = handle;
  }

 private:
  HWND handle_ = nullptr;
  bool should_close_ = false;
};

LRESULT CALLBACK WindowProc(HWND window_handle, UINT message, WPARAM w_param,
                            LPARAM l_param) {
  auto* window_instance = reinterpret_cast<Win32Window*>(
      GetWindowLongPtr(window_handle, GWLP_USERDATA));

  if (message == WM_NCCREATE) {
    const auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(l_param);
    auto* incoming = reinterpret_cast<Win32Window*>(create_struct->lpCreateParams);
    SetWindowLongPtr(window_handle, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(incoming));
    return DefWindowProcW(window_handle, message, w_param, l_param);
  }

  if (window_instance != nullptr) {
    if (message == WM_CLOSE || message == WM_DESTROY) {
      window_instance->OnCloseMessage();
      if (message == WM_DESTROY) {
        PostQuitMessage(0);
      }
      return 0;
    }
  }

  return DefWindowProcW(window_handle, message, w_param, l_param);
}

ATOM EnsureWindowClass(HINSTANCE instance) {
  static ATOM cached_class = 0;
  if (cached_class != 0) {
    return cached_class;
  }

  WNDCLASSW window_class{};
  window_class.lpfnWndProc = WindowProc;
  window_class.hInstance = instance;
  window_class.lpszClassName = L"CookieEngineWindowClass";
  window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));

  cached_class = RegisterClassW(&window_class);
  return cached_class;
}

} // namespace

std::unique_ptr<IPlatformWindow> CreatePlatformWindow(
    const WindowCreateInfo& create_info) {
  HINSTANCE instance = GetModuleHandleW(nullptr);
  const ATOM window_class = EnsureWindowClass(instance);
  if (window_class == 0) {
    return nullptr;
  }

  // Create the owning object first so its pointer can be attached to HWND.
  auto window = std::make_unique<Win32Window>(nullptr);

  const std::wstring wide_title = ToWideString(create_info.title);
  const auto class_name = reinterpret_cast<LPCWSTR>(
      static_cast<ULONG_PTR>(window_class));
  HWND handle = CreateWindowExW(
      0, class_name,
      wide_title.empty() ? L"CookieRuntime" : wide_title.c_str(),
      WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
      create_info.width, create_info.height, nullptr, nullptr, instance,
      window.get());

  if (handle == nullptr) {
    return nullptr;
  }

  window->SetHandle(handle);
  SetWindowLongPtr(handle, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(window.get()));
  return window;
}

} // namespace cookie::platform

#else

namespace cookie::platform {

namespace {

class NullPlatformWindow final : public IPlatformWindow {
 public:
  void PollEvents() override {}
  bool ShouldClose() const override {
    return true;
  }
  void RequestClose() override {}
  void* GetNativeHandle() const override {
    return nullptr;
  }
};

} // namespace

std::unique_ptr<IPlatformWindow> CreatePlatformWindow(
    const WindowCreateInfo& create_info) {
  (void)create_info;
  return std::make_unique<NullPlatformWindow>();
}

} // namespace cookie::platform

#endif

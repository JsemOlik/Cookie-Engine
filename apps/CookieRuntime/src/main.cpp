#include "Cookie/Core/Application.h"

int main() {
  cookie::core::Application app({
      .application_name = "CookieRuntime",
  });

  return app.Run();
}

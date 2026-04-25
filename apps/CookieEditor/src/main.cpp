#include <QApplication>

#include "Cookie/Editor/EditorMainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  EditorMainWindow window;
  window.show();
  return app.exec();
}

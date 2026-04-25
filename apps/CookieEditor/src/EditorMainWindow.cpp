#include "Cookie/Editor/EditorMainWindow.h"

#include <QDockWidget>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QTextEdit>
#include <QWidget>

namespace {

QDockWidget* CreateDock(const QString& title, QWidget* content,
                        QWidget* parent) {
  auto* dock = new QDockWidget(title, parent);
  dock->setAllowedAreas(Qt::AllDockWidgetAreas);
  dock->setWidget(content);
  return dock;
}

QWidget* CreatePlaceholderPanel(const QString& label_text, QWidget* parent) {
  auto* label = new QLabel(label_text, parent);
  label->setAlignment(Qt::AlignCenter);
  return label;
}

} // namespace

EditorMainWindow::EditorMainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle("Cookie Editor");
  resize(1600, 900);

  auto* scene_viewport =
      new QTextEdit("Scene Viewport Placeholder\n\n(Phase 6 skeleton)");
  scene_viewport->setReadOnly(true);
  setCentralWidget(scene_viewport);

  addDockWidget(Qt::LeftDockWidgetArea,
                CreateDock("Hierarchy",
                           CreatePlaceholderPanel("Hierarchy Placeholder", this),
                           this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Inspector",
                           CreatePlaceholderPanel("Inspector Placeholder", this),
                           this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Asset Browser",
                           CreatePlaceholderPanel("Asset Browser Placeholder", this),
                           this));
  addDockWidget(Qt::BottomDockWidgetArea,
                CreateDock("Console",
                           CreatePlaceholderPanel("Console Placeholder", this),
                           this));
  addDockWidget(Qt::RightDockWidgetArea,
                CreateDock("Game Viewport",
                           CreatePlaceholderPanel("Game Viewport Placeholder", this),
                           this));

  auto* file_menu = menuBar()->addMenu("&File");
  file_menu->addAction("E&xit", this, &QWidget::close);

  statusBar()->showMessage("Cookie Editor skeleton loaded");
}

// Copyright (C) 2024 Vincent Chambrin
// For conditions of distribution and use, see copyright notice in LICENSE

#include "qtminesweeper.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>

int main(int argc, char* argv[])
{
  QGuiApplication app{argc, argv};
  QQuickView w;
  w.setTitle("Minesweeper");
  w.setIcon(QIcon(":/assets/Minesweeper/icon/mines.ico"));
  w.engine()->addImportPath("qrc:/qml");

  qmlRegisterType<QMinesweeperGame>("Minesweeper.Backend", 1, 0, "QMinesweeperGame");

  w.setSource(QUrl("qrc:/qml/Minesweeper/GameItem.qml"));
  w.setResizeMode(QQuickView::SizeRootObjectToView);
  w.setMinimumSize(QSize(380, 440));
  w.show();
  return app.exec();
}

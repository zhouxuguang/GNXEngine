#pragma once
#include "EditorContext.h"
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <memory>
class QApplication;
class MainWindow;
class EditorApplication final : public QObject {
  Q_OBJECT
public:
  explicit EditorApplication(QObject *parent = nullptr);
  ~EditorApplication() override;
  bool Initialize(QApplication &application);
  int Run();
  void Shutdown();
private slots:
  void Tick();

private:
  QApplication *mApplication = nullptr;
  std::unique_ptr<EditorContext> mContext;
  std::unique_ptr<MainWindow> mMainWindow;
  QTimer mTimer;
  QElapsedTimer mElapsed;
  bool mFirstTick = true, mInitialized = false, mShuttingDown = false;
};

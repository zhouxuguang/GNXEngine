#include "EditorApplication.h"
#include "AssetImportService.h"
#include "EditorProjectService.h"
#include "EditorRenderHost.h"
#include "EditorSettings.h"
#include "mainwindow.h"
#include "Runtime/AssetManager/include/AssetManager.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include <QApplication>
#include <algorithm>
EditorApplication::EditorApplication(QObject* parent):QObject(parent){connect(&mTimer,&QTimer::timeout,this,&EditorApplication::Tick);}
EditorApplication::~EditorApplication(){Shutdown();}
bool EditorApplication::Initialize(QApplication& app){
  if(mInitialized)return true;mApplication=&app;mContext=std::make_unique<EditorContext>();mContext->settings=std::make_unique<EditorSettings>();mContext->settings->Load();
  if(!AssetManager::AssetManager::Initialize(getAssetsDir())){mContext.reset();return false;}
  mContext->projectService=std::make_unique<EditorProjectService>();mContext->assetImportService=std::make_unique<AssetImportService>();mContext->renderHost=std::make_unique<EditorRenderHost>();
  mMainWindow=std::make_unique<MainWindow>(*mContext);mMainWindow->show();mElapsed.start();mTimer.start(16);mInitialized=true;return true;
}
int EditorApplication::Run(){return mApplication?mApplication->exec():-1;}
void EditorApplication::Tick(){
  if(!mInitialized||!mContext||!mContext->renderHost)return; float dt=mFirstTick?1.0f/60.0f:std::min(float(mElapsed.restart())/1000.0f,0.1f);mFirstTick=false;
  if(mMainWindow&&mMainWindow->isMinimized()){mTimer.setInterval(100);return;}mTimer.setInterval(16);mContext->renderHost->Tick(dt);
}
void EditorApplication::Shutdown(){
  if(mShuttingDown||!mContext)return;mShuttingDown=true;mTimer.stop();if(mContext->assetImportService)mContext->assetImportService->Shutdown();
  if(mMainWindow){mContext->settings->SetWindowGeometry(mMainWindow->saveGeometry());mContext->settings->SetWindowState(mMainWindow->saveState());mContext->settings->Save();mMainWindow.reset();}
  if(mContext->renderHost)mContext->renderHost->Detach();RenderCore::DestroyRenderDevice();AssetManager::AssetManager::Shutdown();mContext.reset();mInitialized=false;
}

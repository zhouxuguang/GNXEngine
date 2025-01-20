//
//  GameEngine.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/20.
//

#ifndef GNX_ENGINE_GAME_ENGINE_INCLUDE_DGNJDJGJDF_INCLUDE
#define GNX_ENGINE_GAME_ENGINE_INCLUDE_DGNJDJGJDF_INCLUDE

#include "PreDefine.h"

NAMESPACE_GNXENGINE_BEGIN

//游戏引擎的入口类
class GameEngine
{
public:
    GameEngine(/* args */);
    ~GameEngine();

    // 初始化
    bool Initlize();

    //卸载
    void Unload();

    bool Tick();

private:
	//窗口类
    //事件处理
    //场景管理
};

NAMESPACE_GNXENGINE_END

#endif
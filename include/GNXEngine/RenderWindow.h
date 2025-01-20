//
//  RenderWindow.h
//  GNXEngine
//
//  Created by zhouxuguang on 2025/1/20.
//

#ifndef GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE
#define GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE

#include "PreDefine.h"

NAMESPACE_GNXENGINE_BEGIN

//渲染窗口
class RenderWindow
{
public:

    RenderWindow() = default;

    virtual void Resize(uint32_t widthPt, uint32_t heightPt) {}

    virtual void SetVSyncEnabled(bool vsync)
    {
        (void)vsync;
    }

    virtual bool IsVSyncEnabled() const { return false; }

    virtual void SetVSyncInterval(unsigned int interval)
    {
        (void)interval;
    }

    const uint32_t GetVSyncInterval() const { return mVSyncInterval; }

protected:
	bool mIsFullScreen;
	bool mClosed;
	uint32_t mVSyncInterval;
};

NAMESPACE_GNXENGINE_END

#endif //GNX_ENGINE_RENDERWINDOW_INCLUDE_DGNJDFHGFHGDF_INCLUDE
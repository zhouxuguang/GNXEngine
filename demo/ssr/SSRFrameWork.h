#ifndef SSR_FRAME_WORK_H
#define SSR_FRAME_WORK_H

#include "Runtime/GNXEngine/include/AppFrameWork.h"

class SSRFrameWork : public GNXEngine::AppFrameWork
{
public:
    explicit SSRFrameWork(const GNXEngine::WindowProps& props);

    void Initlize() override;
    void Resize(uint32_t width, uint32_t height) override;
    void RenderFrame() override;
    void OnEvent(GNXEngine::Event& event) override;

private:
    bool mSceneCreated = false;
};

#endif

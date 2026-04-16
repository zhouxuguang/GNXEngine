//
//  TerrainFrameWork.h
//  terrain
//
//  Terrain Phase 1 Demo - Heightmap driven terrain rendering
//  through the deferred lighting pipeline.
//

#ifndef TERRAIN_FRAMEWORK_H
#define TERRAIN_FRAMEWORK_H

#include "Runtime/GNXEngine/include/AppFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/GNXEngine/include/Events/KeyEvent.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/RenderCore/include/RCTexture.h"

NS_RENDERSYSTEM_BEGIN
class SkyBox;
NS_RENDERSYSTEM_END

class TerrainFrameWork : public GNXEngine::AppFrameWork
{
public:
    TerrainFrameWork(const GNXEngine::WindowProps& props);

    void Initlize() override;
    void Resize(uint32_t width, uint32_t height) override;
    void RenderFrame() override;
    void OnEvent(GNXEngine::Event& e) override;

private:
    RenderSystem::SkyBox* mSkyBox = nullptr;
};

#endif /* TERRAIN_FRAMEWORK_H */

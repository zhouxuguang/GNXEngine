#include "SSRFrameWork.h"
#include "Runtime/RenderSystem/include/SceneManager.h"
#include "Runtime/RenderSystem/include/SceneNode.h"
#include "Runtime/RenderSystem/include/Transform.h"
#include "Runtime/RenderSystem/include/Light.h"
#include "Runtime/RenderSystem/include/Material.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/RenderEngine.h"
#include "Runtime/RenderCore/include/RenderDevice.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/BaseLib/include/DateTime.h"
#include <algorithm>
#include <array>

using namespace RenderSystem;
using namespace RenderCore;
using namespace mathutil;

namespace
{
RCTexture2DPtr CreateConstantTexture(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    auto texture = GetRenderDevice()->CreateTexture2D(
        kTexFormatRGBA8, TextureUsage::TextureUsageShaderRead, 1, 1, 1);
    const std::array<uint8_t, 4> pixel = {r, g, b, a};
    texture->ReplaceRegion(Rect2D(0, 0, 1, 1), 0, pixel.data(), 4);
    return texture;
}

Transform TransformFromMatrix(const Matrix4x4f& matrix)
{
    Transform transform;
    transform.TransformFromMat4(matrix);
    return transform;
}

SceneNode* CreateReferenceModel(
    SceneNode* root,
    const std::string& name,
    const std::string& file,
    const Vector3f& center,
    float normalizationScale,
    const Vector3f& translation,
    const Vector3f& referenceScale,
    bool rotateXMinus90,
    RCTexturePtr metalRoughness)
{
    Matrix4x4f parentMatrix = Matrix4x4f::CreateTranslate(
        translation.x, translation.y, translation.z);
    if (rotateXMinus90)
        parentMatrix = parentMatrix * Matrix4x4f::CreateRotation(1.0f, 0.0f, 0.0f, -90.0f);

    const Vector3f scale(
        referenceScale.x / normalizationScale,
        referenceScale.y / normalizationScale,
        referenceScale.z / normalizationScale);
    parentMatrix = parentMatrix * Matrix4x4f::CreateScale(scale.x, scale.y, scale.z);
    Transform parentTransform = TransformFromMatrix(parentMatrix);
    SceneNode* parent = root->CreateChildSceneNode(
        name + "_transform", parentTransform.position, parentTransform.rotation, parentTransform.scale);

    SceneNode* model = parent->CreateRendererNode(
        name, file, Vector3f(-center.x, -center.y, -center.z),
        Quaternionf(), Vector3f(1.0f, 1.0f, 1.0f));

    if (auto* renderer = model->QueryComponentT<MeshRenderer>())
    {
        for (const MaterialPtr& material : renderer->GetMaterials())
        {
            if (material)
                material->SetTexture("roughnessTexture", metalRoughness);
        }
    }
    return model;
}
}

SSRFrameWork::SSRFrameWork(const GNXEngine::WindowProps& props)
    : GNXEngine::AppFrameWork(props)
{
}

void SSRFrameWork::Initlize()
{
    GNXEngine::AppFrameWork::Initlize();
}

void SSRFrameWork::Resize(uint32_t width, uint32_t height)
{
    AppFrameWork::Resize(width, height);

    SceneManager* scene = SceneManager::GetInstance();
    CameraPtr camera = scene->GetCamera("MainCamera");
    if (!camera)
        camera = scene->CreateCamera("MainCamera");

    // Parameters from the OpenGL SSR reference scene.
    // The reference starts at z=30 and its showcase is captured after backing
    // the same camera away to frame the full reflective floor.
    camera->LookAt(Vector3f(0.0f, 0.0f, 82.0f),
                   Vector3f(0.0f, 0.0f, 81.0f),
                   Vector3f(0.0f, 1.0f, 0.0f));
    camera->SetLens(45.0f, width, height, 0.1f, 300.0f);
    scene->SetSSREnabled(true);

    if (mSceneCreated)
        return;
    mSceneCreated = true;

    const std::string assetRoot = GetProjectAssetDir() + "ssr/";
    // The reference renders SSR only on the ground via stencil.  Use the
    // material roughness mask here so the objects do not reflect themselves.
    auto objectMetalRoughness = CreateConstantTexture(0, 255, 25);
    auto floorMetalRoughness = CreateConstantTexture(0, 20, 0);    // polished reflective floor

    SceneNode* root = scene->GetRootNode();
    CreateReferenceModel(root, "bunny", assetRoot + "bunny.obj",
        Vector3f(-0.149f, 0.8314f, 0.07325f), 0.129908f,
        Vector3f(0.0f, 5.0f, -20.0f), Vector3f(3.0f, 3.0f, 3.0f), false,
        objectMetalRoughness);
    CreateReferenceModel(root, "round_table", assetRoot + "round_table.obj",
        Vector3f(0.0f, -0.149149f, 0.0f), 0.0670522f,
        Vector3f(-17.0f, -9.0f, 0.0f), Vector3f(1.0f, 1.0f, 1.0f), false,
        objectMetalRoughness);
    CreateReferenceModel(root, "teapot", assetRoot + "teapot.obj",
        Vector3f(1.085f, 0.0f, 7.875f), 2.0512f,
        Vector3f(-17.0f, -2.0f, 0.0f), Vector3f(0.5f, 0.5f, 0.5f), true,
        objectMetalRoughness);
    CreateReferenceModel(root, "ground", assetRoot + "ground.obj",
        Vector3f(0.0f, 0.0f, 0.0f), 0.0866025f,
        Vector3f(0.0f, -20.0f, 0.0f), Vector3f(10.0f, 1.0f, 10.0f), false,
        floorMetalRoughness);

    struct ReferenceLight
    {
        Vector3f position;
        Vector3f color;
        float falloffEnd;
    };
    const ReferenceLight lights[] = {
        {Vector3f(20.0f, 10.0f, 10.0f), Vector3f(1.0f, 1.0f, 1.0f), 300.0f},
        {Vector3f(-25.0f, -5.0f, -35.0f), Vector3f(0.224f, 0.42f, 0.659f), 300.0f},
        {Vector3f(25.0f, -5.0f, -35.0f), Vector3f(0.306f, 0.714f, 0.71f), 180.0f}
    };
    for (size_t i = 0; i < std::size(lights); ++i)
    {
        auto* light = static_cast<PointLight*>(scene->CreateLight(
            "ssrLight" + std::to_string(i), Light::PointLight));
        light->setPosition(lights[i].position);
        light->setColor(lights[i].color);
        light->setStrength(Vector3f(2.0f, 2.0f, 2.0f));
        light->setFalloffStart(0.0f);
        light->setFalloffEnd(lights[i].falloffEnd);
    }
}

void SSRFrameWork::RenderFrame()
{
    static uint64_t previous = 0;
    const uint64_t now = baselib::GetTickNanoSeconds();
    const float deltaTime = previous == 0 ? 0.0f : float(now - previous) * 0.000000001f;
    previous = now;

    SceneManager* scene = SceneManager::GetInstance();
    scene->Update(deltaTime);
    scene->Render(nullptr);
}

void SSRFrameWork::OnEvent(GNXEngine::Event& event)
{
    GNXEngine::AppFrameWork::OnEvent(event);
    SceneManager::GetInstance()->OnEvent(event);
}

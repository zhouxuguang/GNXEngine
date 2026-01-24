//
//  SceneSerializer.cpp
//  GNXEngine
//

#include "SceneSerializer.h"
#include "Runtime/RenderSystem/include/Component.h"
#include "Runtime/RenderSystem/include/mesh/Mesh.h"
#include "Runtime/RenderSystem/include/mesh/MeshRenderer.h"
#include "Runtime/RenderSystem/include/skinnedMesh/SkinnedMesh.h"
#include "Runtime/RenderSystem/include/skinnedMesh/SkinnedMeshRenderer.h"
#include "Runtime/RenderSystem/include/animation/SkeletonAnimation.h"
#include "Runtime/BaseLib/include/LogService.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

NAMESPACE_GNXENGINE_BEGIN

SceneSerializer::SceneSerializer()
{
}

SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::SaveScene(const std::string& filepath, RenderSystem::SceneNode* rootNode)
{
    if (!rootNode)
    {
        LOG_ERROR("Root node is null");
        return false;
    }

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to save scene file: %s", filepath.c_str());
        return false;
    }

    // 生成 JSON
    json j = SerializeNodeToJson(rootNode);
    file << j.dump(4); // 4个空格缩进，格式化输出
    file.close();

    LOG_INFO("Scene saved to: %s", filepath.c_str());
    return true;
}

bool SceneSerializer::LoadScene(const std::string& filepath, RenderSystem::SceneNode* rootNode)
{
    if (!rootNode)
    {
        LOG_ERROR("Root node is null");
        return false;
    }

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to load scene file: %s", filepath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    // 清空现有场景（移除所有子节点）
    // 注意：SceneNode 没有 Clear() 方法，需要手动移除
    // 这里我们选择不移除子节点，让调用者决定是否清空
    // 或者可以使用 DestroyChild 来删除所有子节点

    // 加载场景
    try
    {
        json j = json::parse(jsonStr);
        RenderSystem::SceneNode* loadedRoot = DeserializeNodeFromJson(j, rootNode);

        if (!loadedRoot)
        {
            LOG_ERROR("Failed to deserialize scene");
            return false;
        }

        LOG_INFO("Scene loaded from: %s", filepath.c_str());
        return true;
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to parse scene JSON: %s", e.what());
        return false;
    }
}

bool SceneSerializer::SaveSceneNode(const std::string& filepath, RenderSystem::SceneNode* node)
{
    return SaveScene(filepath, node);
}

bool SceneSerializer::LoadSceneNode(const std::string& filepath, RenderSystem::SceneNode* node)
{
    return LoadScene(filepath, node);
}

// 将节点序列化为 JSON
json SceneSerializer::SerializeNodeToJson(RenderSystem::SceneNode* node)
{
    if (!node)
    {
        return json::object();
    }

    json j;
    j["name"] = node->GetName();
    j["visible"] = node->IsVisible();
    j["active"] = node->IsActive();

    // 序列化变换
    json transform;
    auto* transformComp = node->QueryComponentT<RenderSystem::TransformComponent>();
    if (transformComp)
    {
        const auto& pos = transformComp->transform.position;
        const auto& rot = transformComp->transform.rotation;
        const auto& scl = transformComp->transform.scale;

        transform["position"] = {pos.x, pos.y, pos.z};
        // 将四元数转换为欧拉角（度数）
        // TODO: 实现四元数到欧拉角的转换
        transform["rotation"] = {rot.x, rot.y, rot.z, rot.w};  // 保存四元数
        transform["scale"] = {scl.x, scl.y, scl.z};
    }
    else
    {
        // 默认值
        transform["position"] = {0.0, 0.0, 0.0};
        transform["rotation"] = {0.0, 0.0, 0.0, 1.0};  // 默认四元数
        transform["scale"] = {1.0, 1.0, 1.0};
    }
    j["transform"] = transform;

    // 序列化组件
    json components = json::array();

    // 检查 Mesh 组件
    auto* meshRenderer = node->QueryComponentT<RenderSystem::MeshRenderer>();
    if (meshRenderer && meshRenderer->GetSharedMesh())
    {
        json comp;
        comp["type"] = "MeshRenderer";
        // TODO: 需要在 Mesh 类中添加文件路径成员变量
        // comp["meshPath"] = meshRenderer->GetSharedMesh()->GetFilePath();
        components.push_back(comp);
    }

    // 检查 SkinnedMesh 组件
    auto* skinnedMeshRenderer = node->QueryComponentT<RenderSystem::SkinnedMeshRenderer>();
    if (skinnedMeshRenderer && skinnedMeshRenderer->GetSharedMesh())
    {
        json comp;
        comp["type"] = "SkinnedMeshRenderer";
        // TODO: 需要在 SkinnedMesh 类中添加文件路径成员变量
        // comp["meshPath"] = skinnedMeshRenderer->GetSharedMesh()->GetFilePath();
        components.push_back(comp);
    }

    // 检查 SkeletonAnimation 组件
    auto* animation = node->QueryComponentT<RenderSystem::SkeletonAnimation>();
    if (animation)
    {
        json comp;
        comp["type"] = "SkeletonAnimation";
        comp["animationClips"] = json::array(); // TODO: 序列化动画剪辑
        components.push_back(comp);
    }

    j["components"] = components;

    // 递归序列化子节点
    const auto& children = node->GetAllNodes();
    json childrenJson = json::array();
    for (auto* child : children)
    {
        childrenJson.push_back(SerializeNodeToJson(child));
    }
    j["children"] = childrenJson;

    return j;
}

// 从 JSON 反序列化节点
RenderSystem::SceneNode* SceneSerializer::DeserializeNodeFromJson(const json& j, RenderSystem::SceneNode* parent)
{
    if (!j.is_object())
    {
        return nullptr;
    }

    // 创建节点
    RenderSystem::SceneNode* node;
    if (parent)
    {
        // 如果有父节点，使用父节点创建（会自动创建 TransformComponent）
        node = parent->CreateChildSceneNode(
            j.value("name", "Node"),
            mathutil::Vector3f(0, 0, 0),
            mathutil::Quaternionf(1, 0, 0, 0)
        );
    }
    else
    {
        // 如果没有父节点，手动创建并添加 TransformComponent
        node = new RenderSystem::SceneNode();
        node->SetName(j.value("name", "Node"));

        // 创建并添加默认的 TransformComponent
        auto* transformComp = new RenderSystem::TransformComponent(
                                                                   RenderSystem::Transform(
                mathutil::Vector3f(0, 0, 0),
                mathutil::Quaternionf(1, 0, 0, 0),
                mathutil::Vector3f(1, 1, 1)
            )
        );
        node->AddComponent(transformComp);
    }

    // 设置可见性和激活状态
    if (j.contains("visible"))
    {
        node->SetVisible(j["visible"].get<bool>());
    }
    if (j.contains("active"))
    {
        node->SetActive(j["active"].get<bool>());
    }

    // 解析并设置变换
    if (j.contains("transform"))
    {
        const json& transform = j["transform"];
        auto* transformComp = node->QueryComponentT<RenderSystem::TransformComponent>();

        if (transformComp)
        {
            // 位置
            if (transform.contains("position"))
            {
                const json& pos = transform["position"];
                if (pos.is_array() && pos.size() >= 3)
                {
                    transformComp->transform.position = mathutil::Vector3f(
                        pos[0].get<float>(),
                        pos[1].get<float>(),
                        pos[2].get<float>()
                    );
                }
            }

            // 旋转（四元数）
            if (transform.contains("rotation"))
            {
                const json& rot = transform["rotation"];
                if (rot.is_array() && rot.size() >= 4)
                {
                    // 序列化顺序是 [x, y, z, w]，Quaternion 构造函数参数顺序是 [w, x, y, z]
                    transformComp->transform.rotation = mathutil::Quaternionf(
                        rot[3].get<float>(),  // w
                        rot[0].get<float>(),  // x
                        rot[1].get<float>(),  // y
                        rot[2].get<float>()   // z
                    );
                }
            }

            // 缩放
            if (transform.contains("scale"))
            {
                const json& scl = transform["scale"];
                if (scl.is_array() && scl.size() >= 3)
                {
                    transformComp->transform.scale = mathutil::Vector3f(
                        scl[0].get<float>(),
                        scl[1].get<float>(),
                        scl[2].get<float>()
                    );
                }
            }

            // 标记世界变换需要更新
            node->MarkWorldTransformDirty();
        }
    }

    // 解析组件
    if (j.contains("components") && j["components"].is_array())
    {
        for (const auto& comp : j["components"])
        {
            std::string type = comp.value("type", "");

            if (type == "MeshRenderer")
            {
                std::string meshPath = comp.value("meshPath", "");
                // TODO: 加载并添加 MeshRenderer 组件
                // node->CreateRendererNode(name, meshPath, ...);
            }
            else if (type == "SkinnedMeshRenderer")
            {
                std::string meshPath = comp.value("meshPath", "");
                // TODO: 加载并添加 SkinnedMeshRenderer 组件
                // node->CreateRendererNode(name, meshPath, ...);
            }
            else if (type == "SkeletonAnimation")
            {
                // TODO: 添加 SkeletonAnimation 组件
            }
        }
    }

    // 递归加载子节点
    if (j.contains("children") && j["children"].is_array())
    {
        for (const auto& childJson : j["children"])
        {
            RenderSystem::SceneNode* childNode = DeserializeNodeFromJson(childJson, node);
            if (childNode)
            {
                // 已经通过 parent->CreateChildSceneNode() 或 AddSceneNode() 添加了
            }
        }
    }

    return node;
}

std::string SceneSerializer::GetNodeTypeName(RenderSystem::SceneNode* node)
{
    // 根据组件类型返回节点类型名称
    if (node->QueryComponentT<RenderSystem::MeshRenderer>())
    {
        return "MeshNode";
    }
    if (node->QueryComponentT<RenderSystem::SkinnedMeshRenderer>())
    {
        return "SkinnedMeshNode";
    }

    return "Node";
}

// 序列化工具函数

std::string SceneSerializer::SerializeVector3(const mathutil::Vector3f& vec)
{
    json j = {vec.x, vec.y, vec.z};
    return j.dump();
}

std::string SceneSerializer::SerializeVector4(const mathutil::Vector4f& vec)
{
    json j = {vec.x, vec.y, vec.z, vec.w};
    return j.dump();
}

std::string SceneSerializer::SerializeQuaternion(const mathutil::Quaternionf& quat)
{
    json j = {quat.x, quat.y, quat.z, quat.w};
    return j.dump();
}

std::string SceneSerializer::SerializeMatrix4x4(const mathutil::Matrix4x4f& mat)
{
    json j = json::array();
    for (int i = 0; i < 4; ++i)
    {
        json row = json::array();
        for (int j_idx = 0; j_idx < 4; ++j_idx)
        {
            row.push_back(mat[i][j_idx]);
        }
        j.push_back(row);
    }
    return j.dump();
}

// 反序列化工具函数

mathutil::Vector3f SceneSerializer::DeserializeVector3(const std::string& str)
{
    try
    {
        json j = json::parse(str);
        if (j.is_array() && j.size() >= 3)
        {
            return mathutil::Vector3f(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to deserialize Vector3: %s", e.what());
    }
    return mathutil::Vector3f(0.0f, 0.0f, 0.0f);
}

mathutil::Vector4f SceneSerializer::DeserializeVector4(const std::string& str)
{
    try
    {
        json j = json::parse(str);
        if (j.is_array() && j.size() >= 4)
        {
            return mathutil::Vector4f(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to deserialize Vector4: %s", e.what());
    }
    return mathutil::Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
}

mathutil::Quaternionf SceneSerializer::DeserializeQuaternion(const std::string& str)
{
    try
    {
        json j = json::parse(str);
        if (j.is_array() && j.size() >= 4)
        {
            // 序列化顺序是 [x, y, z, w]，Quaternion 构造函数参数顺序是 [w, x, y, z]
            return mathutil::Quaternion(
                j[3].get<float>(),  // w
                j[0].get<float>(),  // x
                j[1].get<float>(),  // y
                j[2].get<float>()   // z
            );
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to deserialize Quaternion: %s", e.what());
    }
    return mathutil::Quaternionf(1.0f, 0.0f, 0.0f, 0.0f);
}

mathutil::Matrix4x4f SceneSerializer::DeserializeMatrix4x4(const std::string& str)
{
    try
    {
        json j = json::parse(str);
        if (j.is_array() && j.size() >= 4)
        {
            mathutil::Matrix4x4f mat;
            for (int i = 0; i < 4; ++i)
            {
                if (j[i].is_array() && j[i].size() >= 4)
                {
                    for (int j_idx = 0; j_idx < 4; ++j_idx)
                    {
                        mat[i][j_idx] = j[i][j_idx].get<float>();
                    }
                }
            }
            return mat;
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("Failed to deserialize Matrix4x4: %s", e.what());
    }
    return mathutil::Matrix4x4f::IDENTITY;
}

NAMESPACE_GNXENGINE_END

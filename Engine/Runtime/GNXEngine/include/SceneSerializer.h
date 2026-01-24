//
//  SceneSerializer.h
//  GNXEngine
//
//  场景序列化（保存/加载场景）
//

#ifndef GNXENGINE_SCENE_SERIALIZER_INCLUDE_SDFJSDJH
#define GNXENGINE_SCENE_SERIALIZER_INCLUDE_SDFJSDJH

#include "PreDefine.h"
#include "Runtime/RenderSystem/include/SceneNode.h"
#include "Runtime/RenderSystem/include/Camera.h"
#include <nlohmann/json.hpp>
#include <string>

NAMESPACE_GNXENGINE_BEGIN

// 场景序列化器
class GNXENGINE_API SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer();

    // 保存场景
    bool SaveScene(const std::string& filepath, RenderSystem::SceneNode* rootNode);

    // 加载场景
    bool LoadScene(const std::string& filepath, RenderSystem::SceneNode* rootNode);

    // 保存场景节点（递归）
    bool SaveSceneNode(const std::string& filepath, RenderSystem::SceneNode* node);

    // 加载场景节点（递归）
    bool LoadSceneNode(const std::string& filepath, RenderSystem::SceneNode* node);

private:
    // 将节点序列化为 JSON（递归）
    nlohmann::json SerializeNodeToJson(RenderSystem::SceneNode* node);

    // 从 JSON 反序列化节点（递归）
    RenderSystem::SceneNode* DeserializeNodeFromJson(const nlohmann::json& j, RenderSystem::SceneNode* parent = nullptr);

    // 获取节点类型名称
    std::string GetNodeTypeName(RenderSystem::SceneNode* node);

    // JSON 工具函数
    std::string SerializeVector3(const mathutil::Vector3f& vec);
    std::string SerializeVector4(const mathutil::Vector4f& vec);
    std::string SerializeQuaternion(const mathutil::Quaternionf& quat);
    std::string SerializeMatrix4x4(const mathutil::Matrix4x4f& mat);

    mathutil::Vector3f DeserializeVector3(const std::string& str);
    mathutil::Vector4f DeserializeVector4(const std::string& str);
    mathutil::Quaternionf DeserializeQuaternion(const std::string& str);
    mathutil::Matrix4x4f DeserializeMatrix4x4(const std::string& str);
};

NAMESPACE_GNXENGINE_END

#endif /* GNXENGINE_SCENE_SERIALIZER_INCLUDE_SDFJSDJH */

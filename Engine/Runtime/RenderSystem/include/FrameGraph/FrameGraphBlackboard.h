#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBLACKBOARD_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_FRAMEGRAPHBLACKBOARD_H

#include <typeindex>
#include <any>
#include <unordered_map>
#include <cassert>
#include "../RSDefine.h"

NS_RENDERSYSTEM_BEGIN

/**
 用于在不同模块之间传递数据
 */
class FrameGraphBlackboard
{
public:
	FrameGraphBlackboard() = default;
	FrameGraphBlackboard(const FrameGraphBlackboard&) = default;
	FrameGraphBlackboard(FrameGraphBlackboard&&) noexcept = default;
	~FrameGraphBlackboard() = default;

	FrameGraphBlackboard& operator=(const FrameGraphBlackboard&) = default;
	FrameGraphBlackboard& operator=(FrameGraphBlackboard&&) noexcept = default;

	template <typename T, typename... Args> T& Add(Args &&...args);

	template <typename T> [[nodiscard]] const T& Get() const;
	template <typename T> [[nodiscard]] const T* TryGet() const;

	template <typename T> [[nodiscard]] T& Get();
	template <typename T> [[nodiscard]] T* TryGet();

	template <typename T> [[nodiscard]] bool Has() const;

private:
	std::unordered_map<std::type_index, std::any> mStorage;
};

template <typename T, typename... Args>
inline T& FrameGraphBlackboard::Add(Args &&...args)
{
	assert(!Has<T>());
	return mStorage[typeid(T)].emplace<T>(T{ std::forward<Args>(args)... });
}

template <typename T> const T& FrameGraphBlackboard::Get() const
{
	assert(Has<T>());
	return std::any_cast<const T&>(mStorage.at(typeid(T)));
}

template <typename T> const T* FrameGraphBlackboard::TryGet() const
{
	auto it = mStorage.find(typeid(T));
	return it != mStorage.cend() ? std::any_cast<const T>(&it->second) : nullptr;
}

template <typename T> inline T& FrameGraphBlackboard::Get()
{
	return const_cast<T&>(const_cast<const FrameGraphBlackboard*>(this)->Get<T>());
}

template <typename T> inline T* FrameGraphBlackboard::TryGet()
{
	return const_cast<T*>(const_cast<const FrameGraphBlackboard*>(this)->TryGet<T>());
}

template <typename T> inline bool FrameGraphBlackboard::Has() const
{
#if __cplusplus >= 202002L
    return mStorage.contains(typeid(T));
#else
    return mStorage.find(typeid(T)) != mStorage.cend();
#endif
}

NS_RENDERSYSTEM_END

#endif

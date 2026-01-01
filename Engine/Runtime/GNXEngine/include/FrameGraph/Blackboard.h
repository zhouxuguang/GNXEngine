#pragma once

#include <typeindex>
#include <any>
#include <unordered_map>
#include <cassert>

NAMESPACE_GNXENGINE_BEGIN

class FrameGraphBlackboard
{
public:
	FrameGraphBlackboard() = default;
	FrameGraphBlackboard(const FrameGraphBlackboard&) = default;
	FrameGraphBlackboard(FrameGraphBlackboard&&) noexcept = default;
	~FrameGraphBlackboard() = default;

	FrameGraphBlackboard& operator=(const FrameGraphBlackboard&) = default;
	FrameGraphBlackboard& operator=(FrameGraphBlackboard&&) noexcept = default;

	template <typename T, typename... Args> T& add(Args &&...args);

	template <typename T> [[nodiscard]] const T& get() const;
	template <typename T> [[nodiscard]] const T* try_get() const;

	template <typename T> [[nodiscard]] T& get();
	template <typename T> [[nodiscard]] T* try_get();

	template <typename T> [[nodiscard]] bool has() const;

private:
	std::unordered_map<std::type_index, std::any> m_storage;
};

template <typename T, typename... Args>
inline T& FrameGraphBlackboard::add(Args &&...args) 
{
	assert(!has<T>());
	return m_storage[typeid(T)].emplace<T>(T{ std::forward<Args>(args)... });
}

template <typename T> const T& FrameGraphBlackboard::get() const 
{
	assert(has<T>());
	return std::any_cast<const T&>(m_storage.at(typeid(T)));
}

template <typename T> const T* FrameGraphBlackboard::try_get() const 
{
	auto it = m_storage.find(typeid(T));
	return it != m_storage.cend() ? std::any_cast<const T>(&it->second) : nullptr;
}

template <typename T> inline T& FrameGraphBlackboard::get() 
{
	return const_cast<T&>(const_cast<const FrameGraphBlackboard*>(this)->get<T>());
}

template <typename T> inline T* FrameGraphBlackboard::try_get() 
{
	return const_cast<T*>(const_cast<const FrameGraphBlackboard*>(this)->try_get<T>());
}

template <typename T> inline bool FrameGraphBlackboard::has() const 
{
#if __cplusplus >= 202002L
    return m_storage.contains(typeid(T));
#else
    return m_storage.find(typeid(T)) != m_storage.cend();
#endif
}

NAMESPACE_GNXENGINE_END

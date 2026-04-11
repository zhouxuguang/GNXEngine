#ifndef GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_RESOURCEENTRY_H
#define GNXENGINE_RENDERSYSYTEM_FRAMEGRAPH_RESOURCEENTRY_H

#include "TypeTraits.h"
#include <memory>
#include <cassert>

NS_RENDERSYSTEM_BEGIN

// 虚拟资源的包装类
class ResourceEntry final 
{
	friend class FrameGraph;

	enum class Type : uint8_t { Transient, Imported };

public:
	ResourceEntry() = delete;
	ResourceEntry(const ResourceEntry&) = delete;
	ResourceEntry(ResourceEntry&&) noexcept = default;

	ResourceEntry& operator=(const ResourceEntry&) = delete;
	ResourceEntry& operator=(ResourceEntry&&) noexcept = delete;

	static constexpr auto kInitialVersion{ 1u };

	[[nodiscard]] auto toString() const { return mConcept->toString(); }

	void create(void* allocator);
	void destroy(void* allocator);

	void preRead(uint32_t flags, void* context) 
	{
		mConcept->preRead(flags, context);
	}
	void preWrite(uint32_t flags, void* context) 
	{
		mConcept->preWrite(flags, context);
	}

	[[nodiscard]] auto getId() const { return mId; }
	[[nodiscard]] auto getVersion() const { return mVersion; }
	[[nodiscard]] auto isImported() const { return mType == Type::Imported; }
	[[nodiscard]] auto isTransient() const { return mType == Type::Transient; }

	template <typename T> [[nodiscard]] T& get();
	template <typename T>
	[[nodiscard]] const typename T::Desc& getDescriptor() const;

private:
	template <typename T>
	ResourceEntry(const Type, uint32_t id, const typename T::Desc&, T&&);

	// http://www.cplusplus.com/articles/oz18T05o/
	// https://www.modernescpp.com/index.php/c-core-guidelines-type-erasure-with-templates

	struct Concept 
	{
		virtual ~Concept() = default;

		virtual void create(void*) = 0;
		virtual void destroy(void*) = 0;

		virtual void preRead(uint32_t flags, void*) = 0;
		virtual void preWrite(uint32_t flags, void*) = 0;

		virtual std::string toString() const = 0;
	};
    
	template <typename T> struct Model final : Concept
	{
		Model(const typename T::Desc&, T&&);

		void create(void* allocator) override;
		void destroy(void* allocator) override;

		void preRead(uint32_t flags, void* context) override 
		{
#if __cplusplus >= 202002L
			if constexpr (has_preRead<T>)
#else
			if constexpr (has_preRead<T>::value)
#endif
            {
                resource.preRead(descriptor, flags, context);
            }
		}
		void preWrite(uint32_t flags, void* context) override 
		{
#if __cplusplus >= 202002L
			if constexpr (has_preWrite<T>)
#else
			if constexpr (has_preWrite<T>::value)
#endif
            {
                resource.preWrite(descriptor, flags, context);
            }
		}

		std::string toString() const override;

		const typename T::Desc descriptor;
		T resource;
	};

	template <typename T> [[nodiscard]] auto* _getModel() const;

private:
	const Type mType;
	const uint32_t mId;
	uint32_t mVersion; // Incremented on each (unique) write declaration.
	std::unique_ptr<Concept> mConcept;

	PassNode* mProducer = nullptr;
	PassNode* mLast = nullptr;
};

inline void ResourceEntry::create(void* allocator) 
{
	assert(isTransient());
	mConcept->create(allocator);
}

inline void ResourceEntry::destroy(void* allocator) 
{
	assert(isTransient());
	mConcept->destroy(allocator);
}

template <typename T> inline T& ResourceEntry::get() 
{
	return _getModel<T>()->resource;
}

template <typename T>
inline const typename T::Desc& ResourceEntry::getDescriptor() const 
{
	return _getModel<T>()->descriptor;
}

//
// (private):
//

template <typename T>
inline ResourceEntry::ResourceEntry(const Type type, uint32_t id, const typename T::Desc& desc, T&& obj)
	: mType(type), mId(id), mVersion(kInitialVersion),
	mConcept(std::make_unique<Model<T>>(desc, std::forward<T>(obj)))
{
}

template <typename T> inline auto* ResourceEntry::_getModel() const 
{
	auto* model = dynamic_cast<Model<T> *>(mConcept.get());
	assert(model && "Invalid type");
	return model;
}

//
// ResourceEntry::Model class:
//

template <typename T>
inline ResourceEntry::Model<T>::Model(const typename T::Desc& desc, T&& obj)
	: descriptor(desc), resource(std::move(obj))
{
}

template <typename T>
inline void ResourceEntry::Model<T>::create(void* allocator) 
{
	resource.create(descriptor, allocator);
}

template <typename T>
inline void ResourceEntry::Model<T>::destroy(void* allocator) 
{
	resource.destroy(descriptor, allocator);
}

template <typename T>
inline std::string ResourceEntry::Model<T>::toString() const 
{
#if __cplusplus >= 202002L
	if constexpr (has_toString<T>)
#else
	if constexpr (has_toString<T>::value)
#endif
		return T::toString(descriptor);
	else
		return "";
}

NS_RENDERSYSTEM_END

#endif

#ifndef GNX_ENGINE__EVENT_INCLUDE_SFJKSJJHJHSDFHDS
#define GNX_ENGINE__EVENT_INCLUDE_SFJKSJJHJHSDFHDS

#include "PreDefine.h"
#include <functional>

NAMESPACE_GNXENGINE_BEGIN

// 事件类型
enum class EventType
{
	None = 0,
	WindowClose, 
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
	AppTick, 
    AppUpdate,
    AppRender,
	KeyPressed, 
    KeyReleased,
    KeyTyped,
	MouseButtonPressed, 
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

// 事件的分类
enum EventCategory
{
	None = 0,
	EventCategoryApplication    = 1,
	EventCategoryInput          = 2,
	EventCategoryKeyboard       = 4,
	EventCategoryMouse          = 8,
	EventCategoryMouseButton    = 16
};

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
							virtual EventType GetEventType() const override { return GetStaticType(); }\
							virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

class Event
{
public:
	virtual ~Event() = default;

	bool handled = false;

	virtual EventType GetEventType() const = 0;
	virtual const char* GetName() const = 0;
	virtual int GetCategoryFlags() const = 0;
	virtual std::string ToString() const { return GetName(); }

	bool IsInCategory(EventCategory category)
	{
		return GetCategoryFlags() & category;
	}
};

// 事件调度器
class EventDispatcher
{
public:
	EventDispatcher(Event& event) : mEvent(event)
	{
	}
    
	template<typename T, typename F>
	bool Dispatch(const F& func)
	{
		if (mEvent.GetEventType() == T::GetStaticType())
		{
            mEvent.handled |= func(static_cast<T&>(mEvent));
			return true;
		}
		return false;
	}
private:
	Event& mEvent;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.ToString();
}

NAMESPACE_GNXENGINE_END

#endif


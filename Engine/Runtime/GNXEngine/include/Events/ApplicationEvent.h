#ifndef GNX_ENGINE_APPLICATION_EVENT_INCLUDE_SFJKSJJ
#define GNX_ENGINE_APPLICATION_EVENT_INCLUDE_SFJKSJJ

#include "Event.h"

NAMESPACE_GNXENGINE_BEGIN

class WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(uint32_t width, uint32_t height) : mWidth(width), mHeight(height) {}

    uint32_t GetWidth() const { return mWidth; }
    uint32_t GetHeight() const { return mHeight; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
		return ss.str();
	}

	EVENT_CLASS_TYPE(WindowResize)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    uint32_t mWidth;
    uint32_t mHeight;
};

class WindowCloseEvent : public Event
{
public:
	WindowCloseEvent() = default;

	EVENT_CLASS_TYPE(WindowClose)
	EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

NAMESPACE_GNXENGINE_END

#endif

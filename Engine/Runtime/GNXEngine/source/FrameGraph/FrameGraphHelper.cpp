#include "FrameGraph/FrameGraphHelper.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTexture.h"
#include "FrameGraph/FrameGraphBuffer.h"

#if 0

FrameGraphResource importTexture(FrameGraph &fg, const std::string_view name,
                                 Texture *texture) {
  assert(texture && *texture);
  return fg.import<FrameGraphTexture>(
    name,
    {
      .extent = texture->getExtent(),
      .numMipLevels = texture->getNumMipLevels(),
      .layers = texture->getNumLayers(),
      .format = texture->getPixelFormat(),
    },
    {texture});
}
Texture &getTexture(FrameGraphPassResources &resources, FrameGraphResource id) {
  return *resources.get<FrameGraphTexture>(id).texture;
}

FrameGraphResource importBuffer(FrameGraph &fg, const std::string_view name,
                                Buffer *buffer) {
  assert(buffer && *buffer);
  return fg.import<FrameGraphBuffer>(name, {.size = buffer->getSize()},
                                     {buffer});
}
Buffer &getBuffer(FrameGraphPassResources &resources, FrameGraphResource id) {
  return *resources.get<FrameGraphBuffer>(id).buffer;
}

#endif

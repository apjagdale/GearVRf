#include "vk_render_to_texture.h"

#ifndef FRAMEWORK_VK_RENDER_TEXTURE_OFFSCREEN_H
#define FRAMEWORK_VK_RENDER_TEXTURE_OFFSCREEN_H

namespace gvr {
    class VkRenderTextureOffScreen : public VkRenderTexture
    {
    public:
        explicit VkRenderTextureOffScreen(int width, int height, int sample_count = 1);
        void bind();
        bool isReady();
        bool readRenderResult(uint8_t **readback_buffer);
    };

}

#endif //FRAMEWORK_VK_RENDER_TEXTURE_OFFSCREEN_H
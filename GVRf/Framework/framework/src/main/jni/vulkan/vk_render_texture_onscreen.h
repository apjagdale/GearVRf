#include "vk_render_to_texture.h"

#ifndef FRAMEWORK_VK_RENDER_TEXTURE_ONSCREEN_H
#define FRAMEWORK_VK_RENDER_TEXTURE_ONSCREEN_H



namespace gvr {
     class VkRenderTextureOnScreen : public VkRenderTexture{
     public:
         explicit VkRenderTextureOnScreen(int width, int height, int sample_count = 1);
         void bind();
         void initVkData();

    };

}

#endif //FRAMEWORK_VK_RENDER_TEXTURE_ONSCREEN_H
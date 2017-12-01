
#include "vk_render_texture_onscreen.h"
#include "vk_render_to_texture.h"
#include "../engine/renderer/vulkan_renderer.h"
#include "vk_imagebase.h"

namespace gvr{
    VkRenderTextureOnScreen::VkRenderTextureOnScreen(int width, int height, int sample_count):VkRenderTexture(width, height, sample_count){
        initVkData();
    }

    void VkRenderTextureOnScreen::bind() {
        if(fbo == nullptr){
            fbo = new VKFramebuffer(mWidth,mHeight);
            createRenderPass();
            VulkanRenderer* vk_renderer= static_cast<VulkanRenderer*>(Renderer::getInstance());

            fbo->createFrameBuffer(vk_renderer->getDevice(), DEPTH_IMAGE | COLOR_IMAGE, mSamples, true);
        }

    }

    void VkRenderTextureOnScreen::initVkData(){
        VulkanRenderer* renderer = static_cast<VulkanRenderer*>(Renderer::getInstance());
        // LOGE("vulkan abhijit rendertexture ");
        mWaitFence = NULL;
        mCmdBuffer = renderer->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        //mWaitFence = renderer->createFenceObject();
    }
}
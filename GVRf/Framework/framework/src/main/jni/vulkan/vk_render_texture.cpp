/* Copyright 2015 Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "../engine/renderer/renderer.h"
#include "vk_render_to_texture.h"
#include "../engine/renderer/vulkan_renderer.h"
#include "vk_texture.h"
namespace gvr{
VkRenderTexture::VkRenderTexture(int width, int height, int fboType, int layers, int sample_count):RenderTexture(sample_count), fbo(nullptr),mWidth(width), mHeight(height), mFboType(fboType), mLayers(layers), mSamples(sample_count){
    initVkData();
}

const VkDescriptorImageInfo& VkRenderTexture::getDescriptorImage(ImageType imageType){
    mImageInfo.imageLayout = imageType == DEPTH_IMAGE ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL :VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mImageInfo.imageView = fbo->getImageView(imageType);
    if(imageType != DEPTH_IMAGE) {
        TextureParameters textureParameters = TextureParameters();
        uint64_t index = textureParameters.getHashCode();
        index = (index << 32) | 1;
        if (getSampler(index) == 0)
            VkTexture::createSampler(textureParameters, 1);
        mImageInfo.sampler = getSampler(index);
    }
    else {
        VkSamplerCreateInfo sampler = {};
        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.maxAnisotropy = 1.0f;
        sampler.minLod = 0.0f;
        sampler.maxLod = 1.0f;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VulkanRenderer *vk_renderer = static_cast<VulkanRenderer *>(Renderer::getInstance());
        VkSampler sampler1;
        VkResult err = vkCreateSampler(vk_renderer->getDevice(), &sampler, nullptr, &sampler1);
        assert(!err);
        mImageInfo.sampler = sampler1;
        LOGE("Abhijit creating sampler of depth shadow map");
    }
    return  mImageInfo;

}

void VkRenderTexture::createRenderPass(){
    VulkanRenderer* vk_renderer= static_cast<VulkanRenderer*>(Renderer::getInstance());
    VkRenderPass renderPass;
    if(mFboType == (DEPTH_IMAGE | COLOR_IMAGE))
        renderPass = vk_renderer->getCore()->createVkRenderPass(NORMAL_RENDERPASS, mSampleCount);
    else
        renderPass = vk_renderer->getCore()->createVkRenderPass(SHADOW_RENDERPASS, mSampleCount);

    clear_values.resize(3);
    fbo->addRenderPass(renderPass);
}

void VkRenderTexture::initVkData(){
    VulkanRenderer* renderer = static_cast<VulkanRenderer*>(Renderer::getInstance());
    mWaitFence = 0;
    mCmdBuffer = renderer->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    mWaitFence = renderer->createFenceObject();
}

VkRenderPassBeginInfo VkRenderTexture::getRenderPassBeginInfo(){
    VkClearValue clear_color;
    VkClearValue clear_depth;

    clear_color.color.float32[0] = mBackColor[0];
    clear_color.color.float32[1] = mBackColor[1];
    clear_color.color.float32[2] = mBackColor[2];
    clear_color.color.float32[3] = mBackColor[3];



    clear_depth.depthStencil.depth = 1.0f;
    clear_depth.depthStencil.stencil = 0;

    //LOGE("AAA clear clor %f %f %f %f ", mBackColor[0], mBackColor[1], mBackColor[2], mBackColor[3] );
    clear_values[0] = clear_color;
    if(mSampleCount > 1) {
        clear_values[1] = clear_color;
        clear_values[2] = clear_depth;
    } else
        clear_values[1] = clear_depth;

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = nullptr;
    rp_begin.renderPass = fbo->getRenderPass();
    rp_begin.framebuffer = fbo->getFramebuffer(layer_index_);
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = fbo->getWidth();
    rp_begin.renderArea.extent.height = fbo->getHeight();
    rp_begin.clearValueCount = clear_values.size();
    rp_begin.pClearValues = clear_values.data();

    return rp_begin;
}

    /*
bool VkRenderTexture::isReady()
{
    VkFence fence =  getFenceObject();
    VkResult err;

    VulkanCore * core = VulkanCore::getInstance();
    if(!core)
    {
        return false;
    }

    VkDevice device = core->getDevice();
    err = vkGetFenceStatus(device, fence);
    while (err != VK_SUCCESS) {
        err = vkWaitForFences(device, 1, &fence , VK_TRUE, 4294967295U);
    }

    return true;
}*/

void VkRenderTexture::beginRendering(Renderer* renderer){
        bind();
        /*if (!isReady())
        {
            return;
        }*/
    VkRenderPassBeginInfo rp_begin = getRenderPassBeginInfo();
    VkViewport viewport = {};
    viewport.height = (float) height();
    viewport.width = (float) width();
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;

    VkRect2D scissor = {};
    scissor.extent.width =(float) width();
    scissor.extent.height =(float) height();
    scissor.offset.x = 0;
    scissor.offset.y = 0;

    vkCmdSetScissor(mCmdBuffer,0,1, &scissor);
    vkCmdSetViewport(mCmdBuffer,0,1,&viewport);
    vkCmdBeginRenderPass(mCmdBuffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
}

/*
 * Bind the framebuffer to the specified layer of the texture array.
 * Create the framebuffer and layered texture if necessary.
 */
    void VkRenderTexture::setLayerIndex(int layerIndex)
    {
        LOGE("Abhijit Layer index %d", layerIndex);
        layer_index_ = layerIndex;
    }

}
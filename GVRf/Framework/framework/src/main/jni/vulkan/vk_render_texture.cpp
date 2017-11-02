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

namespace gvr{
void VkRenderTexture::bind() {
    if(fbo == nullptr){
        fbo = new VKFramebuffer(mWidth,mHeight);
        createRenderPass();
        VulkanRenderer* vk_renderer= reinterpret_cast<VulkanRenderer*>(Renderer::getInstance());

        fbo->createFrameBuffer(vk_renderer->getDevice(), DEPTH_IMAGE | COLOR_IMAGE, mSampleCount);
    }

}
const VkDescriptorImageInfo& VkRenderTexture::getDescriptorImage(){
    LOGE("inside VkRenderTexture::getDescriptorImage");
    mImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mImageInfo.imageView = fbo->getImageView(COLOR_IMAGE);
    uint64_t index = TextureParameters().getHashCode();
    index = (index << 32) | 1;
    mImageInfo.sampler = getSampler(index);
    return  mImageInfo;
}

void VkRenderTexture::createRenderPass(){
    VulkanRenderer* vk_renderer= reinterpret_cast<VulkanRenderer*>(Renderer::getInstance());
    VkRenderPass renderPass = vk_renderer->getCore()->createVkRenderPass(NORMAL_RENDERPASS, mSampleCount);

    clear_values.resize(3);
    fbo->addRenderPass(renderPass);
}
bool VkRenderTexture::isReady(){
    VkResult err;
    VulkanRenderer* renderer = reinterpret_cast<VulkanRenderer*>(Renderer::getInstance());
    VkDevice device = renderer->getDevice();
    err = vkWaitForFences(device, 1, &mWaitFence , VK_TRUE,
                          4294967295U);
    if(err == VK_SUCCESS)
        return true;

    return false;
}
void VkRenderTexture::createFenceObject(VkDevice device){
    VkResult ret = VK_SUCCESS;
    ret = vkCreateFence(device, gvr::FenceCreateInfo(), nullptr, &mWaitFence);
    GVR_VK_CHECK(!ret);
}
    void VkRenderTexture::endRenderingPE(Renderer* renderer) {
        VulkanRenderer* vk_renderer = reinterpret_cast<VulkanRenderer*>(renderer);
        vkCmdEndRenderPass(*(vk_renderer->getCore()->getCurrentCmdBufferPE()));
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
    rp_begin.framebuffer = fbo->getFramebuffer();
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = fbo->getWidth();
    rp_begin.renderArea.extent.height = fbo->getHeight();
    rp_begin.clearValueCount = clear_values.size();
    rp_begin.pClearValues = clear_values.data();

    return rp_begin;
}

void VkRenderTexture::beginRendering(Renderer* renderer){

    VulkanRenderer* vk_renderer = reinterpret_cast<VulkanRenderer*>(renderer);
    VkRenderPassBeginInfo rp_begin = getRenderPassBeginInfo();

   // vkCmdBeginRenderPass(*(vk_renderer->getCore()->getCurrentCmdBuffer()), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
}

    void VkRenderTexture::beginRenderingPE(Renderer* renderer){

        VulkanRenderer* vk_renderer = reinterpret_cast<VulkanRenderer*>(renderer);
        VkRenderPassBeginInfo rp_begin = getRenderPassBeginInfo();

        vkCmdBeginRenderPass(*(vk_renderer->getCore()->getCurrentCmdBufferPE()), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }
bool VkRenderTexture::readVkRenderResult(uint8_t **readback_buffer, VkCommandBuffer& cmd_buffer,VkFence& fence) {

    if(!fbo)
        return true;
    VkResult err;
    VulkanRenderer* vk_renderer = reinterpret_cast<VulkanRenderer*>(Renderer::getInstance());
    VkDevice device = vk_renderer->getDevice();
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd_buffer, &beginInfo);
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = fbo->getImageSize(COLOR_IMAGE);
    VkExtent3D extent3D = {};
    extent3D.width = mWidth;
    extent3D.height = mHeight;
    extent3D.depth = 1;
    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = extent3D;
    vkCmdCopyImageToBuffer(cmd_buffer,  fbo->getImage(COLOR_IMAGE),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           *(fbo->getImageBuffer(COLOR_IMAGE)), 1, &region);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo ssubmitInfo = {};
    ssubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    ssubmitInfo.commandBufferCount = 1;
    ssubmitInfo.pCommandBuffers = &cmd_buffer;

    vkQueueSubmit(vk_renderer->getQueue(), 1, &ssubmitInfo, fence);

    uint8_t *data;
    err = vkWaitForFences(device, 1, &fence, VK_TRUE, 4294967295U);

    VkDeviceMemory mem = fbo->getDeviceMemory(COLOR_IMAGE);
    err = vkMapMemory(device, mem, 0,
                      fbo->getImageSize(COLOR_IMAGE), 0, (void **) &data);
    *readback_buffer = data;
    GVR_VK_CHECK(!err);

    vkUnmapMemory(device, mem);
    // Makes Fence Un-signalled
    err = vkResetFences(device, 1, &fence);

}
}
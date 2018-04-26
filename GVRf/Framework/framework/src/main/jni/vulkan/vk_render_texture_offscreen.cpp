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

#include "vk_render_texture_offscreen.h"
#include "../engine/renderer/vulkan_renderer.h"
#include "vk_imagebase.h"

namespace gvr{
    VkRenderTextureOffScreen::VkRenderTextureOffScreen(int width, int height, int fboType, int layers, int sample_count):VkRenderTexture(width, height, fboType, layers, sample_count){
    }

    void VkRenderTextureOffScreen::createBufferForOculus(){
        VkResult ret = VK_SUCCESS;
        VulkanRenderer *vk_renderer = static_cast<VulkanRenderer *>(Renderer::getInstance());
        VkDevice device = vk_renderer->getDevice();
        VkMemoryRequirements mem_reqs;
        uint32_t memoryTypeIndex;

        // Components currently hard coded to 4
        ret = vkCreateBuffer(device,
                             gvr::BufferCreateInfo(mWidth * mHeight *  4 * mLayers  * sizeof(uint8_t),
                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT), nullptr,
                             &offscreenMemoryHandle);
        GVR_VK_CHECK(!ret);

        // Obtain the memory requirements for this buffer.
        vkGetBufferMemoryRequirements(device, offscreenMemoryHandle, &mem_reqs);
        GVR_VK_CHECK(!ret);

        bool pass = vk_renderer->GetMemoryTypeFromProperties(mem_reqs.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                        &memoryTypeIndex);
        GVR_VK_CHECK(pass);

        ret = vkAllocateMemory(device,
                               gvr::MemoryAllocateInfo(mem_reqs.size, memoryTypeIndex), nullptr,
                               &offscreenMemory);
        GVR_VK_CHECK(!ret);

        ret = vkBindBufferMemory(device, offscreenMemoryHandle, offscreenMemory, 0);
        GVR_VK_CHECK(!ret);
    }

    void VkRenderTextureOffScreen::bind() {
        if(fbo == nullptr){
            fbo = new VKFramebuffer(mWidth,mHeight);
            createRenderPass();
            VulkanRenderer* vk_renderer= static_cast<VulkanRenderer*>(Renderer::getInstance());

            fbo->createFrameBuffer(vk_renderer->getDevice(), mFboType, mLayers, mSamples);

            // Create buffer to copy result for oculus
            createBufferForOculus();
        }
    }

    bool VkRenderTextureOffScreen::isReady(){
        VkResult err;
        VulkanRenderer* renderer = static_cast<VulkanRenderer*>(Renderer::getInstance());
        VkDevice device = renderer->getDevice();
        if(mWaitFence != 0) {
            err = vkGetFenceStatus(device,mWaitFence);
            if (err == VK_SUCCESS)
                return true;

            if(VK_SUCCESS != vkWaitForFences(device, 1, &mWaitFence, VK_TRUE,
                                             4294967295U))
                return false;

        }
        return true;
    }

    bool VkRenderTextureOffScreen::readRenderResult(uint8_t* readback_buffer){

        //wait for rendering to be complete
        if(!isReady()) {
            LOGE("VkRenderTextureOffScreen::readRenderResult: error in rendering");
            return false;
        }

        uint8_t *data;
        bool result = accessRenderResult(&data);
        memcpy(readback_buffer, data, mWidth*mHeight*4);
        unmapDeviceMemory();
        return result;
    }

    bool VkRenderTextureOffScreen::accessRenderResult(uint8_t **readback_buffer) {

        if(!fbo)
            return false;

        VkResult err;
        VulkanRenderer* vk_renderer = static_cast<VulkanRenderer*>(Renderer::getInstance());
        VkDevice device = vk_renderer->getDevice();

        err = vkResetFences(device, 1, &mWaitFence);
        GVR_VK_CHECK(!err);
        vk_renderer->getCore()->beginCmdBuffer(mCmdBuffer);

        VkExtent3D extent3D = {};
        extent3D.width = mWidth;
        extent3D.height = mHeight;
        extent3D.depth = 1;
        VkBufferImageCopy region = {0};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = extent3D;
        vkCmdCopyImageToBuffer(mCmdBuffer,  fbo->getImage(COLOR_IMAGE),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               offscreenMemoryHandle, 1, &region);
        vkEndCommandBuffer(mCmdBuffer);

        VkSubmitInfo ssubmitInfo = {};
        ssubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ssubmitInfo.commandBufferCount = 1;
        ssubmitInfo.pCommandBuffers = &mCmdBuffer;

        vkQueueSubmit(vk_renderer->getQueue(), 1, &ssubmitInfo, mWaitFence);

        uint8_t *data;
        err = vkWaitForFences(device, 1, &mWaitFence, VK_TRUE, 4294967295U);
        GVR_VK_CHECK(!err);
        //VkDeviceMemory mem = fbo->getHostMemory(COLOR_IMAGE);
        err = vkMapMemory(device, offscreenMemory, 0,
                          fbo->getImageSize(COLOR_IMAGE), 0, (void **) &data);

        *readback_buffer = data;
        GVR_VK_CHECK(!err);

        return true;
    }


    void VkRenderTextureOffScreen::unmapDeviceMemory()
    {
        if(!fbo)
            return;

        VulkanRenderer* vk_renderer = static_cast<VulkanRenderer*>(Renderer::getInstance());
        VkDevice device = vk_renderer->getDevice();
        //VkDeviceMemory mem = fbo->getHostMemory(COLOR_IMAGE);
        vkUnmapMemory(device, offscreenMemory);
    }

}
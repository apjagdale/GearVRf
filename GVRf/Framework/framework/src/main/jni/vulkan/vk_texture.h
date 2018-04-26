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

#ifndef FRAMEWORK_VK_TEXTURE_H
#define FRAMEWORK_VK_TEXTURE_H
#include <cstdlib>
#include "vulkan_headers.h"
#include "../objects/textures/image.h"
#include "../objects/textures/texture.h"
#include "vk_imagebase.h"


namespace gvr {
class VkTexture : public  Texture
{
public:
    VkTexture() : Texture() { }

    explicit VkTexture(int texture_type) :
            Texture(texture_type)
    {
        mTexParamsDirty = true;
    }

    virtual ~VkTexture();
    virtual bool isReady();

    const VkImageView& getVkImageView();
    VkSampler getVkSampler();

    virtual const VkDescriptorImageInfo& getDescriptorImage(){
        mImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mImageInfo.imageView = getVkImageView();
        mImageInfo.sampler = getVkSampler();
        return  mImageInfo;
    }
    static void createSampler(TextureParameters&, int maxLod);
private:
    VkTexture(const VkTexture& gl_texture) = delete;
    VkTexture(VkTexture&& gl_texture) = delete;
    VkTexture& operator=(const VkTexture& gl_texture) = delete;
    VkTexture& operator=(VkTexture&& gl_texture) = delete;

    void updateSampler();
    bool updateImage();
    void updateMipVkImage(uint64_t texSize, std::vector<void*>& pixels,std::vector<ImageInfo>& bitmapInfos, std::vector<VkBufferImageCopy>& bufferCopyRegions, VkImageViewType target, VkFormat internalFormat, int mipLevels =1,VkImageCreateFlags flags=0);
    void createMipLevels(VkFormatProperties formatProperties, VulkanRenderer *vk_renderer,
                         VkCommandBufferBeginInfo setupCmdsBeginInfo, std::vector<VkBufferImageCopy> &bufferCopyRegions,
                         int mipLevels, std::vector<ImageInfo> &bitmapInfos, VkImageMemoryBarrier imageMemoryBarrier,
                         VkSubmitInfo submit_info, VkCommandBuffer *buffers, VkQueue queue);

protected:

    VkDescriptorImageInfo mImageInfo;
    static VkSamplerAddressMode MapWrap[];
    static VkFilter MapFilter[];

};

}
#endif //FRAMEWORK_VK_TEXTURE_H
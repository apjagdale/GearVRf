//
// Created by root on 3/2/18.
//

#ifndef FRAMEWORK_VK_LIGHT_H
#define FRAMEWORK_VK_LIGHT_H


#include <string>

#include "objects/light.h"
#include "vulkan/vulkan_material.h"
#include "shaders/shader.h"

namespace gvr
{

/**
 * OpenGL implementation of Material which keeps uniform data
 * in a GLUniformBlock.
 */
    class VKLight : public Light
    {
    public:
        explicit VKLight(const char* uniform_desc, const char* texture_desc)
                :   Light(),
                    uniforms_(uniform_desc, texture_desc, LIGHT_UBO_INDEX, "Lights_ubo")
        {
            uniforms_.useGPUBuffer(true);
        }

        virtual ShaderData& uniforms()
        {
            return uniforms_;
        }

        void useGPUBuffer(bool flag)
        {
            uniforms_.useGPUBuffer(flag);
        }

        virtual const ShaderData& uniforms() const
        {
            return uniforms_;
        }

        /*virtual int makeShaderLayout(std::string& layout)
        {
            LOGE("Abhijit makelayout in vulkan uniform block  useBufferFlag %d", uniforms_.uniforms().usesGPUBuffer());
            std::ostringstream stream;
            if (uniforms_.uniforms().usesGPUBuffer()) {
                stream << "layout (std140, set = 0, binding = " << uniforms_.uniforms().getBindingPoint() << " ) uniform "
                       << uniforms_.uniforms().getBlockName() << " {" << std::endl;
            }
            else {
                stream << "layout (std140, push_constant) uniform PushConstants {" << std::endl;
            }

            forEachUniform([&stream, this](const DataDescriptor::DataEntry& entry) mutable
                                         {
                                             int nelems = entry.Count;
                                             if (entry.IsSet)
                                             {
                                                 if(nelems > 1) {
                                                     stream << " layout(offset=" << entry.Offset << ") " << entry.Type << " " << entry.Name << "[" << nelems << "];" << std::endl;
                                                 }
                                                 else
                                                     stream << " layout(offset=" << entry.Offset << ") " << entry.Type << " " << entry.Name << ";" << std::endl;
                                             }
                                         });

            stream << "};" << std::endl;

            layout = stream.str();
            return uniforms().uniforms().getTotalSize();
        }*/

    protected:
        VulkanMaterial uniforms_;
    };
}

#endif //FRAMEWORK_VK_LIGHT_H

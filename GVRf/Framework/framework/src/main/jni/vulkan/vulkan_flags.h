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

#ifndef FRAMEWORK_VULKAN_FLAGS_H
#define FRAMEWORK_VULKAN_FLAGS_H


#include <unordered_map>
#include "vulkan.h"
#include "GLES3/gl3.h"

namespace  gvr {

    namespace vkflags {

        extern std::unordered_map<int, int> glToVulkan;

        extern void initVkRenderFlags();
    }
}
#endif //FRAMEWORK_VULKAN_FLAGS_H




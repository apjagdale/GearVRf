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


/***************************************************************************
 * Holds scene objects. Can be used by engines.
 ***************************************************************************/

#include <glslang/Include/Common.h>
#include "engine/renderer/renderer.h"
#include "objects/lightlist.h"
#include "objects/scene.h"

namespace gvr {

/*
 * Adds a new light to the scene.
 * Return true if light was added, false if already there or too many lights.
 */
bool LightList::addLight(Light* light)
{
    auto it = std::find(mLightList.begin(), mLightList.end(), light);

    if (it != mLightList.end())
        return false;
    if (mLightList.size() >= Scene::MAX_LIGHTS)
    {
        LOGD("SHADER: light not added, more than %d lights not allowed", Scene::MAX_LIGHTS);
        return false;
    }
    mLightList.push_back(light);
    auto it2 = mClassMap.find(light->getLightClass());
    if (it2 != mClassMap.end())
    {
        light->setLightIndex(it2->second);
        ++(it2->second);
    }
    else
    {
        light->setLightIndex(0);
        mClassMap[light->getLightClass()] = 1;
    }
    mDirty |= 1;
    LOGD("SHADER: %s added to scene", light->getLightClass());
    return true;
}

/*
 * Removes an existing light from the scene.
 * Return true if light was removed, false if light was not in the scene.
 */
bool LightList::removeLight(Light* light)
{
    auto it2 = std::find(mLightList.begin(), mLightList.end(), light);
    if (it2 == mLightList.end())
    {
        return false;
    }
    mLightList.erase(it2);
    /*
     * Decrement the number of lights of this type in
     * the light class map.
     */
    auto it3 = mClassMap.find(light->getLightClass());
    if (it3 != mClassMap.end())
    {
        light->setLightIndex(-1);
        /*
         * If all lights in the class are gone,
         * remove the class from the map.
         */
        if (--(it3->second) <= 0)
        {
            mClassMap.erase(it3);
        }
        /*
         * Removed a light, recompute light indices for all
         * lights of that type
         */
        else
        {
            int index = 0;
            for (auto it = mLightList.begin();
                 it != mLightList.end();
                 ++it)
            {
                Light* l = *it;
                if ((l != NULL) && (light->getLightClass() == l->getLightClass()))
                {
                    l->setLightIndex(index++);
                }
            }
        }
    }
    LOGD("SHADER: %s removed from scene", light->getLightClass());
    mDirty |= 2;
    return true;
}

ShadowMap* LightList::updateLights(Renderer* renderer, Shader* shader)
{
    bool dirty = (mDirty & 2) != 0;
    bool updated = false;
    ShadowMap* shadowMap = NULL;

    if (mDirty & 1)
    {
        createLightBlock(renderer);
    }
    for (auto it = mLightList.begin();
        it != mLightList.end();
        ++it)
    {
        Light* light = *it;
        if (light != NULL)
        {
            ShadowMap* sm = light->getShadowMap();
            if (sm)
            {
                shadowMap = sm;
            }
            if (dirty || light->uniforms().isDirty(ShaderData::MAT_DATA))
            {
                int offset = light->getBlockOffset();
                std::string s = light->uniforms().uniforms().toString();
                LOGE("LIGHTS: %s\n%s\n\n", light->getLightClass(), s.c_str());
                s = light->uniforms().uniforms().dumpFloats();
                LOGE("LIGHTS: %s\n%s\n\n", light->getLightClass(), s.c_str());
                mLightBlock->setAt(offset, light->uniforms().uniforms());
                s = mLightBlock->dumpFloats();
                LOGE("LIGHTS: LIGHTBLOCK\n%s", s.c_str());
                updated = true;
                light->uniforms().clearDirty();
            }
        }
    }
    mDirty = 0;
    if (updated)
    {
        mLightBlock->updateGPU(renderer);
    }
    mLightBlock->bindBuffer(shader, renderer);
    return shadowMap;
}

int LightList::makeShadowMaps(Scene* scene, ShaderManager* shaderManager)
{
    int texIndex = 0;
    for (auto it = mLightList.begin(); it != mLightList.end(); ++it)
    {
        (*it)->makeShadowMap(scene, shaderManager, texIndex);
        ++texIndex;
    }
    return texIndex;
}

bool LightList::createLightBlock(Renderer* renderer)
{
    int numFloats = 0;

    for (auto it = mLightList.begin();
         it != mLightList.end();
         ++it)
    {
        Light* light = *it;
        if (light != NULL)
        {
            light->setBlockOffset(numFloats);
            numFloats += light->getTotalSize() / sizeof(float);
        }
    }
    if ((mLightBlock == NULL) ||
        (numFloats > mLightBlock->getTotalSize()))
    {
        std::string desc("float lightdata");
        mLightBlock = renderer->createUniformBlock(desc.c_str(), LIGHT_UBO_INDEX, "Lights_ubo", numFloats);
        mLightBlock->useGPUBuffer(true);
        return true;
    }
    return false;
}

/*
 * Removes all the lights from the scene.
 */
void LightList::clear()
{
    mClassMap.clear();
    mLightList.clear();
    if (mLightBlock != NULL)
    {
        delete mLightBlock;
        mLightBlock = NULL;
    }
}

void LightList::makeShaderBlock(std::string& layout) const
{
    std::ostringstream stream;
    stream << "layout (std140) uniform Lights_ubo\n{" << std::endl;
    for (auto it = mClassMap.begin(); it != mClassMap.end(); ++it)
    {
        stream << 'U' << it->first << " " << it->first << "s[" << it->second << "];" << std::endl;
    }
    stream << "};" << std::endl;
    layout = stream.str();
}

}

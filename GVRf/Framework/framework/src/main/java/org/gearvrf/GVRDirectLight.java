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

package org.gearvrf;

import org.gearvrf.utility.TextFile;
import org.joml.Matrix4f;

/**
 * Illuminates object in the scene with a directional light source.
 * 
 * The direction of the light is the forward orientation of the scene object
 * the light is attached to. Light is emitted in that direction
 * from infinitely far away.
 *
 * The intensity of the light remains constant and does not fall
 * off with distance from the light.
 *
 * Dlrect light uniforms:
 * {@literal
 *   world_direction       direction of light in world coordinates
 *                         derived from scene object orientation
 *   ambient_intensity     intensity of ambient light emitted
 *   diffuse_intensity     intensity of diffuse light emitted
 *   specular_intensity    intensity of specular light emitted
 *   sm0                   shadow matrix column 1
 *   sm1                   shadow matrix column 2
 *   sm2                   shadow matrix column 3
 *   sm3                   shadow matrix column 4
 * }
 * 
 * Note: some mobile GPU drivers do not correctly pass a mat4 thru so we currently
 * use 4 vec4's instead.
 * 
 * @see GVRPointLight
 * @see GVRSpotLight
 * @see GVRLightBase
 */
public class GVRDirectLight extends GVRLightBase
{
    private static String fragmentShader = null;
    private static String vertexShader = null;
    private boolean useShadowShader = true;

    public GVRDirectLight(GVRContext gvrContext) {
        this(gvrContext, null);
     }

    public GVRDirectLight(GVRContext gvrContext, GVRSceneObject parent)
    {
        super(gvrContext, parent);
        mUniformDescriptor += " vec4 diffuse_intensity"
                + " vec4 ambient_intensity"
                + " vec4 specular_intensity"
                + " float shadow_map_index"
                + " vec4 sm0 vec4 sm1 vec4 sm2 vec4 sm3";
         if (useShadowShader)
         {
             if (fragmentShader == null)
                 fragmentShader = TextFile.readTextFile(gvrContext.getContext(), R.raw.directshadowlight);
             if (vertexShader == null)
                 vertexShader = TextFile.readTextFile(gvrContext.getContext(), R.raw.vertex_shadow);
             mVertexDescriptor = "vec4 shadow_position";
             mVertexShaderSource = vertexShader;
         }
         else if (fragmentShader == null)
         {
             fragmentShader = TextFile.readTextFile(gvrContext.getContext(), R.raw.directlight);
         }
         mFragmentShaderSource = fragmentShader;
         setAmbientIntensity(0.0f, 0.0f, 0.0f, 1.0f);
         setDiffuseIntensity(1.0f, 1.0f, 1.0f, 1.0f);
         setSpecularIntensity(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    /**
     * Get the ambient light intensity.
     * 
     * This designates the color of the ambient reflection.
     * It is multiplied by the material ambient color to derive
     * the hue of the ambient reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code ambient_intensity} to control the intensity of ambient light reflected.
     * 
     * @return The current {@code vec4 ambient_intensity} as a four-element array
     */
    public float[] getAmbientIntensity() {
        return getVec4("ambient_intensity");
    }

    /**
     * Set the ambient light intensity.
     * 
     * This designates the color of the ambient reflection.
     * It is multiplied by the material ambient color to derive
     * the hue of the ambient reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code ambient_intensity} to control the intensity of ambient light reflected.
     * 
     * @param r red component (0 to 1)
     * @param g green component (0 to 1)
     * @param b blue component (0 to 1)
     * @param a alpha component (0 to 1)
     */
    public void setAmbientIntensity(float r, float g, float b, float a) {
        setVec4("ambient_intensity", r, g, b, a);
    }

    /**
     * Get the diffuse light intensity.
     * 
     * This designates the color of the diffuse reflection.
     * It is multiplied by the material diffuse color to derive
     * the hue of the diffuse reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code diffuse_intensity} to control the intensity of diffuse light reflected.
     * 
     * @return The current {@code vec4 diffuse_intensity} as a four-element
     *         array
     */
    public float[] getDiffuseIntensity() {
        return getVec4("diffuse_intensity");
    }

    /**
     * Set the diffuse light intensity.
     * 
     * This designates the color of the diffuse reflection.
     * It is multiplied by the material diffuse color to derive
     * the hue of the diffuse reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code diffuse_intensity} to control the intensity of diffuse light reflected.
     * 
     * @param r red component (0 to 1)
     * @param g green component (0 to 1)
     * @param b blue component (0 to 1)
     * @param a alpha component (0 to 1)
     */
    public void setDiffuseIntensity(float r, float g, float b, float a)
    {
        setVec4("diffuse_intensity", r, g, b, a);
    }

    /**
     * Get the specular intensity of the light.
     *
     * This designates the color of the specular reflection.
     * It is multiplied by the material specular color to derive
     * the hue of the specular reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code specular_intensity} to control the specular intensity.
     *
     * @return The current {@code vec4 specular_intensity} as a four-element array
     */
    public float[] getSpecularIntensity() {
        return getVec4("specular_intensity");
    }

    /**
     * Set the specular intensity of the light.
     * 
     * This designates the color of the specular reflection.
     * It is multiplied by the material specular color to derive
     * the hue of the specular reflection for that material.
     * The built-in phong shader {@link GVRPhongShader} uses a {@code vec4} uniform named
     * {@code specular_intensity} to control the specular intensity.
     * 
     * @param r red component (0 to 1)
     * @param g green component (0 to 1)
     * @param b blue component (0 to 1)
     * @param a alpha component (0 to 1)
     */
    public void setSpecularIntensity(float r, float g, float b, float a)
    {
        setVec4("specular_intensity", r, g, b, a);
    }

    /**
     * Enables or disabled shadow casting for a direct light.
     * Enabling shadows attaches a GVRShadowMap component to the
     * GVRSceneObject which owns the light and provides the
     * component with an orthographic camera for shadow casting.
     * @param enableFlag true to enable shadow casting, false to disable
     */
    public void setCastShadow(boolean enableFlag)
    {
        GVRSceneObject owner = getOwnerObject();

        if (owner != null)
        {
            GVRShadowMap shadowMap = (GVRShadowMap) getComponent(GVRRenderTarget.getComponentType());
            if (enableFlag)
            {
                if (shadowMap != null)
                {
                    shadowMap.setEnable(true);
                }
                else
                {
                    GVRCamera shadowCam = GVRShadowMap.makeOrthoShadowCamera(
                            getGVRContext().getMainScene().getMainCameraRig().getCenterCamera());
                    shadowMap = new GVRShadowMap(getGVRContext(), shadowCam);
                    owner.attachComponent(shadowMap);
                }
            }
            else
                if (shadowMap != null)
                {
                    shadowMap.setEnable(false);
                }
        }
        mCastShadow = enableFlag;
    }

    /**
     * Updates the position, direction and shadow matrix
     * of this light from the transform of scene object that owns it.
     * The shadow matrix is the model/view/projection matrix
     * from the point of view of the light.
     */
    public void onDrawFrame(float frameTime)
    {
        if (!isEnabled() || (getFloat("enabled") <= 0.0f) || (owner == null)) { return; }
        float[] odir = getVec3("world_direction");
        boolean changed = false;
        Matrix4f worldmtx = owner.getTransform().getModelMatrix4f();

        mOldDir.x = odir[0];
        mOldDir.y = odir[1];
        mOldDir.z = odir[2];
        mNewDir.x = 0.0f;
        mNewDir.y = 0.0f;
        mNewDir.z = -1.0f;
        worldmtx.mul(mLightRot);
        worldmtx.transformDirection(mNewDir);
        mNewDir.normalize();
        if ((mOldDir.x != mNewDir.x) || (mOldDir.y != mNewDir.y) || (mOldDir.z != mNewDir.z))
        {
            changed = true;
            setVec3("world_direction", mNewDir.x, mNewDir.y, mNewDir.z);
        }
        GVRShadowMap shadowMap = (GVRShadowMap) getComponent(GVRShadowMap.getComponentType());
        if ((shadowMap != null) && changed && shadowMap.isEnabled())
        {
            computePosition();
            worldmtx.setTranslation(mNewPos);
            shadowMap.setOrthoShadowMatrix(worldmtx, this);
        }
    }

    private void computePosition()
    {
        GVRScene scene = getGVRContext().getMainScene();
        GVRSceneObject.BoundingVolume bv = scene.getRoot().getBoundingVolume();
        float far = scene.getMainCameraRig().getFarClippingDistance();

        mNewPos.x = bv.center.x - far * mNewDir.x;
        mNewPos.y = bv.center.y - far * mNewDir.y;
        mNewPos.z = bv.center.z - far * mNewDir.z;
        setVec3("world_position", mNewPos.x, mNewPos.y, mNewPos.z);
    }
}

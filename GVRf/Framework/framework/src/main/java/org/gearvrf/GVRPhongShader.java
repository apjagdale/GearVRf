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

import java.util.HashMap;
import java.util.List;

import org.gearvrf.utility.TextFile;

import android.content.Context;

import org.gearvrf.R;

   /**
    * Manages a set of variants on vertex and fragment shaders from the same source
    * code.
    */
   public class GVRPhongShader extends GVRShaderTemplate
   {
       private static String fragTemplate = null;
       private static String vtxTemplate = null;
       private static String surfaceShader = null;
       private static String addLight = null;
       private static String vtxShader = null;
       private static String normalShader = null;
       private static String skinShader = null;

       public GVRPhongShader(GVRContext gvrcontext)
       {
           super("float4 ambient_color; float4 diffuse_color; float4 specular_color; float4 emissive_color; float specular_exponent; float line_width",
                 "sampler2D ambientTexture; sampler2D diffuseTexture; sampler2D specularTexture; sampler2D opacityTexture; sampler2D lightmapTexture; sampler2D emissiveTexture",
                 "float3 a_position float3 a_normal float2 a_texcoord float4 a_bone_weights int4 a_bone_indices float4 a_tangent float4 a_bitangent",
                   300);
           if (fragTemplate == null) {
               Context context = gvrcontext.getContext();
               fragTemplate = TextFile.readTextFile(context, R.raw.fragment_template);
               vtxTemplate = TextFile.readTextFile(context, R.raw.vertex_template);
               surfaceShader = TextFile.readTextFile(context, R.raw.phong_surface);
               vtxShader = TextFile.readTextFile(context, R.raw.pos_norm_tex);
               normalShader = TextFile.readTextFile(context, R.raw.normalmap);
               skinShader = TextFile.readTextFile(context, R.raw.vertexskinning);
               addLight = TextFile.readTextFile(context, R.raw.addlight);
           }
           setSegment("FragmentTemplate", fragTemplate);
           setSegment("VertexTemplate", vtxTemplate);
           setSegment("FragmentSurface", surfaceShader);
           setSegment("FragmentAddLight", addLight);
           setSegment("VertexSkinShader", skinShader);
           setSegment("VertexShader", vtxShader);
           setSegment("VertexNormalShader", normalShader);

           mHasVariants = true;
           mUsesLights = true;
       }
       
       public HashMap<String, Integer> getRenderDefines(GVRRenderData rdata, GVRLightBase[] lights) {
           HashMap<String, Integer> defines = super.getRenderDefines(rdata, lights);
           
           if (!rdata.isLightMapEnabled())
               defines.put("lightMapTexture", 0);
           return defines;
       }

       protected void setMaterialDefaults(GVRShaderData material)
       {
           material.setVec4("ambient_color", 0.2f, 0.2f, 0.2f, 1.0f);
           material.setVec4("diffuse_color",0.8f, 0.8f, 0.8f, 1.0f);
           material.setVec4("specular_color",0.0f, 0.0f, 0.0f, 1.0f);
           material.setVec4("emissive_color", 0.0f, 0.0f, 0.0f, 1.0f);
           material.setFloat("specular_exponent", 0.0f);
           material.setFloat("line_width", 1.0f);
       }
   }


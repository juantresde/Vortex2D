//
//  LevelSet.cpp
//  Vortex2D
//

#include "LevelSet.h"
#include "Disable.h"

namespace Vortex2D { namespace  Fluid {

const char * RedistanceFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_levelSet;
    uniform sampler2D u_levelSet0;
    uniform float delta;

    const float dx = 1.0;

    float g(float s, float w, float wxp, float wxn, float wyp, float wyn)
    {
       float a = (w - wxn)/dx;
       float b = (wxp - w)/dx;
       float c = (w - wyn)/dx;
       float d = (wyp - w)/dx;

       if(s > 0)
       {
           float ap = max(a,0);
           float bn = min(b,0);
           float cp = max(c,0);
           float dn = min(d,0);

           return sqrt(max(ap*ap, bn*bn) + max(cp*cp, dn*dn)) - 1.0;
       }
       else
       {
           float an = min(a,0);
           float bp = max(b,0);
           float cn = min(c,0);
           float dp = max(d,0);

           return sqrt(max(an*an, bp*bp) + max(cn*cn, dp*dp)) - 1.0;
       }

    }

    void main()
    {
       float w0 = texture(u_levelSet0, v_texCoord).x;
       float wxp0 = textureOffset(u_levelSet0, v_texCoord, ivec2(1,0)).x;
       float wxn0 = textureOffset(u_levelSet0, v_texCoord, ivec2(-1,0)).x;
       float wyp0 = textureOffset(u_levelSet0, v_texCoord, ivec2(0,1)).x;
       float wyn0 = textureOffset(u_levelSet0, v_texCoord, ivec2(0,-1)).x;
       
       float w = texture(u_levelSet, v_texCoord).x;
       float wxp = textureOffset(u_levelSet, v_texCoord, ivec2(1,0)).x;
       float wxn = textureOffset(u_levelSet, v_texCoord, ivec2(-1,0)).x;
       float wyp = textureOffset(u_levelSet, v_texCoord, ivec2(0,1)).x;
       float wyn = textureOffset(u_levelSet, v_texCoord, ivec2(0,-1)).x;
       
       float s = sign(w0);
       
       if(w0*wxp0 < 0.0 || w0*wxn0 < 0.0 || w0*wyp0 < 0.0 || w0*wyn0 < 0.0)
       {
           float wx0 = wxp0 - wxn0;
           float wy0 = wyp0 - wyn0;
           // FIXME this is bad if length if close to 0
           float d = 2*dx*w0 / sqrt(wx0*wx0 + wy0*wy0);
           out_color = vec4(w - delta * (s * abs(w) - d) / dx, 0.0, 0.0, 0.0);
       }
       else
       {
           out_color = vec4(w - delta * s * g(s, w, wxp, wxn, wyp, wyn), 0.0, 0.0, 0.0);
       }
       
    }
);

const char * LevelSetMaskFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_texture;

    void main()
    {
        float x = texture(u_texture, v_texCoord).x;

        if(x >= 0.0)
        {
            out_color = vec4(1.0, 0.0, 0.0, 0.0);
        }
        else
        {
            discard;
        }
    }
);

LevelSet::LevelSet(const glm::vec2 & size)
    : Buffer(size, 1, true, true)
    , mLevelSet0(size, 1)
    , mRedistance(Renderer::Shader::TexturePositionVert, RedistanceFrag)
    , mIdentity(Renderer::Shader::TexturePositionVert, Renderer::Shader::TexturePositionFrag)
    , mMask(Renderer::Shader::TexturePositionVert, LevelSetMaskFrag)
{
    ClampToEdge();
    Linear();

    mLevelSet0.ClampToEdge();
    mRedistance.Use().Set("delta", 0.1f).Set("u_levelSet", 0).Set("u_levelSet0", 1).Unuse();
    mIdentity.Use().Set("u_texture", 0).Unuse();
    mMask.Use().Set("u_texture", 0).Unuse();
}

void LevelSet::Redistance(bool reinitialize)
{
    Renderer::Disable d(GL_BLEND);

    mLevelSet0 = mIdentity(*this);

    int num_iterations = reinitialize ? 100 : 1;
    for(int i = 0 ; i < num_iterations ; i++)
    {
        Swap() = mRedistance(Back(*this), mLevelSet0);
    }
}

void LevelSet::RenderMask(Vortex2D::Fluid::Buffer & buffer)
{
    Renderer::Enable e(GL_STENCIL_TEST);
    Renderer::DisableColorMask c;

    glStencilFunc(GL_ALWAYS, 1, 0xFF); // write 1 in stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // replace value with above
    glStencilMask(0xFF); // enable stencil writing

    buffer.Swap() = mMask(Back(*this));
    buffer.Swap() = mMask(Back(*this));

    glStencilMask(0x00); // disable stencil writing

}

}}
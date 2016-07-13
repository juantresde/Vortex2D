//
//  Engine.cpp
//  Vortex
//

#include "Engine.h"
#include "Disable.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

namespace Vortex2D { namespace Fluid {

const char * DivFrag = GLSL(
    uniform sampler2D u_texture; // this is the velocity texture
    uniform sampler2D u_obstacles;
    uniform sampler2D u_obstacles_velocity;

    in vec2 v_texCoord;
    out vec4 out_color;

    float mid_point(float a, float b)
    {
        return (a+b)/2.0;
    }

    void main()
    {
        vec2  uv  = texture(u_texture, v_texCoord).xy;
        float uxp = textureOffset(u_texture, v_texCoord, ivec2(1,0)).x;
        float vyp = textureOffset(u_texture, v_texCoord, ivec2(0,1)).y;

        float c   = texture(u_obstacles, v_texCoord).x;
        float cxp = textureOffset(u_obstacles, v_texCoord, ivec2(2,0)).x;
        float cxn = textureOffset(u_obstacles, v_texCoord, ivec2(-2,0)).x;
        float cyp = textureOffset(u_obstacles, v_texCoord, ivec2(0,2)).x;
        float cyn = textureOffset(u_obstacles, v_texCoord, ivec2(0,-2)).x;

        float h_cxn = 1.0 - mid_point(c, cxn);
        float h_cyn = 1.0 - mid_point(c, cyn);
        float h_cxp = 1.0 - mid_point(cxp, c);
        float h_cyp = 1.0 - mid_point(cyp, c);

        vec2 solid_uv = texture(u_obstacles_velocity, v_texCoord).xy;
        float solid_uxp = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(1,0)).x;
        float solid_vyp = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,1)).y;

        float dx = 1.0;
        float div = -(h_cxp * uxp - h_cxn * uv.x + h_cyp * vyp - h_cyn * uv.y) / dx;
        div += ((h_cxp+c-1.0)*solid_uxp - (h_cxn+c-1.0)*solid_uv.x + (h_cyp+c-1.0)*solid_vyp - (h_cyn+c-1.0)*solid_uv.y) / dx;
        
        //pressure, div, 0, 0
        out_color = vec4(0.0, div, 0.0, 0.0);
    }
);

const char * ProjectFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_texture; // this is the velocity texture
    uniform sampler2D u_pressure;
    uniform sampler2D u_obstacles;
    uniform sampler2D u_obstacles_velocity;
    uniform float delta;

    void main()
    {
        vec2 cell = texture(u_texture, v_texCoord).xy;

        float p = texture(u_pressure, v_texCoord).x;
        float pxn = textureOffset(u_pressure, v_texCoord, ivec2(-1,0)).x;
        float pyn = textureOffset(u_pressure, v_texCoord, ivec2(0,-1)).x;

        vec2 pGrad = vec2(p-pxn, p-pyn);
        
        vec2 mask = vec2(1.0);
        vec2 obsV = vec2(0.0);

        if (textureOffset(u_obstacles, v_texCoord, ivec2(2,0)).x > 0.0)
        {
            mask.x = 0.0;
            obsV.x = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(1,0)).x;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(-2,0)).x > 0.0)
        {
            mask.x = 0.0;
            obsV.x = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(-1,0)).x;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(0,2)).x > 0.0)
        {
            mask.y = 0.0;
            obsV.y = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,1)).y;
        }
        if (textureOffset(u_obstacles, v_texCoord, ivec2(0,-2)).x > 0.0)
        {
            mask.y = 0.0;
            obsV.y = textureOffset(u_obstacles_velocity, v_texCoord, ivec2(0,-1)).y;
        }
        
        float dx = 1.0;
        vec2 new_cell = cell - delta * pGrad / dx;
        out_color = vec4(mask * new_cell + obsV, 0.0, 0.0);
    }
);

const char * WeightsFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_dirichlet;
    uniform sampler2D u_neumann;
    uniform float delta;

    float mid_point(float a, float b)
    {
        return (a+b)/2.0;
    }

    void main()
    {
        vec4 dirichlet;
        dirichlet.x = textureOffset(u_dirichlet, v_texCoord, ivec2(1,0)).x;
        dirichlet.y = textureOffset(u_dirichlet, v_texCoord, ivec2(-1,0)).x;
        dirichlet.z = textureOffset(u_dirichlet, v_texCoord, ivec2(0,1)).x;
        dirichlet.w = textureOffset(u_dirichlet, v_texCoord, ivec2(0,-1)).x;

        float c   = texture(u_neumann, v_texCoord).x;
        float cxp = textureOffset(u_neumann, v_texCoord, ivec2(2,0)).x;
        float cxn = textureOffset(u_neumann, v_texCoord, ivec2(-2,0)).x;
        float cyp = textureOffset(u_neumann, v_texCoord, ivec2(0,2)).x;
        float cyn = textureOffset(u_neumann, v_texCoord, ivec2(0,-2)).x;

        vec4 neumann;
        neumann.x = mid_point(cxp, c);
        neumann.y = mid_point(c, cxn);
        neumann.z = mid_point(cyp, c);
        neumann.w = mid_point(c, cyn);

        vec4 weights;
        weights.x = neumann.x > 0.0 ? neumann.x : dirichlet.x;
        weights.y = neumann.y > 0.0 ? neumann.y : dirichlet.y;
        weights.z = neumann.z > 0.0 ? neumann.z : dirichlet.z;
        weights.w = neumann.w > 0.0 ? neumann.w : dirichlet.w;

        float dx = 1.0;
        out_color = delta * (1.0 - weights) / (dx*dx);
    }
);

const char * DiagonalsFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 out_color;

    uniform sampler2D u_texture; // this is the obstacles
    uniform float delta;

    const vec4 q = vec4(1.0);

    float mid_point(float a, float b)
    {
        return (a+b)/2.0;
    }

    void main()
    {
        float c   = texture(u_texture, v_texCoord).x;
        float cxp = textureOffset(u_texture, v_texCoord, ivec2(2,0)).x;
        float cxn = textureOffset(u_texture, v_texCoord, ivec2(-2,0)).x;
        float cyp = textureOffset(u_texture, v_texCoord, ivec2(0,2)).x;
        float cyn = textureOffset(u_texture, v_texCoord, ivec2(0,-2)).x;

        vec4 p;
        p.x = 1.0 - mid_point(cxp, c);
        p.y = 1.0 - mid_point(c, cxn);
        p.z = 1.0 - mid_point(cyp, c);
        p.w = 1.0 - mid_point(c, cyn);

        float dx = 1.0;
        out_color = vec4(delta * dot(p,q) / (dx*dx),0.0, 0.0, 0.0);
    }
);

const char * AdvectFrag = GLSL(
    uniform sampler2D u_texture;
    uniform sampler2D u_velocity;
    uniform float delta;

    in vec2 v_texCoord;
    out vec4 out_color;

    vec4 cubic(vec4 f1, vec4 f2, vec4 f3, vec4 f4, float xd)
    {
        float xd2 = xd * xd;
        float xd3 = xd2 * xd;

        return f1*(     - 0.5*xd  +     xd2 - 0.5*xd3) +
               f2*( 1.0           - 2.5*xd2 + 1.5*xd3) +
               f3*(       0.5*xd  + 2.0*xd2 - 1.5*xd3) +
               f4*(               - 0.5*xd2 + 0.5*xd3);
    }

    vec4 bicubic(sampler2D u_texture, vec2 xy)
    {
        ivec2 ij = ivec2(floor(xy)) - 1;
        vec2 f = xy - (ij + 1);

        vec4 t[16];
        for(int j = 0 ; j < 4 ; ++j)
        {
            for(int i = 0 ; i < 4 ; ++i)
            {
                t[i + 4*j] = texelFetch(u_texture, ij + ivec2(i,j), 0);
            }
        }

        vec4 x = cubic(
                    cubic(t[0], t[4], t[8], t[12], f.y),
                    cubic(t[1], t[5], t[9], t[13], f.y),
                    cubic(t[2], t[6], t[10], t[14], f.y),
                    cubic(t[3], t[7], t[11], t[15], f.y),
                    f.x
                );

        vec4 maxValue = max(max(t[5], t[6]), max(t[9], t[10]));
        vec4 minValue = min(min(t[5], t[6]), min(t[9], t[10]));

        return clamp(x, minValue, maxValue);
    }

    const float a = 2.0/9.0;
    const float b = 3.0/9.0;
    const float c = 4.0/9.0;

    void main(void)
    {
        vec2 pos = gl_FragCoord.xy - 0.5;

        vec2 k1 = texture(u_velocity, v_texCoord).xy;
        vec2 k2 = bicubic(u_velocity, pos - 0.5*delta*k1).xy;
        vec2 k3 = bicubic(u_velocity, pos - 0.75*delta*k2).xy;

        out_color = bicubic(u_texture, pos - a*delta*k1 - b*delta*k2 - c*delta*k3);
    }
);

Engine::Engine(Dimensions dimensions, LinearSolver & linearSolver, float dt)
    : TopBoundary(dimensions.Scale*glm::vec2(dimensions.Size.x, 1.0f))
    , BottomBoundary(dimensions.Scale*glm::vec2(dimensions.Size.x, 1.0f))
    , LeftBoundary(dimensions.Scale*glm::vec2(1.0f, dimensions.Size.y))
    , RightBoundary(dimensions.Scale*glm::vec2(1.0f, dimensions.Size.y))
    , mDimensions(dimensions)
    , mData(dimensions.Size)
    , mLinearSolver(linearSolver)
    , mExtrapolation(dimensions)
    , mVelocity(dimensions.Size, 2, true, true)
    , mDirichletBoundaries(dimensions.Size, 1)
    , mNeumannBoundaries(glm::vec2(2.0f)*dimensions.Size, 1)
    , mBoundariesVelocity(dimensions.Size, 2)
    , mDiv(Renderer::Shader::TexturePositionVert, DivFrag)
    , mProject(Renderer::Shader::TexturePositionVert, ProjectFrag)
    , mAdvect(Renderer::Shader::TexturePositionVert, AdvectFrag)
    , mWeights(Renderer::Shader::TexturePositionVert, WeightsFrag)
    , mDiagonals(Renderer::Shader::TexturePositionVert, DiagonalsFrag)
{
    mVelocity.Clear(glm::vec4(0.0));
    mDirichletBoundaries.Clear(glm::vec4(0.0));

    mNeumannBoundaries.Clear(glm::vec4(0.0));
    mNeumannBoundaries.Linear();
    mBoundariesVelocity.Clear(glm::vec4(0.0));

    mProject.Use().Set("u_texture", 0).Set("u_pressure", 1).Set("u_obstacles", 2).Set("u_obstacles_velocity", 3).Set("delta", dt).Unuse();
    mDiv.Use().Set("u_texture", 0).Set("u_obstacles", 1).Set("u_obstacles_velocity", 2).Unuse();
    mAdvect.Use().Set("delta", dt).Set("u_texture", 0).Set("u_velocity", 1).Unuse();
    mWeights.Use().Set("u_dirichlet", 0).Set("u_neumann", 1).Set("delta", dt).Unuse();
    mDiagonals.Use().Set("u_texture", 0).Set("delta", dt).Unuse();

    TopBoundary.Colour = BottomBoundary.Colour = LeftBoundary.Colour = RightBoundary.Colour = glm::vec4(1.0f);

    TopBoundary.Position = {0.0f, 0.0f};
    BottomBoundary.Position = dimensions.Scale*glm::vec2(0.0f, dimensions.Size.y - 1.0f);
    LeftBoundary.Position = {0.0f, 0.0f};
    RightBoundary.Position = dimensions.Scale*glm::vec2(dimensions.Size.x - 1.0f, 0.0f);
}

void Engine::Solve()
{
    Renderer::Disable d(GL_BLEND);

    mData.Pressure.Clear(glm::vec4(0.0));
    mData.Pressure.ClearStencil();
    RenderMask(mData.Pressure);
    RenderMask(mData.Pressure.Swap());

    mVelocity.ClearStencil();
    RenderMask(mVelocity.Swap());
    RenderMask(mVelocity.Swap());

    {
        Renderer::Enable e(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilMask(0x00);

        auto div = mDiv(mVelocity, mNeumannBoundaries, mBoundariesVelocity);
        auto weights = mWeights(mDirichletBoundaries, mNeumannBoundaries);
        auto diagonals = mDiagonals(mNeumannBoundaries);

        mLinearSolver.Init(mData, div, weights, diagonals);
        mLinearSolver.Solve(mData);

        mVelocity.Swap() = mProject(Back(mVelocity), mData.Pressure, mNeumannBoundaries, mBoundariesVelocity);
    }

    mExtrapolation.Extrapolate(mVelocity, mDirichletBoundaries, mNeumannBoundaries);
    Advect(mVelocity);
}

void Engine::RenderMask(Vortex2D::Fluid::Buffer &mask)
{
    mExtrapolation.RenderMask(mask, mDirichletBoundaries, mNeumannBoundaries);
}

void Engine::RenderDirichlet(Renderer::Drawable & object)
{
    Renderer::Disable d(GL_BLEND);
    mDirichletBoundaries.Render(object, mDimensions.InvScale);
}

void Engine::RenderNeumann(Renderer::Drawable & object)
{
    Renderer::Disable d(GL_BLEND);
    mNeumannBoundaries.Render(object, glm::scale(glm::vec3(2.0f, 2.0f, 1.0f))*mDimensions.InvScale);
}

void Engine::RenderVelocities(Renderer::Drawable & object)
{
    Renderer::Disable d(GL_BLEND);
    mBoundariesVelocity.Render(object, mDimensions.InvScale);
}

void Engine::RenderForce(Renderer::Drawable & object)
{
    Renderer::Enable b(GL_BLEND);
    Renderer::BlendState s(GL_FUNC_ADD, GL_ONE, GL_ONE);

    mVelocity.Render(object, mDimensions.InvScale);
}

void Engine::ClearBoundaries()
{
    mDirichletBoundaries.Clear(glm::vec4(0.0f));
    mNeumannBoundaries.Clear(glm::vec4(0.0f));
    mBoundariesVelocity.Clear(glm::vec4(0.0f));
}

void Engine::ClearVelocities()
{
    mBoundariesVelocity.Clear(glm::vec4(0.0f));
}

void Engine::Advect(Fluid::Buffer & buffer)
{
    Renderer::Disable d(GL_BLEND);
    buffer.Swap() = mAdvect(Back(buffer), Back(mVelocity));
}

}}

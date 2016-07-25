//
//  ConjugateGradient.cpp
//  Vertex2D
//
//  Created by Maximilian Maldacker on 15/09/2015.
//  Copyright (c) 2015 Maximilian Maldacker. All rights reserved.
//

#include "ConjugateGradient.h"
#include "Disable.h"

namespace Vortex2D { namespace Fluid {

const char * DivideFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;
    uniform sampler2D u_other;

    void main()
    {
        float x = texture(u_texture, v_texCoord).x;
        float y = texture(u_other, v_texCoord).x;

        colour_out = vec4(x/y, 0.0, 0.0, 0.0);
    }
);

const char * SwizzleFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;

    void main()
    {
        float x = texture(u_texture, v_texCoord).x;

        colour_out = vec4(0.0, x, 0.0, 0.0);
    }
);

const char * MultiplyAddFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;
    uniform sampler2D u_other;
    uniform sampler2D u_scalar;


    void main()
    {
        float x = texture(u_texture, v_texCoord).x;
        float y = texture(u_other, v_texCoord).x;
        float alpha = texture(u_scalar, vec2(0.5)).x;
        
        colour_out = vec4(x+alpha*y, 0.0, 0.0, 0.0);
    }
);

const char * MultiplyMatrixFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;
    uniform sampler2D u_weights;
    uniform sampler2D u_diagonals;

    void main()
    {
        float x = texture(u_texture, v_texCoord).x;

        vec4 p;
        p.x = textureOffset(u_texture, v_texCoord, ivec2(1,0)).x;
        p.y = textureOffset(u_texture, v_texCoord, ivec2(-1,0)).x;
        p.z = textureOffset(u_texture, v_texCoord, ivec2(0,1)).x;
        p.w = textureOffset(u_texture, v_texCoord, ivec2(0,-1)).x;

        vec4 c = texture(u_weights, v_texCoord);
        float d = texture(u_diagonals, v_texCoord).x;

        float multiply = d * x - dot(p,c);
        colour_out = vec4(multiply, 0.0, 0.0, 0.0);
    }
);

const char * MultiplySubFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;
    uniform sampler2D u_other;
    uniform sampler2D u_scalar;

    void main()
    {
        float x = texture(u_texture, v_texCoord).x;
        float y = texture(u_other, v_texCoord).x;
        float alpha = texture(u_scalar, vec2(0.5)).x;

        colour_out = vec4(x-alpha*y, 0.0, 0.0, 0.0);
    }
);

const char * ResidualFrag = GLSL(
    in vec2 v_texCoord;
    out vec4 colour_out;

    uniform sampler2D u_texture;
    uniform sampler2D u_weights;
    uniform sampler2D u_diagonals;

    void main()
    {
        // cell.x is pressure and cell.y is div
        vec2 cell = texture(u_texture, v_texCoord).xy;

        vec4 p;
        p.x = textureOffset(u_texture, v_texCoord, ivec2(1,0)).x;
        p.y = textureOffset(u_texture, v_texCoord, ivec2(-1,0)).x;
        p.z = textureOffset(u_texture, v_texCoord, ivec2(0,1)).x;
        p.w = textureOffset(u_texture, v_texCoord, ivec2(0,-1)).x;

        vec4 c = texture(u_weights, v_texCoord);
        float d = texture(u_diagonals, v_texCoord).x;

        float residual = dot(p,c) - d * cell.x + cell.y;
        colour_out = vec4(residual, 0.0, 0.0, 0.0);
    }
);

ConjugateGradient::ConjugateGradient(const glm::vec2 & size)
    : r(size, 1, true)
    , s(size, 1, true)
    , alpha({1,1}, 1)
    , beta({1,1}, 1)
    , rho({1,1}, 1)
    , rho_new({1,1}, 1)
    , sigma({1,1}, 1)
    , matrixMultiply(Renderer::Shader::TexturePositionVert, MultiplyMatrixFrag)
    , scalarDivision(Renderer::Shader::TexturePositionVert, DivideFrag)
    , swizzle(Renderer::Shader::TexturePositionVert, SwizzleFrag)
    , multiplyAdd(Renderer::Shader::TexturePositionVert, MultiplyAddFrag)
    , multiplySub(Renderer::Shader::TexturePositionVert, MultiplySubFrag)
    , residual(Renderer::Shader::TexturePositionVert, ResidualFrag)
    , identity(Renderer::Shader::TexturePositionVert, Renderer::Shader::TexturePositionFrag)
    , reduce(size)
    , z(size)
    , preconditioner(size)
{
    residual.Use().Set("u_texture", 0).Set("u_weights", 1).Set("u_diagonals", 2).Unuse();
    identity.Use().Set("u_texture", 0).Unuse();
    matrixMultiply.Use().Set("u_texture", 0).Set("u_weights", 1).Set("u_diagonals", 2).Unuse();
    scalarDivision.Use().Set("u_texture", 0).Set("u_other", 1).Unuse();
    swizzle.Use().Set("u_texture", 0).Unuse();
    multiplyAdd.Use().Set("u_texture", 0).Set("u_other", 1).Set("u_scalar", 2).Unuse();
    multiplySub.Use().Set("u_texture", 0).Set("u_other", 1).Set("u_scalar", 2).Unuse();
}

void ConjugateGradient::Init(LinearSolver::Data & data, OperatorContext3Arg div, OperatorContext2Arg weights, OperatorContext1Arg diagonals)
{
    data.Pressure = div;
    data.Weights = weights;
    data.Diagonal = diagonals;

    preconditioner.Init(data, div, weights, diagonals);
}

void ConjugateGradient::Solve(LinearSolver::Data & data)
{
    Renderer::Enable e(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // r = b - Ax
    r = residual(data.Pressure, data.Weights, data.Diagonal);

    // p = 0
    data.Pressure.Clear(glm::vec4(0.0f));

    // z = M^-1 r
    z.Pressure = swizzle(r);
    preconditioner.Solve(z);

    // s = z
    s = identity(z.Pressure);

    // rho = zTr
    rho = reduce(z.Pressure,r);

    for(int i = 0 ; i < 10; ++i)
    {
        // z = Ap
        z.Pressure = matrixMultiply(s, data.Weights, data.Diagonal);

        // alpha = rho / zTs
        sigma = reduce(z.Pressure,s);
        alpha = scalarDivision(rho, sigma);

        // p = p + alpha * s
        data.Pressure.Swap() = multiplyAdd(Back(data.Pressure), s, alpha);

        // r = r - alpha * z
        r.Swap() = multiplySub(Back(r), z.Pressure, alpha);

        // z = M^-1 r
        z.Pressure = swizzle(r);
        preconditioner.Solve(z);

        // rho_new = zTr
        rho_new = reduce(z.Pressure,r);

        // beta = rho_new / rho
        beta = scalarDivision(rho_new, rho);

        // s = z + beta * s
        s.Swap() = multiplyAdd(z.Pressure,Back(s),beta);

        // rho = rho_new
        rho = identity(rho_new);
    }
}

}}

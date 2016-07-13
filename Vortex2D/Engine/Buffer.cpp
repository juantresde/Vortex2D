//
//  Buffer.cpp
//  Vortex2D
//

#include "Buffer.h"
#include "Shader.h"
#include "Operator.h"

namespace Vortex2D { namespace Fluid {

Buffer::Buffer(const glm::vec2 & size, unsigned components, bool doubled, bool depth)
    : RenderTarget(size.x, size.y)
    , mSprite(size)
{
    Add(size, components, depth);
    if(doubled) Add(size, components, depth);
}

Renderer::Reader Buffer::Get()
{
    return {mTextures.front()};
}

void Buffer::ClearStencil()
{
    for(auto && t : mTextures) t.ClearStencil();
}

void Buffer::Linear()
{
    for(auto && t : mTextures) t.SetAntiAliasTexParameters();
}

void Buffer::ClampToEdge()
{
    for(auto && t : mTextures) t.SetClampToEdgeTexParameters();
}

void Buffer::Clear(const glm::vec4 & colour)
{
    for(auto && t : mTextures) t.Clear(colour);
}

void Buffer::Render(Renderer::Drawable & object, const glm::mat4 & transform)
{
    mTextures.front().Render(object, transform);
}

Buffer & Buffer::Swap()
{
    assert(mTextures.size() == 2);
    std::swap(mTextures.front(), mTextures.back());
    return *this;
}

Renderer::Sprite & Buffer::Sprite()
{
    mSprite.SetTexture(mTextures.front());
    return mSprite;
}

void Buffer::Add(const glm::vec2 & size, unsigned components, bool depth)
{
    mTextures.emplace_back(size.x, size.y,
                           components == 1 ? Renderer::Texture::PixelFormat::RF :
                           components == 2 ? Renderer::Texture::PixelFormat::RGF :
                           Renderer::Texture::PixelFormat::RGBAF,
                           depth ? Renderer::RenderTexture::DepthFormat::DEPTH24_STENCIL8 :
                           Renderer::RenderTexture::DepthFormat::NONE);
    mTextures.back().SetAliasTexParameters();
    mTextures.back().SetClampToBorderTexParameters();
}

}}

//
//  Sprite.h
//  Vortex2D
//

#ifndef Sprite_h
#define Sprite_h

#include "Drawable.h"
#include "Texture.h"
#include "Shader.h"

namespace Vortex2D { namespace Renderer {

/**
 * @brief Drawable to draw a texture with a given program
 */
class Sprite : public Drawable
{
public:
    Sprite(const glm::vec2 & size);
    Sprite(Sprite &&);
    ~Sprite();

    /**
     * @brief Set the texture to be bound when rendering.
     */
    void SetTexture(Texture & texture);

    /**
     * @brief Don't bind any texture when rendering, useful when we bind the texture seperately.
     */
    void NoTexture();

    /**
     * @brief The program to use when rendering.
     */
    void SetProgram(Program & program);

    /**
     * @brief Renders the Sprite to a RenderTarget
     */
    void Render(RenderTarget & target, const glm::mat4 & transform = glm::mat4()) override;

private:
    GLuint mVertexArray;
    GLuint mVertexBuffer;

    Texture * mTexture;
    Program * mProgram;
};

}}

#endif /* Sprite_h */

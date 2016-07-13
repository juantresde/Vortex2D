//
//  Operator.h
//  Vortex2D
//

#ifndef Vortex2D_Operator_h
#define Vortex2D_Operator_h

#include "Buffer.h"
#include "Shader.h"
#include <utility>
#include <cassert>

namespace Vortex2D { namespace Fluid {

/** 
 * @brief Helper class to bind a buffer to a specified unit
 */
template<typename T>
struct BindContext : T
{
    BindContext(T & t, int unit) : T(t), unit(unit) {}
    BindContext(Buffer & b, int unit) : T(b), unit(unit) {}

    void Bind()
    {
        T::Bind(unit);
    }

    int unit;
};

/**
 * @brief Helper class returned from the call operator of Operator.
 */
template<typename ... Buffers>
struct OperatorContext
{
    OperatorContext(Renderer::Program & p, const std::tuple<Buffers...> & bindContexts)
    : Program(p), BindContexts(bindContexts) {}

    template<size_t ...I>
    void BindHelper(std::index_sequence<I...>)
    {
        std::tie((std::get<I>(BindContexts).Bind(),std::ignore)...);
    }

    void Bind()
    {
        BindHelper(std::make_index_sequence<sizeof...(Buffers)>());
    }

    Renderer::Program & Program;
    std::tuple<Buffers...> BindContexts;
};

using OperatorContext1Arg = OperatorContext<BindContext<Front>>;
using OperatorContext2Arg = OperatorContext<BindContext<Front>,BindContext<Front>>;
using OperatorContext3Arg = OperatorContext<BindContext<Front>,BindContext<Front>,BindContext<Front>>;

template<typename ... Buffers>
OperatorContext<Buffers...> MakeOperatorContext(Renderer::Program & p, const std::tuple<Buffers...> & bindContexts)
{
    return {p, bindContexts};
}

#define REQUIRES(...) typename std::enable_if<(__VA_ARGS__), int>::type = 0

/**
 * @brief This is a helper class to write succint code when running a shader
 * with multiple Texture inputs on a Buffer
 */
class Operator
{
public:
    /**
     * @brief Constructor with the Vertex and Fragment shader
     * @param vertex the source of the vertex shader (not a filename)
     * @param fragment the source of the fragment shader (not a filename)
     */
    Operator(const char * vertex, const char * fragment) : mProgram(vertex, fragment)
    {}

    /**
     * @brief Use the Program behind the Operator, allows us to set uniforms
     * @return
     */
    Renderer::Program & Use()
    {
        return mProgram.Use();
    }

    /**
     * @brief An overloaded function call operator which takes as argument a list of Buffer objects.
     * The Buffer objects will have their Texture bind in order and a helper class is returned to the Buffer.
     * This Buffer object will then in turn run the Program of this Operator object on its front RenderTexture.
     * @code
     * Buffer input1(size, 1), input2(size, 1), output(size), 1;
     * Operator op(VertexSrc, FragmentSrc);
     * // this will run shader VertexSrc/FragmentSrc with texture input1 & input2
     * // and render on output
     * output = op(input1, intpu2);
     * @endcode
     */
    template<typename... Args>
    auto operator()(Args && ... args)
    {
        auto bindContexts = BindHelper(0, std::forward<Args>(args)...);
        return MakeOperatorContext(mProgram, bindContexts);
    }

private:
    template<typename T, typename ... Args, REQUIRES(std::is_same<T, Buffer&>())>
    auto BindHelper(int unit, T && input, Args && ... args)
    {
        BindContext<Front> context(input, unit);
        return std::tuple_cat(std::make_tuple(context), BindHelper(unit+1, std::forward<Args>(args)...));
    }

    template<typename T, typename ... Args, REQUIRES(std::is_same<T, Back>())>
    auto BindHelper(int unit, T && input, Args && ... args)
    {
        BindContext<Back> context(input, unit);
        return std::tuple_cat(std::make_tuple(context), BindHelper(unit+1, std::forward<Args>(args)...));
    }

    auto BindHelper(int unit)
    {
        return std::make_tuple();
    }

    Renderer::Program mProgram;
};

}}

#endif

//
//  Engine.h
//  Vortex
//
//  Created by Maximilian Maldacker on 08/04/2014.
//
//

#ifndef __Vortex__Engine__
#define __Vortex__Engine__

#include "SuccessiveOverRelaxation.h"
#include "RenderTexture.h"
#include "Boundaries.h"
#include "Advection.h"
#include "Size.h"
#include "Operator.h"

#include <vector>

namespace Fluid
{

class Engine
{
public:
    Engine(Dimensions dimensions, Boundaries & boundaries, Advection & advection, LinearSolver * linearSolver);
    
    void Solve();

    void Div();
    void Project();

    void LinearInit(Boundaries & boundaries);
    void LinearSolve();

//private:
    Dimensions mDimensions;
    Renderer::Quad mQuad;

    Boundaries & mBoundaries;
    Advection & mAdvection;
    LinearSolver * mLinearSolver;

    Operator mDiv;
    Operator mProject;
};

}

#endif /* defined(__Vortex__Engine__) */

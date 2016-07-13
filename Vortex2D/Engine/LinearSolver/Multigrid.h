//
//  Multigrid.h
//  Vortex2D
//

#ifndef Multigrid_h
#define Multigrid_h

#include "LinearSolver.h"
#include "Operator.h"

namespace Vortex2D { namespace Fluid {

class Multigrid : public LinearSolver
{
public:
    Multigrid(glm::vec2 size);
    
    void Init(Data & data, OperatorContext3Arg div, OperatorContext2Arg weights, OperatorContext1Arg diagonals) override;
    void Solve(Data & data) override;

private:
    Data & GetData(int depth);
    void DampedJacobi(Data & data, int iterations = 10);
    
    int mDepths;
    Operator mProlongate;
    Operator mResidual;
    Operator mRestrict;
    Operator mDampedJacobi;
    Operator mIdentity;

    std::vector<Data> mDatas;
};

}}

#endif
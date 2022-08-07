#ifndef SamplingCA_BlackBox_Solver_h
#define SamplingCA_BlackBox_Solver_h

#include <vector>

using std::vector;

namespace CDCLSolver {

class Solver{
public:
    Solver();
    virtual ~Solver();
    void read_clauses(int nvar, const vector<vector<int> >& clauses);
    void add_assumption(int var, int truth_value);
    void add_assumption(int lit);
    void clear_assumptions();
    bool solve();
    void get_solution(vector<int>& tc);
protected:
    void* internal_solver;
    vector<int> assumptions;
};

}

#endif
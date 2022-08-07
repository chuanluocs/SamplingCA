#ifndef ExtMinisat_h
#define ExtMinisat_h

#include <vector>
#include <numeric>
#include <random>
#include <utility>
#include <algorithm>

using std::vector;
using std::pair;

namespace ExtMinisat {

class SamplingSolver{
public:
    SamplingSolver(int nvar, const vector<vector<int> >& clauses, int seed, bool do_shuffle, int enable_VSIDS);
    virtual ~SamplingSolver();
    void set_prob(const vector<pair<int, int> >& sample_prob);
    void get_solution(vector<int>& tc);
    vector<int> get_solution();
protected:
    void* internal_solver;
    std::mt19937_64 gen;
    bool enable_shuffling;
    vector<int> vecc;
};

}

#endif
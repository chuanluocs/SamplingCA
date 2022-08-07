#include "Ext.h"
#include "Solver.h"

using namespace ExtMinisat;

SamplingSolver::SamplingSolver(
    int nvar, 
    const vector<vector<int> >& clauses, 
    int seed, 
    bool do_shuffle, 
    int enable_VSIDS)
: gen(seed), enable_shuffling(do_shuffle){
    Minisat::Solver* sv = new Minisat::Solver(enable_VSIDS, seed);
    internal_solver = sv;

    vecc.resize(nvar);
    iota(vecc.begin(), vecc.end(), 0);

    sv->shuffled_var.growTo(nvar);
    for (int i = 0; i < nvar; ++i){
        sv->shuffled_var[i] = i;
    }

    Minisat::vec<Minisat::Lit> lits;
    for (const vector<int>& cl: clauses){
        lits.clear();
        for (int rawvar: cl){
            int var = abs(rawvar) - 1;
            while (var >= sv->nVars()) sv->newVar();
            lits.push(Minisat::mkLit(var, rawvar < 0));
        }
        sv->addClause_(lits);
    }

    while (nvar > sv->nVars()){
        sv->newVar();
    }

    sv->user_pol.growTo(sv->nVars(), (Minisat::lbool((uint8_t)2)));
}

SamplingSolver::~SamplingSolver(){ 
    Minisat::Solver* sv = (Minisat::Solver*) internal_solver; 
    delete sv; 
}

void SamplingSolver::set_prob(const vector<pair<int, int> >& sample_prob){
    Minisat::Solver* sv = (Minisat::Solver*)internal_solver; 

    if (!sample_prob.empty()){
        for (int i = 0; i < sv->nVars(); ++i){
            int l0 = sample_prob[i].first, l1 = sample_prob[i].second;
            if (!l0 && !l1) l0 = l1 = 1;
            int t = gen() % (l0 + l1);
            if (t < l0){
                sv->setPolarity(i, (Minisat::lbool((uint8_t)0)));
            } else {
                sv->setPolarity(i, (Minisat::lbool((uint8_t)1)));
            }
        }
    }
}

void SamplingSolver::get_solution(vector<int>& tc){
    Minisat::Solver* sv = (Minisat::Solver*)internal_solver; 

    if (enable_shuffling){
        std::shuffle(vecc.begin(), vecc.end(), gen);
    }
    for (int i = 0; i < sv->nVars(); ++i){
        sv->shuffled_var[i] = vecc[i];
    }

    tc.clear();
    if (!sv->simplify()) return ;

    Minisat::vec<Minisat::Lit> dummy;
    bool res = sv->solve(dummy);

    if (res){
        for (int i = 0; i < sv->nVars(); i++){
            if (sv->model[i] == (Minisat::lbool((uint8_t)0))) tc.emplace_back(1);
            else if (sv->model[i] == (Minisat::lbool((uint8_t)1))) tc.emplace_back(0);
            else tc.emplace_back(-1);
        }
    }
}

vector<int> SamplingSolver::get_solution(){
    vector<int> res;
    get_solution(res);
    return res;
}
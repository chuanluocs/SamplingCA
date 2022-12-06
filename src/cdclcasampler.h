#ifndef SAMPLINGCA_SAMPLER_INCLUDE_H
#define SAMPLINGCA_SAMPLER_INCLUDE_H

#include "pboccsatsolver.h"
#include "../minisat_ext/BlackBoxSolver.h"
#include "../minisat_ext/Ext.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <iostream>
#include <fstream>

using std::vector;
using std::pair;
using std::queue;
using std::ifstream;

class SimpleBitSet {
public:
    SimpleBitSet(): siz(0), cap(0), cnt(0ll){
        arr = nullptr;
    }
    SimpleBitSet(size_t _siz): siz(_siz), cnt(0ll){
        cap = (_siz + 31) >> 5;
        if (cap == 0) cap = 1;
        arr = new uint32_t[cap]{0};
    }
    SimpleBitSet(const SimpleBitSet& bs): siz(bs.siz), cap(bs.cap), cnt(bs.cnt){
        arr = new uint32_t[bs.cap]{0};
        memcpy(arr, bs.arr, sizeof(uint32_t) * cap);
    }
    SimpleBitSet(SimpleBitSet&& bs): siz(bs.siz), cap(bs.cap), cnt(bs.cnt){
        arr = bs.arr;
        bs.arr = nullptr;
        bs.siz = 0, bs.cap = 0, bs.cnt = 0;
    }
    ~SimpleBitSet(){ delete [] arr; }
    SimpleBitSet& operator=(const SimpleBitSet& bs){
        if (this == &bs) return *this;
        delete [] arr;
        siz = bs.siz;
        cap = bs.cap;
        cnt = bs.cnt;
        arr = new uint32_t[cap]{0};
        memcpy(arr, bs.arr, sizeof(uint32_t) * cap);
        return *this;
    }
    SimpleBitSet& operator=(SimpleBitSet&& bs){
        if (this == &bs) return *this;
        delete [] arr;
        siz = bs.siz;
        cap = bs.cap;
        cnt = bs.cnt;
        arr = bs.arr;
        bs.arr = nullptr;
        bs.siz = 0, bs.cap = 0, bs.cnt = 0;
        return *this;
    }
    bool set(size_t idx){
        uint32_t& t1 = arr[idx >> 5];
        uint32_t cg = (1u << (idx & 31));
        if ((t1 & cg) == 0u){
            ++cnt;
            t1 |= cg;
            return true;
        }
        return false;
    }
    bool unset(size_t idx){
        uint32_t& t1 = arr[idx >> 5];
        uint32_t cg = (1u << (idx & 31));
        if ((t1 & cg) != 0u){
            --cnt;
            t1 ^= cg;
            return true;
        }
        return false;
    }
    bool get(size_t idx) const {
        return (arr[idx >> 5] & (1 << (idx & 31))) != 0u;
    }
    long long count() const {
        return cnt;
    }
    void difference_(const SimpleBitSet& bs){
        for (size_t i = 0; i < cap; ++i){
            uint32_t tmp = arr[i] & bs.arr[i];
            if (tmp != 0u){
                arr[i] ^= tmp;
                cnt -= __builtin_popcount(tmp);
            }
        }
    }
    bool init_iter(){
        iter_curpos = iter_curbit = 0;
        while (iter_curpos < cap && arr[iter_curpos] == 0u){
            ++iter_curpos;
        }
        if (iter_curpos < cap){
            iter_curbit = __builtin_ctz(arr[iter_curpos]);
            iter_curpos_msb = 31 - __builtin_clz(arr[iter_curpos]);
            return true;
        }
        return false;
    }
    size_t iter_get() const {
        return (iter_curpos << 5) | iter_curbit;
    }
    bool iter_next(){
        if (iter_curpos == cap) return false;

        if (iter_curbit < iter_curpos_msb){
            uint32_t tmp = arr[iter_curpos];
            do { ++iter_curbit; } while (((tmp >> iter_curbit) & 1u) == 0u);
            return true;
        }
        ++iter_curpos, iter_curbit = 0u;
        while (iter_curpos < cap && arr[iter_curpos] == 0u){
            ++iter_curpos;
        }
        if (iter_curpos == cap){
            return false;
        }
        iter_curbit = __builtin_ctz(arr[iter_curpos]);
        iter_curpos_msb = 31 - __builtin_clz(arr[iter_curpos]);
        return true;
    }
private:
    uint32_t* arr;
    size_t siz;
    size_t cap;
    long long cnt;
    
    size_t iter_curpos;
    uint32_t iter_curbit;
    int iter_curpos_msb;
};

class CDCLCASampler
{
public:
    CDCLCASampler(std::string cnf_file_path, int seed);
    ~CDCLCASampler();
    
    void SetDefaultPara();
    void SetCandidateSetSize(int candidate_set_size);
    void SetTWise(int t_wise);
    void SetWeightedSamplingMethod(bool use_weighted_sampling);
    inline void SetContextAwareMethod(int use_context_aware){ context_aware_method_ = use_context_aware; }
    void SetCNFReductionMethod(bool use_cnf_reduction);
    inline void SetPureCDCL() { flag_use_pure_cdcl_ = true; part_for_sls = 0; }
    inline void SetPureSLS() { flag_use_pure_sls_ = true; part_for_sls = candidate_set_size_; }
    inline void SetReducedCNFPath(std::string reduced_cnf_path) { flag_reduced_cnf_as_temp_ = false; reduced_cnf_file_path_ = reduced_cnf_path; }
    inline void SetTestcaseSetSavePath(std::string testcase_set_path) { testcase_set_save_path_ = testcase_set_path; }
    inline void EnableVSIDSForCDCL() { enable_VSIDS = 1; }
    inline void DisableVarShuffling() { no_shuffle_var_ = true; }
    void SetUpperLimit(std::string answer_file_path);
    void SetFinalStrategy(int x);
    
    void GenerateInitTestcaseSLS();
    void GenerateInitTestcaseCDCL();
    void GenerateTestcase();
    std::vector<int> GetSatTestcaseWithGivenInitSolution(const std::vector<int>& init_solution);
    std::vector<int> GetWeightedSampleInitSolution();
    void GenerateCandidateTestcaseSet();
    int SelectTestcaseFromCandidateSetByTupleNum();

    long long Get2TupleMapIndex(long i, long v_i, long j, long v_j);

    void Init2TupleInfo();
    void Update2TupleInfo();

    void InitPbOCCSATSolver();

    void InitSampleWeightByAppearance();
    void UpdateSampleWeightByAppearance();
    void InitSampleWeightUniformly();

    void InitContextAwareFlipPriority();
    void UpdateContextAwareFlipPriority();
    void UpdateContextAwareFlipPriorityBySampleWeight(const std::vector<int> &init_solution);

    void ReduceCNF();

    void Init();
    
    void GenerateCoveringArray();

    inline long long GetTupleNum() { return num_tuple_; }
    inline long long GetExactAllTupleNum() { return num_tuple_all_exact_; }
    inline long long GetTupleCoverage() { return coverage_tuple_; }

    void SaveTestcaseSet(std::string result_path);
    
private:
    std::string cnf_file_path_;
    std::string reduced_cnf_file_path_;
    std::string testcase_set_save_path_;
    int seed_;
    int t_wise_;
    double cpu_time_;
    int candidate_set_size_;
    bool flag_use_weighted_sampling_;
    int context_aware_method_;
    bool flag_use_cnf_reduction_;
    bool flag_use_pure_cdcl_;
    bool flag_use_pure_sls_;
    bool flag_reduced_cnf_as_temp_;

    std::string cnf_instance_name_;
    std::mt19937_64 rnd_file_id_gen_;

    PbOCCSATSolver *pbo_solver_;

    Mersenne rng_;
    int num_var_;
    int num_clauses_;
    int num_generated_testcase_;
    int selected_candidate_index_;
    SimpleBitSet selected_candidate_bitset_;

    long long num_tuple_;
    long long num_combination_all_possible_;
    long long num_tuple_all_possible_;
    long long num_tuple_all_exact_;
    double coverage_tuple_;

    std::vector<std::vector<int>> testcase_set_;
    std::vector<std::vector<int>> candidate_testcase_set_;
    std::vector<std::vector<int>> candidate_sample_init_solution_set_;
    std::vector<int> var_positive_appearance_count_;
    std::vector<double> var_positive_sample_weight_;
    std::vector<double> context_aware_flip_priority_;

    std::vector<int> count_each_var_positive_uncovered_;
    std::vector<int> count_each_var_negative_uncovered_;

    void (CDCLCASampler::*p_init_context_aware_flip_priority_)();
    void (CDCLCASampler::*p_update_context_aware_flip_priority_)(const std::vector<int> &init_solution);
    void (CDCLCASampler::*p_init_sample_weight_)();
    void (CDCLCASampler::*p_update_sample_weight_)();
    void (CDCLCASampler::*p_reduce_cnf_)();

    void (CDCLCASampler::*p_init_tuple_info_)();
    void (CDCLCASampler::*p_update_tuple_info_)();

    inline void EmptyFunRetVoid() {}
    inline void EmptyFunRetVoid(const std::vector<int>& init_solution) {}

    std::string get_random_prefix();
    void remove_temp_files();

    vector<pair<vector<int>, SimpleBitSet> > pq;
    vector<int> pq_idx;
    SimpleBitSet already_t_wise;
    SimpleBitSet get_gain(const vector<int>& asgn);
    void clear_pq();

    int part_for_sls;

    vector<pair<int, int> > get_prob_in(bool init);

    CDCLSolver::Solver *cdcl_solver;
    ExtMinisat::SamplingSolver *cdcl_sampler;

    void clear_final();
    void (CDCLCASampler::*p_choose_final)();
    int final_strategy;
    void choose_final_plain();
    void choose_final_random_contiguous_solving(bool simplify);
    inline void choose_final_random_contiguous_solving_simpl(){ choose_final_random_contiguous_solving(true); }
    inline void choose_final_random_contiguous_solving_nosimpl(){ choose_final_random_contiguous_solving(false); }
    void choose_final_solution_modifying(bool shf);
    inline void choose_final_solution_modifying_shuffle(){ choose_final_solution_modifying(true); }
    inline void choose_final_solution_modifying_noshuffle(){ choose_final_solution_modifying(false); }

    void find_uncovered_tuples(bool simplify);
    vector<vector<int> > uncovered_tuples;
    vector<vector<int> > uncovered_possible_solutions;

    void get_cadical_solution(vector<int>& tc);

    void read_cnf_header(ifstream& ifs, int& nvar, int& nclauses);
    bool read_cnf();
    bool check_no_clauses();
    vector<vector<int> > clauses;
    vector<vector<int> > pos_in_cls;
    vector<vector<int> > neg_in_cls;

    bool no_shuffle_var_;
    int enable_VSIDS;

    bool use_upperlimit;
    long long upperlimit;

    vector<int> as_backbone;
};

#endif
#include "cdclcasampler.h"
#include <vector>
#include <string>
#include <unistd.h>
#include <chrono>

#include <iostream>
#include <sstream>

using namespace std;

const int kMaxPbOCCSATSeed = 10000000;

CDCLCASampler::CDCLCASampler(string cnf_file_path, int seed): rnd_file_id_gen_(seed)
{
    cnf_file_path_ = cnf_file_path;
    seed_ = seed;
    rng_.seed(seed_);
    SetDefaultPara();
}

CDCLCASampler::~CDCLCASampler()
{
    delete pbo_solver_;
    delete cdcl_sampler;
    for (int i = 2; i <= t_wise_; ++i){
        delete [] combnum[i];
    }
    delete [] combnum;
}

void CDCLCASampler::SetDefaultPara()
{
    candidate_set_size_ = 100;
    candidate_sample_init_solution_set_.resize(candidate_set_size_);
    candidate_testcase_set_.resize(candidate_set_size_);

    part_for_sls = candidate_set_size_ >> 1;

    t_wise_ = 2;
    p_init_tuple_info_ = &CDCLCASampler::Init2TupleInfo;
    p_update_tuple_info_ = &CDCLCASampler::Update2TupleInfo;

    context_aware_method_ = 2;

    flag_use_weighted_sampling_ = true;
    p_init_sample_weight_ = &CDCLCASampler::InitSampleWeightByAppearance;
    p_update_sample_weight_ = &CDCLCASampler::UpdateSampleWeightByAppearance;

    SetPureCDCL();

    p_init_context_aware_flip_priority_ = &CDCLCASampler::InitContextAwareFlipPriority;
    p_update_context_aware_flip_priority_ = &CDCLCASampler::UpdateContextAwareFlipPriorityBySampleWeight;

    flag_use_cnf_reduction_ = true;
    p_reduce_cnf_ = &CDCLCASampler::ReduceCNF;

    no_shuffle_var_ = false;

    int pos = cnf_file_path_.find_last_of('/');
    cnf_instance_name_ = cnf_file_path_.substr(pos + 1);
    size_t suffix_pos = cnf_instance_name_.rfind(".cnf");
    if (suffix_pos != string::npos){
        cnf_instance_name_.replace(suffix_pos, 4, "");
    }

    flag_reduced_cnf_as_temp_ = true;
    reduced_cnf_file_path_ = "/tmp/" + get_random_prefix() + "_reduced.cnf";
    testcase_set_save_path_ = "./" + cnf_instance_name_ + "_testcase_set.txt";

    use_upperlimit = false;

    enable_VSIDS = 0;

    final_strategy = 0;
    p_choose_final = &CDCLCASampler::choose_final_plain;
}

void CDCLCASampler::SetCandidateSetSize(int candidate_set_size)
{
    candidate_set_size_ = candidate_set_size;
    candidate_sample_init_solution_set_.resize(candidate_set_size_);
    candidate_testcase_set_.resize(candidate_set_size_);
}

void CDCLCASampler::SetTWise(int t_wise)
{
    t_wise_ = t_wise;
    if (t_wise_ == 2){
        p_init_tuple_info_ = &CDCLCASampler::Init2TupleInfo;
        p_update_tuple_info_ = &CDCLCASampler::Update2TupleInfo;
    } else if (t_wise_ == 3){
        p_init_tuple_info_ = &CDCLCASampler::Init3TupleInfo;
        p_update_tuple_info_ = &CDCLCASampler::Update3TupleInfo;
    } else if (t_wise_ == 4){
        p_init_tuple_info_ = &CDCLCASampler::Init4TupleInfo;
        p_update_tuple_info_ = &CDCLCASampler::Update4TupleInfo;
    } else if (t_wise_ == 5){
        p_init_tuple_info_ = &CDCLCASampler::Init5TupleInfo;
        p_update_tuple_info_ = &CDCLCASampler::Update5TupleInfo;
    } else if (t_wise_ == 6){
        p_init_tuple_info_ = &CDCLCASampler::Init6TupleInfo;
        p_update_tuple_info_ = &CDCLCASampler::Update6TupleInfo;
    } else {
        cout << "c error!" << endl;
    }
}

void CDCLCASampler::SetWeightedSamplingMethod(bool use_weighted_sampling)
{
    flag_use_weighted_sampling_ = use_weighted_sampling;
    if (flag_use_weighted_sampling_)
    {
        p_init_sample_weight_ = &CDCLCASampler::InitSampleWeightByAppearance;
        p_update_sample_weight_ = &CDCLCASampler::UpdateSampleWeightByAppearance;
    }
    else
    {
        p_init_sample_weight_ = &CDCLCASampler::InitSampleWeightUniformly;
        p_update_sample_weight_ = &CDCLCASampler::EmptyFunRetVoid;
    }
}

void CDCLCASampler::SetCNFReductionMethod(bool use_cnf_reduction)
{
    flag_use_cnf_reduction_ = use_cnf_reduction;
    if (flag_use_cnf_reduction_)
    {
        p_reduce_cnf_ = &CDCLCASampler::ReduceCNF;
    }
    else
    {
        p_reduce_cnf_ = &CDCLCASampler::EmptyFunRetVoid;
    }
}

void CDCLCASampler::SetFinalStrategy(int strategy)
{
    final_strategy = strategy;
    // if (strategy == 0){
    //     p_choose_final = &CDCLCASampler::choose_final_plain;
    // } else if (strategy == 1){
    //     p_choose_final = &CDCLCASampler::choose_final_random_contiguous_solving_simpl;
    // } else if (strategy == 2){
    //     p_choose_final = &CDCLCASampler::choose_final_random_contiguous_solving_nosimpl;
    // } else if (strategy == 3){
    //     p_choose_final = &CDCLCASampler::choose_final_solution_modifying_shuffle;
    // } else if (strategy == 4){
    //     p_choose_final = &CDCLCASampler::choose_final_solution_modifying_noshuffle;
    // }
}

void CDCLCASampler::GenerateInitTestcaseSLS()
{
    bool is_sat = pbo_solver_->solve();
    if (is_sat)
    {
        testcase_set_[0] = pbo_solver_->get_sat_solution();
        num_generated_testcase_ = 1;
    }
    else
    {
        cout << "c PbOCCSAT Failed to Find Initial Sat Solution!" << endl;
    }
}

void CDCLCASampler::GenerateInitTestcaseCDCL()
{
    vector<pair<int, int> > sample_prob = get_prob_in(true);
    cdcl_sampler->set_prob(sample_prob);
    testcase_set_.emplace_back(cdcl_sampler->get_solution());
    num_generated_testcase_ = 1;
}

long long CDCLCASampler::Get2TupleMapIndex(int i, int vi, int j, int vj){
    // for performance assurance, do not check i < j or not
    long long base = (vi << 1 | vj) * num_combination_all_possible_;
    return base + Get2Base(num_var_, i, j);
}

long long CDCLCASampler::Get3TupleMapIndex(int i1, int v1, int i2, int v2, int i3, int v3){
    long long base = (v1 << 2 | v2 << 1 | v3) * num_combination_all_possible_;
    return base + Get3Base(num_var_, i1, i2, i3);
}

long long CDCLCASampler::Get4TupleMapIndex(int i1, int v1, int i2, int v2, int i3, int v3, int i4, int v4){
    long long base = (v1 << 3 | v2 << 2 | v3 << 1 | v4) * num_combination_all_possible_;
    return base + Get4Base(num_var_, i1, i2, i3, i4);
}

long long CDCLCASampler::Get5TupleMapIndex(int i1, int v1, int i2, int v2, int i3, int v3, int i4, int v4, int i5, int v5){
    long long base = (v1 << 4 | v2 << 3 | v3 << 2 | v4 << 1 | v5) * num_combination_all_possible_;
    return base + Get5Base(num_var_, i1, i2, i3, i4, i5);
}
    
long long CDCLCASampler::Get6TupleMapIndex(int i1, int v1, int i2, int v2, int i3, int v3, int i4, int v4, int i5, int v5, int i6, int v6){
    long long base = (v1 << 5 | v2 << 4 | v3 << 3 | v4 << 2 | v5 << 1 | v6) * num_combination_all_possible_;
    return base + Get6Base(num_var_, i1, i2, i3, i4, i5, i6);
}

void CDCLCASampler::Init2TupleInfo()
{
    num_combination_all_possible_ = num_var_ * (num_var_ - 1) / 2;
    num_tuple_all_possible_ = num_combination_all_possible_ * 4;
}

void CDCLCASampler::Init3TupleInfo()
{
    num_combination_all_possible_ = combnum[3][num_var_];
    num_tuple_all_possible_ = num_combination_all_possible_ * 8;
}

void CDCLCASampler::Init4TupleInfo()
{
    num_combination_all_possible_ = combnum[4][num_var_];
    num_tuple_all_possible_ = num_combination_all_possible_ * 16;
}

void CDCLCASampler::Init5TupleInfo()
{
    num_combination_all_possible_ = combnum[5][num_var_];
    num_tuple_all_possible_ = num_combination_all_possible_ * 32;
}

void CDCLCASampler::Init6TupleInfo()
{
    num_combination_all_possible_ = combnum[6][num_var_];
    num_tuple_all_possible_ = num_combination_all_possible_ * 64;
}

void CDCLCASampler::Update2TupleInfo(){
    int index_testcase = num_generated_testcase_ - 1;
    const vector<int>& testcase = testcase_set_[index_testcase];

    for (int i = 0; i < num_var_ - 1; i++){
        for (int j = i + 1; j < num_var_; j++){
            // TODO improve this
            long long index_tuple = Get2TupleMapIndex(i, testcase[i], j, testcase[j]);
            bool res = already_t_wise.set(index_tuple);
            if (res){
                ++num_tuple_;
            }
        }
    }
}

void CDCLCASampler::Update3TupleInfo(){
    int index_testcase = num_generated_testcase_ - 1;
    const vector<int>& testcase = testcase_set_[index_testcase];

    for (int i1 = 0; i1 < num_var_; i1++){
        for (int i2 = i1 + 1; i2 < num_var_; i2++){
            for (int i3 = i2 + 1; i3 < num_var_; i3++){
                long long index_tuple = Get3TupleMapIndex(i1, testcase[i1], i2, testcase[i2], i3, testcase[i3]);
                bool res = already_t_wise.set(index_tuple);
                if (res){
                    ++num_tuple_;
                }
            }
        }
    }
}

void CDCLCASampler::Update4TupleInfo(){
    int index_testcase = num_generated_testcase_ - 1;
    const vector<int>& testcase = testcase_set_[index_testcase];

    for (int i1 = 0; i1 < num_var_; i1++){
        for (int i2 = i1 + 1; i2 < num_var_; i2++){
            for (int i3 = i2 + 1; i3 < num_var_; i3++){
                for (int i4 = i3 + 1; i4 < num_var_; i4++){
                    long long index_tuple = Get4TupleMapIndex(i1, testcase[i1], i2, testcase[i2], i3, testcase[i3], i4, testcase[i4]);
                    bool res = already_t_wise.set(index_tuple);
                    if (res){
                        ++num_tuple_;
                    }
                }
            }
        }
    }
}

void CDCLCASampler::Update5TupleInfo(){
    int index_testcase = num_generated_testcase_ - 1;
    const vector<int>& testcase = testcase_set_[index_testcase];

    for (int i1 = 0; i1 < num_var_; i1++){
        for (int i2 = i1 + 1; i2 < num_var_; i2++){
            for (int i3 = i2 + 1; i3 < num_var_; i3++){
                for (int i4 = i3 + 1; i4 < num_var_; i4++){
                    for (int i5 = i4 + 1; i5 < num_var_; i5++){
                        long long index_tuple = Get5TupleMapIndex(i1, testcase[i1], i2, testcase[i2], i3, testcase[i3], i4, testcase[i4], i5, testcase[i5]);
                        bool res = already_t_wise.set(index_tuple);
                        if (res){
                            ++num_tuple_;
                        }
                    }
                }
            }
        }
    }
}

void CDCLCASampler::Update6TupleInfo(){
    int index_testcase = num_generated_testcase_ - 1;
    const vector<int>& testcase = testcase_set_[index_testcase];

    for (int i1 = 0; i1 < num_var_; i1++){
        for (int i2 = i1 + 1; i2 < num_var_; i2++){
            for (int i3 = i2 + 1; i3 < num_var_; i3++){
                for (int i4 = i3 + 1; i4 < num_var_; i4++){
                    for (int i5 = i4 + 1; i5 < num_var_; i5++){
                        for (int i6 = i5 + 1; i6 < num_var_; i6++){
                            long long index_tuple = Get6TupleMapIndex(i1, testcase[i1], i2, testcase[i2], i3, testcase[i3], i4, testcase[i4], i5, testcase[i5], i6, testcase[i6]);
                            bool res = already_t_wise.set(index_tuple);
                            if (res){
                                ++num_tuple_;
                            }
                        }
                    }
                }
            }
        }
    }
}

void CDCLCASampler::InitSampleWeightByAppearance()
{
    var_positive_appearance_count_.resize(num_var_);
    var_positive_sample_weight_.resize(num_var_);
}

void CDCLCASampler::UpdateSampleWeightByAppearance()
{
    int new_testcase_index = num_generated_testcase_ - 1;
    const vector<int>& new_testcase = testcase_set_[new_testcase_index];
    for (int v = 0; v < num_var_; v++)
    {
        var_positive_appearance_count_[v] += new_testcase[v];
        var_positive_sample_weight_[v] = 1. - double(var_positive_appearance_count_[v]) / num_generated_testcase_;
    }
}

void CDCLCASampler::InitSampleWeightUniformly()
{
    var_positive_sample_weight_.resize(num_var_, 0.5);
}

void CDCLCASampler::InitContextAwareFlipPriority()
{
    context_aware_flip_priority_.resize(num_var_, 0.);
}

void CDCLCASampler::UpdateContextAwareFlipPriority()
{
    vector<int> init_solution = candidate_sample_init_solution_set_[selected_candidate_index_];
    vector<int> sat_testcase = candidate_testcase_set_[selected_candidate_index_];
    vector<int> var_flip_count(num_var_);
    for (int v = 0; v < num_var_; v++)
    {
        var_flip_count[v] = abs(init_solution[v] - sat_testcase[v]);
    }
    for (int v = 0; v < num_var_; v++)
    {
        context_aware_flip_priority_[v] += var_flip_count[v];
    }
    pbo_solver_->set_var_flip_priority_ass_unaware(context_aware_flip_priority_);
}

void CDCLCASampler::UpdateContextAwareFlipPriorityBySampleWeight(const vector<int> &init_solution)
{
    for (int v = 0; v < num_var_; v++)
    {
        if (init_solution[v])
            context_aware_flip_priority_[v] = var_positive_sample_weight_[v];
        else
            context_aware_flip_priority_[v] = 1. - var_positive_sample_weight_[v];
    }

    pbo_solver_->set_var_flip_priority_ass_aware(context_aware_flip_priority_);
}

void CDCLCASampler::ReduceCNF()
{
    string cmd = "./bin/coprocessor -enabled_cp3 -up -subsimp -no-bve -no-bce"
                 " -no-dense -dimacs=" +
                 reduced_cnf_file_path_ + " " + cnf_file_path_;

    int return_val = system(cmd.c_str());

    cnf_file_path_ = reduced_cnf_file_path_;
}

void CDCLCASampler::InitPbOCCSATSolver()
{
    pbo_solver_ = new PbOCCSATSolver(cnf_file_path_, rng_.next(kMaxPbOCCSATSeed), num_var_);
}

void CDCLCASampler::Init()
{
    num_var_ = 0;

    if (!check_no_clauses()){
        (this->*p_reduce_cnf_)();
    } else {
        flag_use_cnf_reduction_ = false;
    }
    
    InitPbOCCSATSolver();

    read_cnf();

    cdcl_sampler = new ExtMinisat::SamplingSolver(num_var_, clauses, seed_, !no_shuffle_var_, enable_VSIDS);

    if (flag_use_pure_cdcl_){
        GenerateInitTestcaseCDCL();
    } else {
        GenerateInitTestcaseSLS();
    }

    // 构建组合数表
    combnum = new long long *[t_wise_ + 1];
    for (int i = 2; i <= t_wise_; ++i){
        combnum[i] = new long long[num_var_ + 1]{0};
    }
    for (int i = 2; i <= num_var_; ++i){
        combnum[2][i] = 1ll * i * (i - 1) / 2;
        for (int j = 3; j <= t_wise_; ++j){
            combnum[j][i] = combnum[j][i - 1] + combnum[j - 1][i - 1];
        }
    }

    uncovered_tuple_tmp = vector<pair<int, int> >(t_wise_, {0, 0});

    (this->*p_init_tuple_info_)();

    already_t_wise = SimpleBitSet(num_tuple_all_possible_);
    num_tuple_ = 0;
    
    (this->*p_init_sample_weight_)();
    (this->*p_init_context_aware_flip_priority_)();
}

vector<int> CDCLCASampler::GetSatTestcaseWithGivenInitSolution(const vector<int> &init_solution)
{
    int pbo_seed = rng_.next(kMaxPbOCCSATSeed);
    pbo_solver_->set_seed(pbo_seed);

    pbo_solver_->set_init_solution(init_solution);

    (this->*p_update_context_aware_flip_priority_)(init_solution);

    bool is_sat = pbo_solver_->solve();

    if (is_sat)
    {
        return pbo_solver_->get_sat_solution();
    }
    else
    {
        cout << "c PbOCCSAT Failed to Find Sat Solution!" << endl;
    }
}

vector<int> CDCLCASampler::GetWeightedSampleInitSolution()
{
    vector<int> weighted_sample_init_solution(num_var_, 0);
    for (int v = 0; v < num_var_; v++)
    {
        if (rng_.nextClosed() < var_positive_sample_weight_[v])
        {
            weighted_sample_init_solution[v] = 1;
        }
    }
    return weighted_sample_init_solution;
}

void CDCLCASampler::GenerateCandidateTestcaseSet()
{
    vector<pair<int, int> > sample_prob = get_prob_in(false);

    for (int i = 0; i < part_for_sls; i++){
        candidate_sample_init_solution_set_[i] = GetWeightedSampleInitSolution();
        candidate_testcase_set_[i] = GetSatTestcaseWithGivenInitSolution(candidate_sample_init_solution_set_[i]);
    }
    
    for (int i = part_for_sls; i < candidate_set_size_; i++){
        cdcl_sampler->set_prob(sample_prob);
        cdcl_sampler->get_solution(candidate_testcase_set_[i]);
    }
}

int CDCLCASampler::SelectTestcaseFromCandidateSetByTupleNum()
{
    for (int i = 0; i < candidate_set_size_; ++i){
        pq.emplace_back(candidate_testcase_set_[i], get_gain(candidate_testcase_set_[i]));
        pq_idx.push_back(0);
    }
    
    iota(pq_idx.begin(), pq_idx.end(), 0);
    sort(pq_idx.begin(), pq_idx.end(), [&](int i, int j){
        return pq[i].second.count() > pq[j].second.count();
    });

    return pq_idx[0];
}

void CDCLCASampler::GenerateTestcase()
{
    GenerateCandidateTestcaseSet();
    selected_candidate_index_ = SelectTestcaseFromCandidateSetByTupleNum();

    int testcase_index = num_generated_testcase_;
    testcase_set_.emplace_back(pq[selected_candidate_index_].first);
    selected_candidate_bitset_ = pq[selected_candidate_index_].second;
}

void CDCLCASampler::GenerateCoveringArray()
{
    auto start_time = chrono::system_clock::now().time_since_epoch();
    Init();

    for (num_generated_testcase_ = 1; ; num_generated_testcase_++)
    {
        (this->*p_update_tuple_info_)();
        if (use_upperlimit && upperlimit == num_tuple_){
            break;
        }
        if (num_generated_testcase_ > 1) {
            clear_pq();
        }
        cout << num_generated_testcase_ << ": " << num_tuple_ << endl;
        (this->*p_update_sample_weight_)();
        GenerateTestcase();
        if (!selected_candidate_bitset_.count()){
            testcase_set_.pop_back();
            break;
        }
    }
    
    clear_final();

    auto end_time = chrono::system_clock::now().time_since_epoch();
    cpu_time_ = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    cout << "c Generate testcase set finished, containing " << testcase_set_.size() << " testcases!" << endl;
    cout << "c CPU time cost by generating testcase set: " << cpu_time_ << " seconds" << endl;
    SaveTestcaseSet(testcase_set_save_path_);

    cout << "c " << t_wise_ << "-tuple number of generated testcase set: " << num_tuple_ << endl;

    remove_temp_files();
}

void CDCLCASampler::SaveTestcaseSet(string result_path)
{
    ofstream res_file(result_path);
    for (const vector<int>& testcase: testcase_set_)
    {
        for (int v = 0; v < num_var_; v++)
        {
            res_file << testcase[v] << " ";
        }
        res_file << endl;
    }
    res_file.close();
    cout << "c Testcase set saved in " << result_path << endl;
}

vector<pair<int, int> > CDCLCASampler::get_prob_in(bool init){
    vector<pair<int, int> > res;
    res.reserve(num_var_);
    if (!init && context_aware_method_ != 0){
        for (int i = 0; i < num_var_; ++i){
            int v1 = context_aware_method_ == 1 ? 0: (num_generated_testcase_ - var_positive_appearance_count_[i]);
            int v2 = context_aware_method_ == 1 ? 0: var_positive_appearance_count_[i];
            res.emplace_back(v1, v2);
        }
    }
    return res;
}

SimpleBitSet CDCLCASampler::get_gain(const vector<int>& asgn){
    SimpleBitSet res(num_tuple_all_possible_);
    
    // 懒得再搞一堆函数指针了
    if (t_wise_ == 2){
        for (int i = 0; i < num_var_ - 1; i++){
            for (int j = i + 1; j < num_var_; j++){
                long long index_tuple = Get2TupleMapIndex(i, asgn[i], j, asgn[j]);
                if (!already_t_wise.get(index_tuple)){
                    res.set(index_tuple);
                }
            }
        }
    } else if (t_wise_ == 3){
        for (int i1 = 0; i1 < num_var_; i1++){
            for (int i2 = 0; i2 < num_var_; i2++){
                for (int i3 = 0; i3 < num_var_; i3++){
                    long long index_tuple = Get3TupleMapIndex(i1, asgn[i1], i2, asgn[i2], i3, asgn[i3]);
                    if (!already_t_wise.get(index_tuple)){
                        res.set(index_tuple);
                    }
                }
            }
        }
    } else if (t_wise_ == 4){
        for (int i1 = 0; i1 < num_var_; i1++){
            for (int i2 = 0; i2 < num_var_; i2++){
                for (int i3 = 0; i3 < num_var_; i3++){
                    for (int i4 = 0; i4 < num_var_; i4++){
                        long long index_tuple = Get4TupleMapIndex(i1, asgn[i1], i2, asgn[i2], i3, asgn[i3], i4, asgn[i4]);
                        if (!already_t_wise.get(index_tuple)){
                            res.set(index_tuple);
                        }
                    }
                }
            }
        }
    } else if (t_wise_ == 5){
        for (int i1 = 0; i1 < num_var_; i1++){
            for (int i2 = 0; i2 < num_var_; i2++){
                for (int i3 = 0; i3 < num_var_; i3++){
                    for (int i4 = 0; i4 < num_var_; i4++){
                        for (int i5 = 0; i5 < num_var_; i5++){
                            long long index_tuple = Get5TupleMapIndex(i1, asgn[i1], i2, asgn[i2], i3, asgn[i3], i4, asgn[i4], i5, asgn[i5]);
                            if (!already_t_wise.get(index_tuple)){
                                res.set(index_tuple);
                            }
                        }
                    }
                }
            }
        }
    } else if (t_wise_ == 6){
        for (int i1 = 0; i1 < num_var_; i1++){
            for (int i2 = 0; i2 < num_var_; i2++){
                for (int i3 = 0; i3 < num_var_; i3++){
                    for (int i4 = 0; i4 < num_var_; i4++){
                        for (int i5 = 0; i5 < num_var_; i5++){
                            for (int i6 = 0; i6 < num_var_; i6++){
                                long long index_tuple = Get6TupleMapIndex(i1, asgn[i1], i2, asgn[i2], i3, asgn[i3], i4, asgn[i4], i5, asgn[i5], i6, asgn[i6]);
                                if (!already_t_wise.get(index_tuple)){
                                    res.set(index_tuple);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return res;
}

void CDCLCASampler::remove_temp_files(){
    string cmd;
    int ret;
    if (flag_reduced_cnf_as_temp_ && flag_use_cnf_reduction_){
        cmd = "rm " + reduced_cnf_file_path_;
        ret = system(cmd.c_str());
    }
}

string CDCLCASampler::get_random_prefix()
{
    return cnf_instance_name_ + to_string(getpid()) + to_string(rnd_file_id_gen_());
}

void CDCLCASampler::clear_pq()
{
    for (auto& bs: pq){
        bs.second.difference_(selected_candidate_bitset_);
    }

    sort(pq.begin(), pq.end(), [](const pair<vector<int>, SimpleBitSet>& si, const pair<vector<int>, SimpleBitSet>& sj){
        return si.second.count() > sj.second.count();
    });

    int cur_pqsize = (int) pq.size();
    while (cur_pqsize > candidate_set_size_ || (cur_pqsize > 0 && pq.back().second.count() == 0)){
        pq.pop_back();
        pq_idx.pop_back();
        --cur_pqsize;
    }
}

// 这个应该没那么耗时，写成关于 twise 递归的过程？

long long CDCLCASampler::GetXTupleMapIndex(const vector<pair<int, int> >& vec){
    long long index_tuple = -1;
    // TODO 可以把 GetXTupleMapIndex 改为一个 multiplex，按需取用参数
    if (t_wise_ == 2){
        index_tuple = Get2TupleMapIndex(vec[0].first, vec[0].second,
                                        vec[1].first, vec[1].second);
    } else if (t_wise_ == 3){
        index_tuple = Get3TupleMapIndex(vec[0].first, vec[0].second,
                                        vec[1].first, vec[1].second,
                                        vec[2].first, vec[2].second);
    } else if (t_wise_ == 4){
        index_tuple = Get4TupleMapIndex(vec[0].first, vec[0].second,
                                        vec[1].first, vec[1].second,
                                        vec[2].first, vec[2].second,
                                        vec[3].first, vec[3].second);
    } else if (t_wise_ == 5){
        index_tuple = Get5TupleMapIndex(vec[0].first, vec[0].second,
                                        vec[1].first, vec[1].second,
                                        vec[2].first, vec[2].second,
                                        vec[3].first, vec[3].second,
                                        vec[4].first, vec[4].second);
    } else if (t_wise_ == 6){
        index_tuple = Get6TupleMapIndex(vec[0].first, vec[0].second,
                                        vec[1].first, vec[1].second,
                                        vec[2].first, vec[2].second,
                                        vec[3].first, vec[3].second,
                                        vec[4].first, vec[4].second,
                                        vec[5].first, vec[5].second);
    } 
    return index_tuple;
}

void CDCLCASampler::find_uncovered_tuples_pre(int cur, bool simplify){
    if (cur == t_wise_){
        long long index_tuple = GetXTupleMapIndex(uncovered_tuple_tmp);
        if (!already_t_wise.get(index_tuple)){
            bool flag = true;
            if (simplify){
                for (int i = 0; i < t_wise_; ++i){
                    cdcl_solver->add_assumption(uncovered_tuple_tmp[i].first, uncovered_tuple_tmp[i].second);
                }
                bool res = cdcl_solver->solve();
                if (!res){
                    --num_tuple_all_exact_;
                    flag = false;
                } else {
                    vector<int> tc(num_var_, 0);
                    get_cadical_solution(tc);
                    uncovered_possible_solutions.emplace_back(tc);
                }
                cdcl_solver->clear_assumptions();
            }
            if (flag){
                vector<int> compressed_uncovered_tuple;
                for (auto& pa: uncovered_tuple_tmp){
                    // compressed_uncovered_tuple.push_back(make_pair(pa.first, pa.second == 1));
                    compressed_uncovered_tuple.push_back(pa.second ? (pa.first + 1): (-pa.first - 1));
                }
                uncovered_tuples.emplace_back(compressed_uncovered_tuple);
            }
        }
        
        return ;
    }

    // cur 和 tuple 下标: [0, t_wise)
    int lst = -1; 
    if (cur > 0) lst = uncovered_tuple_tmp[cur - 1].first;
    int ubd = num_var_ - t_wise_ + cur;
    for (int i = lst + 1; i <= ubd; ++i){
        for (int v = 0; v < 2; ++v){
            if (as_backbone[i] != -1 && v != as_backbone[i]){
                // 清除
                int pp = t_wise_ - cur - 1;
                int numpp = num_var_ - i - 1;
                long long res = 0;
                if (pp == 0) res = 1;
                else if (pp == 1) res = numpp;
                else res = combnum[pp][numpp];
                num_tuple_all_exact_ -= res * (1ll << pp);
                continue;
            }
            uncovered_tuple_tmp[cur].first = i;
            uncovered_tuple_tmp[cur].second = v;
            find_uncovered_tuples_pre(cur + 1, simplify);
        }
    }
}

/*
void CDCLCASampler::find_uncovered_tuples(bool simplify){
    for (int i = 0; i < num_var_ - 1; ++i){
        for (int v_i = 0; v_i < 2; ++v_i){
            if (as_backbone[i] != -1 && v_i != as_backbone[i]){
                num_tuple_all_exact_ -= 2ll * (num_var_ - i - 1);
                continue;
            }
            for (int j = i + 1; j < num_var_; ++j){
                for (int v_j = 0; v_j < 2; ++v_j){
                    if (as_backbone[j] != -1 && v_j != as_backbone[j]){
                        --num_tuple_all_exact_;
                        continue;
                    }
                    long long index_tuple = Get2TupleMapIndex(i, v_i, j, v_j);
                    if (!already_t_wise.get(index_tuple)){
                        bool flag = true;
                        if (simplify){
                            cdcl_solver->add_assumption(i, v_i);
                            cdcl_solver->add_assumption(j, v_j);
                            bool res = cdcl_solver->solve();
                            if (!res){
                                --num_tuple_all_exact_;
                                flag = false;
                            } else {
                                vector<int> tc(num_var_, 0);
                                get_cadical_solution(tc);
                                uncovered_possible_solutions.emplace_back(tc);
                            }
                            cdcl_solver->clear_assumptions();
                        }
                        if (flag){
                            uncovered_tuples.emplace_back(vector<int>{v_i ? (i+1):(-i-1), v_j ? (j+1):(-j-1)});
                        }
                    }
                }
            }
        }
    }
}
*/

void CDCLCASampler::get_cadical_solution(vector<int>& tc){
    cdcl_solver->get_solution(tc);
}

void CDCLCASampler::choose_final_plain()
{
    find_uncovered_tuples_pre(0, true);

    size_t uncovered_size = uncovered_tuples.size();
    size_t ftc_size = uncovered_possible_solutions.size(), i = 0;
    while (i < ftc_size && uncovered_size > 0){
        for (int j = 0; j < t_wise_; ++j){
            bool s = uncovered_tuples[i][j] < 0;
            if (s){
                uncovered_tuple_tmp[j].first = -uncovered_tuples[i][j] - 1;
                uncovered_tuple_tmp[j].second = 0;
            } else {
                uncovered_tuple_tmp[j].first = uncovered_tuples[i][j] - 1;
                uncovered_tuple_tmp[j].second = 1;
            }
        }
        long long tuple_index = GetXTupleMapIndex(uncovered_tuple_tmp);
        if (already_t_wise.get(tuple_index)){
            ++i;
            continue;
        }

        testcase_set_.emplace_back(uncovered_possible_solutions[i]);
        ++num_generated_testcase_;
        (this->*p_update_tuple_info_)();
        cout << num_generated_testcase_ << ": " << num_tuple_ << endl;
        ++i;
    }
}

void CDCLCASampler::clear_final()
{
    cout << endl << "c Clear final: fix-up all remaining tuples ..." << endl;

    num_tuple_all_exact_ = num_tuple_all_possible_;
    cdcl_solver = new CDCLSolver::Solver;
    cdcl_solver->read_clauses(num_var_, clauses);

    (this->*p_choose_final)();

    delete cdcl_solver;

    coverage_tuple_ = double(num_tuple_) / num_tuple_all_exact_;

    cout << "c All possible 2-tuple number: " << num_tuple_all_exact_ << endl;
    cout << "c 2-tuple coverage: " << coverage_tuple_ << endl;
}

void CDCLCASampler::read_cnf_header(ifstream& ifs, int& nvar, int& nclauses){
    string line;
    istringstream iss;
    int this_num_var;
    
    while (getline(ifs, line)){
        if (line.substr(0, 1) == "c")
            continue;
        else if (line.substr(0, 1) == "p"){
            string tempstr1, tempstr2;
            iss.clear();
            iss.str(line);
            iss.seekg(0, ios::beg);
            iss >> tempstr1 >> tempstr2 >> this_num_var >> nclauses;
            if (nvar < this_num_var)
                nvar = this_num_var;
            break;
        }
    }
}

bool CDCLCASampler::check_no_clauses(){
    ifstream fin(cnf_file_path_.c_str());
    if (!fin.is_open()) return true;

    int num_clauses_original;
    read_cnf_header(fin, num_var_, num_clauses_original);

    fin.close();
    return num_clauses_original <= 0;
}

bool CDCLCASampler::read_cnf(){
    ifstream fin(cnf_file_path_.c_str());
    if (!fin.is_open()) return false;

    read_cnf_header(fin, num_var_, num_clauses_);

    if (num_var_ <= 0 || num_clauses_ < 0){
        fin.close();
        return false;
    }

    pos_in_cls.resize(num_var_ + 1, vector<int>());
    neg_in_cls.resize(num_var_ + 1, vector<int>());
    clauses.resize(num_clauses_);
    as_backbone.resize(num_var_, -1);

    for (int c = 0; c < num_clauses_; ++c)
    {
        int cur_lit;
        fin >> cur_lit;
        while (cur_lit != 0)
        {
            int v = abs(cur_lit);
            if (cur_lit > 0) pos_in_cls[v].push_back(c);
            else neg_in_cls[v].push_back(c);
            clauses[c].emplace_back(cur_lit);
            fin >> cur_lit;
        }

        if ((int) clauses[c].size() == 1){
            int bv = abs(clauses[c][0]) - 1;
            as_backbone[bv] = clauses[c][0] > 0 ? 1: 0;
        }
    }

    fin.close();

    return true;
}

void CDCLCASampler::SetUpperLimit(std::string answer_file_path){
    FILE *in = fopen(answer_file_path.c_str(), "r");
    int ret = fscanf(in, "%lld", &upperlimit);
    use_upperlimit = true;
    fclose(in);
}
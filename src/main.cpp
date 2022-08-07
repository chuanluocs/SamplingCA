#include <signal.h>
#include "cdclcasampler.h"

using namespace std;

void HandleInterrupt(int sig) 
{
	cout << "c" << endl;
	cout << "c caught signal... exiting" << endl;
	exit(-1);
}

void SetupSignalHandler() 
{
	signal(SIGTERM, HandleInterrupt);
	signal(SIGINT, HandleInterrupt);
	signal(SIGQUIT, HandleInterrupt);
	signal(SIGKILL, HandleInterrupt);
}

double ComputeCPUTime(clock_t start, clock_t stop)
{
	double run_time = (double)(stop - start) / CLOCKS_PER_SEC;
	return run_time;
}

struct Argument
{
	string input_cnf_path;
	string reduced_cnf_file_path;
	string output_testcase_path;
	string answer_file_path;
	int seed;
	int t_wise;
	int candidate_set_size;
	int use_weighted_sampling;
	int use_context_aware;
	int use_cnf_reduction;
	
	bool use_pure_cdcl;
	bool use_pure_sls;
	int final_strategy;

	bool flag_no_shuffle_var;

	bool flag_input_cnf_path;
	bool flag_reduced_cnf_file_path;
	bool flag_output_testcase_path;
	bool flag_t_wise;
	bool flag_candidate_set_size;
	bool flag_use_weighted_sampling;
	bool flag_use_context_aware;
	bool flag_use_cnf_reduction;
	bool flag_final_strategy;
	bool flag_answer_file_path;
	bool flag_use_VSIDS;
};

bool ParseArgument(int argc, char **argv, Argument &argu)
{	
	argu.seed = 1;
	argu.flag_input_cnf_path = false;
	argu.flag_reduced_cnf_file_path = false;
	argu.flag_output_testcase_path = false;
	argu.flag_t_wise = false;
	argu.flag_candidate_set_size = false;
	argu.flag_use_weighted_sampling = false;
	argu.flag_use_context_aware = false;
	argu.flag_use_cnf_reduction = false;

	argu.flag_answer_file_path = false;
	argu.flag_final_strategy = false;
	argu.use_pure_cdcl = false;
	argu.use_pure_sls = false;
	argu.flag_no_shuffle_var = false;
	argu.flag_use_VSIDS = false;

	if (argc < 2)
	{
		return false;
	}

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-input_cnf_path") == 0)
		{
			i++;
			if(i>=argc) return false;
			argu.input_cnf_path = argv[i];
			argu.flag_input_cnf_path = true;
			continue;
		}
		else if (strcmp(argv[i], "-output_testcase_path") == 0)
		{
			i++;
			if(i>=argc) return false;
			argu.output_testcase_path = argv[i];
			argu.flag_output_testcase_path = true;
			continue;
		}
		else if (strcmp(argv[i], "-reduced_cnf_path") == 0)
		{
			i++;
			if(i>=argc) return false;
			argu.reduced_cnf_file_path = argv[i];
			argu.flag_reduced_cnf_file_path = true;
			continue;
		}
		else if (strcmp(argv[i], "-sample_until_file") == 0)
		{
			i++;
			if(i>=argc) return false;
			argu.answer_file_path = argv[i];
			argu.flag_answer_file_path = true;
			continue;
		}
		else if(strcmp(argv[i], "-seed") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.seed);
			continue;
		}
		else if(strcmp(argv[i], "-strategy") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.final_strategy);
			argu.flag_final_strategy = true;
			continue;
		}
		else if(strcmp(argv[i], "-t_wise") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.t_wise);
			argu.flag_t_wise = true;
			continue;
		}
		else if(strcmp(argv[i], "-k") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.candidate_set_size);
			argu.flag_candidate_set_size = true;
			continue;
		}
		else if(strcmp(argv[i], "-use_dynamic_updating_sampling_prob") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.use_weighted_sampling);
			argu.flag_use_weighted_sampling = true;
			continue;
		}
		else if(strcmp(argv[i], "-use_diversity_aware_heuristic_search") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.use_context_aware);
			++argu.use_context_aware;
			argu.flag_use_context_aware = true;
			continue;
		}
		else if(strcmp(argv[i], "-use_formula_simplification") == 0)
		{
			i++;
			if(i>=argc) return false;
			sscanf(argv[i], "%d", &argu.use_cnf_reduction);
			argu.flag_use_cnf_reduction = true;
			continue;
		}
		else if(strcmp(argv[i], "-use_VSIDS") == 0)
		{
			argu.flag_use_VSIDS = true;
			continue;
		}
		else if(strcmp(argv[i], "-pure_cdcl") == 0)
		{
			argu.use_pure_cdcl = true;
			continue;
		}
		else if(strcmp(argv[i], "-pure_sls") == 0)
		{
			argu.use_pure_sls = true;
			continue;
		}
		else if(strcmp(argv[i], "-no_shuffle_var") == 0)
		{
			argu.flag_no_shuffle_var = true;
			continue;
		}
		else
		{
			return false;
		}
	}

	int pos = argu.input_cnf_path.find_last_of( '/' );
    string cnf_file_name = argu.input_cnf_path.substr(pos + 1);
	cnf_file_name.replace(cnf_file_name.find(".cnf"), 4, "");

	if(argu.flag_input_cnf_path) return true;
	else return false;
}

int main(int argc, char **argv)
{
	SetupSignalHandler();

	Argument argu;

	if (!ParseArgument(argc, argv, argu))
	{
		cout << "c Argument Error!" << endl;
		return -1;
	}

	CDCLCASampler ca_sampler(argu.input_cnf_path, argu.seed);
	if (argu.flag_t_wise)
		ca_sampler.SetTWise(argu.t_wise);
	if (argu.flag_candidate_set_size)
		ca_sampler.SetCandidateSetSize(argu.candidate_set_size);
	if (argu.flag_use_weighted_sampling)
		ca_sampler.SetWeightedSamplingMethod(argu.use_weighted_sampling);
	if (argu.flag_use_context_aware)
		ca_sampler.SetContextAwareMethod(argu.use_context_aware);
	if (argu.flag_use_cnf_reduction)
		ca_sampler.SetCNFReductionMethod(argu.use_cnf_reduction);
	if (argu.flag_reduced_cnf_file_path)
		ca_sampler.SetReducedCNFPath(argu.reduced_cnf_file_path);
	if (argu.flag_answer_file_path)
		ca_sampler.SetUpperLimit(argu.answer_file_path);
	if (argu.flag_output_testcase_path)
		ca_sampler.SetTestcaseSetSavePath(argu.output_testcase_path);
	if (argu.use_pure_cdcl)
		ca_sampler.SetPureCDCL();
	if (argu.use_pure_sls)
		ca_sampler.SetPureSLS();
	if (argu.flag_no_shuffle_var)
		ca_sampler.DisableVarShuffling();
	if (argu.flag_final_strategy)
		ca_sampler.SetFinalStrategy(argu.final_strategy);
	if (argu.flag_use_VSIDS)
		ca_sampler.EnableVSIDSForCDCL();

	ca_sampler.GenerateCoveringArray();

	return 0;
}

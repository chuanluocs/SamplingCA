# *SamplingCA*: Effective and Efficient Sampling-based Pairwise Testing for Highly Configurable Software Systems

*SamplingCA* is an effective and efficient sampling-based approach for pairwise testing of highly configurable software systems. This repository includes the implementation of *SamplingCA*, the testing instances adopted in the experiments and the experimental results. 

## Purpose of *SamplingCA*

Combinatorial interaction testing (CIT) is a popular testing methodology for testing interactions of options of highly configurable systems. In the context of CIT, covering arrays are the test suites that can cover all such interactions, possibly under certain constraints. Particularly, pairwise covering arrays (PCAs) are widely employed, since they can achieve a good balance between testing costs and fault detection capability. 

*SamplingCA* is a state-of-the-art algorithm for generating small-sized PCAs efficiently. In our implementation of *SamplingCA*, the input is a system under test (SUT), modeled as a Boolean formula in CNF in [Dimacs format](http://www.satcompetition.org/2011/format-benchmarks2011.html). It outputs a PCA of the given SUT, where each line represents a valid configuration. 

## How to Obtain *SamplingCA*

*SamplingCA* is [publicly available on Github](https://github.com/chuanluocs/SamplingCA). To obtain *SamplingCA*, a user may use `git clone` to get a local copy of the Github repository:

```
git clone --recursive https://github.com/chuanluocs/SamplingCA.git
```

Note that *SamplingCA* is based on two git submodules: [minisat](https://github.com/niklasso/minisat) and [riss-solver](https://github.com/nmanthey/riss-solver). Therefore, we recommend users call `git clone` with the `--recursive` option in order to initialize the submodules properly, as the command above demonstrates. 

## Instructions for Building *SamplingCA*

See [INSTALL.md](INSTALL.md). 

## Instructions for Running *SamplingCA*

After building *SamplingCA*, users may run it with the following command: 

```
./SamplingCA -input_cnf_path [INSTANCE_PATH] <optional_parameters> <optional_flags>
```

`-input_cnf_path` is the only required parameter. As mentioned above, the input file for *SamplingCA* must be in [Dimacs format](http://www.satcompetition.org/2011/format-benchmarks2011.html). The directory named `cnf_instances/` contains all 125 testing instances, which can be taken as input. Users may also use [FeatureIDE](https://github.com/FeatureIDE/FeatureIDE/) to generate input files from existing configurable systems. 

For the optional parameters, we list them as follows. 

| Parameter | Allowed Values | Default Value | Description | 
| - | - | - | - |
| `-output_testcase_path` | string | `./[INPUT_CNF_NAME]_testcase_set.txt` | path to which the generated PCA is saved |
| `-seed` | integer | 1 | random seed | 
| `-k` | positive integer | 100 | the number of candidates per round | 
| `-use_formula_simplification` | 0 or 1 | 1 | 1 if the input will be simplified with `bin/coprocessor`, 0 otherwise |
| `-use_diversity_aware_heuristic_search` | 0 or 1 | 1 | 1 if the SAT solving algorithm will be context-aware, 0 otherwise |

For the optional flags, we list them as follows. 

| Flag | Description | 
| - | - |
| `-pure_sls` | if set, the sampling phase will be replaced with LS-Sampling |
| `-no_shuffle_var` | if set, the variable order randomization strategy will be disabled |

Note that when running *SamplingCA*, the last lines of the console log will report the following information:
1. the size of the generated PCA; 
2. the total number of valid pairwise tuples of the given SUT; 
3. the total running time

The output PCA consists of multiple lines with each line representing a valid configuration. For example, for a SUT with 3 options **x**, **y** and **z** in order, a line `0 1 0` means that **x=0, y=1, z=0** is a valid configuration. 

**Tip:** When the parameter `-use_formula_simplification` is set to 1, *SamplingCA* will call `bin/coprocessor` to simplify the input during the execution. However, since `bin/coprocessor` is a precompiled binary file, it may not work on machines with specific hardware or operating systems. In this case, to reproduce the experiments precisely, we recommend users use the instances in `simplified_instances/`, which are the simplified counterparts of those in `cnf_instances/`. Alternatively, users can also build `coprocessor` from source by following the instructions in the submodule [riss-solver](https://github.com/nmanthey/riss-solver). 

## Example Command for Running *SamplingCA*

```
./SamplingCA -input_cnf_path cnf_instances/XSEngine.cnf -seed 1 -k 100
```

The command above calls *SamplingCA* to solve the instance `cnf_instances/XSEngine.cnf` by setting the random seed to 1 and the number of candidate test cases per iteration round of the sampling phase to 100. The generated PCA will be saved in `./XSEngine.cnf_testcase_set.txt`. 

For a simple case study, users may check the `examples/` directory. 

## Commands for Reproducing the Experimental Results

### Experiments for RQ1 and RQ2

```bash
for instance in ` ls cnf_instances `
do
    ./SamplingCA -input_cnf_path cnf_instances/${instance} -output_testcase_path ${instance}_testcase_set.txt
done
```

### Experiments for RQ3

```bash
for instance in ` ls cnf_instances `
do
    # Alt-1
    ./SamplingCA -input_cnf_path cnf_instances/${instance} -output_testcase_path ${instance}_testcase_set_alt1.txt -pure_sls
    # Alt-2
    ./SamplingCA -input_cnf_path cnf_instances/${instance} -output_testcase_path ${instance}_testcase_set_alt2.txt -use_diversity_aware_heuristic_search 0
    # Alt-3
    ./SamplingCA -input_cnf_path cnf_instances/${instance} -output_testcase_path ${instance}_testcase_set_alt3.txt -no_shuffle_var
done
```

### Experiments for RQ4

```bash
for instance in ` ls cnf_instances `
do
    for k in 10 50 100 500 1000
    do
        ./SamplingCA -input_cnf_path cnf_instances/${instance} -output_testcase_path ${instance}_testcase_set_k_${k}.txt -k ${k}
    done
done
```

## Implementation of *SamplingCA*

The implementation of *SamplingCA* includes two parts:
- The sub-directory `minisat_ext` includes the implementation of the *ContextSAT* algorithm proposed in our paper, based on `minisat`. 
- The sub-directory `src` includes the implementation of the other parts of *SamplingCA*. 

## Testing Instances for Evaluating *SamplingCA*

The directory named `cnf_instances/` contains all 125 testing instances.

## Experimental Results

The directory `experimental_results` contains three `.csv` files for presenting the experimental results. 
- `Results_of_SamplingCA_and_its_SOTA_competitors.csv`: Results of *SamplingCA* and its state-of-the-art competitors on all testing instances. 
- `Results_of_SamplingCA_and_its_alternative_versions.csv`: Results of *SamplingCA* and its alternative versions on all testing instances.
- `Results_of_SamplingCA_with_different_hyper-parameter_settings.csv`: Results of *SamplingCA* with different hyper-parameter settings on all testing instances.

## Main Developers

- Chuan Luo (<chuanluophd@outlook.com>)
- Qiyuan Zhao (<zqy1018@hotmail.com>)

## Related Paper

Chuan Luo, Qiyuan Zhao, Shaowei Cai, Hongyu Zhang, Chunming Hu. *SamplingCA: Effective and Efficient Sampling-based Pairwise Testing for Highly Configurable Software Systems.* To appear in Proceedings of ESEC/FSE 2022. 

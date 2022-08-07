# Instructions for Building *SamplingCA*

**Note:** There are some requirements of *SamplingCA* listed in [REQUIREMENTS.md](REQUIREMENTS.md). We recommend users check them before building and running *SamplingCA*. 

*SamplingCA* can be built by simply calling `make` at the root directory of *SamplingCA*. For example, after cloning *SamplingCA* into the directory `SamplingCA`, a user can then call the following commands: 

```
cd SamplingCA/
make
```

**Tip:** From our experiences, there might be compatability issues related to [minisat](https://github.com/niklasso/minisat) when using specific versions of `g++` or operating systems, which will incur errors during the building process. In that case, users may refer to the [Issues of minisat on Github](https://github.com/niklasso/minisat/issues) or contact us for help. 

# Quick Installation Testing

```
./SamplingCA -input_cnf_path cnf_instances/axtls.cnf -use_formula_simplification 0
```

By running the command above, *SamplingCA* should generate a PCA of the given input `cnf_instances/axtls.cnf`, saved in `./axtls_testcase_set.txt`. The user should find the following information in the console output: 
- Testcase set saved in `./axtls_testcase_set.txt`
- 2-tuple number of generated testcase set: 16212

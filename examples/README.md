# Example

**Note**: Before reading this file, we suggest users go through the Section 2 of our paper to get familiar with the notations and definitions in that section (*e.g.*, validity of test cases and Boolean formulae). 

This file illustrates a simple case study for *SamplingCA*. 

Consider a simple configurable system with 4 options: **a**, **b**, **c**, **d**. Assume that all of them can only take 0 or 1. Then we can use a quadruple in the form of **(a=0,b=1,c=0,d=1)** to represent a configuration, assigning **a**, **b**, **c**, **d** with 0, 1, 0, 1 respectively. Similarly, we use a tuple in the form of **(a=0,b=1)** to represent a pairwise tuple **{(a,0), (b,1)}**. 

Besides, assume that there are 4 constraints over these options:
1. **a=0** or **b=1**
2. **b=0** or **c=0** or **d=1**
3. **a=1** or **c=1**
4. **c=1** or **d=0**

As stated in the paper, in order to guarantee the efficiency and accuracy of testing, each test case in a test suite should satisfy all constraints. And by the definition of valid pairwise tuples, there are totally 20 valid pairwise tuples:
- **(a=0,b=0)**, **(a=0,b=1)**, **(a=0,c=1)**, **(a=0,d=0)**
- **(a=0,d=1)**, **(a=1,b=1)**, **(a=1,c=0)**, **(a=1,c=1)**
- **(a=1,d=0)**, **(a=1,d=1)**, **(b=0,c=1)**, **(b=0,d=0)**
- **(b=0,d=1)**, **(b=1,c=0)**, **(b=1,c=1)**, **(b=1,d=0)**
- **(b=1,d=1)**, **(c=0,d=0)**, **(c=1,d=0)**, **(c=1,d=1)**

We can transform this configurable system into a Boolean formula in CNF **F**, where:
1. There are 4 Boolean variables in **V(F)**, namely **xa**, **xb**, **xc**, **xd**, corresponding to the options **a**, **b**, **c**, **d** respectively. 
2. **F=(-xa or xb) and (-xb or -xc or xd) and (xa or xc) and (xc or -xd)**, where **-x** represents the negation of the variable **x**, **and** represents the conjunction and **or** represents the disjunction. Each clause in **C(F)** corresponds to a constraint of the system. 

After the transformation, we store this Boolean formula **F** in `examples/simple.cnf` in [Dimacs format](http://www.satcompetition.org/2011/format-benchmarks2011.html), so that *SamplingCA* can take it as input. To generate a pairwise covering array (PCA) for the simple configurable system, in the directory where *SamplingCA* resides and `examples/` is a sub-directory, call the following command: 
```
./SamplingCA -input_cnf_path examples/simple.cnf 
```

*SamplingCA* will finish execution quickly and output the PCA in `simple_testcase_set.txt`. The output may look like this: 
```
1 1 0 0 
0 0 1 1 
1 1 1 1 
0 0 1 0 
0 1 1 1 
```

The output can be interpreted as follows. It represents a PCA with the following 5 test cases:
- **(a=1,b=1,c=0,d=0)**
- **(a=0,b=0,c=1,d=1)**
- **(a=1,b=1,c=1,d=1)**
- **(a=0,b=0,c=1,d=0)**
- **(a=0,b=1,c=1,d=1)**

Note that although the output may be different among machines, it is guaranteed to be a PCA of our simple configurable system. That is, the output is guaranteed to cover all the 20 valid pairwise tuples. 

To summarize, by knowing the information of our simple configurable system and transforming it into a Boolean formula in CNF stored in a `.cnf` file, we can run *SamplingCA* to construct its PCA. For users' own case studies, the procedure is similar. 

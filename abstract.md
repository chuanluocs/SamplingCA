# Artifact for *SamplingCA: Effective and Efficient Sampling-based Pairwise Testing for Highly Configurable Software Systems*

*SamplingCA* is an effective and efficient sampling-based approach for pairwise testing of highly configurable software systems. This file serves as a brief description of our artifact, including our implementation of *SamplingCA* (henceforth referred to as *SamplingCA*), the testing instances adopted in the experiments and the experimental results.

## Purpose of *SamplingCA*

Combinatorial interaction testing (CIT) is a popular testing methodology for testing interactions of options of highly configurable systems. In the context of CIT, covering arrays are the test suites that can cover all such interactions, possibly under certain constraints. Particularly, pairwise covering arrays (PCAs) are widely employed, since they can achieve a good balance between testing costs and fault detection capability. 

*SamplingCA* is a state-of-the-art algorithm for generating small-sized PCAs efficiently. In our implementation of *SamplingCA*, the input is a system under test (SUT), modeled as a Boolean formula in CNF in [Dimacs format](http://www.satcompetition.org/2011/format-benchmarks2011.html). It outputs a PCA of the given SUT, where each line represents a valid configuration. 

## Requirements

Software/Hardware requirements of *SamplingCA* are listed in [REQUIREMENTS.md](REQUIREMENTS.md). 

## Get Started

We present the instructions for obtaining, building and running *SamplingCA* in [README.md](README.md) and [INSTALL.md](INSTALL.md). 

## About Badges

We apply for the following badges: **Available**, **Functional** and **Reusable**. We present our reasons for applying them in [STATUS.md](STATUS.md). 

## License

*SamplingCA* uses the GPL-3.0 license. Check [LICENSE.md](LICENSE.md) for more information. 

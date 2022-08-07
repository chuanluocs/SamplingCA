# Requirements

## Requirements for Hardware

We recommend users run *SamplingCA* on a machine with more than 8GB of RAM. 
Our empirical study shows that running *SamplingCA* with `cnf_instances/uClinux-config.cnf` as input and `k` set to 100 costs approximately 8GB of memory. 

## Requirements for Software

We recommend users build and run *SamplingCA* on a 64-bit GNU/Linux operating system. We have tested *SamplingCA* on a x86-64 Ubuntu 18.04. 

Besides, the following software packages should be installed before building *SamplingCA*:
- `make` with version >= 4.1
- `g++` with version >= 7.5.0

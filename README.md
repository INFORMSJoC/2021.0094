[![INFORMS Journal on Computing Logo](https://INFORMSJoC.github.io/logos/INFORMS_Journal_on_Computing_Header.jpg)](https://pubsonline.informs.org/journal/ijoc)

# Integrated Backup Rolling Stock Allocation and Timetable Rescheduling with Uncertain Time-Variant Passenger Demand Under Disruptive Events

This archive is distributed in association with the [INFORMS Journal on
Computing](https://pubsonline.informs.org/journal/ijoc) under the [MIT License](LICENSE).

The software and data in this repository are a snapshot of the software and data
that were used in the research reported on in the paper "Integrated Backup Rolling Stock Allocation and Timetable Rescheduling with Uncertain Time-Variant Passenger Demand Under Disruptive Events" by J. Yin, L. Yang, D. Andrea, T. Tang and Z. Gao.
The snapshot is based on 
[this SHA](https://github.com/tkralphs/JoCTemplate/commit/f7f30c63adbcb0811e5a133e1def696b74f3ba15) 
in the development repository. 

**Important: This code is being developed on an on-going basis at https://github.com/JerryYINJIATENG/TARP. Please go there if you would like to
get a more recent version or would like support**

## Cite

To cite this software, please cite the [paper](https://doi.org/10.1287/ijoc.2019.0934) using its DOI and the software itself, using the following DOI.

[![DOI](https://zenodo.org/badge/285853815.svg)](https://zenodo.org/badge/latestdoi/285853815)

Below is the BibTex for citing this version of the code.

```
@article{BARP,
  author =        {Jiateng Yin, Lixing Yang, D'Ariano Andrea, Tao Tang, Ziyou Gao},
  publisher =     {INFORMS Journal on Computing},
  title =         {{Integrated Backup Rolling Stock Allocation and Timetable Rescheduling with Uncertain Time-Variant Passenger Demand Under Disruptive Events} Version v1.0},
  year =          {2022},
  doi =           {10.5281/zenodo.3977566},
  url =           {https://github.com/INFORMSJoC/2021.0094},
}  
```

## Description

This repository includes the computational results, source codes and source data (with no conflict of interest) for the experiments presented in the paper. 

## Requirements
For these experiments, the following requirments should be satisfied
* C++ run on Windows 10 (with SDK higher than 10.0.150630.0)
* CPLEX 12.80 Acamidic version
* at least 8 GB RAM.

## Results
The results for each instance, involving the fleet size of rolling stocks, ATT (unit: second), value of objective function, CPU time and optamality gap, are presented in [results](results) by "NO BRS", "Randomly solution", "LSM" and "ILM".

## Replicating

To replicate the results in [results](results), please follow the instructions below.

### Contents of this repository

#### data files

#### code files

### Steps to implement the code
* Adjust the path of libraries ilcplex.h and ilocplex.h in the project as the default path in your PC
* Add linkers and CPLEX libraries to the correct position
* Change the path of input data, involving the number of arriving passengers and alighting passengers in folder [data](data)

## Ongoing Development

This code is being developed on an on-going basis at the author's
[Github site](https://github.com/tkralphs/JoCTemplate).

## Support

For support in using this software, submit an
[issue](https://github.com/tkralphs/JoCTemplate/issues/new).

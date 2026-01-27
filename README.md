<p align="center">
  <picture>
  	<source media="(prefers-color-scheme: dark)" srcset="logo/light.png">
  	<source media="(prefers-color-scheme: light)" srcset="logo/dark.png">
  <img alt="athena-logo" src="logo/dark.png" width="400">
</picture>
  <h3 align="center">Synergizing Data Prefetching and Off-Chip Prediction <br> via Online Reinforcement Learning
  </h3>
</p>

<p align="center">
    <a href="https://github.com/CMU-SAFARI/Athena/blob/master/LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-yellow.svg"></a>
    <a href="https://github.com/CMU-SAFARI/Athena/releases"><img alt="GitHub release" src="https://img.shields.io/github/release/CMU-SAFARI/Athena"></a>
    <a href="https://arxiv.org/abs/2601.17615"><img src="https://img.shields.io/badge/cs.AR-2601.17615-b31b1b?logo=arxiv&logoColor=red" alt="DOI"></a>
</p>

<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#what-is-athena">What is Athena?</a></li>
    <li><a href="#repository-overview">Repository Overview</a></li>
    <li><a href="#citation">Citation</a></li>
    <li><a href="#building-the-simulator">Building the Simulator</a></li>
    <li><a href="#obtaining-traces">Obtaining Traces</a></li>
    <li><a href="#running-experiments">Running Experiments</a></li>
    <li><a href="#understanding-results">Understanding Results</a></li>
    <li><a href="#brief-code-walkthrough">Brief Code Walkthrough</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>


## What is Athena?

Athena is a reinforcement learning (RL) based policy to coordinate off-chip predictor (OCP) with multiple data prefetchers employed at various cache levels of a high-performance processor. Athena operates in epoch of workload execution (i.e., N committed instructions). At the end of an epoch, Athena observes multiple system-level features (e.g., prefetcher and/or OCP accuracy, main memory bandwidth usage) and takes a coordination action (i.e., enabling the OCP and/or prefetcher, and adjusting prefetcher aggressiveness). It also receives a numerical reward from the processor subsystem at the end of every epoch that measures change in multiple system-level metrics (e.g., execution cycles, LLC miss latency) and use it to autonomously train the coordination policy. 


## Repository Overview

The repository supports:

- **Multiple data prefetchers and off-chip predictors:**
  - L1D Prefetchers: IPCP, Berti
  - L2C Prefetchers: Pythia, SPP+PPF, MLOP, SMS
  - Off-Chip Predictors (OCP): POPET, HMP, TTP

- **Coordination mechanisms compared:**
  - Naive (naive combination)
  - TLP (Two Level Perceptron)
  - HPAC (Hierarchical Prefetcher Aggressiveness Control)
  - MAB (Micro-Armed Bandit)
  - **Athena** (our proposed learning-based coordination)

## Citation

Athena was published and presented in HPCA in February 2026, at Sydney, Australia.

> _Rahul Bera, Zhenrong Lang, Caroline Hengartner, Konstantinos Kanellopoulos, Rakesh Kumar, Mohammad Sadrosadati, and Onur Mutlu, "[Athena: Synergizing Data Prefetching and Off-Chip Prediction via Online Reinforcement Learning]()", In Proceedings of the 32nd International Symposium on High-Performance Computer Architecture (HPCA), 2026_

If you find this repository useful, please cite the paper using:

```
@inproceedings{athena,
  title           = {{Athena: Synergizing Data Prefetching and Off-Chip Prediction via Online Reinforcement Learning}},
  author          = {Bera, Rahul and Lang, Zhenrong and Hengartner, Caroline and Kanellopoulos, Konstantinos and Kumar, Rakesh and Sadrosadati, Mohammad and Mutlu, Onur},
  booktitle       = {HPCA},
  year            = {2026}
}
```


## Building the Simulator

### 0. Prerequisite

This repository has been tested with the following system configuration:
- GNU Make 4.3
- GCC/G++ 11.3.0
- Python 3.12.5
- xz 5.8.1
- gzip 1.10
- curl 7.81.0
- slur-wlm 21.08.5 

### 1. Clone the repository
```bash
git clone https://github.com/CMU-SAFARI/Athena.git
cd Athena
```

### 2. Set up the environment
```bash
source setvars.sh
```
This sets the `ATHENA_HOME` environment variable required by all scripts.

### 3. Build the simulator
```bash
make clean
make -j$(nproc)
```

### 4. Verify the build
```bash
ls bin/champsim
# Should show: bin/champsim
```

The build produces a single binary `bin/champsim` that supports all prefetcher and off-chip predictor configurations via command-line arguments.


## Obtaining Traces

Reproducing the results from the paper requires downloading the workload traces as mentioned below. However, this repository is fully compatible with any ChampSim traces.

### Download Instructions

1. Download the workload traces 
```bash
curl -L "https://zenodo.org/api/records/17850673/files-archive" -o download.zip
```
2. Unzip the workload traces
```bash
mkdir traces
unzip download.zip -d $ATHENA_HOME/traces
```


### Verify Trace Integrity
```bash
mv checksum.txt $ATHENA_HOME/traces
cd $ATHENA_HOME/traces
sha256sum -c checksum.txt
```

### Trace Count
The full evaluation uses **100 workload traces** across four benchmark suites:
- SPEC: 49 traces
- PARSEC: 13 traces
- LIGRA: 13 traces
- CVP: 25 traces


## Running Experiments

### Using the Athena Tool

Before launching the experiments, please make sure to update `DEFAULT_NCORES`, `DEFAULT_PARTITION`, `DEFAULT_HOSTNAME`, and other Slurm-related settings in `$ATHENA_HOME/scripts/config.py`

The `athena.py` script provides a simple push-button interface to reproduce major results from the paper. The script should be used as follows:

```bash

cd $ATHENA_HOME/scripts

# Launch experiments (requires Slurm cluster)
python athena.py -L <FigureID> 
# Summarize results from simulation outputs
python athena.py -S <FigureID>
# Relaunch failed experiments
python athena.py -R <FigureID>
# Visualize results
python athena.py -V <FigureID>
```

The FigureID can take any value from the following list. Each ID corresponds to the respective figure in the paper.

| FigureID | Description |
|--------|-------------|
| Fig7 | Speedup in cache design 1 (CD1) with one OCP and one prefetcher at L2C |
| Fig9 | Speedup in CD2 with one OCP and one prefetcher at L1D |
| Fig10 | Speedup in CD3 with one OCP and two prefetchers at L2C |
| Fig11 | Speedup in CD4 with one OCP and one prefetcher each at L1D and L2C |
| Fig12a | Performance sensitivity to L2C prefetcher in CD1 |
| Fig12b | Performance sensitivity to OCP in CD1 |
| Fig12c | Performance sensitivity to OCP request issue latency in CD1 |
| Fig13 | Performance sensitivity to L1D prefetcher in CD4 |
| Fig14 | Performance sensitivity to main memory bandwidth in CD4 |
| Fig19 | Speedup in coordinating multiple prefetchers at L2C, without any OCP |

> Note that: each FigureID has a corresponding "lite" version (e.g., Fig7-lite) that launches experiment needed to measure only Athena's benefit, not other competitive mechanisms. The lite versions can be used to significantly reduce number of experiments yet reproducing Athena's results.

### Example: Reproducing Figure 7

```bash
# 1. Set environment
source setvars.sh

# 2. Launch experiments (on Slurm cluster)
cd $ATHENA_HOME/scripts
python athena.py -L Fig7 

# 3. Wait for jobs to complete (check with squeue)
# Each trace-experiment combination takes ~3 hours

# 4. Summarize results
python athena.py -S Fig7

# 5. Relaunch experiments, if needed (and summarize again)
python athena.py -R Fig7
python athena.py -S Fig7

# 6. Visualize
python athena.py -V Fig7
```

## Understanding Results

### Experiment Output

Simulation outputs are stored in `experiments/<Figure>/`:
- `<trace>_<experiment>.out` - Simulation statistics
- `<trace>_<experiment>.err` - Error/debug output

### Aggregated CSV File

Aggregated results in `experiments/results/<Figure>.csv`:

| Column | Description |
|--------|-------------|
| Trace | Workload trace name |
| Exp | Experiment configuration name |
| Core_0_cumulative_IPC | Instructions Per Cycle (main metric) |
| Filter | 1 if all experiments for this trace completed |

### Key Metrics

The primary metric is **IPC Speedup** over baseline (no prefetching or OCP):
```
Speedup = IPC_experiment / IPC_baseline
```

Results are aggregated using **geometric mean** across traces, grouped by:
- Workload type (SPEC, PARSEC, LIGRA, CVP)
- Prefetcher-sensitivity (adverse vs. friendly)
- Overall

## Brief Code Walkthrough

> Athena was code-named Oogway (named after the [all-knowing grand master](https://kungfupanda.fandom.com/wiki/Oogway) from Kung Fu Panda). Hence any mention of Oogway anywhere in the code inadvertently means Athena.

This repository is organized as follows:

```
oogway/
├── bin/                    # Compiled simulator binary (champsim)
├── branch/                 # Branch predictor implementations
├── config/                 # Configuration files (.ini)
│   ├── oogway_dev.ini      # Athena configuration
│   ├── pythia.ini          # Pythia configuration
│   └── ...
├── experiments/            # Experiment outputs and results
│   ├── Fig5a/              # Raw simulation outputs for Fig5a
│   ├── Fig5b/              # Raw simulation outputs for Fig5b
│   ├── ...
│   └── results/            # Aggregated CSVs and plots
├── inc/                    # Header files
│   ├── oogway.h            # Athena implementation
│   ├── scooby.h            # Pythia prefetcher
│   └── ...
├── obj/                    # Object files (generated during build)
├── prefetcher/             # Prefetcher implementations
├── replacement/            # Cache replacement policies
├── scripts/                # Experiment management scripts
│   ├── athena.py           # Main entry point for experiments
│   ├── config.py           # Experiment configurations
│   ├── generate.py         # Job generation
│   ├── rollup.py           # Result aggregation
│   ├── visualize.py        # Visualization
│   └── ...
├── src/                    # ChampSim core source files
├── traces/                 # Trace files (user must download)
├── checksum.txt            # SHA256 checksums for trace verification
├── Makefile                # Build configuration
├── setvars.sh              # Environment setup script
└── wrapper.sh              # Job wrapper for Slurm
```

- All the necessary source files for Athena can be found inside `inc/` and `src/` directories.
- Athena's default configuration parameters are defined in `config/oogway_dev.ini`.
- `Oogway::train_and_take_action()` is the high-level entry function to Athena that (1) takes a coordination action and (2) trains the RL model at the end of every execution epoch. It gets called by the `ooo_cpu::retire_rob()`, which captures the current system state (e.g., main memory bandwidth usage, OCP and prefetchers' accuracy, cache pollution, etc.) and pass it to Athena. Athena takes this state, (1) computes the reward based on all the system-level metrics it has observed during the epoch (i.e., cycle count, LLC load miss latency, mispredicted branches, etc.), (2) makes the decision for the next epoch, and (3) uses the computed reward to train the RL model.
- The RL model is defined in `inc/learning_engine_hashed.h` and `src/learning_engine_hashed.cc`.

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Contact

Please contact [Rahul Bera](mailto:write2bera@gmail.com) and [Zhenrong Lang](mailto:davidlang0832@gmail.com) if you have any questions/suggestions.
# Athena: Synergizing Data Prefetching and Off-Chip Prediction via Online Reinforcement Learning

This artifact contains the implementation and evaluation infrastructure for **Athena**, a reinforcement learning-based technique for synergizing data prefetching and off-chip prediction. The artifact is built on top of ChampSim, a trace-driven CPU simulator.

---

## Table of Contents

1. [Overview](#overview)
2. [Directory Structure](#directory-structure)
3. [Building the Simulator](#building-the-simulator)
4. [Obtaining Traces](#obtaining-traces)
5. [Running Experiments](#running-experiments)
6. [Understanding Results](#understanding-results)

---

## Overview

The artifact supports:

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

---

## Directory Structure

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

---

## Building the Simulator


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

---

## Obtaining Traces

The artifact requires workload traces to run simulations. Traces are **not included** due to size (~30 GB total).

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

---

## Running Experiments

### Using the Athena Tool

Before launching the experiments, please make sure to update `DEFAULT_NCORES`, `DEFAULT_PARTITION`, `DEFAULT_HOSTNAME`, and other Slurm-related settings in `$ATHENA_HOME/scripts/config.py`

The `athena.py` script provides a unified interface for all experiment operations.

```bash

cd $ATHENA_HOME/scripts

# Launch experiments (requires Slurm cluster)
python athena.py -L <Figure> 
# Summarize results from simulation outputs
python athena.py -S <Figure>
# Relaunch failed experiments
python athena.py -R <Figure>
# Visualize results
python athena.py -V <Figure>
```

### Available Experiments (Figures)

| Figure | Description | Coordination | Components |
|--------|-------------|--------------|------------|
| Fig5a | OCP + L2C | POPET + Pythia | CD1 |
| Fig5b | OCP + L1D | POPET + IPCP | CD2 |
| Fig5c | OCP + 2 L2C | POPET + SMS + Pythia | CD3 |
| Fig5d | OCP + L1D + L2C | POPET + IPCP + Pythia | CD4 |
| Fig6b | L2C Type Sensitivity | POPET + various L2C | Pythia/SPP+PPF/MLOP/SMS |
| Fig6c | OCP Latency Sensitivity | POPET + Pythia | 6/18/30 cycles |
| Fig6d | OCP Type Sensitivity | Various OCP + Pythia | POPET/HMP/TTP |
| Fig7a | L1D Type Sensitivity | POPET + various L1D + Pythia | IPCP/Berti |
| Fig7b | Bandwidth Sensitivity | POPET + IPCP + Pythia | 1.6-12.8 GB/s |
| Fig8 | L2C Only | SMS + Pythia (no OCP) | Generalizability Study |

### Example: Running Figure 5a

```bash
# 1. Set environment
source setvars.sh

# 2. Launch experiments (on Slurm cluster)
cd $ATHENA_HOME/scripts
python athena.py -L Fig5a 

# 3. Wait for jobs to complete (check with squeue)
# Each trace-experiment combination takes ~3 hours

# 4. Summarize results
python athena.py -S Fig5a

# 5. Relaunch experiments if needed (and summarize again)
python athena.py -R Fig5a
python athena.py -S Fig5a

# 6. Visualize
python athena.py -V Fig5a
```

## Understanding Results

### Output Format

Simulation outputs are stored in `experiments/<Figure>/`:
- `<trace>_<experiment>.out` - Simulation statistics
- `<trace>_<experiment>.err` - Error/debug output

### CSV Format

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

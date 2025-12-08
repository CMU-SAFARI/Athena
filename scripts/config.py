"""
Configuration file for Athena experiment management system.
Contains all constants, defaults, and configuration mappings.
"""

import os
from typing import Dict, List

# Default values for job generation
DEFAULT_NCORES = 1
DEFAULT_PARTITION = "cpu_part"
DEFAULT_HOSTNAME = "kratos"
DEFAULT_SLURM_USERNAME = "zhlang"

# Experiment variable definitions
EXP_VARIABLES = {
    'BASE': '--warmup_instructions=100000000 --simulation_instructions=500000000 --llc_replacement_type=ship --config=$(ATHENA_HOME)/config/nopref.ini --rob_partition_size=64,128,320 --rob_frontal_partition_ids=0 --rob_dorsal_partition_ids=2 --dram_io_freq=400',
    
    # L1D prefetchers
    'IPCP': '--l1d_prefetcher_types=ipcp',
    'BERTI': '--l1d_prefetcher_types=berti',
    
    # L2C prefetchers
    'PYTHIA': '--l2c_prefetcher_types=scooby --config=$(ATHENA_HOME)/config/pythia.ini --l2c_prefetcher_force_prefetch_at_llc=true',
    'SMS': '--l2c_prefetcher_types=sms --config=$(ATHENA_HOME)/config/sms.ini --l2c_prefetcher_force_prefetch_at_llc=true',
    'SPP+PPF': '--l2c_prefetcher_types=spp_ppf_dev --config=$(ATHENA_HOME)/config/spp_ppf_dev.ini --l2c_prefetcher_force_prefetch_at_llc=true',
    'MLOP': '--l2c_prefetcher_types=mlop --config=$(ATHENA_HOME)/config/mlop.ini --l2c_prefetcher_force_prefetch_at_llc=true',
    
    # Off-Chip Predictors (OCP)
    'POPET': '--config=$(ATHENA_HOME)/config/hermes_base.ini --config=$(ATHENA_HOME)/config/ocp_hermes.ini',
    'HMP': '--config=$(ATHENA_HOME)/config/hermes_base.ini --config=$(ATHENA_HOME)/config/ocp_hmp_ensemble.ini',
    'TTP': '--config=$(ATHENA_HOME)/config/hermes_base.ini --config=$(ATHENA_HOME)/config/ocp_ttp.ini',
    
    # Coordination mechanisms
    'TLP': '--config=$(ATHENA_HOME)/config/hermes_base.ini --config=$(ATHENA_HOME)/config/ocp_tlp.ini',
    'HPAC': '--config=$(ATHENA_HOME)/config/oogway_dev.ini --heuristic_enable=true',
    'MAB': '--config=$(ATHENA_HOME)/config/oogway_dev.ini --mab_enable=true',
    'ATHENA': '--config=$(ATHENA_HOME)/config/oogway_dev.ini',
    
    # Cache Design configurations
    'CD1': '--og_multi_prefetcher_enable=false --og_prefetcher_level=2',  # OCP + L2C
    'CD2': '--og_multi_prefetcher_enable=false --og_prefetcher_level=1',  # OCP + L1D
    'CD3': '--og_multi_prefetcher_enable=true --og_coordination_mode=l2c_l2c',  # OCP + 2 L2C
    'CD4': '--og_multi_prefetcher_enable=true --og_coordination_mode=l1d_l2c',  # OCP + L1D + L2C
    'CD5': '--og_multi_prefetcher_enable=true --og_l2c_only_coordination=true',  # 2 L2C
}

# Experiment configurations (merges experiments with their variable definitions)
EXPERIMENTS = {
    'Fig5a': {
        'description': 'OCP + L2C',
        'experiments': {
            'Baseline': ['BASE'],
            'POPET': ['BASE', 'POPET'],
            'Pythia': ['BASE', 'PYTHIA'],
            'Naive-POPET-Pythia-': ['BASE', 'POPET', 'PYTHIA'],
            'HPAC-POPET-Pythia-': ['BASE', 'POPET', 'PYTHIA', 'CD1', 'HPAC'],
            'MAB-POPET-Pythia-': ['BASE', 'POPET', 'PYTHIA', 'CD1', 'MAB'],
            'Athena-POPET-Pythia-': ['BASE', 'POPET', 'PYTHIA', 'CD1', 'ATHENA'],
        }
    },
    'Fig5b': {
        'description': 'OCP + L1D',
        'experiments': {
            'Baseline': ['BASE'],
            'POPET': ['BASE', 'POPET'],
            'IPCP': ['BASE', 'IPCP'],
            'Naive-POPET-IPCP-': ['BASE', 'POPET', 'IPCP'],
            'TLP-POPET-IPCP-': ['BASE', 'TLP', 'IPCP'],
            'HPAC-POPET-IPCP-': ['BASE', 'POPET', 'IPCP', 'CD2', 'HPAC'],
            'MAB-POPET-IPCP-': ['BASE', 'POPET', 'IPCP', 'CD2', 'MAB'],
            'Athena-POPET-IPCP-': ['BASE', 'POPET', 'IPCP', 'CD2', 'ATHENA'],
        }
    },
    'Fig5c': {
        'description': 'OCP + 2 L2C',
        'experiments': {
            'Baseline': ['BASE'],
            'POPET': ['BASE', 'POPET'],
            'SMS-Pythia': ['BASE', 'SMS', 'PYTHIA'],
            'Naive-POPET-SMS-Pythia-': ['BASE', 'POPET', 'SMS', 'PYTHIA'],
            'HPAC-POPET-SMS-Pythia-': ['BASE', 'POPET', 'SMS', 'PYTHIA', 'CD3', 'HPAC'],
            'MAB-POPET-SMS-Pythia-': ['BASE', 'POPET', 'SMS', 'PYTHIA', 'CD3', 'MAB'],
            'Athena-POPET-SMS-Pythia-': ['BASE', 'POPET', 'SMS', 'PYTHIA', 'CD3', 'ATHENA'],
        }
    },
    'Fig5d': {
        'description': 'OCP + L1D + L2C',
        'experiments': {
            'Baseline': ['BASE'],
            'POPET': ['BASE', 'POPET'],
            'IPCP-Pythia': ['BASE', 'IPCP', 'PYTHIA'],
            'Naive-POPET-IPCP-Pythia-': ['BASE', 'POPET', 'IPCP', 'PYTHIA'],
            'TLP-POPET-IPCP-Pythia-': ['BASE', 'TLP', 'IPCP', 'PYTHIA'],
            'HPAC-POPET-IPCP-Pythia-': ['BASE', 'POPET', 'IPCP', 'PYTHIA', 'CD4', 'HPAC'],
            'MAB-POPET-IPCP-Pythia-': ['BASE', 'POPET', 'IPCP', 'PYTHIA', 'CD4', 'MAB'],
            'Athena-POPET-IPCP-Pythia-': ['BASE', 'POPET', 'IPCP', 'PYTHIA', 'CD4', 'ATHENA'],
        }
    }
}


# Default paths
DEFAULT_RESULTS_DIR = 'experiments/results'  # Relative to ATHENA_HOME

# Get ATHENA_HOME from environment
ATHENA_HOME = os.environ.get('ATHENA_HOME', '')

def get_athena_home() -> str:
    """Get ATHENA_HOME from environment, raise error if not set."""
    if not ATHENA_HOME:
        raise ValueError("$ATHENA_HOME environment variable is not defined. Have you sourced setvars.sh?")
    return ATHENA_HOME

# Workload type enumeration
class WorkloadType:
    SPEC = 'SPEC'
    PARSEC = 'PARSEC'
    LIGRA = 'LIGRA'
    CVP = 'CVP'

# Trace data configuration (from hpca_final_1.tlist)
# Each entry contains: path, knobs, workload_type, adverse (prefetcher-adverse if True)
TRACE_DATA = {
    # SPEC benchmarks (SPEC CPU 2006 and 2017)
    '429.mcf-192B': {
        'path': '$ATHENA_HOME/traces/429.mcf-192B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '433.milc-127B': {
        'path': '$ATHENA_HOME/traces/433.milc-127B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '433.milc-274B': {
        'path': '$ATHENA_HOME/traces/433.milc-274B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '433.milc-337B': {
        'path': '$ATHENA_HOME/traces/433.milc-337B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '437.leslie3d-134B': {
        'path': '$ATHENA_HOME/traces/437.leslie3d-134B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '437.leslie3d-149B': {
        'path': '$ATHENA_HOME/traces/437.leslie3d-149B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '437.leslie3d-265B': {
        'path': '$ATHENA_HOME/traces/437.leslie3d-265B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '437.leslie3d-271B': {
        'path': '$ATHENA_HOME/traces/437.leslie3d-271B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '437.leslie3d-273B': {
        'path': '$ATHENA_HOME/traces/437.leslie3d-273B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '445.gobmk-30B': {
        'path': '$ATHENA_HOME/traces/445.gobmk-30B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '445.gobmk-36B': {
        'path': '$ATHENA_HOME/traces/445.gobmk-36B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '450.soplex-247B': {
        'path': '$ATHENA_HOME/traces/450.soplex-247B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '450.soplex-92B': {
        'path': '$ATHENA_HOME/traces/450.soplex-92B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '459.GemsFDTD-1169B': {
        'path': '$ATHENA_HOME/traces/459.GemsFDTD-1169B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '459.GemsFDTD-1320B': {
        'path': '$ATHENA_HOME/traces/459.GemsFDTD-1320B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '459.GemsFDTD-1491B': {
        'path': '$ATHENA_HOME/traces/459.GemsFDTD-1491B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '462.libquantum-1343B': {
        'path': '$ATHENA_HOME/traces/462.libquantum-1343B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '470.lbm-1274B': {
        'path': '$ATHENA_HOME/traces/470.lbm-1274B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '471.omnetpp-188B': {
        'path': '$ATHENA_HOME/traces/471.omnetpp-188B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '473.astar-42B': {
        'path': '$ATHENA_HOME/traces/473.astar-42B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '481.wrf-1254B': {
        'path': '$ATHENA_HOME/traces/481.wrf-1254B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '482.sphinx3-1100B': {
        'path': '$ATHENA_HOME/traces/482.sphinx3-1100B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '482.sphinx3-1297B': {
        'path': '$ATHENA_HOME/traces/482.sphinx3-1297B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '482.sphinx3-1522B': {
        'path': '$ATHENA_HOME/traces/482.sphinx3-1522B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '482.sphinx3-234B': {
        'path': '$ATHENA_HOME/traces/482.sphinx3-234B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '482.sphinx3-417B': {
        'path': '$ATHENA_HOME/traces/482.sphinx3-417B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '483.xalancbmk-127B': {
        'path': '$ATHENA_HOME/traces/483.xalancbmk-127B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '483.xalancbmk-716B': {
        'path': '$ATHENA_HOME/traces/483.xalancbmk-716B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '483.xalancbmk-736B': {
        'path': '$ATHENA_HOME/traces/483.xalancbmk-736B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '602.gcc_s-1850B': {
        'path': '$ATHENA_HOME/traces/602.gcc_s-1850B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '602.gcc_s-2226B': {
        'path': '$ATHENA_HOME/traces/602.gcc_s-2226B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '602.gcc_s-734B': {
        'path': '$ATHENA_HOME/traces/602.gcc_s-734B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '603.bwaves_s-891B': {
        'path': '$ATHENA_HOME/traces/603.bwaves_s-891B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '605.mcf_s-1554B': {
        'path': '$ATHENA_HOME/traces/605.mcf_s-1554B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '605.mcf_s-1644B': {
        'path': '$ATHENA_HOME/traces/605.mcf_s-1644B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '605.mcf_s-472B': {
        'path': '$ATHENA_HOME/traces/605.mcf_s-472B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '605.mcf_s-484B': {
        'path': '$ATHENA_HOME/traces/605.mcf_s-484B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '607.cactuBSSN_s-3477B': {
        'path': '$ATHENA_HOME/traces/607.cactuBSSN_s-3477B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '607.cactuBSSN_s-4004B': {
        'path': '$ATHENA_HOME/traces/607.cactuBSSN_s-4004B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '619.lbm_s-2676B': {
        'path': '$ATHENA_HOME/traces/619.lbm_s-2676B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '619.lbm_s-2677B': {
        'path': '$ATHENA_HOME/traces/619.lbm_s-2677B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '619.lbm_s-3766B': {
        'path': '$ATHENA_HOME/traces/619.lbm_s-3766B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '619.lbm_s-4268B': {
        'path': '$ATHENA_HOME/traces/619.lbm_s-4268B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '623.xalancbmk_s-202B': {
        'path': '$ATHENA_HOME/traces/623.xalancbmk_s-202B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '623.xalancbmk_s-592B': {
        'path': '$ATHENA_HOME/traces/623.xalancbmk_s-592B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '623.xalancbmk_s-700B': {
        'path': '$ATHENA_HOME/traces/623.xalancbmk_s-700B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '627.cam4_s-573B': {
        'path': '$ATHENA_HOME/traces/627.cam4_s-573B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': False,
    },
    '649.fotonik3d_s-10881B': {
        'path': '$ATHENA_HOME/traces/649.fotonik3d_s-10881B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    '649.fotonik3d_s-7084B': {
        'path': '$ATHENA_HOME/traces/649.fotonik3d_s-7084B.champsimtrace.xz',
        'knobs': '',
        'type': WorkloadType.SPEC,
        'adverse': True,
    },
    
    # PARSEC benchmarks
    'parsec_2.1.canneal.simlarge.prebuilt.drop_4750M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.canneal.simlarge.prebuilt.drop_4750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.canneal.simlarge.prebuilt.drop_5000M.length_15M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.canneal.simlarge.prebuilt.drop_5000M.length_15M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.facesim.simlarge.prebuilt.drop_1500M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.facesim.simlarge.prebuilt.drop_1500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.facesim.simlarge.prebuilt.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.facesim.simlarge.prebuilt.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.fluidanimate.simlarge.prebuilt.drop_9500M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.fluidanimate.simlarge.prebuilt.drop_9500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    'parsec_2.1.raytrace.simlarge.prebuilt.drop_23500M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.raytrace.simlarge.prebuilt.drop_23500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.raytrace.simlarge.prebuilt.drop_23750M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.raytrace.simlarge.prebuilt.drop_23750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.raytrace.simlarge.prebuilt.drop_25500M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.raytrace.simlarge.prebuilt.drop_25500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': False,
    },
    'parsec_2.1.streamcluster.simlarge.prebuilt.drop_0M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.streamcluster.simlarge.prebuilt.drop_0M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    'parsec_2.1.streamcluster.simlarge.prebuilt.drop_14750M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.streamcluster.simlarge.prebuilt.drop_14750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    'parsec_2.1.streamcluster.simlarge.prebuilt.drop_250M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.streamcluster.simlarge.prebuilt.drop_250M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    'parsec_2.1.streamcluster.simlarge.prebuilt.drop_4750M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.streamcluster.simlarge.prebuilt.drop_4750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    'parsec_2.1.streamcluster.simlarge.prebuilt.drop_6250M.length_250M': {
        'path': '$ATHENA_HOME/traces/parsec_2.1.streamcluster.simlarge.prebuilt.drop_6250M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.PARSEC,
        'adverse': True,
    },
    
    # LIGRA benchmarks
    'ligra_BC.com-lj.ungraph.gcc_6.3.0_O3.drop_500M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_BC.com-lj.ungraph.gcc_6.3.0_O3.drop_500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_BFS.com-lj.ungraph.gcc_6.3.0_O3.drop_500M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_BFS.com-lj.ungraph.gcc_6.3.0_O3.drop_500M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_BFSCC.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_BFSCC.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_CF.com-lj.ungraph.gcc_6.3.0_O3.drop_154750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_CF.com-lj.ungraph.gcc_6.3.0_O3.drop_154750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': True,
    },
    'ligra_Components-Shortcut.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Components-Shortcut.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_Components.com-lj.ungraph.gcc_6.3.0_O3.drop_15750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Components.com-lj.ungraph.gcc_6.3.0_O3.drop_15750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_Components.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Components.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_1250M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_1250M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_24000M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_24000M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': True,
    },
    'ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_56500M.length_208M': {
        'path': '$ATHENA_HOME/traces/ligra_PageRankDelta.com-lj.ungraph.gcc_6.3.0_O3.drop_56500M.length_208M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_Radii.com-lj.ungraph.gcc_6.3.0_O3.drop_5000M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Radii.com-lj.ungraph.gcc_6.3.0_O3.drop_5000M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_Radii.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Radii.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    'ligra_Triangle.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M': {
        'path': '$ATHENA_HOME/traces/ligra_Triangle.com-lj.ungraph.gcc_6.3.0_O3.drop_750M.length_250M.champsimtrace.xz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.LIGRA,
        'adverse': False,
    },
    
    # CVP benchmarks (secret compute traces)
    'secret_compute_fp_105': {
        'path': '$ATHENA_HOME/traces/compute_fp_105.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_fp_4': {
        'path': '$ATHENA_HOME/traces/compute_fp_4.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_fp_45': {
        'path': '$ATHENA_HOME/traces/compute_fp_45.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_fp_59': {
        'path': '$ATHENA_HOME/traces/compute_fp_59.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_fp_78': {
        'path': '$ATHENA_HOME/traces/compute_fp_78.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_12': {
        'path': '$ATHENA_HOME/traces/compute_int_12.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_21': {
        'path': '$ATHENA_HOME/traces/compute_int_21.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_24': {
        'path': '$ATHENA_HOME/traces/compute_int_24.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_243': {
        'path': '$ATHENA_HOME/traces/compute_int_243.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_422': {
        'path': '$ATHENA_HOME/traces/compute_int_422.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_428': {
        'path': '$ATHENA_HOME/traces/compute_int_428.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_446': {
        'path': '$ATHENA_HOME/traces/compute_int_446.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_452': {
        'path': '$ATHENA_HOME/traces/compute_int_452.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_539': {
        'path': '$ATHENA_HOME/traces/compute_int_539.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_554': {
        'path': '$ATHENA_HOME/traces/compute_int_554.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_555': {
        'path': '$ATHENA_HOME/traces/compute_int_555.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_568': {
        'path': '$ATHENA_HOME/traces/compute_int_568.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_719': {
        'path': '$ATHENA_HOME/traces/compute_int_719.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_759': {
        'path': '$ATHENA_HOME/traces/compute_int_759.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_76': {
        'path': '$ATHENA_HOME/traces/compute_int_76.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_788': {
        'path': '$ATHENA_HOME/traces/compute_int_788.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_820': {
        'path': '$ATHENA_HOME/traces/compute_int_820.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_859': {
        'path': '$ATHENA_HOME/traces/compute_int_859.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
    'secret_compute_int_878': {
        'path': '$ATHENA_HOME/traces/compute_int_878.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': True,
    },
    'secret_compute_int_951': {
        'path': '$ATHENA_HOME/traces/compute_int_951.champsim.gz',
        'knobs': '--warmup_instructions=100000000 --simulation_instructions=150000000',
        'type': WorkloadType.CVP,
        'adverse': False,
    },
}


def get_trace_info(name: str) -> dict:
    """Get trace information including path, knobs, type, and adverse."""
    if name not in TRACE_DATA:
        raise ValueError(f"Unknown trace: {name}")
    return TRACE_DATA[name].copy()


def get_traces_by_type(workload_type: str) -> List[str]:
    """Get list of trace names filtered by workload type."""
    return [name for name, data in TRACE_DATA.items() if data['type'] == workload_type]


def get_all_trace_names() -> List[str]:
    """Get list of all trace names."""
    return list(TRACE_DATA.keys())


# Default metrics configuration for rollup
DEFAULT_METRICS = [
    {'NAME': 'Core_0_cumulative_IPC', 'TYPE': 'sum'},
]

# Extended metrics including coverage and accuracy
EXTENDED_METRICS = [
    {'NAME': 'Core_0_cumulative_IPC', 'TYPE': 'sum'},
    {'NAME': 'Core_0_offchip_pred_precision', 'TYPE': 'sum'},
    {'NAME': 'Core_0_offchip_pred_recall', 'TYPE': 'sum'},
]


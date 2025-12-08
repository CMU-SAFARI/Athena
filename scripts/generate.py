"""
Job generation module for Athena.
Handles parsing of experiment configurations and generation of Slurm batch scripts.
All resources come from config.py - no external file dependencies.
"""

import os
import subprocess
from typing import List, Dict, Optional
from config import (EXPERIMENTS, DEFAULT_NCORES, DEFAULT_PARTITION,
                    DEFAULT_HOSTNAME, get_athena_home, EXP_VARIABLES,
                    TRACE_DATA, WorkloadType)


def get_experiments_for_cd(cd: str) -> List[Dict[str, str]]:
    """
    Get list of experiments for a specific experiment configuration from config.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d)
    
    Returns:
        List of experiment dictionaries with NAME and KNOBS keys.
    """
    if cd not in EXPERIMENTS:
        raise ValueError(f"Unknown experiment: {cd}. Must be one of: {list(EXPERIMENTS.keys())}")
    
    experiments = []
    for exp_name, var_list in EXPERIMENTS[cd]['experiments'].items():
        # Combine variable values to get full knobs string
        knobs = ' '.join(EXP_VARIABLES[var] for var in var_list)
        experiments.append({
            'NAME': exp_name,
            'KNOBS': knobs
        })
    
    return experiments


def get_traces(workload_types: Optional[List[str]] = None) -> List[Dict[str, str]]:
    """
    Get trace list from TRACE_DATA in config.py.
    
    Args:
        workload_types: Optional list of workload types to filter by 
                       (e.g., [WorkloadType.SPEC, WorkloadType.PARSEC])
                       If None, returns all traces.
    
    Returns:
        List of trace dictionaries with NAME, TRACE, KNOBS, and TYPE keys.
    """
    trace_info = []
    
    for name, data in TRACE_DATA.items():
        # Filter by workload type if specified
        if workload_types is not None and data['type'] not in workload_types:
            continue
        
        trace_info.append({
            'NAME': name,
            'TRACE': data['path'],
            'KNOBS': data['knobs'],
            'TYPE': data['type'],
        })
    
    return trace_info


class JobGenerator:
    """Generate Slurm job commands."""
    
    def __init__(self, exe: str, cd: str,
                 ncores: int = DEFAULT_NCORES, partition: str = DEFAULT_PARTITION,
                 hostname: str = DEFAULT_HOSTNAME, extra: Optional[str] = None,
                 output_dir: Optional[str] = None,
                 workload_types: Optional[List[str]] = None):
        """
        Initialize JobGenerator.
        
        Args:
            exe: Path to the executable
            cd: Experiment configuration (Fig5a-Fig5d)
            ncores: Number of cores per job
            partition: Slurm partition
            hostname: Hostname prefix for nodes
            extra: Extra Slurm options
            output_dir: Output directory for results
            workload_types: Filter traces by workload type
        """
        self.exe = exe
        self.cd = cd
        self.ncores = ncores
        self.partition = partition
        self.hostname = hostname
        self.extra = extra
        self.output_dir = output_dir or os.getcwd()
        
        # Get ATHENA_HOME from environment
        self.athena_home = get_athena_home()
        
        # Get experiments for this CD from config
        self.experiments = get_experiments_for_cd(cd)
        print(f"Loaded {len(self.experiments)} experiments for {cd}")
        
        # Get traces from config
        self.traces = get_traces(workload_types=workload_types)
        print(f"Loaded {len(self.traces)} traces from config")
    
    def substitute_variables(self, text: str, exp_name: str, trace_name: str) -> str:
        """Substitute variables in command line."""
        text = text.replace('$(ATHENA_HOME)', self.athena_home)
        text = text.replace('$(EXP)', exp_name)
        text = text.replace('$(TRACE)', trace_name)
        text = text.replace('$(NCORES)', str(self.ncores))
        return text
    
    def generate_sbatch_commands(self) -> List[str]:
        """Generate list of sbatch commands.
        
        Returns:
            List of sbatch command strings ready to execute.
        """
        commands = []
        
        # Generate commands for each trace-experiment combination
        for trace in self.traces:
            trace_name = trace.get('NAME', '')
            trace_input = trace.get('TRACE', '')
            trace_knobs = trace.get('KNOBS', '')
            
            for exp in self.experiments:
                exp_name = exp['NAME']
                exp_knobs = exp['KNOBS']
                
                # Build command line
                parts = [exp_knobs]
                if trace_knobs:
                    parts.append(trace_knobs)
                parts.append(f"-traces {trace_input}")
                full_knobs = ' '.join(parts)
                
                # Substitute variables
                full_knobs = self.substitute_variables(full_knobs, exp_name, trace_name)
                
                # Slurm submission
                slurm_cmd = f"sbatch -p {self.partition} --mincpus=1"
                
                if self.extra:
                    slurm_cmd += f" {self.extra}"
                
                # Output files directly in the output directory
                out_file = os.path.join(self.output_dir, f"{trace_name}_{exp_name}.out")
                err_file = os.path.join(self.output_dir, f"{trace_name}_{exp_name}.err")
                
                job_name = f"{trace_name}_{exp_name}"
                slurm_cmd += f" -c {self.ncores} -J {job_name} -o {out_file} -e {err_file}"
                
                wrapper_script = os.path.join(self.athena_home, "wrapper.sh")
                exe_path = self.substitute_variables(self.exe, exp_name, trace_name)
                full_knobs_quoted = f'"{full_knobs}"'
                
                cmdline = f"{slurm_cmd} {wrapper_script} {exe_path} {full_knobs_quoted}"
                commands.append(cmdline)
        
        return commands


def generate_jobs(exe: str, cd: str,
                  ncores: int = DEFAULT_NCORES, partition: str = DEFAULT_PARTITION,
                  hostname: str = DEFAULT_HOSTNAME, extra: Optional[str] = None,
                  workload_types: Optional[List[str]] = None) -> str:
    """
    Submit slurm jobs for specified Cache Design.
    
    All experiment and trace configurations come from config.py.
    Jobs are submitted directly to $ATHENA_HOME/experiments/<CD>/.
    
    Args:
        exe: Path to the executable
        cd: Experiment configuration (Fig5a-Fig5d)
        ncores: Number of cores per job
        partition: Slurm partition
        hostname: Hostname prefix for nodes
        extra: Extra Slurm options
        workload_types: Filter traces by workload type (e.g., [WorkloadType.SPEC])
    
    Returns:
        All submitted commands as a string
    """
    athena_home = get_athena_home()
    
    # Create output directory in $ATHENA_HOME/experiments/<CD>
    output_dir = os.path.join(athena_home, "experiments", cd)
    print(f"Creating experiment folder at {output_dir}")
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate job commands
    generator = JobGenerator(
        exe=exe,
        cd=cd,
        ncores=ncores,
        partition=partition,
        hostname=hostname,
        extra=extra,
        output_dir=output_dir,
        workload_types=workload_types
    )
    
    # Submit jobs directly
    original_dir = os.getcwd()
    os.chdir(output_dir)
    print(f"Changed to directory: {output_dir}")
    
    commands = generator.generate_sbatch_commands()
    print(f"Submitting {len(commands)} jobs...")
    
    for i, cmd in enumerate(commands, 1):
        print(f"[{i}/{len(commands)}] {cmd}")
        subprocess.run(cmd, shell=True)
    
    os.chdir(original_dir)
    print(f"\nSubmitted {len(commands)} jobs to slurm.")
    print(f"Output files will be in: {output_dir}")
    return '\n'.join(commands)

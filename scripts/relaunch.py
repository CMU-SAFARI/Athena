"""
Relaunch module for Athena.
Detects failed experiments and relaunches them.
All resources come from config.py - no external file dependencies.
"""

import os
import subprocess
from typing import List, Tuple, Optional
from config import (EXPERIMENTS, DEFAULT_NCORES, DEFAULT_PARTITION,
                    DEFAULT_HOSTNAME, DEFAULT_METRICS, TRACE_DATA,
                    EXP_VARIABLES, get_athena_home, get_all_trace_names)
from rollup import parse_output_file


def get_failed_experiments(cd: str, input_dir: Optional[str] = None,
                           ext: str = "out") -> List[Tuple[str, str]]:
    """
    Detect failed experiments by checking for missing or incomplete output files.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d)
        input_dir: Directory containing output files (default: $ATHENA_HOME/experiments/<exp>)
        ext: Extension of output files (default: "out")
    
    Returns:
        List of (trace_name, exp_name) tuples for failed experiments
    """
    if cd not in EXPERIMENTS:
        raise ValueError(f"Unknown experiment: {cd}. Must be one of: {list(EXPERIMENTS.keys())}")
    
    athena_home = get_athena_home()
    
    # Determine input directory
    if input_dir is None:
        input_dir = os.path.join(athena_home, "experiments", cd)
    
    # Get trace names and experiment names
    trace_names = get_all_trace_names()
    exp_names = list(EXPERIMENTS[cd]['experiments'].keys())
    
    # Check each experiment
    failed = []
    metrics = DEFAULT_METRICS
    
    for trace_name in trace_names:
        for exp_name in exp_names:
            # Look for output file
            log_file = os.path.join(input_dir, exp_name, f"{trace_name}_{exp_name}.{ext}")
            if not os.path.exists(log_file):
                log_file = os.path.join(input_dir, f"{trace_name}_{exp_name}.{ext}")
            
            # Parse and check if all metrics are present
            records = parse_output_file(log_file, ext)
            
            if not records:
                failed.append((trace_name, exp_name))
            else:
                # Check if all required metrics are present
                all_metrics_present = True
                for metric in metrics:
                    if metric['NAME'] not in records:
                        all_metrics_present = False
                        break
                
                if not all_metrics_present:
                    failed.append((trace_name, exp_name))
    
    return failed


def relaunch_experiments(cd: str, input_dir: Optional[str] = None,
                        exe: str = '$ATHENA_HOME/bin/champsim',
                        ncores: int = DEFAULT_NCORES,
                        partition: str = DEFAULT_PARTITION,
                        hostname: str = DEFAULT_HOSTNAME,
                        extra: Optional[str] = None,
                        ext: str = "out",
                        dry_run: bool = False) -> int:
    """
    Detect and relaunch failed experiments.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d)
        input_dir: Directory containing output files (default: $ATHENA_HOME/experiments/<exp>)
        exe: Path to executable
        ncores: Number of cores per job
        partition: Slurm partition
        hostname: Hostname prefix
        extra: Extra Slurm options
        ext: Extension of output files
        dry_run: If True, only print commands without executing
    
    Returns:
        Number of jobs relaunched
    """
    athena_home = get_athena_home()
    
    # Determine output directory
    if input_dir is None:
        output_dir = os.path.join(athena_home, "experiments", cd)
    else:
        output_dir = input_dir
    
    # Get failed experiments
    failed = get_failed_experiments(cd, input_dir, ext)
    
    if not failed:
        print(f"\nNo failed experiments found for {cd}. All experiments completed successfully!")
        return 0
    
    print(f"\n{'='*60}")
    print(f"Relaunch Summary for {cd}")
    print(f"{'='*60}")
    print(f"Found {len(failed)} failed experiments")
    
    if dry_run:
        print(f"\nDRY RUN - Commands that would be executed:")
        print(f"{'='*60}")
    else:
        print(f"\nRelaunching {len(failed)} experiments...")
    
    # Substitute ATHENA_HOME in exe path
    exe_path = exe.replace('$ATHENA_HOME', athena_home).replace('$(ATHENA_HOME)', athena_home)
    
    # Generate and submit jobs for each failed experiment
    commands = []
    
    for trace_name, exp_name in failed:
        # Get trace info
        trace_data = TRACE_DATA[trace_name]
        trace_path = trace_data['path']
        trace_knobs = trace_data['knobs']
        
        # Get experiment knobs
        var_list = EXPERIMENTS[cd]['experiments'][exp_name]
        exp_knobs = ' '.join(EXP_VARIABLES[var] for var in var_list)
        
        # Replace ATHENA_HOME in knobs
        exp_knobs = exp_knobs.replace('$(ATHENA_HOME)', athena_home)
        
        # Build full command
        parts = [exp_knobs]
        if trace_knobs:
            parts.append(trace_knobs)
        parts.append(f"-traces {trace_path}")
        full_knobs = ' '.join(parts)
        
        # Build slurm command
        slurm_cmd = f"sbatch -p {partition} --mincpus=1"
        
        if extra:
            slurm_cmd += f" {extra}"
        
        out_file = os.path.join(output_dir, f"{trace_name}_{exp_name}.out")
        err_file = os.path.join(output_dir, f"{trace_name}_{exp_name}.err")
        job_name = f"{trace_name}_{exp_name}"
        
        slurm_cmd += f" -c {ncores} -J {job_name} -o {out_file} -e {err_file}"
        
        wrapper_script = os.path.join(athena_home, "wrapper.sh")
        full_knobs_quoted = f'"{full_knobs}"'
        
        cmdline = f"{slurm_cmd} {wrapper_script} {exe_path} {full_knobs_quoted}"
        commands.append((trace_name, exp_name, cmdline))
    
    # Execute or print commands
    original_dir = os.getcwd()
    os.chdir(output_dir)
    
    for i, (trace_name, exp_name, cmd) in enumerate(commands, 1):
        if dry_run:
            print(f"  [{i}/{len(commands)}] {trace_name} + {exp_name}")
            print(f"      {cmd}")
        else:
            print(f"  [{i}/{len(commands)}] Relaunching {trace_name} + {exp_name}")
            subprocess.run(cmd, shell=True)
    
    os.chdir(original_dir)
    
    if not dry_run:
        print(f"\n{'='*60}")
        print(f"Relaunched {len(failed)} experiments")
        print(f"Output directory: {output_dir}")
    
    return len(failed)


def main():
    """Command-line interface for relaunch."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Relaunch failed experiments')
    parser.add_argument('cd', choices=['Fig5a', 'Fig5b', 'Fig5c', 'Fig5d'],
                       help='Experiment configuration (Fig5a-Fig5d)')
    parser.add_argument('--input-dir', dest='input_dir',
                       help='Directory containing output files (default: $ATHENA_HOME/experiments/<CD>)')
    parser.add_argument('--exe', default='$ATHENA_HOME/bin/champsim',
                       help='Path to executable (default: $ATHENA_HOME/bin/champsim)')
    parser.add_argument('--ncores', type=int, default=DEFAULT_NCORES,
                       help=f'Number of cores (default: {DEFAULT_NCORES})')
    parser.add_argument('--partition', default=DEFAULT_PARTITION,
                       help=f'Slurm partition (default: {DEFAULT_PARTITION})')
    parser.add_argument('--hostname', default=DEFAULT_HOSTNAME,
                       help=f'Hostname prefix (default: {DEFAULT_HOSTNAME})')
    parser.add_argument('--extra', help='Extra Slurm options')
    parser.add_argument('--ext', default='out', help='Extension of output files (default: out)')
    parser.add_argument('--dry-run', dest='dry_run', action='store_true',
                       help='Print commands without executing')
    
    args = parser.parse_args()
    
    relaunch_experiments(
        cd=args.cd,
        input_dir=args.input_dir,
        exe=args.exe,
        ncores=args.ncores,
        partition=args.partition,
        hostname=args.hostname,
        extra=args.extra,
        ext=args.ext,
        dry_run=args.dry_run
    )


if __name__ == '__main__':
    main()


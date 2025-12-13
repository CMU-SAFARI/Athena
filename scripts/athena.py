#!/usr/bin/env python3
"""
Athena: Main entry point for experiment management system.
Provides unified interface for launching, visualizing, summarizing, and relaunching experiments.
All resources come from config.py - no external file dependencies.
"""

import argparse
import sys

from config import (DEFAULT_NCORES, DEFAULT_PARTITION, DEFAULT_HOSTNAME,
                    WorkloadType, EXPERIMENTS)
from generate import generate_jobs
from visualize import visualize_results
from rollup import rollup_stats
from relaunch import relaunch_experiments


def launch_experiments(args):
    """Launch experiments for a specific Cache Design."""
    # Parse workload types if specified
    workload_types = None
    if hasattr(args, 'workload_types') and args.workload_types:
        type_map = {
            'SPEC': WorkloadType.SPEC,
            'PARSEC': WorkloadType.PARSEC,
            'LIGRA': WorkloadType.LIGRA,
            'CVP': WorkloadType.CVP,
        }
        workload_types = [type_map[t.upper()] for t in args.workload_types if t.upper() in type_map]
    
    try:
        generate_jobs(
            exe=args.exe,
            cd=args.cd,
            ncores=args.ncores,
            partition=args.partition,
            hostname=args.hostname,
            extra=args.extra,
            workload_types=workload_types
        )
    except Exception as e:
        print(f"Error generating jobs: {e}", file=sys.stderr)
        sys.exit(1)


def visualize_experiments(args):
    """Visualize experiment results."""
    try:
        visualize_results(
            cd=args.cd,
            csv_path=args.csv,
            output=args.output
        )
    except Exception as e:
        print(f"Error visualizing results: {e}", file=sys.stderr)
        sys.exit(1)


def summarize_experiments(args):
    """Summarize (rollup) statistics from experiment outputs."""
    try:
        rollup_stats(
            cd=args.cd,
            input_dir=args.input_dir,
            ext=args.ext,
            output_csv=args.output
        )
    except Exception as e:
        print(f"Error summarizing statistics: {e}", file=sys.stderr)
        sys.exit(1)


def relaunch_failed(args):
    """Relaunch failed experiments."""
    try:
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
    except Exception as e:
        print(f"Error relaunching experiments: {e}", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='Athena: Experiment launcher, visualizer, and management tool for HPCA experiments',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Launch Fig5a experiments (submits to $ATHENA_HOME/experiments/Fig5a/)
  python athena.py -L Fig5a --exe $ATHENA_HOME/bin/champsim

  # Launch Fig5a experiments filtered by workload type
  python athena.py -L Fig5a --exe $ATHENA_HOME/bin/champsim --workload-types SPEC PARSEC

  # Summarize results (output to $ATHENA_HOME/experiments/results/Fig5c.csv)
  python athena.py -S Fig5c
  
  # Summarize with custom input/output directories
  python athena.py -S Fig5d --input-dir /path/to/outputs --output /path/to/results.csv

  # Relaunch failed experiments
  python athena.py -R Fig5d
  
  # Dry-run relaunch (show commands without executing)
  python athena.py -R Fig5d --dry-run

  # Visualize Fig5a results (reads $ATHENA_HOME/experiments/results/Fig5a.csv,
  #                          outputs $ATHENA_HOME/experiments/results/Fig5a.png)
  python athena.py -V Fig5a
  
  # Visualize with custom CSV input
  python athena.py -V Fig5a --csv /path/to/results.csv
        """
    )
    
    # Main mode selection
    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument('-L', '--launch', dest='mode', action='store_const', const='launch',
                           help='Launch experiments mode')
    mode_group.add_argument('-S', '--summarize', dest='mode', action='store_const', const='summarize',
                           help='Summarize (rollup) statistics mode')
    mode_group.add_argument('-R', '--relaunch', dest='mode', action='store_const', const='relaunch',
                           help='Relaunch failed experiments mode')
    mode_group.add_argument('-V', '--visualize', dest='mode', action='store_const', const='visualize',
                           help='Visualize results mode')
    
    # Experiment selection (required for all modes)
    parser.add_argument('cd', choices=['Fig5a', 'Fig5b', 'Fig5c', 'Fig5d', 'Fig6b', 'Fig6c', 'Fig6d', 'Fig7a', 'Fig7b', 'Fig8'],
                       help='Experiment configuration to use (Fig5a-Fig5d, Fig6b-Fig6d, Fig7a-Fig7b, Fig8)')
    
    # Launch/Relaunch arguments
    parser.add_argument('--exe', default='$ATHENA_HOME/bin/champsim',
                       help='Path to executable (default: $ATHENA_HOME/bin/champsim)')
    parser.add_argument('--workload-types', dest='workload_types', nargs='+',
                       choices=['SPEC', 'PARSEC', 'LIGRA', 'CVP'],
                       help='Filter traces by workload type (launch mode only)')
    parser.add_argument('--ncores', type=int, default=DEFAULT_NCORES,
                       help=f'Number of cores (default: {DEFAULT_NCORES})')
    parser.add_argument('--partition', default=DEFAULT_PARTITION,
                       help=f'Slurm partition (default: {DEFAULT_PARTITION})')
    parser.add_argument('--hostname', default=DEFAULT_HOSTNAME,
                       help=f'Hostname prefix (default: {DEFAULT_HOSTNAME})')
    parser.add_argument('--extra', help='Extra Slurm options')
    
    # Relaunch-specific arguments
    parser.add_argument('--dry-run', dest='dry_run', action='store_true',
                       help='Print relaunch commands without executing (relaunch mode only)')
    
    # Visualization-specific arguments
    parser.add_argument('--csv', 
                       help='Path to CSV file for visualization (default: $ATHENA_HOME/experiments/results/<exp>.csv)')
    
    # Summarize/Relaunch arguments
    parser.add_argument('--input-dir', dest='input_dir',
                       help='Directory containing .out files (default: $ATHENA_HOME/experiments/<exp>)')
    parser.add_argument('--ext', default='out', help='Extension of output files (default: out)')
    
    # Shared output argument
    parser.add_argument('--output', '-o', 
                       help='Output file path (default: $ATHENA_HOME/experiments/results/<exp>.csv or .png)')
    
    args = parser.parse_args()
    
    # Route to appropriate function
    if args.mode == 'launch':
        launch_experiments(args)
    elif args.mode == 'summarize':
        summarize_experiments(args)
    elif args.mode == 'relaunch':
        relaunch_failed(args)
    elif args.mode == 'visualize':
        visualize_experiments(args)
    else:
        parser.error("Must specify one of: -L (launch), -S (summarize), -R (relaunch), -V (visualize)")


if __name__ == '__main__':
    main()

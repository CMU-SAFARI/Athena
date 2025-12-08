"""
Rollup module for Athena.
Aggregates statistics from experiment output files into CSV format.
All resources come from config.py - no external file dependencies.
"""

import os
import statistics
from typing import List, Dict, Optional, Tuple, Set
from config import (TRACE_DATA, EXPERIMENTS, DEFAULT_METRICS, DEFAULT_RESULTS_DIR,
                    get_all_trace_names, get_athena_home)


def trim(s: str) -> str:
    """Trim whitespace from string."""
    return s.strip()


def calculate_statistic(values: List[str], metric_type: str):
    """Calculate statistic based on metric type. Returns float or str for array type."""
    if not values:
        return 0.0 if metric_type != "array" else ""
    
    if metric_type == "array":
        # Return as-is (comma-separated)
        return ','.join(str(v).strip() for v in values if str(v).strip())
    
    # Filter out empty strings and convert to float
    numeric_values = []
    for v in values:
        v = trim(str(v))
        if v:
            try:
                numeric_values.append(float(v))
            except ValueError:
                continue
    
    if not numeric_values:
        return 0.0
    
    if metric_type == "sum":
        return sum(numeric_values)
    elif metric_type == "mean":
        return statistics.mean(numeric_values)
    elif metric_type == "nzmean":
        # Non-zero mean
        non_zero = [v for v in numeric_values if v != 0]
        if non_zero:
            return statistics.mean(non_zero)
        return 0.0
    elif metric_type == "min":
        return min(numeric_values)
    elif metric_type == "max":
        return max(numeric_values)
    elif metric_type == "standard_deviation":
        if len(numeric_values) > 1:
            return statistics.stdev(numeric_values)
        return 0.0
    elif metric_type == "variance":
        if len(numeric_values) > 1:
            return statistics.variance(numeric_values)
        return 0.0
    else:
        raise ValueError(f"Invalid metric type: {metric_type}")


def parse_output_file(log_file: str, ext: str) -> Dict[str, str]:
    """Parse output file and extract key-value pairs."""
    records = {}
    
    if not os.path.exists(log_file):
        return records
    
    with open(log_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            
            if ext == "stats":
                # Format: key=value
                if '=' in line:
                    key, value = line.split('=', 1)
                    records[trim(key)] = trim(value)
            else:
                # Format: key value (space-separated, exactly one space)
                space_count = line.count(' ')
                if space_count == 1:
                    parts = line.split(' ', 1)
                    if len(parts) == 2:
                        key = trim(parts[0])
                        value = trim(parts[1])
                        records[key] = value
    
    return records


def get_results_dir() -> str:
    """Get the results directory path ($ATHENA_HOME/experiments/results)."""
    athena_home = get_athena_home()
    return os.path.join(athena_home, DEFAULT_RESULTS_DIR)


def get_default_csv_path(cd: str) -> str:
    """Get the default CSV path for a cache design ($ATHENA_HOME/experiments/results/<CD>.csv)."""
    return os.path.join(get_results_dir(), f"{cd}.csv")


def rollup_stats(cd: str, input_dir: Optional[str] = None,
                 metrics: Optional[List[Dict[str, str]]] = None,
                 ext: str = "out",
                 output_csv: Optional[str] = None) -> Tuple[str, List[Tuple[str, str]]]:
    """
    Rollup statistics using trace and experiment info from config.py.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d)
        input_dir: Directory containing output files (default: $ATHENA_HOME/experiments/<exp>)
        metrics: List of metric dicts (default: DEFAULT_METRICS from config)
        ext: Extension of output files (default: "out")
        output_csv: Path to output CSV file (default: $ATHENA_HOME/experiments/results/<exp>.csv)
    
    Returns:
        Tuple of (CSV content as string, list of failed (trace, exp) tuples)
    """
    if cd not in EXPERIMENTS:
        raise ValueError(f"Unknown experiment: {cd}. Must be one of: {list(EXPERIMENTS.keys())}")
    
    athena_home = get_athena_home()
    
    # Determine input directory (where .out files are)
    if input_dir is None:
        input_dir = os.path.join(athena_home, "experiments", cd)
    
    # Determine output CSV path
    if output_csv is None:
        results_dir = get_results_dir()
        os.makedirs(results_dir, exist_ok=True)
        output_csv = get_default_csv_path(cd)
    
    # Get trace names from config
    trace_names = get_all_trace_names()
    
    # Get experiment names for this experiment config
    exp_names = list(EXPERIMENTS[cd]['experiments'].keys())
    
    # Use default metrics if not provided
    if metrics is None:
        metrics = DEFAULT_METRICS
    
    # Track failed experiments
    failed_experiments: List[Tuple[str, str]] = []
    
    # Prepare CSV output
    csv_lines = []
    
    # Header
    header = ["Trace", "Exp"]
    for metric in metrics:
        header.append(metric['NAME'])
    header.append("Filter")
    csv_lines.append(header)
    
    # Counters for summary
    total_jobs = 0
    successful_jobs = 0
    
    # Process each trace
    for trace_name in trace_names:
        per_trace_result = {}
        per_trace_success = {}
        
        for exp_name in exp_names:
            total_jobs += 1
            
            # Look for output file in experiment-specific directory first
            log_file = os.path.join(input_dir, exp_name, f"{trace_name}_{exp_name}.{ext}")
            if not os.path.exists(log_file):
                # Fallback to input_dir root with trace-exp naming
                log_file = os.path.join(input_dir, f"{trace_name}_{exp_name}.{ext}")
            
            metric_values = []
            records = parse_output_file(log_file, ext)
            job_success = True
            
            if records:
                # Extract metric values
                for metric in metrics:
                    metric_name = metric['NAME']
                    metric_type = metric['TYPE']
                    
                    if metric_name in records:
                        value = records[metric_name]
                        
                        if metric_type == "array":
                            # Array type: use value as-is
                            metric_values.append(value)
                        else:
                            # Parse comma-separated values and calculate statistic
                            tokens = [t.strip() for t in value.split(',') if t.strip()]
                            stat_value = calculate_statistic(tokens, metric_type)
                            metric_values.append(stat_value)
                    else:
                        # Metric not found
                        metric_values.append(0)
                        job_success = False
            else:
                # File doesn't exist or is empty
                job_success = False
                for metric in metrics:
                    metric_values.append(0)
            
            per_trace_result[exp_name] = metric_values
            per_trace_success[exp_name] = job_success
            
            if job_success:
                successful_jobs += 1
            else:
                failed_experiments.append((trace_name, exp_name))
        
        # Determine if all experiments for this trace passed
        all_exps_passed = all(per_trace_success.values())
        
        # Write results for this trace
        for exp_name in exp_names:
            metric_values = per_trace_result[exp_name]
            filter_value = 1 if all_exps_passed else 0
            
            row = [trace_name, exp_name] + [str(v) for v in metric_values] + [str(filter_value)]
            csv_lines.append(row)
    
    # Convert to CSV string
    output = []
    for row in csv_lines:
        output.append(','.join(row))
    
    csv_content = '\n'.join(output)
    
    # Write to file
    with open(output_csv, 'w') as f:
        f.write(csv_content)
    
    # Print summary
    print(f"\n{'='*60}")
    print(f"Rollup Summary for {cd}")
    print(f"{'='*60}")
    print(f"Total experiments: {total_jobs}")
    print(f"Successful: {successful_jobs}")
    print(f"Failed: {len(failed_experiments)}")
    print(f"Success rate: {successful_jobs/total_jobs*100:.1f}%")
    print(f"{'='*60}")
    print(f"CSV written to: {output_csv}")
    
    if failed_experiments:
        print(f"\nTo relaunch failed experiments, run:")
        print(f"  python athena.py -R {cd}")
    
    return csv_content, failed_experiments


def main():
    """Command-line interface for rollup."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Rollup statistics from experiment outputs')
    parser.add_argument('cd', choices=['Fig5a', 'Fig5b', 'Fig5c', 'Fig5d'],
                       help='Experiment configuration (Fig5a-Fig5d)')
    parser.add_argument('--input-dir', dest='input_dir',
                       help='Directory containing output files (default: $ATHENA_HOME/experiments/<CD>)')
    parser.add_argument('--ext', default='out', help='Extension of output files (default: out)')
    parser.add_argument('--output', '-o', 
                       help='Output CSV file path (default: $ATHENA_HOME/experiments/results/<CD>.csv)')
    
    args = parser.parse_args()
    
    rollup_stats(
        cd=args.cd,
        input_dir=args.input_dir,
        ext=args.ext,
        output_csv=args.output
    )


if __name__ == '__main__':
    main()

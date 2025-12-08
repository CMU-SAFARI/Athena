"""
Visualization module for Athena.
Standalone module for visualizing experiment results.
"""

import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path
from typing import Optional, Dict

from config import EXPERIMENTS, DEFAULT_RESULTS_DIR, TRACE_DATA, WorkloadType, get_athena_home

# Display names for categories (shorter for plot labels)
CATEGORY_DISPLAY_NAMES = {
    WorkloadType.SPEC: 'SPEC',
    WorkloadType.PARSEC: 'PARSEC',
    WorkloadType.LIGRA: 'Ligra',
    WorkloadType.CVP: 'CVP',
    'Prefetcher-adverse': 'Prefetcher-adverse',
    'Prefetcher-friendly': 'Prefetcher-friendly',
    'Overall': 'Overall',
}


def get_results_dir() -> str:
    """Get the results directory path ($ATHENA_HOME/experiments/results)."""
    athena_home = get_athena_home()
    return os.path.join(athena_home, DEFAULT_RESULTS_DIR)


def get_default_csv_path(cd: str) -> str:
    """Get the default CSV path for a cache design ($ATHENA_HOME/experiments/results/<CD>.csv)."""
    return os.path.join(get_results_dir(), f"{cd}.csv")


def get_default_png_path(cd: str) -> str:
    """Get the default PNG path for a cache design ($ATHENA_HOME/experiments/results/<CD>.png)."""
    return os.path.join(get_results_dir(), f"{cd}.png")


def load_and_filter_data(csv_path: str, config_type: str) -> pd.DataFrame:
    """Load CSV data and filter based on configuration type and completion status."""
    try:
        df = pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"Error: CSV file not found at {csv_path}")
        sys.exit(1)
    
    # Filter for completed experiments (Filter = 1)
    df = df[df['Filter'] == 1]
    
    # Get experiments for this configuration from EXPERIMENTS
    config_experiments = list(EXPERIMENTS[config_type]['experiments'].keys())
    
    # Filter data for this configuration's experiments
    df_filtered = df[df['Exp'].isin(config_experiments)]
    
    return df_filtered


def calculate_category_speedups(df: pd.DataFrame, config_type: str) -> Dict[str, Dict[str, float]]:
    """Calculate speedup by trace category using TRACE_DATA from config.py."""
    results = {}
    
    # Workload type categories from config.py
    workload_categories = [WorkloadType.SPEC, WorkloadType.PARSEC, 
                           WorkloadType.LIGRA, WorkloadType.CVP]
    
    # Get baseline data from the full dataset (not filtered by config)
    baseline_data = df[df['Exp'] == 'Baseline'].set_index('Trace')['Core_0_cumulative_IPC']
    
    # Calculate speedup for each experiment by category
    for exp in EXPERIMENTS[config_type]['experiments'].keys():
        if exp == 'Baseline':
            continue
            
        exp_data = df[df['Exp'] == exp]
        if exp_data.empty:
            continue
            
        category_speedups = {}
        
        # Initialize category lists for workload types
        for category in workload_categories:
            category_speedups[category] = []
        
        # Categorize by workload type using TRACE_DATA
        for _, row in exp_data.iterrows():
            trace = row['Trace']
            trace_category = TRACE_DATA[trace]['type'] if trace in TRACE_DATA else 'Other'
            
            if trace_category in workload_categories and trace in baseline_data.index:
                baseline_ipc = baseline_data[trace]
                exp_ipc = row['Core_0_cumulative_IPC']
                speedup = exp_ipc / baseline_ipc
                category_speedups[trace_category].append(speedup)
        
        # Handle sensitivity-based categories (using 'adverse' field from TRACE_DATA)
        category_speedups['Prefetcher-adverse'] = []
        category_speedups['Prefetcher-friendly'] = []
        
        for _, row in exp_data.iterrows():
            trace = row['Trace']
            if trace not in TRACE_DATA:
                continue
            sensitivity_category = 'Prefetcher-adverse' if TRACE_DATA[trace]['adverse'] else 'Prefetcher-friendly'
            
            if trace in baseline_data.index:
                baseline_ipc = baseline_data[trace]
                exp_ipc = row['Core_0_cumulative_IPC']
                speedup = exp_ipc / baseline_ipc
                category_speedups[sensitivity_category].append(speedup)
        
        # Calculate geometric mean for each category
        category_geomeans = {}
        for category, speedups in category_speedups.items():
            if speedups:
                category_geomeans[category] = np.exp(np.mean(np.log(speedups)))
            else:
                category_geomeans[category] = 1.0  # No speedup if no data
        
        # Calculate overall geometric mean
        all_speedups = []
        for _, row in exp_data.iterrows():
            trace = row['Trace']
            if trace in baseline_data.index:
                baseline_ipc = baseline_data[trace]
                exp_ipc = row['Core_0_cumulative_IPC']
                speedup = exp_ipc / baseline_ipc
                all_speedups.append(speedup)
        
        if all_speedups:
            category_geomeans['Overall'] = np.exp(np.mean(np.log(all_speedups)))
        else:
            category_geomeans['Overall'] = 1.0
        
        results[exp] = category_geomeans
    
    return results


def create_bar_plot(category_speedups: Dict[str, Dict[str, float]], config_type: str):
    """Create bar plot showing speedup by category using WorkloadType from config.py."""
    config_info = EXPERIMENTS[config_type]
    
    # Prepare data for plotting - use WorkloadType enum values and special categories
    experiments = list(category_speedups.keys())
    categories = [WorkloadType.SPEC, WorkloadType.PARSEC, WorkloadType.LIGRA, 
                  WorkloadType.CVP, 'Prefetcher-adverse', 'Prefetcher-friendly', 'Overall']
    
    # Get display names for x-axis labels
    category_labels = [CATEGORY_DISPLAY_NAMES.get(cat, str(cat)) for cat in categories]
    
    # Create data matrix
    data_matrix = []
    for exp in experiments:
        row = []
        for cat in categories:
            if cat in category_speedups[exp]:
                row.append(category_speedups[exp][cat])
            else:
                row.append(1.0)  # No speedup if no data
        data_matrix.append(row)
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(14, 8))
    
    # Set up bar positions
    x = np.arange(len(categories))
    width = 0.8 / len(experiments)
    
    # Create bars for each experiment
    colors = plt.cm.Set3(np.linspace(0, 1, len(experiments)))
    
    for i, exp in enumerate(experiments):
        bars = ax.bar(x + i * width, data_matrix[i], width, 
                     label=exp, color=colors[i], alpha=0.8)
    
    # Customize the plot
    ax.set_xlabel('Workload Categories', fontsize=12)
    ax.set_ylabel('Geometric Mean Speedup', fontsize=12)
    ax.set_title(f'{config_type} ({config_info["description"]}) - Performance Speedup by Category', 
                fontsize=14, fontweight='bold')
    ax.set_xticks(x + width * (len(experiments) - 1) / 2)
    ax.set_xticklabels(category_labels, rotation=45, ha='right')
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    ax.grid(True, alpha=0.3)
    ax.axhline(y=1.0, color='red', linestyle='--', alpha=0.7, label='Baseline')
    
    # Set y-axis limits
    ax.set_ylim(0.7, 1.3)
    
    plt.tight_layout()
    return fig


def visualize_results(cd: str, csv_path: Optional[str] = None, output: Optional[str] = None):
    """
    Visualize experiment results for a specific experiment configuration.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d)
        csv_path: Path to CSV file with results (default: $ATHENA_HOME/experiments/results/<exp>.csv)
        output: Path to save the plot (default: $ATHENA_HOME/experiments/results/<exp>.png)
    """
    if cd not in EXPERIMENTS:
        raise ValueError(f"Invalid experiment: {cd}. Must be one of: {list(EXPERIMENTS.keys())}")
    
    # Determine CSV path
    if csv_path is None:
        csv_path = get_default_csv_path(cd)
        print(f"Using default CSV: {csv_path}")
    
    # Determine output path
    if output is None:
        output = get_default_png_path(cd)
    
    # Ensure results directory exists
    os.makedirs(os.path.dirname(output), exist_ok=True)
    
    # Load full dataset first (needed for baseline comparison)
    print(f"Loading full dataset...")
    try:
        df_full = pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"Error: CSV file not found at {csv_path}")
        sys.exit(1)
    
    # Filter for completed experiments (Filter = 1)
    df_full = df_full[df_full['Filter'] == 1]
    
    # Load and filter data for specific configuration
    print(f"Loading data for {cd} configuration...")
    df = load_and_filter_data(csv_path, cd)
    
    if df.empty:
        print(f"No data found for configuration {cd}")
        sys.exit(1)
    
    # Count successful traces (traces where all experiments finished)
    successful_traces = df['Trace'].nunique()
    print(f"Found {successful_traces} successful traces for {cd}")
    
    # Calculate speedups by category (using full dataset for baseline)
    print("Calculating speedups by category...")
    category_speedups = calculate_category_speedups(df_full, cd)
    
    if not category_speedups:
        print("No speedup data calculated. Check if experiments are complete.")
        sys.exit(1)
    
    # Create and display plot
    print("Creating visualization...")
    fig = create_bar_plot(category_speedups, cd)
    
    # Save plot
    fig.savefig(output, dpi=300, bbox_inches='tight')
    print(f"Plot saved to {output}")
    
    # Print summary statistics
    print(f"\nSummary for {cd} ({EXPERIMENTS[cd]['description']}):")
    print("=" * 60)
    
    for exp, speeds in category_speedups.items():
        print(f"\n{exp}:")
        for cat, speedup in speeds.items():
            # Use display name for category
            cat_name = CATEGORY_DISPLAY_NAMES.get(cat, str(cat))
            print(f"  {cat_name}: {speedup:.3f}x")


def main():
    """Command-line interface for visualization."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Visualize HPCA experiment performance data')
    parser.add_argument('cd', choices=['Fig5a', 'Fig5b', 'Fig5c', 'Fig5d'],
                       help='Experiment configuration to visualize')
    parser.add_argument('--csv', 
                       help='Path to CSV file (default: $ATHENA_HOME/experiments/results/<CD>.csv)')
    parser.add_argument('--output', '-o',
                       help='Output PNG file path (default: $ATHENA_HOME/experiments/results/<CD>.png)')
    
    args = parser.parse_args()
    
    visualize_results(args.cd, args.csv, args.output)


if __name__ == '__main__':
    main()

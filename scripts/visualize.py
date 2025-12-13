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
from typing import Optional, Dict, List

from config import EXPERIMENTS, DEFAULT_RESULTS_DIR, TRACE_DATA, WorkloadType, get_athena_home


def is_sensitivity_study(config_type: str) -> bool:
    """Check if the configuration is a sensitivity study."""
    return EXPERIMENTS.get(config_type, {}).get('sensitivity_type') == 'grouped'

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


def calculate_sensitivity_speedups(df: pd.DataFrame, config_type: str) -> Dict[str, Dict[str, float]]:
    """
    Calculate speedup for sensitivity studies.
    Returns a dict: {category: {experiment_label: geomean_speedup}}
    
    Supports per-category baselines via 'category_baselines' field in config.
    If not present, falls back to a single 'Baseline' experiment.
    """
    config_info = EXPERIMENTS[config_type]
    categories = config_info['categories']
    category_suffixes = config_info['category_suffix']
    experiment_labels = config_info['experiment_labels']
    
    # Check if per-category baselines are defined
    category_baselines = config_info.get('category_baselines', None)
    
    # If no per-category baselines, use single global baseline
    if category_baselines is None:
        baseline_data_map = {
            cat: df[df['Exp'] == 'Baseline'].set_index('Trace')['Core_0_cumulative_IPC']
            for cat in categories
        }
    else:
        # Build per-category baseline data
        baseline_data_map = {}
        for cat_idx, category in enumerate(categories):
            baseline_name = category_baselines[cat_idx]
            baseline_df = df[df['Exp'] == baseline_name]
            if not baseline_df.empty:
                baseline_data_map[category] = baseline_df.set_index('Trace')['Core_0_cumulative_IPC']
            else:
                # Fallback to empty series if baseline not found
                baseline_data_map[category] = pd.Series(dtype=float)
    
    results = {}
    
    for cat_idx, category in enumerate(categories):
        suffix = category_suffixes[cat_idx]
        results[category] = {}
        baseline_data = baseline_data_map[category]
        
        for exp_label in experiment_labels:
            # Construct the full experiment name
            exp_name = f"{exp_label}{suffix}"
            
            exp_data = df[df['Exp'] == exp_name]
            if exp_data.empty:
                results[category][exp_label] = 1.0
                continue
            
            # Calculate speedup over baseline for all traces
            speedups = []
            for _, row in exp_data.iterrows():
                trace = row['Trace']
                if trace in baseline_data.index:
                    baseline_ipc = baseline_data[trace]
                    exp_ipc = row['Core_0_cumulative_IPC']
                    speedup = exp_ipc / baseline_ipc
                    speedups.append(speedup)
            
            # Calculate geometric mean
            if speedups:
                results[category][exp_label] = np.exp(np.mean(np.log(speedups)))
            else:
                results[category][exp_label] = 1.0
    
    return results


def create_sensitivity_bar_plot(category_speedups: Dict[str, Dict[str, float]], config_type: str):
    """
    Create grouped bar plot for sensitivity studies.
    X-axis: categories (e.g., 6 cycles, 18 cycles, 30 cycles)
    Bars within each group: experiment labels (e.g., POPET, Pythia, Naive, etc.)
    """
    config_info = EXPERIMENTS[config_type]
    categories = config_info['categories']
    experiment_labels = config_info['experiment_labels']
    
    # Create the plot
    fig, ax = plt.subplots(figsize=(14, 8))
    
    # Set up bar positions
    x = np.arange(len(categories))
    n_experiments = len(experiment_labels)
    width = 0.8 / n_experiments
    
    # Define colors for experiments
    colors = plt.cm.Set2(np.linspace(0, 1, n_experiments))
    
    # Create bars for each experiment label across categories
    for i, exp_label in enumerate(experiment_labels):
        values = [category_speedups[cat].get(exp_label, 1.0) for cat in categories]
        offset = (i - n_experiments / 2 + 0.5) * width
        bars = ax.bar(x + offset, values, width, 
                     label=exp_label, color=colors[i], alpha=0.85, edgecolor='black', linewidth=0.5)
        
        # Add value labels on bars
        for bar, val in zip(bars, values):
            height = bar.get_height()
            ax.annotate(f'{val:.3f}',
                       xy=(bar.get_x() + bar.get_width() / 2, height),
                       xytext=(0, 3),
                       textcoords="offset points",
                       ha='center', va='bottom', fontsize=7, rotation=90)
    
    # Customize the plot
    ax.set_xlabel('Sensitivity Parameter', fontsize=12, fontweight='bold')
    ax.set_ylabel('Geometric Mean Speedup', fontsize=12, fontweight='bold')
    ax.set_title(f'{config_type}: {config_info["description"]}', 
                fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(categories, fontsize=11)
    ax.legend(bbox_to_anchor=(1.02, 1), loc='upper left', fontsize=10)
    ax.grid(True, alpha=0.3, axis='y')
    ax.axhline(y=1.0, color='red', linestyle='--', alpha=0.7, linewidth=1.5)
    
    # Set y-axis limits with some padding
    all_values = [v for cat_data in category_speedups.values() for v in cat_data.values()]
    if all_values:
        y_min = min(0.9, min(all_values) - 0.05)
        y_max = max(1.1, max(all_values) + 0.1)
        ax.set_ylim(y_min, y_max)
    
    plt.tight_layout()
    return fig


def visualize_results(cd: str, csv_path: Optional[str] = None, output: Optional[str] = None):
    """
    Visualize experiment results for a specific experiment configuration.
    
    Args:
        cd: Experiment configuration (Fig5a-Fig5d, Fig6b-Fig6d)
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
    
    # Check if this is a sensitivity study
    if is_sensitivity_study(cd):
        # Sensitivity study: group by parameter categories
        print("Calculating sensitivity study speedups...")
        category_speedups = calculate_sensitivity_speedups(df_full, cd)
        
        if not category_speedups:
            print("No speedup data calculated. Check if experiments are complete.")
            sys.exit(1)
        
        # Create sensitivity plot
        print("Creating sensitivity study visualization...")
        fig = create_sensitivity_bar_plot(category_speedups, cd)
        
        # Save plot
        fig.savefig(output, dpi=300, bbox_inches='tight')
        print(f"Plot saved to {output}")
        
        # Print summary statistics for sensitivity study
        config_info = EXPERIMENTS[cd]
        print(f"\nSummary for {cd} ({config_info['description']}):")
        print("=" * 60)
        
        for category, exp_data in category_speedups.items():
            print(f"\n{category}:")
            for exp_label, speedup in exp_data.items():
                print(f"  {exp_label}: {speedup:.3f}x")
    else:
        # Regular experiment: group by workload categories
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
    parser.add_argument('cd', choices=list(EXPERIMENTS.keys()),
                       help='Experiment configuration to visualize (Fig5a-Fig5d for main results, Fig6b-Fig6d for sensitivity studies)')
    parser.add_argument('--csv', 
                       help='Path to CSV file (default: $ATHENA_HOME/experiments/results/<CD>.csv)')
    parser.add_argument('--output', '-o',
                       help='Output PNG file path (default: $ATHENA_HOME/experiments/results/<CD>.png)')
    
    args = parser.parse_args()
    
    visualize_results(args.cd, args.csv, args.output)


if __name__ == '__main__':
    main()

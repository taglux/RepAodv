#!/usr/bin/env python3
import argparse
import sys
import os
import pandas as pd

def parse_args():
    parser = argparse.ArgumentParser(description="Hybrid OMNeT++ Log Analyzer for debugging")
    parser.add_argument("run_prefix", help="Run prefix, e.g., AttackTestRA1-#0")
    parser.add_argument("--csv", default="simulations/result/simulation_log.csv", help="Path to simulation_log.csv")
    parser.add_argument("--results-dir", default="simulations/results", help="Path to .sca/.vec directory")
    return parser.parse_args()

def parse_csv_events(csv_path, run_name_filter=None):
    events = []
    if not os.path.exists(csv_path):
        print(f"Warning: {csv_path} not found.")
        return events
        
    with open(csv_path, 'r') as f:
        for line in f:
            parts = line.strip().split(';')
            if len(parts) >= 5 and "timestamp" not in parts[0]:
                events.append({
                    "time": float(parts[0]),
                    "node": parts[1],
                    "role": parts[2],
                    "event": parts[3],
                    "details": parts[4]
                })
    return pd.DataFrame(events)

def extract_trust_timeline(events_df):
    if events_df.empty:
        return {}
    trust_events = events_df[events_df['event'] == 'TRUST_STATE_CHANGE']
    timeline = {}
    for _, row in trust_events.iterrows():
        node = row['node']
        if node not in timeline:
            timeline[node] = []
        timeline[node].append((row['time'], row['details']))
    return timeline

def extract_scave_metrics(results_dir, run_prefix):
    try:
        from omnetpp.scave import results as res
    except ImportError:
        print("Error: omnetpp.scave not found.")
        return pd.DataFrame()
        
    sca_path = os.path.join(results_dir, f"{run_prefix}.sca")
    if not os.path.exists(sca_path):
        print(f"Warning: Scave file {sca_path} not found.")
        return pd.DataFrame()

    try:
        res.add_inputs(sca_path)
        df = res.get_scalars('name =~ "*packetDrop*"')
        if df.empty:
            return df
            
        df = df[df['name'].str.contains(':count')]
        df = df[~df['name'].str.contains('NotAddressedToUs')]
        df = df[df['value'] > 0]
        df['name'] = df['name'].str.replace(':count', '', regex=False)
        return df
    except Exception as e:
        print(f"Scave extraction error: {e}")
        return pd.DataFrame()

def generate_markdown_report(run_prefix, trust_timeline, scave_df):
    report = f"# Debug Report: {run_prefix}\n\n"
    
    report += "## Critical Metrics (Packet Drops Breakdown)\n"
    if not scave_df.empty and 'module' in scave_df.columns:
        for module, group in scave_df.groupby('module'):
            report += f"### {module}\n"
            for _, row in group.iterrows():
                report += f"- **{row['name']}**: {int(row['value'])} packets\n"
    else:
        report += "No critical drops found (this is good!).\n"
        
    report += "\n## Trust Timeline (from CSV)\n"
    if not trust_timeline:
        report += "No trust events recorded.\n"
    else:
        for node, events in trust_timeline.items():
            report += f"### {node}\n"
            for time, detail in events:
                report += f"- `[{time:.3f}s]` {detail}\n"
                
    return report

def main():
    args = parse_args()
    print(f"Analyzing run: {args.run_prefix}")
    
    events_df = parse_csv_events(args.csv)
    trust_timeline = extract_trust_timeline(events_df)
    
    scave_df = extract_scave_metrics(args.results_dir, args.run_prefix)
    
    report = generate_markdown_report(args.run_prefix, trust_timeline, scave_df)
    
    out_file = f"simulations/result/{args.run_prefix}_debug_report.md"
    with open(out_file, "w") as f:
        f.write(report)
    print(f"Report written to {out_file}")

if __name__ == "__main__":
    main()

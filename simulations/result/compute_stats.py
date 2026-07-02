#!/usr/bin/env python3
"""
Read raw per-run metrics CSV and output mean ± std per config.

Input CSV columns:  Config,Run,PDR,Overhead,AvgDelayMs,DetectionTime
Output CSV columns: Config,PDR_mean,PDR_std,Overhead_mean,Overhead_std,
                    AvgDelayMs_mean,AvgDelayMs_std,DetectionTime_mean,DetectionTime_std,N
"""

import csv
import sys
import math
from collections import defaultdict


def mean(vals):
    return sum(vals) / len(vals) if vals else 0.0


def std(vals):
    if len(vals) < 2:
        return 0.0
    m = mean(vals)
    return math.sqrt(sum((x - m) ** 2 for x in vals) / (len(vals) - 1))


def main():
    if len(sys.argv) < 2:
        print("Usage: compute_stats.py raw_results.csv [output.csv]", file=sys.stderr)
        sys.exit(1)

    raw_path = sys.argv[1]
    out_path = sys.argv[2] if len(sys.argv) > 2 else None

    data = defaultdict(lambda: {"pdr": [], "overhead": [], "delay": [], "detection": []})

    with open(raw_path, newline="") as f:
        reader = csv.reader(f)
        next(reader)  # skip header
        for row in reader:
            if len(row) < 4:
                continue
            cfg = row[0].strip()
            try:
                pdr = float(row[2]) if row[2].strip() else None
                overhead = float(row[3]) if row[3].strip() else None
                delay = float(row[4]) if len(row) > 4 and row[4].strip() else None
                detection = float(row[5]) if len(row) > 5 and row[5].strip() else None
            except ValueError:
                continue
            if pdr is not None:
                data[cfg]["pdr"].append(pdr)
            if overhead is not None:
                data[cfg]["overhead"].append(overhead)
            if delay is not None:
                data[cfg]["delay"].append(delay)
            if detection is not None:
                data[cfg]["detection"].append(detection)

    header = (
        "Config,PDR_mean,PDR_std,"
        "Overhead_mean,Overhead_std,"
        "AvgDelayMs_mean,AvgDelayMs_std,"
        "DetectionTime_mean,DetectionTime_std,N"
    )

    rows = []
    config_order = list(data.keys())
    for cfg in config_order:
        d = data[cfg]
        n = len(d["pdr"])
        det_vals = d["detection"]
        det_mean = f"{mean(det_vals):.3f}" if det_vals else ""
        det_std  = f"{std(det_vals):.3f}" if len(det_vals) > 1 else ""
        rows.append(
            f"{cfg},"
            f"{mean(d['pdr']):.1f},{std(d['pdr']):.2f},"
            f"{mean(d['overhead']):.3f},{std(d['overhead']):.4f},"
            f"{mean(d['delay']):.1f},{std(d['delay']):.2f},"
            f"{det_mean},{det_std},{n}"
        )

    output = header + "\n" + "\n".join(rows) + "\n"

    if out_path:
        with open(out_path, "w") as f:
            f.write(output)
        print(f"Written to {out_path}")
    else:
        print(output)


if __name__ == "__main__":
    main()

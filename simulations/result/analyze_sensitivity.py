#!/usr/bin/env python3
"""
Sensitivity analysis aggregator for REP-AODV.

Reads sensitivity_raw.csv (produced by run_sensitivity.sh), computes mean±std
per (Param, Value) point, marks nominal values, and identifies the stable range
(PDR >= 95% of nominal).

Usage:
    python3 analyze_sensitivity.py [raw_csv] [output_csv]
      raw_csv    defaults to sensitivity_raw.csv in script dir
      output_csv defaults to sensitivity_results.csv in script dir
"""

import csv
import os
import sys
from collections import defaultdict
from typing import Dict, List, Optional, Tuple

# Nominal (design-point) values for each parameter
NOMINALS = {
    "ackTimeout":       "0.5s",
    "gossipThreshold":  "0.35",
    "suspectThreshold": "0.40",
    "evidenceHalfLife": "60s",
}

# Stable-range threshold: PDR must be >= this fraction of nominal PDR
STABLE_FRACTION = 0.95


def mean(xs: List[float]) -> Optional[float]:
    return sum(xs) / len(xs) if xs else None


def std(xs: List[float]) -> Optional[float]:
    if len(xs) < 2:
        return 0.0
    m = mean(xs)
    return (sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) ** 0.5


def load_raw(path: str) -> Dict[Tuple[str, str], Dict[str, List[float]]]:
    """
    Returns {(param, value): {"pdr": [...], "detection": [...]}}.
    """
    data: Dict[Tuple[str, str], Dict[str, List[float]]] = defaultdict(
        lambda: {"pdr": [], "detection": []}
    )
    if not os.path.exists(path):
        print(f"File not found: {path}", file=sys.stderr)
        return data

    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            param = row.get("Param", "").strip()
            value = row.get("Value", "").strip()
            pdr_s = row.get("PDR", "").strip()
            det_s = row.get("DetectionTime", "").strip()
            if not param or not value:
                continue
            key = (param, value)
            try:
                data[key]["pdr"].append(float(pdr_s))
            except ValueError:
                pass
            try:
                data[key]["detection"].append(float(det_s))
            except ValueError:
                pass
    return data


def build_table(
    data: Dict[Tuple[str, str], Dict[str, List[float]]]
) -> List[Dict]:
    rows = []
    for (param, value), vals in sorted(data.items()):
        pdrs = vals["pdr"]
        dets = vals["detection"]
        rows.append({
            "Param": param,
            "Value": value,
            "PDR_mean": mean(pdrs),
            "PDR_std": std(pdrs),
            "DetTime_mean": mean(dets),
            "DetTime_std": std(dets),
            "N": len(pdrs),
            "nominal": NOMINALS.get(param) == value,
        })
    return rows


def annotate_stable(rows: List[Dict]) -> None:
    """Mark rows where PDR_mean >= STABLE_FRACTION * nominal PDR for that param."""
    # Collect nominal PDR per param
    nominal_pdr: Dict[str, float] = {}
    for r in rows:
        if r["nominal"] and r["PDR_mean"] is not None:
            nominal_pdr[r["Param"]] = r["PDR_mean"]

    for r in rows:
        nom = nominal_pdr.get(r["Param"])
        if nom and r["PDR_mean"] is not None:
            r["stable"] = r["PDR_mean"] >= STABLE_FRACTION * nom
        else:
            r["stable"] = None


def _fmt(v: Optional[float], decimals: int = 1) -> str:
    return f"{v:.{decimals}f}" if v is not None else "n/a"


def print_table(rows: List[Dict]) -> None:
    current_param = None
    header = (
        f"{'Param':<18} {'Value':<8} {'PDR_mean':>8} {'PDR_std':>7}"
        f" {'DetTime_mean':>12} {'DetTime_std':>11}  {'N':>3}  {'Note'}"
    )
    sep = "-" * len(header)

    for r in rows:
        if r["Param"] != current_param:
            current_param = r["Param"]
            print()
            print(header)
            print(sep)

        note = ""
        if r["nominal"]:
            note += "* nominal"
        if r.get("stable") is False:
            note += "  !! unstable"

        print(
            f"{r['Param']:<18} {r['Value']:<8}"
            f" {_fmt(r['PDR_mean']):>8} {_fmt(r['PDR_std']):>7}"
            f" {_fmt(r['DetTime_mean'], 2):>12} {_fmt(r['DetTime_std'], 2):>11}"
            f"  {r['N']:>3}  {note}"
        )


def save_csv(rows: List[Dict], path: str) -> None:
    fields = [
        "Param", "Value", "PDR_mean", "PDR_std",
        "DetTime_mean", "DetTime_std", "N", "nominal", "stable",
    ]
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for r in rows:
            out = dict(r)
            for k in ("PDR_mean", "PDR_std", "DetTime_mean", "DetTime_std"):
                out[k] = f"{r[k]:.3f}" if r[k] is not None else ""
            writer.writerow(out)
    print(f"\nSaved: {path}")


def print_summary(rows: List[Dict]) -> None:
    print("\n=== STABLE RANGE SUMMARY ===")
    params = sorted({r["Param"] for r in rows})
    for param in params:
        param_rows = [r for r in rows if r["Param"] == param]
        stable = [r for r in param_rows if r.get("stable")]
        values = [r["Value"] for r in stable]
        nom_pdr = next((r["PDR_mean"] for r in param_rows if r["nominal"]), None)
        print(f"  {param}: stable range = {values or 'none'}"
              f"  (nominal PDR {_fmt(nom_pdr)}%,"
              f" threshold {STABLE_FRACTION*100:.0f}% = {_fmt(nom_pdr * STABLE_FRACTION if nom_pdr else None)}%)")


def main() -> None:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    args = sys.argv[1:]
    raw_path = args[0] if len(args) > 0 else os.path.join(script_dir, "sensitivity_raw.csv")
    out_path = args[1] if len(args) > 1 else os.path.join(script_dir, "sensitivity_results.csv")

    data = load_raw(raw_path)
    if not data:
        print("No data loaded. Run run_sensitivity.sh first.")
        return

    rows = build_table(data)
    annotate_stable(rows)

    print("=== SENSITIVITY ANALYSIS RESULTS ===")
    print(f"Input:  {raw_path}")
    print(f"Stable threshold: PDR >= {STABLE_FRACTION*100:.0f}% of nominal")
    print_table(rows)
    print_summary(rows)
    save_csv(rows, out_path)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Statistical verification for REP-AODV vs baseline AODV.

The input is the raw per-run CSV produced by simulations/run_all.sh:
Config,Run,PDR,Overhead,AvgDelayMs,DetectionTime

Outputs are written next to the input CSV:
  wilcoxon_results.csv
  mannwhitney_results.csv
  detection_time_results.csv
  fpr_ci.csv
  rank_details.csv
  stat_metadata.json
"""

from __future__ import annotations

import hashlib
import json
import math
import platform
import sys
import warnings
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable

import numpy as np
import pandas as pd
import scipy
from scipy import stats


RA_NUMS = [1, 2, 3, 4, 5]
ALPHA = 0.05
ALTERNATIVE = "greater"

WILCOXON_ZERO_METHOD = "wilcox"
WILCOXON_CORRECTION = False
WILCOXON_METHOD = "exact"

MANNWHITNEY_METHOD = "exact"
MANNWHITNEY_USE_CONTINUITY = False

RANK_METHOD = "average"
BOOTSTRAP_SEED = 20260529
BOOTSTRAP_SAMPLES = 20000

FPR_FALSE_POSITIVES = 0
FPR_TRIALS = 50
FPR_CONFIDENCE = 0.95


def default_raw_path() -> Path:
    candidates = [
        Path("raw_results.csv"),
        Path("result/raw_results.csv"),
        Path("simulations/result/raw_results.csv"),
    ]
    for path in candidates:
        if path.exists():
            return path
    return candidates[0]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def to_float_series(series: pd.Series) -> pd.Series:
    return pd.to_numeric(series, errors="coerce")


def holm_bonferroni(p_values: Iterable[float]) -> np.ndarray:
    p = np.asarray(list(p_values), dtype=float)
    adjusted = np.full_like(p, np.nan, dtype=float)
    valid = np.isfinite(p)
    valid_indices = np.where(valid)[0]
    m = len(valid_indices)
    if m == 0:
        return adjusted

    order = valid_indices[np.argsort(p[valid])]
    p_sorted = p[order]
    multipliers = m - np.arange(m)
    adjusted_sorted = p_sorted * multipliers
    adjusted_sorted = np.maximum.accumulate(adjusted_sorted)
    adjusted_sorted = np.clip(adjusted_sorted, 0.0, 1.0)
    adjusted[order] = adjusted_sorted
    return adjusted


def significance(p_adj: float) -> str:
    if not np.isfinite(p_adj):
        return "na"
    if p_adj < 0.001:
        return "***"
    if p_adj < 0.01:
        return "**"
    if p_adj < ALPHA:
        return "*"
    return "ns"


def has_ties(values: np.ndarray) -> bool:
    clean = values[np.isfinite(values)]
    return len(np.unique(clean)) < len(clean)


def cohens_d_paired(diff: np.ndarray) -> float:
    clean = diff[np.isfinite(diff)]
    if len(clean) < 2:
        return math.nan
    sd = np.std(clean, ddof=1)
    if sd == 0:
        return math.nan
    return float(np.mean(clean) / sd)


def paired_samples(df: pd.DataFrame, ra: int) -> pd.DataFrame:
    rep_cfg = f"AttackTestRA{ra}"
    base_cfg = f"BaselineAttackRA{ra}"

    rep = df.loc[df["Config"] == rep_cfg, ["Run", "PDR"]].copy()
    base = df.loc[df["Config"] == base_cfg, ["Run", "PDR"]].copy()
    rep["PDR"] = to_float_series(rep["PDR"])
    base["PDR"] = to_float_series(base["PDR"])

    merged = rep.merge(base, on="Run", how="inner", suffixes=("_rep", "_base"))
    merged = merged.sort_values("Run").reset_index(drop=True)
    merged = merged.dropna(subset=["PDR_rep", "PDR_base"]).reset_index(drop=True)
    return merged


def rank_details(pair: str, merged: pd.DataFrame) -> pd.DataFrame:
    diff = (merged["PDR_rep"] - merged["PDR_base"]).to_numpy(dtype=float)
    finite = np.isfinite(diff)
    included = finite & (diff != 0.0)

    ranks = np.full(len(diff), np.nan, dtype=float)
    if np.any(included):
        ranks[included] = stats.rankdata(np.abs(diff[included]), method=RANK_METHOD)

    return pd.DataFrame(
        {
            "Pair": pair,
            "Run": merged["Run"],
            "REP_PDR": merged["PDR_rep"],
            "Base_PDR": merged["PDR_base"],
            "Diff": diff,
            "Sign": np.sign(diff),
            "ZeroDiff": finite & (diff == 0.0),
            "IncludedInWilcoxon": included,
            "AbsDiff": np.where(finite, np.abs(diff), np.nan),
            "AbsDiffRank": ranks,
            "RankMethod": RANK_METHOD,
        }
    )


def wilcoxon_row(pair: str, merged: pd.DataFrame) -> tuple[dict, pd.DataFrame]:
    rep = merged["PDR_rep"].to_numpy(dtype=float)
    base = merged["PDR_base"].to_numpy(dtype=float)
    diff = rep - base
    finite = np.isfinite(diff)
    diff = diff[finite]

    nonzero = diff != 0.0
    diff_nz = diff[nonzero]
    abs_diff = np.abs(diff_nz)
    ranks = stats.rankdata(abs_diff, method=RANK_METHOD) if len(diff_nz) else np.array([])

    w_plus = float(np.sum(ranks[diff_nz > 0.0])) if len(diff_nz) else 0.0
    w_minus = float(np.sum(ranks[diff_nz < 0.0])) if len(diff_nz) else 0.0
    rank_sum = w_plus + w_minus

    if rank_sum > 0:
        # Conventional matched-pairs rank-biserial correlation.
        rank_biserial = (w_plus - w_minus) / rank_sum
        w_plus_fraction = w_plus / rank_sum
    else:
        rank_biserial = math.nan
        w_plus_fraction = math.nan

    warning_texts: list[str] = []
    if len(diff_nz) == 0:
        stat = math.nan
        p_raw = math.nan
        warning_texts.append("all paired differences are zero")
    else:
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            result = stats.wilcoxon(
                diff_nz,
                zero_method=WILCOXON_ZERO_METHOD,
                correction=WILCOXON_CORRECTION,
                alternative=ALTERNATIVE,
                method=WILCOXON_METHOD,
            )
        stat = float(result.statistic)
        p_raw = float(result.pvalue)
        warning_texts.extend(str(w.message) for w in caught)

    details = rank_details(pair, merged)
    row = {
        "Pair": pair,
        "N_total": int(len(diff)),
        "N_effective": int(len(diff_nz)),
        "ZeroDiffs": int(np.sum(diff == 0.0)),
        "REP_mean": float(np.mean(rep)),
        "REP_std": float(np.std(rep, ddof=1)) if len(rep) > 1 else math.nan,
        "Base_mean": float(np.mean(base)),
        "Base_std": float(np.std(base, ddof=1)) if len(base) > 1 else math.nan,
        "Delta_pp": float(np.mean(diff)) if len(diff) else math.nan,
        "W_plus": w_plus,
        "W_minus": w_minus,
        "W_stat_scipy": stat,
        "p_raw": p_raw,
        "rank_biserial_r": float(rank_biserial),
        "W_plus_fraction": float(w_plus_fraction),
        "cohen_d_paired_reference": cohens_d_paired(diff),
        "ties_abs_diff": bool(has_ties(abs_diff)),
        "rank_method": RANK_METHOD,
        "zero_method": WILCOXON_ZERO_METHOD,
        "alternative": ALTERNATIVE,
        "method": WILCOXON_METHOD,
        "correction": WILCOXON_CORRECTION,
        "scipy_warnings": " | ".join(warning_texts),
    }
    return row, details


def mannwhitney_row(pair: str, merged: pd.DataFrame) -> dict:
    rep = merged["PDR_rep"].to_numpy(dtype=float)
    base = merged["PDR_base"].to_numpy(dtype=float)
    rep = rep[np.isfinite(rep)]
    base = base[np.isfinite(base)]
    n1 = len(rep)
    n2 = len(base)

    warning_texts: list[str] = []
    if n1 == 0 or n2 == 0:
        u_rep = math.nan
        p_raw = math.nan
    else:
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            result = stats.mannwhitneyu(
                rep,
                base,
                use_continuity=MANNWHITNEY_USE_CONTINUITY,
                alternative=ALTERNATIVE,
                method=MANNWHITNEY_METHOD,
            )
        u_rep = float(result.statistic)
        p_raw = float(result.pvalue)
        warning_texts.extend(str(w.message) for w in caught)

    denom = n1 * n2
    rank_biserial = (2.0 * u_rep / denom - 1.0) if denom else math.nan

    return {
        "Pair": pair,
        "N_REP": int(n1),
        "N_Base": int(n2),
        "REP_mean": float(np.mean(rep)) if n1 else math.nan,
        "REP_std": float(np.std(rep, ddof=1)) if n1 > 1 else math.nan,
        "Base_mean": float(np.mean(base)) if n2 else math.nan,
        "Base_std": float(np.std(base, ddof=1)) if n2 > 1 else math.nan,
        "Delta_pp": float(np.mean(rep) - np.mean(base)) if n1 and n2 else math.nan,
        "U_rep": u_rep,
        "p_raw": p_raw,
        "rank_biserial_r": float(rank_biserial),
        "common_language_rep_gt_base": float(u_rep / denom) if denom else math.nan,
        "ties_combined": bool(has_ties(np.concatenate([rep, base]))),
        "alternative": ALTERNATIVE,
        "method": MANNWHITNEY_METHOD,
        "use_continuity": MANNWHITNEY_USE_CONTINUITY,
        "scipy_warnings": " | ".join(warning_texts),
    }


def bootstrap_median_ci(values: np.ndarray) -> tuple[float, float]:
    clean = values[np.isfinite(values)]
    if len(clean) == 0:
        return math.nan, math.nan
    rng = np.random.default_rng(BOOTSTRAP_SEED)
    samples = rng.choice(clean, size=(BOOTSTRAP_SAMPLES, len(clean)), replace=True)
    medians = np.median(samples, axis=1)
    low, high = np.percentile(medians, [2.5, 97.5])
    return float(low), float(high)


def detection_time_rows(df: pd.DataFrame) -> list[dict]:
    rows = []
    for ra in RA_NUMS:
        cfg = f"AttackTestRA{ra}"
        subset = df.loc[df["Config"] == cfg, ["DetectionTime"]].copy()
        subset["DetectionTime"] = to_float_series(subset["DetectionTime"])
        values = subset["DetectionTime"].dropna().to_numpy(dtype=float)
        q1, median, q3 = (
            np.percentile(values, [25, 50, 75]) if len(values) else (math.nan, math.nan, math.nan)
        )
        ci_low, ci_high = bootstrap_median_ci(values)
        rows.append(
            {
                "Pair": f"RA{ra}",
                "Config": cfg,
                "N": int(len(values)),
                "Detection_median_s": float(median),
                "Detection_q1_s": float(q1),
                "Detection_q3_s": float(q3),
                "Detection_iqr_s": float(q3 - q1) if len(values) else math.nan,
                "Median_ci95_low_s": ci_low,
                "Median_ci95_high_s": ci_high,
                "ci_method": "bootstrap_percentile",
                "bootstrap_samples": BOOTSTRAP_SAMPLES,
                "bootstrap_seed": BOOTSTRAP_SEED,
            }
        )
    return rows


def fpr_row() -> dict:
    result = stats.binomtest(FPR_FALSE_POSITIVES, FPR_TRIALS, alternative="less")
    ci = result.proportion_ci(confidence_level=FPR_CONFIDENCE, method="exact")
    return {
        "FalsePositives": FPR_FALSE_POSITIVES,
        "Trials": FPR_TRIALS,
        "FPR": FPR_FALSE_POSITIVES / FPR_TRIALS,
        "ConfidenceLevel": FPR_CONFIDENCE,
        "Alternative": "less",
        "Method": "Clopper-Pearson exact one-sided upper",
        "CI_low": float(ci.low),
        "CI_high": float(ci.high),
    }


def metadata(raw_path: Path) -> dict:
    return {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "raw_results_path": str(raw_path),
        "raw_results_sha256": sha256_file(raw_path),
        "python_version": platform.python_version(),
        "platform": platform.platform(),
        "scipy_version": scipy.__version__,
        "numpy_version": np.__version__,
        "pandas_version": pd.__version__,
        "alpha": ALPHA,
        "alternative": ALTERNATIVE,
        "holm_bonferroni": "sort ascending, multiply by m-i, cumulative max, clip to 1",
        "wilcoxon": {
            "zero_method": WILCOXON_ZERO_METHOD,
            "correction": WILCOXON_CORRECTION,
            "method": WILCOXON_METHOD,
            "rank_method": RANK_METHOD,
            "effect": "rank_biserial_r = (W_plus - W_minus) / (W_plus + W_minus)",
            "w_plus_fraction": "W_plus / (W_plus + W_minus), also saved for normalized positive-rank mass",
            "p_floor_one_sided_n10": 1.0 / (2**10),
            "holm_floor_five_tests_n10": 5.0 / (2**10),
        },
        "mannwhitney": {
            "method": MANNWHITNEY_METHOD,
            "use_continuity": MANNWHITNEY_USE_CONTINUITY,
            "effect": "2 * U_rep / (n1 * n2) - 1; positive means REP-AODV has larger PDR",
            "ties_note": "SciPy exact method is requested; ties are flagged in output.",
        },
        "detection_time": {
            "summary": "median, Q1, Q3, IQR",
            "ci_method": "bootstrap percentile CI for median",
            "bootstrap_samples": BOOTSTRAP_SAMPLES,
            "bootstrap_seed": BOOTSTRAP_SEED,
        },
        "fpr": {
            "false_positives": FPR_FALSE_POSITIVES,
            "trials": FPR_TRIALS,
            "confidence": FPR_CONFIDENCE,
            "alternative": "less",
            "method": "Clopper-Pearson exact",
        },
    }


def write_csv(df: pd.DataFrame, path: Path) -> None:
    df.to_csv(path, index=False, float_format="%.6g")
    print(f"  saved {path}")


def main() -> None:
    raw_path = Path(sys.argv[1]) if len(sys.argv) > 1 else default_raw_path()
    if not raw_path.exists():
        print(f"ERROR: raw results file not found: {raw_path}", file=sys.stderr)
        sys.exit(1)

    out_dir = raw_path.parent if raw_path.parent != Path("") else Path(".")
    df = pd.read_csv(raw_path)
    required = {"Config", "Run", "PDR", "DetectionTime"}
    missing = required - set(df.columns)
    if missing:
        print(f"ERROR: missing columns in {raw_path}: {sorted(missing)}", file=sys.stderr)
        sys.exit(1)

    wilcoxon_rows = []
    mann_rows = []
    details = []

    for ra in RA_NUMS:
        pair = f"RA{ra}"
        merged = paired_samples(df, ra)
        if len(merged) == 0:
            print(f"WARNING: no paired rows for {pair}", file=sys.stderr)
        w_row, pair_details = wilcoxon_row(pair, merged)
        wilcoxon_rows.append(w_row)
        mann_rows.append(mannwhitney_row(pair, merged))
        details.append(pair_details)

    wilcoxon_df = pd.DataFrame(wilcoxon_rows)
    mann_df = pd.DataFrame(mann_rows)
    wilcoxon_df["p_adj"] = holm_bonferroni(wilcoxon_df["p_raw"])
    mann_df["p_adj"] = holm_bonferroni(mann_df["p_raw"])
    wilcoxon_df["sig"] = wilcoxon_df["p_adj"].apply(significance)
    mann_df["sig"] = mann_df["p_adj"].apply(significance)

    detection_df = pd.DataFrame(detection_time_rows(df))
    fpr_df = pd.DataFrame([fpr_row()])
    rank_details_df = pd.concat(details, ignore_index=True) if details else pd.DataFrame()

    print("\n=== Wilcoxon signed-rank: REP-AODV > BaselineAttack (PDR) ===")
    print(f"alpha={ALPHA}, alternative={ALTERNATIVE}, method={WILCOXON_METHOD}, "
          f"zero_method={WILCOXON_ZERO_METHOD}, correction={WILCOXON_CORRECTION}")
    print(
        wilcoxon_df[
            [
                "Pair",
                "N_effective",
                "Delta_pp",
                "W_plus",
                "W_minus",
                "p_raw",
                "p_adj",
                "rank_biserial_r",
                "W_plus_fraction",
                "cohen_d_paired_reference",
                "sig",
            ]
        ].to_string(index=False)
    )

    print("\n=== Mann-Whitney U: REP-AODV > BaselineAttack (PDR) ===")
    print(f"alpha={ALPHA}, alternative={ALTERNATIVE}, method={MANNWHITNEY_METHOD}")
    print(
        mann_df[
            ["Pair", "N_REP", "N_Base", "Delta_pp", "U_rep", "p_raw", "p_adj", "rank_biserial_r", "sig"]
        ].to_string(index=False)
    )

    print("\n=== Detection time summary (REP-AODV attack configs only) ===")
    print(
        detection_df[
            [
                "Pair",
                "N",
                "Detection_median_s",
                "Detection_q1_s",
                "Detection_q3_s",
                "Median_ci95_low_s",
                "Median_ci95_high_s",
            ]
        ].to_string(index=False)
    )

    print("\n=== False-positive rate CI ===")
    print(fpr_df.to_string(index=False))

    write_csv(wilcoxon_df, out_dir / "wilcoxon_results.csv")
    write_csv(mann_df, out_dir / "mannwhitney_results.csv")
    write_csv(detection_df, out_dir / "detection_time_results.csv")
    write_csv(fpr_df, out_dir / "fpr_ci.csv")
    write_csv(rank_details_df, out_dir / "rank_details.csv")

    meta_path = out_dir / "stat_metadata.json"
    meta_path.write_text(json.dumps(metadata(raw_path), indent=2, sort_keys=True) + "\n")
    print(f"  saved {meta_path}")


if __name__ == "__main__":
    main()

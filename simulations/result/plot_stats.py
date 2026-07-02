#!/usr/bin/env python3
"""
Visualization of REP-AODV statistical analysis results.

Reads CSV files produced by wilcoxon_test.py and generates publication-ready plots:
  fig1_pdr.png          — PDR по прогонам (основная фигура, точки + медиана)
  fig2_effect_size.png  — размер эффекта (Вилкоксон + Манн–Уитни)
  fig3_detection_time.png — время обнаружения с IQR и 95% ДИ
  fig4_overhead_delay.png — накладные расходы и задержка
  fig5_fpr.png          — FPR с Clopper–Pearson CI
  fig_aux_pdr_bars.png  — вспомогательная: PDR mean±std (не использовать как основную)

Usage:
    python3 result/plot_stats.py [result_dir]
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import pandas as pd
from scipy.stats import binom

RESULT_DIR = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent

# ── colour palette ────────────────────────────────────────────────────────────
C_BASELINE  = "#4C72B0"   # blue  – no attack, no protection
C_ATTACK    = "#DD8452"   # orange – unprotected attack
C_PROTECTED = "#55A868"   # green  – REP-AODV under attack
C_MOBILE    = "#8172B2"   # purple – mobile topology

SIG_COLORS = {"***": "#2ca02c", "**": "#98df8a", "*": "#ffbb78", "ns": "#d62728", "na": "#aaaaaa"}

RA_LABELS = [
    "RA1\n(центр сетки)",
    "RA2\n(предфин. узлы)",
    "RA3\n(крит. маршрут)",
    "RA4\n(соседние)",
    "RA5\n(диагональная пара)",
]

RA_SUBTITLES = [
    "центр сетки",
    "предфин. узлы",
    "крит. маршрут",
    "соседние",
    "диагональная пара",
]


def fmt(v: float, d: int = 2) -> str:
    """Число с русской десятичной запятой."""
    return f"{v:.{d}f}".replace(".", ",")


def fmt_p(pv: float) -> str:
    """p-значение с русской запятой; <0,0001 вместо округления до нуля."""
    if not np.isfinite(pv):
        return "p = н/д"
    if pv < 0.0001:
        return "p < 0,0001"
    return "p = " + f"{pv:.4f}".replace(".", ",")


def order_stat_ci(values: np.ndarray, confidence: float = 0.95) -> tuple[float, float]:
    """Безраспределительный ДИ медианы через порядковые статистики (Conover).

    Ищет наименьший j такой, что 1 - 2*P(Bin(n,0.5)≤j-1) ≥ confidence,
    возвращает [X_(j), X_(n-j+1)].  Для n=10, 95%: CI = [X_(2), X_(9)],
    фактическое покрытие ≈ 97,85 %.
    """
    clean = np.sort(values[np.isfinite(values)])
    n = len(clean)
    if n < 2:
        return math.nan, math.nan
    for j in range(1, n // 2 + 1):
        if 1.0 - 2.0 * float(binom.cdf(j - 1, n, 0.5)) >= confidence:
            return float(clean[j - 1]), float(clean[n - j])
    return float(clean[0]), float(clean[-1])


def load(name: str) -> pd.DataFrame:
    path = RESULT_DIR / name
    if not path.exists():
        raise FileNotFoundError(f"Missing: {path}. Run wilcoxon_test.py first.")
    return pd.read_csv(path)


def savefig(fig: plt.Figure, name: str) -> None:
    path = RESULT_DIR / name
    fig.savefig(path, dpi=150, bbox_inches="tight")
    print(f"  saved {path}")
    plt.close(fig)


# ─────────────────────────────────────────────────────────────────────────────
# Вспомогательная фигура – PDR mean±std (не использовать как основную:
# усы уходят ниже нуля при бимодальном baseline, т.к. данные ненормальные)
# ─────────────────────────────────────────────────────────────────────────────
def fig_pdr_aux(pdr: pd.DataFrame, raw: pd.DataFrame) -> None:
    ra_nums = [1, 2, 3, 4, 5]
    x = np.arange(len(ra_nums))
    width = 0.26

    baseline_pdr  = [pdr.loc[pdr["Config"] == f"BaselineAttackRA{r}", "PDR_mean"].iat[0] for r in ra_nums]
    baseline_std  = [pdr.loc[pdr["Config"] == f"BaselineAttackRA{r}", "PDR_std"].iat[0]  for r in ra_nums]
    protected_pdr = [pdr.loc[pdr["Config"] == f"AttackTestRA{r}",     "PDR_mean"].iat[0] for r in ra_nums]
    protected_std = [pdr.loc[pdr["Config"] == f"AttackTestRA{r}",     "PDR_std"].iat[0]  for r in ra_nums]

    ref_pdr = pdr.loc[pdr["Config"] == "BaselineAODV", "PDR_mean"].iat[0]
    ref_std = pdr.loc[pdr["Config"] == "BaselineAODV", "PDR_std"].iat[0]

    fig, ax = plt.subplots(figsize=(9, 5))

    bars_b = ax.bar(x - width/2, baseline_pdr,  width, yerr=baseline_std,  capsize=4,
                    color=C_ATTACK,    label="Без защиты (атака)", alpha=0.9, zorder=3)
    bars_p = ax.bar(x + width/2, protected_pdr, width, yerr=protected_std, capsize=4,
                    color=C_PROTECTED, label="Предложенный механизм (атака)", alpha=0.9, zorder=3)

    ax.axhline(ref_pdr, color=C_BASELINE, linewidth=1.8, linestyle="--",
               label=f"Базовый AODV (без атаки, {ref_pdr:.1f}%)")
    ax.fill_between([-0.6, len(ra_nums) - 0.4],
                    ref_pdr - ref_std, ref_pdr + ref_std,
                    color=C_BASELINE, alpha=0.12)

    ax.set_xticks(x)
    ax.set_xticklabels(RA_LABELS, fontsize=10)
    ax.set_ylabel("PDR, %", fontsize=11)
    ax.set_xlabel("Сценарий атаки", fontsize=11)

    ax.set_ylim(0, 115)
    ax.yaxis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.legend(fontsize=9, loc="upper right")

    # annotate delta
    for xi, bp, pp in zip(x, baseline_pdr, protected_pdr):
        delta = pp - bp
        ax.annotate(f"+{delta:.1f}pp", xy=(xi, max(pp, bp) + 3),
                    ha="center", va="bottom", fontsize=8, color="#333333")

    savefig(fig, "fig_aux_pdr_bars.png")


# ─────────────────────────────────────────────────────────────────────────────
# Figure 2 – Effect size + significance table
# ─────────────────────────────────────────────────────────────────────────────
def fig_effect(wilcoxon: pd.DataFrame, mannwhitney: pd.DataFrame) -> None:
    pairs = wilcoxon["Pair"].tolist()
    n = len(pairs)
    fig, axes = plt.subplots(1, 2, figsize=(11, 5), sharey=True)

    for ax, df, panel_label, r_col, p_col in [
        (axes[0], wilcoxon,    "Критерий Вилкоксона (парный)",      "rank_biserial_r", "p_adj"),
        (axes[1], mannwhitney, "Критерий Манна–Уитни (независимый)", "rank_biserial_r", "p_adj"),
    ]:
        r_vals = df[r_col].to_numpy(dtype=float)
        p_vals = df[p_col].to_numpy(dtype=float)
        sigs   = df["sig"].tolist()
        y = np.arange(n)

        ax.barh(y, r_vals, color=C_PROTECTED, edgecolor="white", linewidth=0.8, height=0.6)

        for yi, rv, pv, sig in zip(y, r_vals, p_vals, sigs):
            stars = sig if sig not in ("ns", "na") else ""
            label = f"r = {fmt(rv)},  {fmt_p(pv)}"
            if stars:
                label += f"  {stars}"
            ax.text(rv + 0.02, yi, label, va="center", fontsize=8.5)

        ax.set_yticks(y)
        ax.set_yticklabels(pairs, fontsize=10)
        ax.set_xlim(0, 1.45)
        ax.set_xlabel("Ранговая бисериальная корреляция (r)", fontsize=10)
        # название критерия — текстовая аннотация внутри панели (не title)
        ax.text(0.02, 0.98, panel_label, transform=ax.transAxes,
                ha="left", va="top", fontsize=9, style="italic")
        vline05 = ax.axvline(0.5, color="gray", linestyle=":",  linewidth=1)
        vline08 = ax.axvline(0.8, color="gray", linestyle="--", linewidth=1)
        ax.xaxis.grid(True, linestyle=":", alpha=0.5)
        ax.set_axisbelow(True)

    # легенда только для опорных линий
    axes[1].legend(
        handles=[
            plt.Line2D([0], [0], color="gray", linestyle=":",  linewidth=1, label="r = 0,5 (средний эффект)"),
            plt.Line2D([0], [0], color="gray", linestyle="--", linewidth=1, label="r = 0,8 (большой эффект)"),
        ],
        fontsize=8.5, loc="lower right",
    )

    fig.tight_layout()
    fig.subplots_adjust(bottom=0.12)
    savefig(fig, "fig2_effect_size.png")


# ─────────────────────────────────────────────────────────────────────────────
# Figure 3 – Detection time box plot + CI
# ─────────────────────────────────────────────────────────────────────────────
def fig_detection(detection: pd.DataFrame, raw: pd.DataFrame) -> None:
    pairs = detection["Pair"].tolist()
    n = len(pairs)
    x = np.arange(n)

    medians = detection["Detection_median_s"].to_numpy(dtype=float)
    q1      = detection["Detection_q1_s"].to_numpy(dtype=float)
    q3      = detection["Detection_q3_s"].to_numpy(dtype=float)

    # безраспределительный ДИ медианы через порядковые статистики
    ci_lo = np.empty(n)
    ci_hi = np.empty(n)
    for i, pair in enumerate(pairs):
        ra = pair  # "RA1" … "RA5"
        cfg = f"AttackTest{ra}"
        vals = pd.to_numeric(
            raw.loc[raw["Config"] == cfg, "DetectionTime"], errors="coerce"
        ).dropna().to_numpy(dtype=float)
        ci_lo[i], ci_hi[i] = order_stat_ci(vals)

    fig, ax = plt.subplots(figsize=(8, 5))

    ax.bar(x, medians, width=0.5, color=C_PROTECTED, alpha=0.8, label="Медиана", zorder=3)

    # IQR
    for xi, lo, hi in zip(x, q1, q3):
        ax.vlines(xi, lo, hi, color="#2d6a4f", linewidth=2.5, zorder=4)
        ax.hlines([lo, hi], xi - 0.15, xi + 0.15, color="#2d6a4f", linewidth=1.5, zorder=4)

    # ДИ медианы (порядковые статистики)
    ax.errorbar(x, medians, yerr=[medians - ci_lo, ci_hi - medians],
                fmt="none", color="#c62828", linewidth=1.2, capsize=6, capthick=1.5,
                label="95% ДИ медианы (порядк. статистики)", zorder=5)

    ax.set_xticks(x)
    ax.set_xticklabels(pairs, fontsize=10)
    ax.set_ylabel("Время обнаружения, с", fontsize=11)
    ax.set_xlabel("Сценарий атаки", fontsize=11)
    ax.yaxis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.legend(fontsize=9)

    for xi, lo, hi in zip(x, q1, q3):
        ax.annotate(f"IQR = {fmt(hi - lo, 1)} с",
                    xy=(xi, hi + 1), ha="center", fontsize=8, color="#555")

    savefig(fig, "fig3_detection_time.png")


# ─────────────────────────────────────────────────────────────────────────────
# Figure 4 – Overhead and delay comparison
# ─────────────────────────────────────────────────────────────────────────────
def fig_overhead(pdr: pd.DataFrame) -> None:
    configs = ["BaselineAODV", "RepAODV"] + [f"AttackTestRA{r}" for r in [1, 2, 3, 4, 5]]
    labels  = ["Базовый\nAODV", "Предл.\nмеханизм\n(без атаки)"] + [f"RA{r}" for r in [1, 2, 3, 4, 5]]
    colors  = [C_BASELINE, C_BASELINE] + [C_PROTECTED] * 5

    sub = pdr[pdr["Config"].isin(configs)].set_index("Config").reindex(configs)
    overhead = sub["Overhead_mean"].to_numpy(dtype=float)
    oh_std   = sub["Overhead_std"].to_numpy(dtype=float)
    delay    = sub["AvgDelayMs_mean"].to_numpy(dtype=float)
    dl_std   = sub["AvgDelayMs_std"].to_numpy(dtype=float)

    x = np.arange(len(configs))
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # overhead
    ax1.bar(x, overhead, yerr=oh_std, capsize=4, color=colors, alpha=0.85, width=0.6, zorder=3)
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, fontsize=9)
    ax1.set_ylabel("Относительные накладные расходы", fontsize=10)

    ax1.yaxis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    ax1.set_axisbelow(True)

    # delay
    ax2.bar(x, delay, yerr=dl_std, capsize=4, color=colors, alpha=0.85, width=0.6, zorder=3)
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels, fontsize=9)
    ax2.set_ylabel("Средняя задержка, мс", fontsize=10)

    ax2.yaxis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    ax2.set_axisbelow(True)

    legend_patches = [
        mpatches.Patch(color=C_BASELINE,  label="Базовый AODV (без атаки)"),
        mpatches.Patch(color=C_PROTECTED, label="Предложенный механизм (с атакой)"),
    ]
    fig.legend(handles=legend_patches, loc="upper center", ncol=2,
               fontsize=9, bbox_to_anchor=(0.5, 1.02))

    fig.tight_layout()
    savefig(fig, "fig4_overhead_delay.png")


# ─────────────────────────────────────────────────────────────────────────────
# Figure 5 – FPR with Clopper-Pearson CI
# ─────────────────────────────────────────────────────────────────────────────
def fig_fpr(fpr: pd.DataFrame) -> None:
    row = fpr.iloc[0]
    fpr_val  = float(row["FPR"])
    ci_hi    = float(row["CI_high"])
    trials   = int(row["Trials"])
    fp       = int(row["FalsePositives"])
    conf     = float(row["ConfidenceLevel"])

    fig, ax = plt.subplots(figsize=(5, 4))
    ax.bar([0], [fpr_val], width=0.4, color=C_BASELINE, alpha=0.85, zorder=3, label="FPR")
    ci_hi_fmt = fmt(ci_hi, 4)
    ax.errorbar([0], [fpr_val], yerr=[[fpr_val - 0.0], [ci_hi - fpr_val]],
                fmt="none", color="#c62828", linewidth=1.5, capsize=8, capthick=2,
                label=f"Верхняя граница {conf*100:.0f}% ДИ ({ci_hi_fmt})")
    ax.axhline(0.05, color="gray", linestyle="--", linewidth=1.2, label="α = 0,05")
    ax.set_xticks([0])
    ax.set_xticklabels([f"FPR\n({fp}/{trials} ложных)"], fontsize=10)
    ax.set_ylabel("Вероятность ложного срабатывания", fontsize=10)

    ax.set_ylim(0, 0.12)
    ax.yaxis.grid(True, linestyle=":", alpha=0.6, zorder=0)
    ax.set_axisbelow(True)
    ax.legend(fontsize=9)

    ax.annotate(f"FPR = 0 %\n< {ci_hi_fmt} с вероятностью {conf*100:.0f} %",
                xy=(0, fpr_val + 0.002), ha="center", fontsize=9, color="#333")

    savefig(fig, "fig5_fpr.png")


# ─────────────────────────────────────────────────────────────────────────────
# Figure 1 – PDR по прогонам (основная фигура)
# ─────────────────────────────────────────────────────────────────────────────
def fig_pdr_scatter(raw: pd.DataFrame) -> None:
    ra_nums = [1, 2, 3, 4, 5]
    fig, axes = plt.subplots(1, len(ra_nums), figsize=(13, 5), sharey=True)

    for ax, ra, subtitle in zip(axes, ra_nums, RA_SUBTITLES):
        rep_vals  = raw.loc[raw["Config"] == f"AttackTestRA{ra}",     "PDR"].to_numpy(dtype=float)
        base_vals = raw.loc[raw["Config"] == f"BaselineAttackRA{ra}", "PDR"].to_numpy(dtype=float)

        jitter = np.random.default_rng(42).uniform(-0.07, 0.07, size=max(len(rep_vals), len(base_vals)))
        ax.scatter(np.zeros(len(base_vals)) + jitter[:len(base_vals)], base_vals,
                   color=C_ATTACK,    alpha=0.8, s=40, zorder=3)
        ax.scatter(np.ones(len(rep_vals))  + jitter[:len(rep_vals)], rep_vals,
                   color=C_PROTECTED, alpha=0.8, s=40, zorder=3)

        ax.hlines(np.mean(base_vals), -0.25, 0.25, color=C_ATTACK,    linewidth=2, zorder=4)
        ax.hlines(np.mean(rep_vals),   0.75, 1.25, color=C_PROTECTED, linewidth=2, zorder=4)

        ax.set_xticks([0, 1])
        ax.set_xticklabels(["Без\nзащиты", "Предл.\nмех-м"], fontsize=9)
        ax.text(0.5, 1.01, f"RA{ra} ({subtitle})", transform=ax.transAxes,
                ha="center", va="bottom", fontsize=9)
        ax.set_xlim(-0.5, 1.5)
        ax.set_ylim(-5, 110)
        ax.yaxis.grid(True, linestyle=":", alpha=0.5, zorder=0)
        ax.set_axisbelow(True)

    axes[0].set_ylabel("PDR, %", fontsize=11)
    handles = [
        mpatches.Patch(color=C_ATTACK,    label="Без защиты (атака)"),
        mpatches.Patch(color=C_PROTECTED, label="Предложенный механизм (атака)"),
    ]
    fig.legend(handles=handles, loc="upper center", ncol=2,
               fontsize=9, bbox_to_anchor=(0.5, 1.0))

    fig.tight_layout()
    fig.subplots_adjust(top=0.84)   # резервируем место под легенду сверху
    savefig(fig, "fig1_pdr.png")


# ─────────────────────────────────────────────────────────────────────────────
# main
# ─────────────────────────────────────────────────────────────────────────────
def main() -> None:
    print(f"Reading data from: {RESULT_DIR}\n")

    pdr        = load("pdr_results.csv")
    raw        = load("raw_results.csv")
    wilcoxon   = load("wilcoxon_results.csv")
    mannwhitney = load("mannwhitney_results.csv")
    detection  = load("detection_time_results.csv")
    fpr        = load("fpr_ci.csv")

    print("Generating figures...")
    fig_pdr_scatter(raw)           # fig1 — основная фигура PDR
    fig_effect(wilcoxon, mannwhitney)
    fig_detection(detection, raw)
    fig_overhead(pdr)
    fig_fpr(fpr)
    fig_pdr_aux(pdr, raw)          # вспомогательная: mean±std

    print("\nDone. 6 figures saved to", RESULT_DIR)


if __name__ == "__main__":
    main()

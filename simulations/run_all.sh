#!/bin/bash
# Run all simulation configs (10 repetitions each) and collect metrics.
# Usage: bash run_all.sh [output_file]
#   output_file defaults to result/pdr_results.csv

set -e
cd "$(dirname "$0")"

RESULTS="${1:-result/pdr_results.csv}"
RAW="result/raw_results.csv"
REPEATS=10

echo "=== Building (release) ==="
make -j4 MODE=release -C .. > result/last_build.log 2>&1 && echo "Build OK" || { echo "Build FAILED — see result/last_build.log"; exit 1; }

configs=(
    "BaselineAODV"
    "RepAODV"
    "BaselineAttackRA1"
    "BaselineAttackRA2"
    "BaselineAttackRA3"
    "BaselineAttackRA4"
    "BaselineAttackRA5"
    "AttackTestRA1"
    "AttackTestRA2"
    "AttackTestRA3"
    "AttackTestRA4"
    "AttackTestRA5"
    "RepMobileAODV"
    "AttackMobileRA1"
    "BaselineAttackMobileRA1"
)

echo "Config,Run,PDR,Overhead,AvgDelayMs,DetectionTime" > "$RAW"

for cfg in "${configs[@]}"; do
    for r in $(seq 0 $((REPEATS-1))); do
        echo "=== Running $cfg (run $r) ==="
        rm -f result/simulation_log.csv
        ./run omnetpp.ini -c "$cfg" -r "$r" -u Cmdenv > "result/last_run.log" 2>&1
        metrics=$(python3 result/analyze_log.py result/simulation_log.csv --csv 2>/dev/null || echo ",,," )
        echo "$cfg,$r,$metrics" >> "$RAW"
        echo "  metrics: $metrics"
    done
done

echo ""
echo "=== Computing statistics ==="
python3 result/compute_stats.py "$RAW" "$RESULTS"

echo ""
echo "=== Statistical verification ==="
python3 result/wilcoxon_test.py "$RAW"

echo ""
echo "=== RESULTS ==="
cat "$RESULTS"

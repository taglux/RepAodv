#!/bin/bash
# Sensitivity analysis for REP-AODV.
# Varies one parameter at a time over AttackTestRA1, collects PDR + DetectionTime.
#
# Usage: bash run_sensitivity.sh [output_csv]
#   output_csv defaults to result/sensitivity_raw.csv
#
# Runtime: ~120 runs × ~30s ≈ 60 min. Run in background:
#   nohup bash run_sensitivity.sh > result/sensitivity.log 2>&1 &

set -e
cd "$(dirname "$0")"

SEEDS=5
RAW="${1:-result/sensitivity_raw.csv}"

echo "Param,Value,Seed,PDR,Overhead,AvgDelayMs,DetectionTime" > "$RAW"
echo "=== Sensitivity analysis — $(date) ==="
echo "Output: $RAW"

run_param() {
    local param=$1
    local ini_key=$2
    shift 2
    local values=("$@")

    echo ""
    echo "--- Parameter: $param ---"
    for val in "${values[@]}"; do
        TMPINI=$(mktemp /tmp/sens_XXXXXX.ini)
        printf '[Config SensRun]\nextends = AttackTestRA1\n%s = %s\n' \
               "$ini_key" "$val" > "$TMPINI"

        for seed in $(seq 0 $((SEEDS - 1))); do
            rm -f result/simulation_log.csv
            ./run omnetpp.ini "$TMPINI" -c SensRun -r "$seed" \
                  -u Cmdenv > result/last_run.log 2>&1
            m=$(python3 result/analyze_log.py result/simulation_log.csv --csv \
                2>/dev/null || echo ",,,")
            echo "$param,$val,$seed,$m" >> "$RAW"
            echo "  $param=$val  seed=$seed  metrics: $m"
        done

        rm -f "$TMPINI"
    done
}

run_param ackTimeout       "**.routing.ackTimeout"       \
    0.3s 0.4s 0.5s 0.6s 0.7s 0.8s 0.9s 1.0s

run_param gossipThreshold  "**.routing.gossipThreshold"  \
    0.20 0.25 0.30 0.35 0.40 0.45 0.50

run_param suspectThreshold "**.routing.suspectThreshold" \
    0.30 0.35 0.40 0.45 0.50

run_param evidenceHalfLife "**.routing.evidenceHalfLife" \
    30s 60s 90s 120s

echo ""
echo "=== Aggregating results ==="
python3 result/analyze_sensitivity.py "$RAW"
echo "Done — $(date)"

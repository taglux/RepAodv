#!/bin/bash

CONFIGS=(
    "BaselineAODV"
    "RepAODV"
    "AttackTestRA1"
    "AttackTestRA2"
    "AttackTestRA3"
    "AttackTestRA4"
    "AttackTestRA5"
    "BaselineAttackRA1"
    "BaselineAttackRA2"
    "BaselineAttackRA3"
    "BaselineAttackRA4"
    "BaselineAttackRA5"
)
LOG_CSV="simulations/result/simulation_log.csv"
RESULTS_SUMMARY="simulations/result/final_comparison.txt"

echo "Configuration | PDR (%) | Overhead | Delay (ms) | Detection (s) | Blackholes" > $RESULTS_SUMMARY
echo "---|---|---|---|---|---" >> $RESULTS_SUMMARY

for CFG in "${CONFIGS[@]}"; do
    echo "Running $CFG..."
    rm -f $LOG_CSV
    cd simulations
    ./run -c $CFG -r 0 -u Cmdenv > last_run.log 2>&1
    cd ..
    
    # Extract metrics using analyze_log.py --csv
    METRICS=$(python3 simulations/result/analyze_log.py --csv $LOG_CSV)
    
    # Count unique neighbors quarantined or permanently banned by reputation system
    BH_COUNT=$(grep "TRUST_STATE_CHANGE" $LOG_CSV | grep "state=[23]" | grep -oP "neighbor=\K[^,]+" | sort -u | wc -l)
    
    # Format: pdr,overhead,avg_delay_ms,detection_time
    IFS=',' read -r PDR OVERHEAD DELAY DETECT <<< "$METRICS"
    
    echo "$CFG | $PDR | $OVERHEAD | $DELAY | $DETECT | $BH_COUNT detected" >> $RESULTS_SUMMARY
done

cat $RESULTS_SUMMARY

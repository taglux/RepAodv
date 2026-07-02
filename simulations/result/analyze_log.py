#!/usr/bin/env python3
"""
Simulation Log Analyzer for AODV with Reputation-based Blackhole Detection.

Analyzes CSV logs produced by the OMNeT++ RepAodv simulation and prints
a structured report covering routing, data delivery, blackhole behaviour,
reputation system effectiveness, and packet drops.

Usage:
    python3 analyze_log.py [path_to_csv] [--csv]
"""

import csv
import sys
import os
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Tuple

@dataclass
class Event:
    timestamp: float
    node_id: str
    node_type: str
    event_type: str
    details: str
    details_dict: Dict[str, str] = field(default_factory=dict)

def parse_details(details_str: str) -> Dict[str, str]:
    result = {}
    if not details_str:
        return result
    for pair in details_str.split(","):
        if "=" in pair:
            k, v = pair.split("=", 1)
            result[k.strip()] = v.strip()
    return result

def load_events(csv_path: str) -> List[Event]:
    events = []
    if not os.path.exists(csv_path):
        return events
    with open(csv_path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=";")
        for row in reader:
            if len(row) < 5:
                continue
            if row[0] == "timestamp" or "simTime" in row[0]: # handle header if present
                continue
            try:
                ts = float(row[0].strip())
            except ValueError:
                continue
            events.append(
                Event(
                    timestamp=ts,
                    node_id=row[1].strip(),
                    node_type=row[2].strip(),
                    event_type=row[3].strip(),
                    details=row[4].strip(),
                    details_dict=parse_details(row[4].strip()),
                )
            )
    return events

def compute_metrics(events: List[Event]) -> Dict:
    data_send = [e for e in events if e.event_type == "DATA_SEND"]
    data_recv  = [e for e in events if e.event_type == "DATA_RECV"]
    trust_changes = [e for e in events if e.event_type == "TRUST_STATE_CHANGE"]
    
    # PDR - use (src, dest, id) as unique packet identifier
    sent_packets = set()
    for e in data_send:
        d = e.details_dict
        sent_packets.add((d.get("src"), d.get("dest"), d.get("id")))
    
    recv_packets = set()
    for e in data_recv:
        d = e.details_dict
        recv_packets.add((d.get("src"), d.get("dest"), d.get("id")))
    
    total_sent = len(sent_packets)
    # Only count received packets that were actually logged as sent
    total_recv = len(recv_packets.intersection(sent_packets))
    pdr = (total_recv / total_sent * 100) if total_sent else 0.0

    # Delay — match by unique packet key (src, dest, id)
    send_times = {}
    for e in data_send:
        d = e.details_dict
        key = (d.get("src"), d.get("dest"), d.get("id"))
        if key[2] and key[2] != "0":
            send_times[key] = e.timestamp

    delays = []
    for e in data_recv:
        d = e.details_dict
        key = (d.get("src"), d.get("dest"), d.get("id"))
        if key in send_times:
            delay = e.timestamp - send_times[key]
            if delay >= 0:
                delays.append(delay)
                # Remove from send_times to avoid double counting if duplicate received
                del send_times[key]

    avg_delay_ms = (sum(delays) / len(delays) * 1000) if delays else 0.0

    # Detection time
    detection_time = ""
    start_time = events[0].timestamp if events else 0.0
    for e in trust_changes:
        st = e.details_dict.get("state")
        if st in ("2", "3"): # QUARANTINE or PERMANENT
            detection_time = f"{e.timestamp - start_time:.3f}"
            break

    # Overhead (just count 2ACKs as overhead for now)
    acks = sum(1 for e in events if e.event_type == "2ACK_SENT")
    overhead = acks / total_sent if total_sent else 0.0

    return {
        "pdr": pdr,
        "overhead": overhead,
        "avg_delay_ms": avg_delay_ms,
        "detection_time": detection_time,
    }

def analyze(events: List[Event]) -> None:
    if not events:
        print("No events found.")
        return

    ts_min = events[0].timestamp
    ts_max = events[-1].timestamp
    duration = ts_max - ts_min

    print("=== GENERAL OVERVIEW ===")
    print(f"Total events: {len(events)}")
    print(f"Duration: {duration:.3f} s")

    metrics = compute_metrics(events)
    print("\n=== METRICS ===")
    print(f"PDR: {metrics['pdr']:.1f}%")
    print(f"Overhead: {metrics['overhead']:.3f} (2ACKs / DATA_SEND)")
    print(f"Avg Delay: {metrics['avg_delay_ms']:.1f} ms")
    print(f"Detection Time: {metrics['detection_time']} s")

    print("\n=== EVENT COUNTS ===")
    event_counts = Counter(e.event_type for e in events)
    for etype, cnt in event_counts.most_common():
        print(f"  {etype:<25}: {cnt}")

    drops = [e for e in events if "DROP" in e.event_type]
    if drops:
        print("\n=== DROPS ===")
        by_type = Counter(e.event_type for e in drops)
        for t, c in by_type.most_common():
            print(f"  {t:<25}: {c}")

def main():
    csv_mode = "--csv" in sys.argv
    args = [a for a in sys.argv[1:] if a != "--csv"]
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = args[0] if args else os.path.join(script_dir, "simulation_log.csv")

    events = load_events(csv_path)

    if csv_mode:
        m = compute_metrics(events)
        print(f"{m['pdr']:.1f},{m['overhead']:.3f},{m['avg_delay_ms']:.1f},{m['detection_time']}")
    else:
        analyze(events)

if __name__ == "__main__":
    main()

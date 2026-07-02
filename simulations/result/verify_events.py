#!/usr/bin/env python3
"""
Event-log verifier for REP-AODV.

Analyses simulation_log.csv to quantify BH detection coverage, false-positive
rate, detection timeline, and gossip-propagated quarantines.

Usage:
    python3 verify_events.py [path_to_csv]
"""

import csv
import os
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Set, Tuple


@dataclass
class Event:
    timestamp: float
    node_id: str
    node_type: str
    event_type: str
    details: str
    details_dict: Dict[str, str] = field(default_factory=dict)


def parse_details(s: str) -> Dict[str, str]:
    result = {}
    for pair in s.split(","):
        if "=" in pair:
            k, v = pair.split("=", 1)
            result[k.strip()] = v.strip()
    return result


def load_events(path: str) -> List[Event]:
    events = []
    if not os.path.exists(path):
        print(f"File not found: {path}", file=sys.stderr)
        return events
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=";")
        for row in reader:
            if len(row) < 5:
                continue
            if row[0].strip() in ("timestamp", "simTime"):
                continue
            try:
                ts = float(row[0].strip())
            except ValueError:
                continue
            d = row[4].strip()
            events.append(Event(
                timestamp=ts,
                node_id=row[1].strip(),
                node_type=row[2].strip(),
                event_type=row[3].strip(),
                details=d,
                details_dict=parse_details(d),
            ))
    return events


# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def identify_bh_ips(events: List[Event]) -> Tuple[Set[str], Set[str], Set[str]]:
    """
    Returns (bh_ips, quarantined_ips, sender_ips).
    bh_ips = quarantined_ips - sender_ips (heuristic: nodes that were quarantined
    but never sent user-data are the attackers).
    """
    sender_ips: Set[str] = set()
    for e in events:
        if e.event_type == "DATA_SEND":
            ip = e.details_dict.get("src")
            if ip:
                sender_ips.add(ip)

    quarantined_ips: Set[str] = set()
    for e in events:
        if e.event_type == "TRUST_STATE_CHANGE":
            state = e.details_dict.get("state", "")
            if state in ("2", "3"):
                ip = e.details_dict.get("neighbor")
                if ip:
                    quarantined_ips.add(ip)

    bh_ips = quarantined_ips - sender_ips
    return bh_ips, quarantined_ips, sender_ips


def check_false_positives(
    quarantined_ips: Set[str], sender_ips: Set[str]
) -> Tuple[Set[str], float]:
    fp_ips = quarantined_ips & sender_ips
    fp_rate = len(fp_ips) / max(len(quarantined_ips), 1)
    return fp_ips, fp_rate


def compute_detection_timeline(
    events: List[Event], bh_ips: Set[str]
) -> Dict:
    """
    Returns a dict with:
      t_first_bh_drop      – sim time of first DATAGRAM_BLACKHOLE_DROP
      t_last_bh_drop       – sim time of last  DATAGRAM_BLACKHOLE_DROP
      bh_drop_total        – total DATAGRAM_BLACKHOLE_DROP count
      per_ip               – {ip: {t_first_rep_update, t_quarantine}} for each BH IP
      t_min_quarantine     – earliest quarantine across all BH IPs (or None)
      bh_drops_after_quar  – DATAGRAM_BLACKHOLE_DROP events after t_min_quarantine
    """
    bh_drops = [e for e in events if e.event_type == "DATAGRAM_BLACKHOLE_DROP"]

    t_first_bh_drop: Optional[float] = bh_drops[0].timestamp if bh_drops else None
    t_last_bh_drop: Optional[float] = bh_drops[-1].timestamp if bh_drops else None
    bh_drop_total = len(bh_drops)

    per_ip: Dict[str, Dict] = {}
    for ip in bh_ips:
        rep_updates = [
            e for e in events
            if e.event_type == "REPUTATION_UPDATE" and e.details_dict.get("neighbor") == ip
        ]
        quar_events = [
            e for e in events
            if e.event_type == "TRUST_STATE_CHANGE"
            and e.details_dict.get("neighbor") == ip
            and e.details_dict.get("state", "") in ("2", "3")
        ]
        per_ip[ip] = {
            "t_first_rep_update": rep_updates[0].timestamp if rep_updates else None,
            "t_quarantine": quar_events[0].timestamp if quar_events else None,
        }

    quarantine_times = [
        v["t_quarantine"] for v in per_ip.values() if v["t_quarantine"] is not None
    ]
    t_min_quarantine = min(quarantine_times) if quarantine_times else None
    # All BHs quarantined at this point — drops after this are true residual
    t_max_quarantine = max(quarantine_times) if quarantine_times else None

    bh_drops_after_first_quar = 0
    bh_drops_after_all_quar = 0
    for e in bh_drops:
        if t_min_quarantine is not None and e.timestamp > t_min_quarantine:
            bh_drops_after_first_quar += 1
        if t_max_quarantine is not None and e.timestamp > t_max_quarantine:
            bh_drops_after_all_quar += 1

    return {
        "t_first_bh_drop": t_first_bh_drop,
        "t_last_bh_drop": t_last_bh_drop,
        "bh_drop_total": bh_drop_total,
        "per_ip": per_ip,
        "t_min_quarantine": t_min_quarantine,
        "t_max_quarantine": t_max_quarantine,
        "bh_drops_after_first_quar": bh_drops_after_first_quar,
        "bh_drops_after_all_quar": bh_drops_after_all_quar,
    }


def analyze_gossip_effect(events: List[Event], bh_ips: Set[str]) -> Dict:
    """
    For each node that quarantined a BH IP: check whether it had any prior
    TRUST_SHARE_RECV (gossip) events, and whether it sent any 2ACKs that
    would indicate direct forwarding observation of the BH.

    Note: REPUTATION_UPDATE can be triggered both by 2ACK timeouts (direct)
    and by processing incoming gossip, so it does not cleanly separate the two.
    We use TRUST_SHARE_RECV as the gossip signal and presence of REPUTATION_UPDATE
    before any gossip as the "direct-first" indicator.

    Returns {
        "total_detecting_pairs": int,  # (node, bh_ip) pairs
        "gossip_received_before_quar": int,  # nodes that got gossip before quarantine
        "rep_before_gossip": int,  # nodes that had REPUTATION_UPDATE before any gossip
        "details": [(node_id, bh_ip, t_quar, t_first_rep, t_first_gossip)]
    }
    """
    node_quarantines: Dict[str, Dict[str, float]] = defaultdict(dict)
    for e in events:
        if (e.event_type == "TRUST_STATE_CHANGE"
                and e.details_dict.get("state", "") in ("2", "3")):
            ip = e.details_dict.get("neighbor")
            if ip in bh_ips:
                # keep earliest quarantine per (node, ip)
                prev = node_quarantines[e.node_id].get(ip, float("inf"))
                if e.timestamp < prev:
                    node_quarantines[e.node_id][ip] = e.timestamp

    details = []
    gossip_recv_count = 0
    rep_before_gossip_count = 0

    for node_id, bh_dict in node_quarantines.items():
        for bh_ip, t_quar in bh_dict.items():
            # First REPUTATION_UPDATE for bh_ip at this node before quarantine
            rep_events = [
                e for e in events
                if e.node_id == node_id
                and e.event_type == "REPUTATION_UPDATE"
                and e.details_dict.get("neighbor") == bh_ip
                and e.timestamp <= t_quar
            ]
            t_first_rep = rep_events[0].timestamp if rep_events else None

            # First TRUST_SHARE_RECV at this node before quarantine
            gossip_events = [
                e for e in events
                if e.node_id == node_id
                and e.event_type == "TRUST_SHARE_RECV"
                and e.timestamp <= t_quar
            ]
            t_first_gossip = gossip_events[0].timestamp if gossip_events else None

            if t_first_gossip is not None:
                gossip_recv_count += 1
            if t_first_rep is not None and (
                t_first_gossip is None or t_first_rep <= t_first_gossip
            ):
                rep_before_gossip_count += 1

            details.append((node_id, bh_ip, t_quar, t_first_rep, t_first_gossip))

    return {
        "total_detecting_pairs": len(details),
        "gossip_received_before_quar": gossip_recv_count,
        "rep_before_gossip": rep_before_gossip_count,
        "details": details,
    }


# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------

def _fmt_time(t: Optional[float]) -> str:
    return f"{t:.3f} s" if t is not None else "n/a"


def _fmt_delta(t: Optional[float], ref: Optional[float]) -> str:
    if t is None or ref is None:
        return "n/a"
    return f"(Δ = +{t - ref:.3f} s)"


def print_report(events: List[Event]) -> None:
    if not events:
        print("No events found in log.")
        return

    bh_ips, quarantined_ips, sender_ips = identify_bh_ips(events)
    fp_ips, fp_rate = check_false_positives(quarantined_ips, sender_ips)
    tl = compute_detection_timeline(events, bh_ips)
    gossip = analyze_gossip_effect(events, bh_ips)

    t_first = tl["t_first_bh_drop"]
    t_min_quar = tl["t_min_quarantine"]

    # --- Section 1 ---
    print("=" * 60)
    print("BH DETECTION VERIFICATION")
    print("=" * 60)
    if bh_ips:
        print(f"BH nodes detected (quarantined): {len(bh_ips)}")
        for ip in sorted(bh_ips):
            print(f"  {ip}")
    else:
        print("No BH nodes detected (no quarantine events, or log has no attack).")
    if quarantined_ips - bh_ips:
        print(f"Quarantined nodes (sender overlap removed): "
              f"{sorted(quarantined_ips - bh_ips)}")

    # --- Section 2 ---
    print()
    print("=" * 60)
    print("FALSE POSITIVE RATE")
    print("=" * 60)
    print(f"Quarantined IPs : {len(quarantined_ips)}")
    print(f"Sender IPs      : {len(sender_ips)}")
    print(f"False positives : {len(fp_ips)}  ({fp_rate * 100:.1f}%)")
    if fp_ips:
        print(f"  FP IPs: {sorted(fp_ips)}")

    # --- Section 3 ---
    print()
    print("=" * 60)
    print("DETECTION TIMELINE")
    print("=" * 60)
    print(f"Total DATAGRAM_BLACKHOLE_DROP : {tl['bh_drop_total']}")
    print(f"First BH_DROP                 : {_fmt_time(t_first)}")
    print(f"Last  BH_DROP                 : {_fmt_time(tl['t_last_bh_drop'])}")
    print()

    for ip in sorted(bh_ips):
        pi = tl["per_ip"].get(ip, {})
        t_rep = pi.get("t_first_rep_update")
        t_quar = pi.get("t_quarantine")
        print(f"  IP {ip}:")
        print(f"    First REPUTATION_UPDATE : {_fmt_time(t_rep)} "
              f"{_fmt_delta(t_rep, t_first)}")
        print(f"    First QUARANTINE        : {_fmt_time(t_quar)} "
              f"{_fmt_delta(t_quar, t_first)}")

    print()
    print(f"BH_DROPs after FIRST quarantine : {tl['bh_drops_after_first_quar']}"
          f"  (includes second BH still active)")
    print(f"BH_DROPs after ALL  quarantined : {tl['bh_drops_after_all_quar']}"
          f"  (true residual; expected ≈ 0)")

    # --- Section 4 ---
    print()
    print("=" * 60)
    print("GOSSIP EFFECT")
    print("=" * 60)
    total = gossip["total_detecting_pairs"]
    if total == 0:
        print("No nodes quarantined any BH IPs.")
    else:
        g = gossip["gossip_received_before_quar"]
        d = gossip["rep_before_gossip"]
        print(f"(node, BH-IP) quarantine pairs : {total}")
        print(f"  received gossip before quar  : {g}  ({g/total*100:.0f}%)")
        print(f"  had REPUTATION_UPDATE first  : {d}  ({d/total*100:.0f}%)")
        print()
        print("  Detail (node → BH-IP: t_rep_update / t_gossip / t_quarantine):")
        for node_id, bh_ip, t_quar, t_rep, t_gos in sorted(gossip["details"]):
            rep_s = f"{t_rep:.2f}s" if t_rep is not None else "   n/a"
            gos_s = f"{t_gos:.2f}s" if t_gos is not None else "   n/a"
            print(f"    {node_id:12s} → {bh_ip:15s}  "
                  f"rep={rep_s}  gossip={gos_s}  quar={t_quar:.2f}s")

    # --- Dissertation summary ---
    print()
    print("=" * 60)
    print("DISSERTATION SUMMARY TABLE")
    print("=" * 60)
    if bh_ips and t_first is not None:
        header = (f"{'BH IP':<16} {'t_first_drop':>12} {'t_rep_update':>13}"
                  f" {'t_quarantine':>13} {'Δ_detect':>10}")
        print(header)
        print("-" * len(header))
        for ip in sorted(bh_ips):
            pi = tl["per_ip"].get(ip, {})
            t_rep = pi.get("t_first_rep_update")
            t_quar = pi.get("t_quarantine")
            delta = f"{t_quar - t_first:.1f}s" if t_quar is not None else "n/a"
            print(f"{ip:<16} {_fmt_time(t_first):>12} {_fmt_time(t_rep):>13} "
                  f"{_fmt_time(t_quar):>13} {delta:>10}")
        print()
        print(f"FP rate: {fp_rate * 100:.1f}% | "
              f"BH_DROPs after all quarantined: {tl['bh_drops_after_all_quar']} | "
              f"Nodes w/ gossip before quar: {gossip['gossip_received_before_quar']}/{total}")
    else:
        print("No BH activity detected in this log.")


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = args[0] if args else os.path.join(script_dir, "simulation_log.csv")
    events = load_events(csv_path)
    print_report(events)


if __name__ == "__main__":
    main()

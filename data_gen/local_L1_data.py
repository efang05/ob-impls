import argparse
import csv
import random
from collections import namedtuple
from statistics import mean
from typing import List, Sequence, Optional
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
DEFAULT_CSV = PROJECT_ROOT / "data" / "local_L1_data.csv"

Update = namedtuple("Update", "seq bid_px bid_sz ask_px ask_sz")


def count_out_of_order(updates: Sequence[Update]) -> tuple[int, float]:
    out_of_order = 0
    lags = []
    highest = -1
    for u in updates:
        if u.seq < highest:
            out_of_order += 1
            lags.append(highest - u.seq)
        else:
            highest = u.seq
    avg_lag = mean(lags) if lags else 0.0
    return out_of_order, avg_lag


def write_csv(path: str, updates: Sequence[Update]) -> None:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(Update._fields)
        w.writerows(updates)


def generate_updates(
    n: int,
    reorder_rate: float = 5e-4,
    max_gap: int = 3,
    gap_decay: float = 0.4,
    seed: Optional[int] = None,
) -> List[Update]:

    if seed is not None:
        random.seed(seed)

    updates: List[Update] = []
    px = 100.00
    for seq in range(n):
        px += random.uniform(-0.01, 0.01)
        bid_px = round(px, 2)
        ask_px = round(px + 0.01, 2)
        bid_sz = random.randint(1, 100)
        ask_sz = random.randint(1, 100)
        updates.append(Update(seq, bid_px, bid_sz, ask_px, ask_sz))

    arrival = updates
    i = 0
    while i < n:
        if random.random() < reorder_rate:
            gap = 1
            while gap < max_gap and random.random() < gap_decay:
                gap += 1
            j = min(i + gap, n - 1)
            arrival[i], arrival[j] = arrival[j], arrival[i]
            i += gap
        i += 1
    return arrival


if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Generate L1 feed test data")
    p.add_argument(
        "--n",
        type=int,
        default=100_000,
        help="Number of messages to simulate (default: 100k)",
    )
    p.add_argument(
        "--reorder",
        type=float,
        default=5e-4,
        help="Out of-order probability per msg (default: 0.05%)",
    )
    p.add_argument(
        "--max-gap",
        type=int,
        default=3,
        help="Maximum sequence gap for delayed packet (default: 3)",
    )
    p.add_argument(
        "--gap-decay",
        type=float,
        default=0.4,
        help="Geometric decay factor for gaps (default: 0.4)",
    )
    p.add_argument(
        "--seed", type=int, default=None, help="RNG seed for reproducibility"
    )

    args = p.parse_args()

    updates = generate_updates(
        n=args.n,
        reorder_rate=args.reorder,
        max_gap=args.max_gap,
        gap_decay=args.gap_decay,
        seed=args.seed,
    )

    ooo_count, avg_lag = count_out_of_order(updates)
    print(
        f"Generated {len(updates):,} updates - out-of-order: {ooo_count} ({ooo_count/len(updates):.4%}), "
        f"avg lag {avg_lag:.2f} seqs."
    )

    write_csv(DEFAULT_CSV, updates)
    print(f"CSV written to {DEFAULT_CSV}")

#!/usr/bin/env python3
"""Check the temporal alignment between the images and traj_truth.txt, and re-evaluate runs.

1) Alignment test (needs an LB_DEBUG_ODOM log of the slice, online or offline odometry):
       LB_DEBUG_ODOM=1 ./build/lambda_brief_loc ... 2> odom.log
       python3 scripts/check_gt_alignment.py --gt ../Datasets/ADM/0036/traj_truth.txt --odom odom.log
   For each lag L it correlates the VO yaw increments / speeds of transition k with those of
   GT transition k+L. The best L is the shift between images and GT rows (ADM/0036: L = -2,
   i.e. image t matches GT row t-2 -> use --gt_offset 86 instead of 84).

2) Re-evaluation of existing results with a shifted GT (no re-run needed):
       python3 scripts/check_gt_alignment.py --gt .../traj_truth.txt --errors res/errors.txt [more...] --lag -2
   errors.txt column 0 is the slice frame index; GT row = frame + row0 + lag, where
   row0 = start_frame - gt_offset (0 for the usual --start_frame 84 / offset 84 on ADM/0036).
"""
import argparse
import math
import numpy as np


def load_gt(path):
    rows = [list(map(float, l.split())) for l in open(path) if l.strip()]
    return np.array(rows)  # x y z yaw_deg


def load_odom(path):
    out = {}
    for line in open(path):
        if not line.startswith("ODOM "):
            continue
        p = line.split()
        f = int(p[1])
        dx, dy, dz, dyaw = map(float, p[2:6])
        out[f] = (dx, dy, dz, dyaw, int(p[6]) != 0)
    return out


def alignment(gt, odom, row0, z_gain, max_lag):
    n = len(gt)
    frames = sorted(odom)
    gyaw = np.radians(gt[:, 3])
    print(f"{'lag':>4} {'corr dyaw':>10} {'corr speed':>11}   (image transition k vs GT transition k+lag)")
    best = None
    for lag in range(-max_lag, max_lag + 1):
        a_y, b_y, a_s, b_s = [], [], [], []
        for f in frames:
            dx, dy, dz, dyaw, ok = odom[f]
            if not ok:
                continue
            r1 = f + row0 + lag          # GT row of the image f (shifted)
            r0 = r1 - 1
            if r0 < 0 or r1 >= n:
                continue
            a_y.append(dyaw)
            b_y.append(math.remainder(gyaw[r1] - gyaw[r0], 2 * math.pi))
            a_s.append(math.hypot(dx, dy) * z_gain * gt[r0, 2])
            b_s.append(math.hypot(gt[r1, 0] - gt[r0, 0], gt[r1, 1] - gt[r0, 1]))
        if len(a_y) < 10:
            continue
        cy = np.corrcoef(a_y, b_y)[0, 1]
        cs = np.corrcoef(a_s, b_s)[0, 1]
        print(f"{lag:>4} {cy:>10.3f} {cs:>11.3f}")
        score = cy + cs
        if best is None or score > best[1]:
            best = (lag, score)
    if best:
        print(f"\nBest lag: {best[0]:+d}  ->  image t <-> GT row t{best[0]:+d}; "
              f"gt_offset should be (current offset) {-best[0]:+d}")


def reevaluate(gt, files, row0, lag, m_per_px):
    t = np.arange(len(gt))
    print(f"{'file':60s} {'mean(lag0)':>10} {'mean(lag)':>10} {'med(lag0)':>10} {'med(lag)':>10}")
    for path in files:
        d = np.loadtxt(path, skiprows=1, ndmin=2)
        f = d[:, 0]
        ex, ey = d[:, 1], d[:, 2]

        def err(L):
            s = f + row0 + L
            ok = (s >= 0) & (s <= len(gt) - 1)
            gx = np.interp(s, t, gt[:, 0])
            gy = np.interp(s, t, gt[:, 1])
            e = np.hypot(ex - gx, ey - gy) * m_per_px
            e[~ok] = np.nan
            return e

        e0, e1 = err(0), err(lag)
        m = ~np.isnan(e0) & ~np.isnan(e1)
        print(f"{path[-60:]:60s} {np.mean(e0[m]):>10.2f} {np.mean(e1[m]):>10.2f} "
              f"{np.median(e0[m]):>10.2f} {np.median(e1[m]):>10.2f}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gt", required=True)
    ap.add_argument("--odom", help="LB_DEBUG_ODOM log (stderr of lambda_brief_loc)")
    ap.add_argument("--errors", nargs="*", default=[])
    ap.add_argument("--row0", type=int, default=0, help="start_frame - gt_offset (default 0)")
    ap.add_argument("--lag", type=float, default=-2.0, help="shift for --errors (frames, may be fractional)")
    ap.add_argument("--max_lag", type=int, default=4)
    ap.add_argument("--z_gain", type=float, default=0.87)
    ap.add_argument("--m_per_px", type=float, default=0.188679245)
    a = ap.parse_args()
    gt = load_gt(a.gt)
    if a.odom:
        alignment(gt, load_odom(a.odom), a.row0, a.z_gain, a.max_lag)
    if a.errors:
        reevaluate(gt, a.errors, a.row0, a.lag, a.m_per_px)


if __name__ == "__main__":
    main()

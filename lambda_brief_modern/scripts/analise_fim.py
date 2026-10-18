#!/usr/bin/env python3
"""Metricas e graficos para a analise do fim do voo ADM2 (docs/analise_fim_do_voo_adm2.md).

Le um ou mais errors.txt (formato: frame meanPX meanPY meanPZ trueX trueY trueZ
error_pos_m error_yaw_deg) e imprime/])salva uma tabela de metricas por execucao,
opcionalmente recalculadas contra o GT corrigido (traj_truth_corr.txt), alem de dois
graficos por grupo: erro (m) x frame e z_est/z_GT x frame.

Uso:
  python3 scripts/analise_fim.py \
      --run "analise_fim/E1_repro_seed42:seed42 (artigo)" \
      --run "analise_fim/E2_seed1:seed1" \
      --group-name E2_seeds --outdir analise_fim/plots \
      --corr-gt "../Datasets/ADM/0036/traj_truth_corr.txt" \
      --pixel-to-meter 0.188679245
"""
import argparse
import os
import sys
import math

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SEGMENTS = [(0, 23), (24, 84), (85, 119), (120, 174), (175, 195)]


def read_errors(path):
    rows = []
    with open(path) as f:
        header = f.readline()
        for line in f:
            parts = line.split()
            if len(parts) < 9:
                continue
            rows.append([float(x) for x in parts])
    arr = np.array(rows)
    return {
        "frame": arr[:, 0].astype(int),
        "meanPX": arr[:, 1], "meanPY": arr[:, 2], "meanPZ": arr[:, 3],
        "trueX": arr[:, 4], "trueY": arr[:, 5], "trueZ": arr[:, 6],
        "error_pos_m": arr[:, 7], "error_yaw_deg": arr[:, 8],
    }


def read_corr_gt(path, row_offset, n_frames):
    rows = []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) < 4:
                continue
            rows.append([float(x) for x in parts])
    arr = np.array(rows)
    sl = arr[row_offset:row_offset + n_frames]
    return {"x": sl[:, 0], "y": sl[:, 1], "z": sl[:, 2], "yaw_deg": sl[:, 3]}


def recompute_error_against_corr(d, corr, pixel_to_meter):
    n = min(len(d["frame"]), len(corr["x"]))
    dx = (d["meanPX"][:n] - corr["x"][:n]) * pixel_to_meter
    dy = (d["meanPY"][:n] - corr["y"][:n]) * pixel_to_meter
    err = np.hypot(dx, dy)
    return err


def segment_mask(frames, lo, hi):
    return (frames >= lo) & (frames <= hi)


def metrics_for_run(label, path, corr_gt=None, corr_row_offset=0, pixel_to_meter=0.188679245):
    d = read_errors(os.path.join(path, "errors.txt"))
    frames = d["frame"]
    err = d["error_pos_m"]
    z_ratio = d["meanPZ"] / d["trueZ"]

    m = {"label": label, "path": path, "n_frames": len(frames)}
    m["mean"] = float(np.mean(err))
    m["median"] = float(np.median(err))
    m["rmse"] = float(np.sqrt(np.mean(err ** 2)))
    m["final"] = float(err[-1])
    m["last10_mean"] = float(np.mean(err[-10:]))

    below30 = np.where(err < 30.0)[0]
    m["first_frame_below_30"] = int(frames[below30[0]]) if len(below30) else None
    m["n_frames_below_30"] = int(len(below30))

    for lo, hi in SEGMENTS:
        mask = segment_mask(frames, lo, hi)
        m[f"seg_{lo}_{hi}_mean"] = float(np.mean(err[mask])) if mask.any() else float("nan")
        m[f"seg_{lo}_{hi}_zratio"] = float(np.mean(z_ratio[mask])) if mask.any() else float("nan")

    if corr_gt is not None:
        err_corr = recompute_error_against_corr(d, corr_gt, pixel_to_meter)
        m["mean_corr"] = float(np.mean(err_corr))
        m["median_corr"] = float(np.median(err_corr))
        m["rmse_corr"] = float(np.sqrt(np.mean(err_corr ** 2)))
        m["final_corr"] = float(err_corr[-1])
        m["last10_mean_corr"] = float(np.mean(err_corr[-10:]))
        for lo, hi in SEGMENTS:
            mask = segment_mask(frames[:len(err_corr)], lo, hi)
            m[f"seg_{lo}_{hi}_mean_corr"] = float(np.mean(err_corr[mask])) if mask.any() else float("nan")

    m["_data"] = d
    return m


def print_table(all_metrics, out_md=None):
    cols = ["label", "mean", "median", "rmse", "final", "last10_mean",
            "first_frame_below_30", "n_frames_below_30"]
    lines = []
    header = "| " + " | ".join(cols) + " |"
    sep = "|" + "|".join(["---"] * len(cols)) + "|"
    lines.append(header)
    lines.append(sep)
    for m in all_metrics:
        row = []
        for c in cols:
            v = m.get(c)
            if v is None:
                row.append("-")
            elif isinstance(v, float):
                row.append(f"{v:.2f}")
            else:
                row.append(str(v))
        lines.append("| " + " | ".join(row) + " |")
    table = "\n".join(lines)
    print(table)
    if out_md:
        with open(out_md, "a") as f:
            f.write(table + "\n\n")
    return table


def plot_group(all_metrics, group_name, outdir):
    os.makedirs(outdir, exist_ok=True)
    fig1, ax1 = plt.subplots(figsize=(10, 5))
    fig2, ax2 = plt.subplots(figsize=(10, 5))
    for m in all_metrics:
        d = m["_data"]
        ax1.plot(d["frame"], d["error_pos_m"], label=m["label"], linewidth=1.3)
        ax2.plot(d["frame"], d["meanPZ"] / d["trueZ"], label=m["label"], linewidth=1.3)

    ax1.set_xlabel("frame (offset da fatia)")
    ax1.set_ylabel("erro de posicao (m)")
    ax1.set_title(f"Erro de posicao x frame — {group_name}")
    ax1.axhline(30.0, color="gray", linestyle="--", linewidth=0.8, label="30 m")
    ax1.legend(fontsize=8)
    ax1.grid(alpha=0.3)
    fig1.tight_layout()
    p1 = os.path.join(outdir, f"{group_name}_erro.png")
    fig1.savefig(p1, dpi=130)
    plt.close(fig1)

    ax2.set_xlabel("frame (offset da fatia)")
    ax2.set_ylabel("z_est / z_GT")
    ax2.set_title(f"z_est/z_GT x frame — {group_name}")
    ax2.axhline(1.0, color="gray", linestyle="--", linewidth=0.8)
    ax2.axhline(0.87, color="gray", linestyle=":", linewidth=0.8, label="0.87 (escala fisica)")
    ax2.legend(fontsize=8)
    ax2.grid(alpha=0.3)
    fig2.tight_layout()
    p2 = os.path.join(outdir, f"{group_name}_zratio.png")
    fig2.savefig(p2, dpi=130)
    plt.close(fig2)

    return p1, p2


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", action="append", required=True,
                     help="dir:label do resultado (repetivel)")
    ap.add_argument("--group-name", required=True)
    ap.add_argument("--outdir", default="analise_fim/plots")
    ap.add_argument("--corr-gt", default=None,
                     help="traj_truth_corr.txt para recalcular o erro")
    ap.add_argument("--corr-gt-row-offset", type=int, default=0,
                     help="linha do corr-gt correspondente ao frame 0 desta execucao")
    ap.add_argument("--pixel-to-meter", type=float, default=0.188679245)
    ap.add_argument("--table-out", default=None)
    args = ap.parse_args()

    corr_gt = None
    all_metrics = []
    for spec in args.run:
        path, _, label = spec.partition(":")
        label = label or path
        cg = None
        if args.corr_gt:
            d_tmp = read_errors(os.path.join(path, "errors.txt"))
            cg = read_corr_gt(args.corr_gt, args.corr_gt_row_offset, len(d_tmp["frame"]))
        m = metrics_for_run(label, path, corr_gt=cg, corr_row_offset=args.corr_gt_row_offset,
                             pixel_to_meter=args.pixel_to_meter)
        all_metrics.append(m)

    if args.table_out:
        with open(args.table_out, "a") as f:
            f.write(f"\n### {args.group_name}\n\n")
    print(f"\n=== {args.group_name} ===")
    print_table(all_metrics, args.table_out)

    p1, p2 = plot_group(all_metrics, args.group_name, args.outdir)
    print(f"Graficos salvos em: {p1} , {p2}")


if __name__ == "__main__":
    main()

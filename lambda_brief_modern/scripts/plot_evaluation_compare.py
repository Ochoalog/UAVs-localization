#!/usr/bin/env python3
"""
plot_evaluation_compare.py — figura de avaliação (estilo evaluation.png) comparando abBRIEF × λ-BRIEF

Para cada (voo, mapa, regime) do benchmark gera uma figura 2×2:
  (a) trajetória no mapa: GT re-sincronizado + estimativa da seed representativa de cada descritor
      (seed de MAE mediano), sobre um recorte do próprio mapa
  (b) erro de posição por frame: todas as seeds (finas) + mediana (grossa), limiar de convergência
      e frame mediano em que cada descritor convergiu
  (c) CDF do erro de posição (todos os frames de todas as seeds)
  (d) erro de yaw por frame (mediana entre seeds)

O erro de posição é recalculado contra <dataset>/traj_truth_sync.txt (igual ao consolidate_benchmark.py),
então os números batem com summary.md. O yaw vem do errors.txt (GT original do motor).

Uso (de dentro de lambda_brief_modern/):
  python3 scripts/plot_evaluation_compare.py --benchmark_dir experimentos/2026-10-15_piloto_baro_vs_noalt
  python3 scripts/plot_evaluation_compare.py --benchmark_dir experimentos/2026-10-15_piloto_baro_vs_noalt \
      --datasets ADM0036 --maps ADM_35_17_08_2018 --regimes final_baro
  python3 scripts/plot_evaluation_compare.py --benchmark_dir experimentos/2026-10-15_piloto_baro_vs_noalt --seed 3   # uma seed só
  python3 scripts/plot_evaluation_compare.py --benchmark_dir experimentos/2026-10-15_piloto_baro_vs_noalt --descs abBRIEF  # só um descritor
  --y_cap 0  deixa o eixo do erro automático (por padrão corta em 150 m para o rastreio ficar legível)

Saída: <benchmark_dir>/summary_traj_truth_sync/evaluation/<voo>_<mapa>_<regime>.png
"""

import argparse
import json
import os
import sys

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
from consolidate_benchmark import (discover, load_errors, run_context, eval_gt_file, run_metrics,  # noqa: E402
                                   COLORS, SURFACE, INK, INK2, GRID, style_axes, desc_label)

GT_COLOR = "#1a8f3a"
DESC_ORDER = ["abBRIEF", "lambdaBRIEF"]
REGIME_LABEL = {"final_baro": "duas fases + barômetro real", "final_noalt": "duas fases sem altímetro",
                "final_gtalt": "duas fases + altímetro emulado (GT)", "final": "duas fases + altímetro emulado (GT)",
                "vanilla": "vanilla"}


def load_yaw(path):
    """Coluna error_yaw_deg do errors.txt, indexada por frame."""
    out = {}
    with open(path) as f:
        f.readline()
        for line in f:
            p = line.split()
            if len(p) >= 9:
                try:
                    out[int(float(p[0]))] = abs(float(p[8]))
                except ValueError:
                    pass
    return out


def load_gt_xy(dataset_dir, gt_name):
    path = os.path.join(dataset_dir, gt_name)
    if not os.path.exists(path):
        return None
    return np.array([[float(v) for v in l.split()[:2]] for l in open(path) if l.strip()])


def map_path_of(seed_dir):
    cmd_path = os.path.join(seed_dir, "cmd.txt")
    if not os.path.exists(cmd_path):
        return None
    cmd = json.load(open(cmd_path))
    return cmd[cmd.index("--map") + 1] if "--map" in cmd else None


def load_run(r, gt_name, conv_thresh, conv_frames, lost_thresh):
    d = load_errors(r["path"])
    ctx = run_context(r)
    if d is None or ctx is None:
        return None
    frames, err = eval_gt_file(d, ctx, gt_name)
    if frames is None or len(err) < conv_frames:
        return None
    keep = np.isin(d["frame"], frames)
    yaw = load_yaw(r["path"])
    r = dict(r)
    r.update(ctx)
    r["frames"] = frames
    r["err"] = err
    r["estX"] = d["estX"][keep]
    r["estY"] = d["estY"][keep]
    r["yaw"] = np.array([yaw.get(int(f), np.nan) for f in frames])
    r["m"] = run_metrics(err, conv_thresh, conv_frames, lost_thresh)
    return r


def representative(runs, seed=None):
    if seed is not None:
        for r in runs:
            if r["seed"] == seed:
                return r
        return None
    ordered = sorted(runs, key=lambda r: r["m"]["mae"])
    return ordered[(len(ordered) - 1) // 2]


def common_matrix(runs, key):
    common = sorted(set.intersection(*[set(r["frames"].tolist()) for r in runs]))
    cs = set(common)
    mat = np.array([[v for f, v in zip(r["frames"], r[key]) if f in cs] for r in runs], dtype=float)
    return np.array(common), mat


def draw_map(ax, map_path, xs, ys, pad=150):
    if not map_path or not os.path.exists(map_path):
        return False
    try:
        img = mpimg.imread(map_path)
    except Exception as exc:  # noqa: BLE001
        print(f"  [aviso] não li o mapa {map_path}: {exc}")
        return False
    h, w = img.shape[:2]
    x0 = int(max(0, np.nanmin(xs) - pad)); x1 = int(min(w, np.nanmax(xs) + pad))
    y0 = int(max(0, np.nanmin(ys) - pad)); y1 = int(min(h, np.nanmax(ys) + pad))
    if x1 <= x0 or y1 <= y0:
        return False
    ax.imshow(img[y0:y1, x0:x1], extent=(x0, x1, y1, y0), alpha=0.55, zorder=0)
    return True


def plot_case(dataset, map_name, regime, by_desc, gt_name, conv_thresh, seed, use_map, out_path, y_cap=150.0):
    descs = [d for d in DESC_ORDER if d in by_desc] + [d for d in by_desc if d not in DESC_ORDER]
    reps = {d: representative(by_desc[d], seed) for d in descs}
    reps = {d: r for d, r in reps.items() if r is not None}
    if not reps:
        return False
    any_run = next(iter(reps.values()))
    gt = load_gt_xy(any_run["dataset_dir"], gt_name)
    idx = any_run["start_frame"] + any_run["frames"]
    idx = idx[(idx >= 0) & (idx < len(gt))]
    gtx, gty = gt[idx, 0], gt[idx, 1]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10.5), facecolor=SURFACE)
    n_seeds = {d: len(by_desc[d]) for d in descs}
    which = (f"seed {seed}" if seed is not None else
             f"trajetória = seed de MAE mediano; curvas = {max(n_seeds.values())} seeds")
    fig.suptitle(f"{dataset} · {map_name} · {REGIME_LABEL.get(regime, regime)}\n"
                 f"{' × '.join(desc_label(d) for d in descs)} ({which}; GT re-sincronizado)", color=INK, fontsize=12)

    # (a) trajetória
    ax = axes[0, 0]
    style_axes(ax)
    allx = np.concatenate([gtx] + [r["estX"] for r in reps.values()])
    ally = np.concatenate([gty] + [r["estY"] for r in reps.values()])
    has_map = use_map and draw_map(ax, map_path_of(any_run["seed_dir"]), allx, ally)
    ax.plot(gtx, gty, "-o", color=GT_COLOR, ms=2.5, lw=2.0, label="Ground truth", zorder=3)
    ax.plot(gtx[0], gty[0], marker="*", color=GT_COLOR, ms=14, mec="white", zorder=5)
    for d, r in reps.items():
        ax.plot(r["estX"], r["estY"], "--s", color=COLORS.get(d, INK2), ms=2.2, lw=1.4,
                label=f"{desc_label(d)} · seed {r['seed']} (MAE {r['m']['mae']:.1f} m)", zorder=4)
        t = r["m"]["ttf"]
        if t is not None and t < len(r["estX"]):
            ax.plot(r["estX"][t], r["estY"][t], "o", color=COLORS.get(d, INK2), ms=9, mec="white", mew=1.5,
                    zorder=6)
    ax.set_title("Trajetória no mapa (● = frame em que convergiu, ★ = início)", color=INK, fontsize=10)
    ax.set_xlabel("x no mapa (px)", color=INK2, fontsize=9)
    ax.set_ylabel("y no mapa (px)", color=INK2, fontsize=9)
    if not has_map:
        ax.invert_yaxis()
    ax.set_aspect("equal", adjustable="datalim")
    ax.legend(fontsize=7.5, loc="best", framealpha=0.9)

    # (b) erro por frame
    ax = axes[0, 1]
    style_axes(ax)
    ymax = 0
    for d in descs:
        runs = by_desc[d] if seed is None else [reps[d]]
        c = COLORS.get(d, INK2)
        frames, mat = common_matrix(runs, "err")
        if len(runs) > 1:
            for row in mat:
                ax.plot(frames, row, color=c, lw=0.7, alpha=0.25)
        med = np.median(mat, axis=0)
        ymax = max(ymax, float(np.percentile(mat, 99)))
        ttfs = [r["m"]["ttf"] for r in runs if r["m"]["ttf"] is not None]
        conv = f"{sum(r['m']['converged'] for r in runs)}/{len(runs)} convergiram"
        if ttfs:
            conv += f", frame mediano {int(np.median(ttfs))}"
        ax.plot(frames, med, color=c, lw=2.2, label=f"{desc_label(d)} — {conv}")
        if ttfs:
            ax.axvline(np.median(ttfs), color=c, ls=":", lw=1.4)
    ax.axhline(conv_thresh, color=INK2, ls="--", lw=1, label=f"limiar de convergência ({conv_thresh:.0f} m)")
    top = min(max(ymax * 1.05, conv_thresh * 2), y_cap) if y_cap > 0 else max(ymax * 1.05, conv_thresh * 2)
    ax.set_ylim(0, top)
    if ymax > top:
        ax.text(0.01, 0.97, f"↑ início fora da escala (até {ymax:.0f} m)", transform=ax.transAxes,
                color=INK2, fontsize=7.5, va="top")
    ax.set_title("Erro de posição por frame (finas = seeds, grossa = mediana, ⋮ = convergiu)",
                 color=INK, fontsize=10)
    ax.set_xlabel("frame da fatia", color=INK2, fontsize=9)
    ax.set_ylabel("erro (m)", color=INK2, fontsize=9)
    ax.legend(fontsize=7.5, loc="upper right", framealpha=0.9)

    # (c) CDF
    ax = axes[1, 0]
    style_axes(ax)
    for d in descs:
        runs = by_desc[d] if seed is None else [reps[d]]
        e = np.sort(np.concatenate([r["err"] for r in runs]))
        p = np.arange(1, len(e) + 1) / len(e)
        frac = float(np.mean(e < conv_thresh))
        ax.plot(e, p, color=COLORS.get(d, INK2), lw=2.2,
                label=f"{desc_label(d)} — mediana {np.median(e):.1f} m, {frac:.0%} dos frames < {conv_thresh:.0f} m")
    ax.axvline(conv_thresh, color=INK2, ls="--", lw=1)
    ax.set_xscale("log")
    ax.set_title("Distribuição acumulada do erro (todos os frames)", color=INK, fontsize=10)
    ax.set_xlabel("erro de posição (m, escala log)", color=INK2, fontsize=9)
    ax.set_ylabel("fração dos frames", color=INK2, fontsize=9)
    ax.legend(fontsize=7.5, loc="lower right", framealpha=0.9)

    # (d) yaw
    ax = axes[1, 1]
    style_axes(ax)
    for d in descs:
        runs = by_desc[d] if seed is None else [reps[d]]
        frames, mat = common_matrix(runs, "yaw")
        med = np.nanmedian(mat, axis=0)
        ax.plot(frames, med, color=COLORS.get(d, INK2), lw=1.8,
                label=f"{desc_label(d)} — média {np.nanmean(med):.1f}°")
    ax.set_title("Erro de yaw por frame (mediana entre seeds)", color=INK, fontsize=10)
    ax.set_xlabel("frame da fatia", color=INK2, fontsize=9)
    ax.set_ylabel("erro (graus)", color=INK2, fontsize=9)
    ax.legend(fontsize=7.5, loc="upper right", framealpha=0.9)

    fig.tight_layout(rect=(0, 0, 1, 0.955))
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=170, facecolor=SURFACE)
    plt.close(fig)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--benchmark_dir", default="results_final/B_piloto",
                    help="pasta do benchmark (padrão: o piloto; caminhos antigos são achados em experimentos/)")
    ap.add_argument("--gt_file", default="traj_truth_sync.txt")
    ap.add_argument("--datasets", nargs="*", default=None)
    ap.add_argument("--maps", nargs="*", default=None)
    ap.add_argument("--regimes", nargs="*", default=None)
    ap.add_argument("--descs", nargs="*", default=None,
                    help="só estes descritores (ex.: --descs abBRIEF gera a figura só do abBRIEF)")
    ap.add_argument("--seed", type=int, default=None, help="mostra só esta seed (padrão: todas + representativa)")
    ap.add_argument("--no_map", action="store_true", help="não desenha o recorte do mapa no painel (a)")
    ap.add_argument("--conv_thresh", type=float, default=25.0)
    ap.add_argument("--conv_frames", type=int, default=10)
    ap.add_argument("--lost_thresh", type=float, default=50.0)
    ap.add_argument("--y_cap", type=float, default=150.0,
                    help="teto do eixo do erro por frame em m (0 = automático); o pico inicial fica cortado")
    ap.add_argument("--out_dir", default=None)
    args = ap.parse_args()
    import experimentos as exp
    args.benchmark_dir = exp.resolve(args.benchmark_dir)

    out_dir = args.out_dir or os.path.join(args.benchmark_dir,
                                           f"summary_{os.path.splitext(args.gt_file)[0]}", "evaluation")
    groups = {}
    for r in discover(args.benchmark_dir):
        if args.datasets and r["dataset"] not in args.datasets:
            continue
        if args.maps and r["map"] not in args.maps:
            continue
        if args.regimes and r["regime"] not in args.regimes:
            continue
        if args.descs and r["desc"] not in args.descs:
            continue
        lr = load_run(r, args.gt_file, args.conv_thresh, args.conv_frames, args.lost_thresh)
        if lr is None:
            print(f"  [aviso] ignorado: {r['path']}")
            continue
        groups.setdefault((r["dataset"], r["map"], r["regime"]), {}).setdefault(r["desc"], []).append(lr)

    if not groups:
        print("Nada encontrado. Confira --benchmark_dir e os filtros.")
        sys.exit(1)

    for (ds, mp, rg), by_desc in sorted(groups.items()):
        suffix = f"_seed{args.seed}" if args.seed is not None else ""
        if args.descs:
            suffix = "_" + "_".join(args.descs) + suffix
        out = os.path.join(out_dir, f"{ds}_{mp}_{rg}{suffix}.png")
        if plot_case(ds, mp, rg, by_desc, args.gt_file, args.conv_thresh, args.seed, not args.no_map, out, args.y_cap):
            print(f"[ok] {out}")
    print(f"\nFiguras em: {out_dir}")


if __name__ == "__main__":
    main()

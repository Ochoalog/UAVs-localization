#!/usr/bin/env python3
"""
consolidate_benchmark.py — Consolidação estatística do benchmark abBRIEF × λ-BRIEF

Lê <benchmark_dir>/<dataset>/<mapa>/<descritor>_regime_<regime>/seed_<N>/errors.txt
(estrutura do benchmark_runner.py) e gera, em <benchmark_dir>/summary_<gt>/:

  runs.csv      métricas por execução
  groups.csv    métricas por grupo (dataset × mapa × regime × descritor)
  tests.csv     testes pareados de cada variante λ contra o abBRIEF, por mapa e agregados
  summary.md    tabelas prontas + conclusões tiradas dos p-valores corrigidos (Holm)
  tables.tex    tabela principal em LaTeX
  plots/        comparacao_ab_vs_lambda.png (convergência, rastreio e MAE por voo × regime, com IC 95%),
                convergencia_por_frame.png (% localizadas por frame; cor = descritor, traço = regime),
                <voo>_<regime>_grade.png (por mapa) e <voo>_<regime>_cdf_rastreio.png

Métricas por execução
  convergiu   primeiro frame com erro < --conv_thresh (25 m) por --conv_frames (10) frames seguidos
  ttf         esse frame (índice na fatia)
  rastreio    mediana e média do erro do frame de convergência até o fim (só se convergiu)
  perdas      nº de frames com erro > --lost_thresh (50 m) depois de convergir
  mae / rmse / mediana / final / final_ok (< conv_thresh) do voo inteiro
  s_frame     tempo médio por frame (traj_estimated.txt)
Métrica do artigo (por grupo): ε_t = sqrt(média entre seeds do erro²) no frame t; reporta a média em t.

Testes (pareados por seed: mesma nuvem inicial e mesma odometria)
  convergência  McNemar exato (binomial nos pares discordantes)
  MAE           Wilcoxon pareado, todas as seeds em comum
  rastreio      Wilcoxon pareado, seeds em que os dois convergiram (n ≥ 6)
  Correção de Holm sobre os mapas de cada (dataset, regime, variante, métrica).
  "agregado" = todos os pares mapa × seed do dataset (e de todos os datasets).

Ground truth da avaliação
  --gt_file traj_truth_sync.txt  (recomendado) reavalia o erro contra o GT re-sincronizado pelo
      scripts/extract_altimeter.py (GPS do log no instante exato de cada imagem). Usa a estimativa
      gravada em errors.txt; dataset_dir, start_frame e pixel_to_meter vêm do cmd.txt da execução
      (ou do DATASET_CONFIGS do benchmark_runner.py para resultados antigos sem cmd.txt).
  --gt_lag L  ou  --gt_lag ADM0036=-2,UFRGS=3  reavalia com o GT do frame f + L (inteiro, por
      dataset). Medido pelos timestamps: ADM0036 ≈ −2,3; UFRGS ≈ +2,5; ADM0035 ≈ −0,5.

Mapas duplicados
  Se dois mapas dão errors.txt idênticos em todas as seeds, o segundo é descartado
  (ex.: ADM_35_05_09_2018 = ADM_35_12_09_2018). --keep_duplicates desliga isso.

Uso
  python3 scripts/consolidate_benchmark.py --benchmark_dir experimentos/<data>_A_artigo
  python3 scripts/consolidate_benchmark.py --benchmark_dir experimentos/<data>_B_engine --gt_file traj_truth_sync.txt
  # registra (ou substitui) UM bloco no HISTORICO_RESULTADOS.md:
  python3 scripts/consolidate_benchmark.py --benchmark_dir experimentos/<data>_B_engine --write_history --tag BENCHMARK_FINAL_v1_B
"""

import argparse
import csv
import datetime
import glob
import hashlib
import math
import os
import re
import sys

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from scipy import stats

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
HISTORICO_FILE = os.path.join(MODERN_DIR, "HISTORICO_RESULTADOS.md")
BASELINE = "abBRIEF"
# Ao descartar um mapa duplicado, mantém o nome canônico (o 05/09 é cópia do 12/09).
PREFERRED_MAP_NAMES = ["ADM_35_12_09_2018"]

# Paleta categórica validada (skill dataviz, modo claro). Ordem fixa por descritor.
COLORS = {"lambdaBRIEF": "#2a78d6", "abBRIEF": "#eb6834"}
EXTRA_COLORS = ["#1baf7a", "#6250d6", "#e87ba4", "#008300"]
SURFACE, INK, INK2, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"


# --------------------------------------------------------------------------- leitura

def load_errors(path):
    """errors.txt -> dict de arrays (frame, estX, estY, trueX, trueY, err_m)."""
    rows = []
    with open(path) as f:
        f.readline()
        for line in f:
            p = line.split()
            if len(p) >= 8:
                try:
                    rows.append([float(v) for v in p[:8]])
                except ValueError:
                    continue
    if not rows:
        return None
    a = np.array(rows)
    return {"frame": a[:, 0].astype(int), "estX": a[:, 1], "estY": a[:, 2],
            "trueX": a[:, 4], "trueY": a[:, 5], "err": a[:, 7]}


def shift_gt(d, lag):
    """Erro com o GT do frame f + lag. Escala m/px recuperada do próprio arquivo."""
    if lag == 0:
        return d["frame"], d["err"]
    px = np.hypot(d["estX"] - d["trueX"], d["estY"] - d["trueY"])
    ok = px > 1.0
    if not ok.any():
        return d["frame"], d["err"]
    scale = float(np.median(d["err"][ok] / px[ok]))
    gt = {int(f): (x, y) for f, x, y in zip(d["frame"], d["trueX"], d["trueY"])}
    frames, errs = [], []
    for f, ex, ey in zip(d["frame"], d["estX"], d["estY"]):
        g = gt.get(int(f) + lag)
        if g is None:
            continue
        frames.append(int(f))
        errs.append(math.hypot(ex - g[0], ey - g[1]) * scale)
    return np.array(frames), np.array(errs)


_GT_CACHE = {}


def run_context(r):
    """dataset_dir, start_frame e pixel_to_meter da execução (cmd.txt ou configuração padrão)."""
    cmd_path = os.path.join(r["seed_dir"], "cmd.txt")
    ctx = {}
    if os.path.exists(cmd_path):
        import json
        cmd = json.load(open(cmd_path))
        for key, name in (("--dataset_dir", "dataset_dir"), ("--start_frame", "start_frame"),
                          ("--pixel_to_meter", "pixel_to_meter")):
            if key in cmd:
                ctx[name] = cmd[cmd.index(key) + 1]
    if len(ctx) < 3:
        try:
            sys.path.insert(0, SCRIPT_DIR)
            from benchmark_runner import DATASET_CONFIGS
            cfg = DATASET_CONFIGS.get(r["dataset"])
            if cfg:
                ctx.setdefault("dataset_dir", cfg["dataset_dir"])
                ctx.setdefault("start_frame", cfg["start_frame"])
                ctx.setdefault("pixel_to_meter", cfg["pixel_to_meter"])
        except ImportError:
            pass
    if len(ctx) < 3:
        return None
    return {"dataset_dir": ctx["dataset_dir"], "start_frame": int(ctx["start_frame"]),
            "pixel_to_meter": float(ctx["pixel_to_meter"])}


def eval_gt_file(d, ctx, gt_name):
    path = os.path.join(ctx["dataset_dir"], gt_name)
    if path not in _GT_CACHE:
        if not os.path.exists(path):
            _GT_CACHE[path] = None
        else:
            _GT_CACHE[path] = np.array([[float(v) for v in l.split()[:2]] for l in open(path) if l.strip()])
    gt = _GT_CACHE[path]
    if gt is None:
        return None, None
    idx = ctx["start_frame"] + d["frame"]
    ok = (idx >= 0) & (idx < len(gt))
    err = np.hypot(d["estX"][ok] - gt[idx[ok], 0], d["estY"][ok] - gt[idx[ok], 1]) * ctx["pixel_to_meter"]
    return d["frame"][ok], err


def parse_lags(spec):
    """'-2' -> {'*': -2};  'ADM0036=-2,UFRGS=3' -> {'ADM0036': -2, 'UFRGS': 3}."""
    if spec is None or str(spec).strip() == "":
        return {"*": 0}
    spec = str(spec)
    if "=" not in spec:
        return {"*": int(spec)}
    out = {"*": 0}
    for part in spec.split(","):
        k, v = part.split("=")
        out[k.strip()] = int(v)
    return out


def mean_s_per_frame(seed_dir):
    path = os.path.join(seed_dir, "traj_estimated.txt")
    if not os.path.exists(path):
        return None
    try:
        a = np.loadtxt(path, skiprows=1, ndmin=2)
        return float(np.mean(a[:, -1])) if a.size else None
    except Exception:
        return None


def discover(bench_dir):
    pat = os.path.join(bench_dir, "*", "*", "*_regime_*", "seed_*", "errors.txt")
    runs = []
    for path in sorted(glob.glob(pat)):
        seed_dir = os.path.dirname(path)
        desc_dir = os.path.dirname(seed_dir)
        map_dir = os.path.dirname(desc_dir)
        ds_dir = os.path.dirname(map_dir)
        desc, regime = os.path.basename(desc_dir).split("_regime_", 1)
        m = re.match(r"seed_(\d+)$", os.path.basename(seed_dir))
        if not m or os.path.getsize(path) < 100:
            continue
        runs.append({"dataset": os.path.basename(ds_dir), "map": os.path.basename(map_dir),
                     "regime": regime, "desc": desc, "seed": int(m.group(1)),
                     "path": path, "seed_dir": seed_dir})
    return runs


# --------------------------------------------------------------------------- métricas

def run_metrics(err, conv_thresh, conv_frames, lost_thresh):
    n = len(err)
    conv = None
    below = err < conv_thresh
    for i in range(0, n - conv_frames + 1):
        if below[i:i + conv_frames].all():
            conv = i
            break
    m = {
        "n_frames": n,
        "mae": float(np.mean(err)),
        "median": float(np.median(err)),
        "rmse": float(np.sqrt(np.mean(err ** 2))),
        "final": float(err[-1]),
        "final_ok": bool(err[-1] < conv_thresh),
        "frac_below": float(np.mean(below)),
        "converged": conv is not None,
        "ttf": conv,
        "track_median": None, "track_mean": None, "lost_frames": None,
    }
    if conv is not None:
        tail = err[conv:]
        m["track_median"] = float(np.median(tail))
        m["track_mean"] = float(np.mean(tail))
        m["lost_frames"] = int(np.sum(tail > lost_thresh))
    return m


def wilson(k, n, z=1.96):
    if n == 0:
        return (float("nan"), float("nan"))
    p = k / n
    den = 1 + z * z / n
    c = (p + z * z / (2 * n)) / den
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / den
    return (max(0.0, c - h), min(1.0, c + h))


def bootstrap_median_ci(x, n_boot=5000, seed=0):
    x = np.asarray([v for v in x if v is not None], dtype=float)
    if len(x) < 3:
        return (float("nan"), float("nan"))
    rng = np.random.default_rng(seed)
    meds = np.median(rng.choice(x, size=(n_boot, len(x)), replace=True), axis=1)
    return (float(np.percentile(meds, 2.5)), float(np.percentile(meds, 97.5)))


def mcnemar_exact(conv_a, conv_b):
    b = sum(1 for x, y in zip(conv_a, conv_b) if x and not y)
    c = sum(1 for x, y in zip(conv_a, conv_b) if y and not x)
    n = b + c
    if n == 0:
        return 1.0, b, c
    k = min(b, c)
    try:
        p = stats.binomtest(k, n, 0.5).pvalue
    except AttributeError:  # scipy < 1.7
        p = min(1.0, 2 * sum(math.comb(n, i) for i in range(k + 1)) / 2 ** n)
    return float(p), b, c


def wilcoxon_paired(a, b, min_n=6):
    a, b = np.asarray(a, float), np.asarray(b, float)
    if len(a) < min_n:
        return None
    d = b - a
    if np.allclose(d, 0):
        return 1.0
    try:
        return float(stats.wilcoxon(a, b).pvalue)
    except ValueError:
        return None


def holm(pvals):
    """Correção de Holm-Bonferroni. Aceita None (fica None)."""
    idx = [i for i, p in enumerate(pvals) if p is not None]
    out = [None] * len(pvals)
    order = sorted(idx, key=lambda i: pvals[i])
    m = len(order)
    running = 0.0
    for rank, i in enumerate(order):
        adj = min(1.0, (m - rank) * pvals[i])
        running = max(running, adj)
        out[i] = running
    return out


# --------------------------------------------------------------------------- agregação

def find_duplicate_maps(runs):
    """Mapas cujos errors.txt são idênticos em todas as seeds de todos os descritores."""
    sig = {}
    for r in runs:
        key = (r["dataset"], r["map"])
        sig.setdefault(key, []).append((r["regime"], r["desc"], r["seed"], r["hash"]))
    by_sig = {}
    for key, lst in sig.items():
        s = hashlib.md5(repr(sorted(lst)).encode()).hexdigest()
        by_sig.setdefault((key[0], s), []).append(key[1])
    dups = {}
    for (ds, _), maps in by_sig.items():
        if len(maps) > 1:
            maps = sorted(maps, key=lambda m: (m not in PREFERRED_MAP_NAMES, m))
            for extra in maps[1:]:
                dups[(ds, extra)] = maps[0]
    return dups


def group_summary(runs):
    convs = [r["converged"] for r in runs]
    k, n = sum(convs), len(runs)
    lo, hi = wilson(k, n)
    tracks = [r["track_median"] for r in runs if r["converged"]]
    tlo, thi = bootstrap_median_ci(tracks)
    ttfs = [r["ttf"] for r in runs if r["ttf"] is not None]
    lost = [r["lost_frames"] for r in runs if r["lost_frames"] is not None]
    spf = [r["s_frame"] for r in runs if r["s_frame"] is not None]
    # Métrica do artigo: RMS entre seeds por frame (só frames presentes em todas as execuções)
    common = set.intersection(*[set(r["frames"].tolist()) for r in runs]) if runs else set()
    fr = sorted(common)
    if fr:
        mat = np.array([[e for f, e in zip(r["frames"], r["err"]) if f in common] for r in runs])
        eps = np.sqrt(np.mean(mat ** 2, axis=0))
        eps_mean = float(np.mean(eps))
    else:
        eps_mean = float("nan")
    return {
        "n_runs": n,
        "conv_rate": k / n if n else float("nan"), "conv_lo": lo, "conv_hi": hi,
        "ttf_median": float(np.median(ttfs)) if ttfs else None,
        "track_median": float(np.median(tracks)) if tracks else None,
        "track_lo": tlo, "track_hi": thi,
        "track_mean": float(np.mean([r["track_mean"] for r in runs if r["converged"]])) if tracks else None,
        "lost_mean": float(np.mean(lost)) if lost else None,
        "mae_mean": float(np.mean([r["mae"] for r in runs])),
        "mae_median": float(np.median([r["mae"] for r in runs])),
        "final_ok_rate": float(np.mean([r["final_ok"] for r in runs])),
        "eps_paper": eps_mean,
        "s_frame": float(np.mean(spf)) if spf else None,
    }


def paired_tests(base_runs, var_runs):
    A = {r["seed"]: r for r in base_runs}
    B = {r["seed"]: r for r in var_runs}
    seeds = sorted(set(A) & set(B))
    if not seeds:
        return None
    ca = [A[s]["converged"] for s in seeds]
    cb = [B[s]["converged"] for s in seeds]
    p_conv, b, c = mcnemar_exact(ca, cb)
    mae_a = [A[s]["mae"] for s in seeds]
    mae_b = [B[s]["mae"] for s in seeds]
    both = [s for s in seeds if A[s]["converged"] and B[s]["converged"]]
    tr_a = [A[s]["track_median"] for s in both]
    tr_b = [B[s]["track_median"] for s in both]
    return {
        "n_pairs": len(seeds),
        "conv_base": float(np.mean(ca)), "conv_var": float(np.mean(cb)),
        "d_conv_pp": 100 * (float(np.mean(cb)) - float(np.mean(ca))),
        "mcnemar_b_only_base": b, "mcnemar_c_only_var": c, "p_conv": p_conv,
        "d_mae_median": float(np.median(np.array(mae_b) - np.array(mae_a))),
        "p_mae": wilcoxon_paired(mae_a, mae_b),
        "n_both_conv": len(both),
        "d_track_median": float(np.median(np.array(tr_b) - np.array(tr_a))) if both else None,
        "p_track": wilcoxon_paired(tr_a, tr_b),
    }


def verdict(p_adj, delta, better_if_negative, alpha=0.05):
    if p_adj is None:
        return "n insuficiente"
    if p_adj >= alpha:
        return "sem diferença significativa"
    good = (delta < 0) if better_if_negative else (delta > 0)
    return "λ melhor" if good else "λ pior"


# --------------------------------------------------------------------------- gráficos

def color_for(desc, order):
    if desc in COLORS:
        return COLORS[desc]
    extras = [d for d in order if d not in COLORS]
    return EXTRA_COLORS[extras.index(desc) % len(EXTRA_COLORS)]


def style_axes(ax):
    ax.set_facecolor(SURFACE)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=INK2, labelsize=8)
    ax.grid(True, color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)


def desc_label(desc):
    if desc == "abBRIEF":
        return "abBRIEF (α=0)"
    if desc == "lambdaBRIEF":
        return "λ-BRIEF (α=0.5)"
    m = re.match(r"lambdaBRIEF_a(.+)$", desc)
    return f"λ-BRIEF (α={m.group(1)})" if m else desc


def curves(runs):
    common = sorted(set.intersection(*[set(r["frames"].tolist()) for r in runs]))
    cs = set(common)
    mat = np.array([[e for f, e in zip(r["frames"], r["err"]) if f in cs] for r in runs])
    return np.array(common), mat


def plot_grid(dataset, regime, maps, descs, data, conv_thresh, out_path):
    """Linha de cima: % de execuções com erro < limiar por frame. Linha de baixo: mediana + IQR."""
    ncol = len(maps)
    fig, axes = plt.subplots(2, ncol, figsize=(3.6 * ncol + 0.6, 6.2), dpi=150, squeeze=False,
                             sharex="col")
    fig.patch.set_facecolor(SURFACE)
    for j, mp in enumerate(maps):
        a_top, a_bot = axes[0, j], axes[1, j]
        style_axes(a_top); style_axes(a_bot)
        for d in descs:
            runs = data.get((mp, d))
            if not runs:
                continue
            fr, mat = curves(runs)
            col = color_for(d, descs)
            a_top.plot(fr, 100 * np.mean(mat < conv_thresh, axis=0), color=col, lw=2,
                       label=f"{desc_label(d)}, n = {len(runs)}")
            med = np.median(mat, axis=0)
            q1, q3 = np.percentile(mat, 25, axis=0), np.percentile(mat, 75, axis=0)
            a_bot.fill_between(fr, q1, q3, color=col, alpha=0.15, linewidth=0)
            a_bot.plot(fr, med, color=col, lw=2)
        a_top.set_title(mp, fontsize=9, color=INK)
        a_top.set_ylim(0, 102)
        a_bot.set_yscale("log")
        a_bot.axhline(conv_thresh, color=INK2, lw=1, ls="--")
        a_bot.set_xlabel("frame da fatia", fontsize=8, color=INK2)
        if j == 0:
            a_top.set_ylabel(f"% execuções com erro < {conv_thresh:g} m", fontsize=8, color=INK2)
            a_bot.set_ylabel("erro (m) — mediana e IQR", fontsize=8, color=INK2)
    handles, labels = axes[0, 0].get_legend_handles_labels()
    if not handles:
        for ax in axes[0]:
            handles, labels = ax.get_legend_handles_labels()
            if handles:
                break
    fig.suptitle(f"{dataset} — regime {regime}", y=0.985, fontsize=11, color=INK, fontweight="bold")
    fig.tight_layout(rect=(0, 0, 1, 0.925))
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.96), ncol=len(descs),
               frameon=False, fontsize=9, labelcolor=INK)
    fig.savefig(out_path, facecolor=SURFACE)
    plt.close(fig)


REGIME_LABEL = {"final_baro": "barômetro", "final_noalt": "sem altímetro", "final_gtalt": "alt. emulado (GT)",
                "vanilla": "artigo (vanilla)"}
REGIME_DASH = {"final_baro": "-", "final_noalt": "--", "final_gtalt": ":", "vanilla": "-."}


def plot_summary_comparison(runs, conv_thresh, out_path):
    """Comparativo λ × ab por voo e regime (mapas agregados): convergência, rastreio e MAE com IC 95%."""
    groups = {}
    for r in runs:
        groups.setdefault((r["dataset"], r["regime"]), {}).setdefault(r["desc"], []).append(r)
    keys = sorted(groups)
    if not keys:
        return
    descs = sorted({d for g in groups.values() for d in g}, key=lambda d: (d != BASELINE, d != "lambdaBRIEF", d))
    fig, axes = plt.subplots(1, 3, figsize=(max(9, 2.2 * len(keys) + 3), 4.4), dpi=150)
    fig.patch.set_facecolor(SURFACE)
    width = 0.8 / max(1, len(descs))
    metrics = [
        ("Convergência (%)", lambda rs: (100 * np.mean([r["converged"] for r in rs]),
                                          *[100 * v for v in wilson(sum(r["converged"] for r in rs), len(rs))])),
        ("Erro de rastreio após convergir (m)",
         lambda rs: _med_ci([r["track_median"] for r in rs if r["converged"]])),
        ("Erro médio do voo — MAE (m)", lambda rs: _med_ci([r["mae"] for r in rs])),
    ]
    for ax, (title, fn) in zip(axes, metrics):
        style_axes(ax)
        for j, d in enumerate(descs):
            xs, ys, lo, hi = [], [], [], []
            for i, k in enumerate(keys):
                rs = groups[k].get(d)
                if not rs:
                    continue
                v, a, b = fn(rs)
                if v is None or not np.isfinite(v):
                    continue
                xs.append(i + (j - (len(descs) - 1) / 2) * width)
                ys.append(v); lo.append(v - a if np.isfinite(a) else 0); hi.append(b - v if np.isfinite(b) else 0)
            if xs:
                ax.errorbar(xs, ys, yerr=[lo, hi], fmt="o", ms=7, color=color_for(d, descs), ecolor=color_for(d, descs),
                            elinewidth=2, capsize=0, label=desc_label(d), zorder=3)
        ax.set_xticks(range(len(keys)))
        ax.set_xticklabels([f"{ds}\n{REGIME_LABEL.get(rg, rg)}" for ds, rg in keys], fontsize=8, color=INK2)
        ax.set_title(title, fontsize=9, color=INK)
        if "Convergência" in title:
            ax.set_ylim(-3, 103)
        else:
            ax.set_ylim(bottom=0)
    n = {k: {d: len(v) for d, v in g.items()} for k, g in groups.items()}
    nmin = min(min(v.values()) for v in n.values())
    handles, labels = axes[0].get_legend_handles_labels()
    fig.suptitle(f"λ-BRIEF × abBRIEF (mediana / taxa com IC 95%; mapas agregados; ≥ {nmin} execuções por ponto)",
                 fontsize=10, color=INK, y=0.99)
    fig.tight_layout(rect=(0, 0, 1, 0.88))
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.93), ncol=len(descs),
               frameon=False, fontsize=9, labelcolor=INK)
    fig.savefig(out_path, facecolor=SURFACE)
    plt.close(fig)


def _med_ci(vals):
    vals = [v for v in vals if v is not None]
    if not vals:
        return (None, float("nan"), float("nan"))
    lo, hi = bootstrap_median_ci(vals)
    return (float(np.median(vals)), lo, hi)


def plot_convergence_by_regime(runs, conv_thresh, out_path):
    """Por voo: % de execuções com erro < limiar a cada frame. Cor = descritor, traço = regime."""
    by_ds = {}
    for r in runs:
        by_ds.setdefault(r["dataset"], {}).setdefault((r["regime"], r["desc"]), []).append(r)
    dss = sorted(by_ds)
    if not dss:
        return
    descs = sorted({d for g in by_ds.values() for _, d in g}, key=lambda d: (d != BASELINE, d != "lambdaBRIEF", d))
    fig, axes = plt.subplots(1, len(dss), figsize=(4.6 * len(dss) + 0.6, 3.9), dpi=150, squeeze=False)
    fig.patch.set_facecolor(SURFACE)
    for ax, ds in zip(axes[0], dss):
        style_axes(ax)
        for (rg, d), rs in sorted(by_ds[ds].items()):
            fr, mat = curves(rs)
            if len(fr) == 0:
                continue
            ax.plot(fr, 100 * np.mean(mat < conv_thresh, axis=0), color=color_for(d, descs),
                    ls=REGIME_DASH.get(rg, "-"), lw=2,
                    label=f"{desc_label(d)} — {REGIME_LABEL.get(rg, rg)} (n={len(rs)})")
        ax.set_title(ds, fontsize=10, color=INK)
        ax.set_ylim(0, 102)
        ax.set_xlabel("frame da fatia", fontsize=8, color=INK2)
        ax.set_ylabel(f"% execuções com erro < {conv_thresh:g} m", fontsize=8, color=INK2)
    seen = {}
    for ax in axes[0]:
        for h, l in zip(*ax.get_legend_handles_labels()):
            seen.setdefault(l.split(" (n=")[0], h)
    fig.suptitle("Localização ao longo do voo: cor = descritor, traço = regime (mapas agregados)",
                 fontsize=10, color=INK)
    fig.tight_layout(rect=(0, 0.13, 1, 0.94))
    fig.legend(list(seen.values()), list(seen.keys()), loc="lower center", ncol=2, frameon=False,
               fontsize=8, labelcolor=INK, bbox_to_anchor=(0.5, 0.0))
    fig.savefig(out_path, facecolor=SURFACE)
    plt.close(fig)


def plot_track_cdf(dataset, regime, descs, data, maps, out_path):
    fig, ax = plt.subplots(figsize=(6.5, 4.2), dpi=150)
    fig.patch.set_facecolor(SURFACE)
    style_axes(ax)
    drawn = False
    for d in descs:
        vals = [r["track_median"] for mp in maps for r in data.get((mp, d), []) if r["converged"]]
        if not vals:
            continue
        x = np.sort(vals)
        y = np.arange(1, len(x) + 1) / len(x)
        ax.step(x, y, where="post", color=color_for(d, descs), lw=2,
                label=f"{desc_label(d)}, {len(x)} execuções que convergiram")
        drawn = True
    if not drawn:
        plt.close(fig)
        return
    ax.set_xlabel("erro de rastreio por execução (mediana após convergir, m)", fontsize=8, color=INK2)
    ax.set_ylabel("fração acumulada", fontsize=8, color=INK2)
    ax.set_title(f"{dataset} — regime {regime}: erro de rastreio", fontsize=10, color=INK)
    ax.legend(frameon=False, fontsize=8, labelcolor=INK, loc="lower right")
    fig.tight_layout()
    fig.savefig(out_path, facecolor=SURFACE)
    plt.close(fig)


# --------------------------------------------------------------------------- saídas

def fmt(v, nd=1, pct=False):
    if v is None or (isinstance(v, float) and math.isnan(v)):
        return "—"
    return f"{100 * v:.0f}%" if pct else f"{v:.{nd}f}"


def fmt_p(p):
    if p is None:
        return "—"
    return f"{p:.3f}" if p >= 0.001 else f"{p:.1e}"


def write_csv(path, rows, fields):
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        w.writeheader()
        for r in rows:
            w.writerow({k: ("" if r.get(k) is None else r.get(k)) for k in fields})


def build_markdown(args, groups, tests, dups, n_runs):
    L = []
    L.append(f"# Benchmark abBRIEF × λ-BRIEF — consolidado")
    L.append("")
    L.append(f"- Pasta: `{os.path.abspath(args.benchmark_dir)}`")
    L.append(f"- Gerado em {datetime.datetime.now():%d/%m/%Y %H:%M}; {n_runs} execuções")
    if args.gt_file:
        L.append(f"- Ground truth: `{args.gt_file}` (re-sincronizado pelos timestamps)")
    else:
        L.append(f"- Ground truth: traj_truth.txt com lag {args.gt_lag}")
    L.append(f"- Convergência: erro < {args.conv_thresh:g} m por {args.conv_frames} frames seguidos; "
             f"perda de rastro: erro > {args.lost_thresh:g} m após convergir")
    if dups:
        for (ds, mp), keep in sorted(dups.items()):
            L.append(f"- **Mapa duplicado descartado:** {ds}/{mp} (idêntico a {keep})")
    L.append("")
    L.append("## Por grupo")
    L.append("")
    L.append("| Dataset | Regime | Mapa | Descritor | n | Convergência (IC 95%) | TTF med. | Rastreio med. (IC 95%) | Perdas | MAE méd. / med. | Final < limiar | ε artigo | s/frame |")
    L.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for g in groups:
        L.append(
            f"| {g['dataset']} | {g['regime']} | {g['map']} | {desc_label(g['desc'])} | {g['n_runs']} | "
            f"{fmt(g['conv_rate'], pct=True)} ({fmt(g['conv_lo'], pct=True)}–{fmt(g['conv_hi'], pct=True)}) | "
            f"{fmt(g['ttf_median'], 0)} | {fmt(g['track_median'])} ({fmt(g['track_lo'])}–{fmt(g['track_hi'])}) m | "
            f"{fmt(g['lost_mean'])} | {fmt(g['mae_mean'])} / {fmt(g['mae_median'])} m | "
            f"{fmt(g['final_ok_rate'], pct=True)} | {fmt(g['eps_paper'])} m | {fmt(g['s_frame'], 2)} |")
    L.append("")
    L.append("## Testes pareados contra o abBRIEF")
    L.append("")
    L.append("Δ = variante − abBRIEF. p ajustado por Holm sobre os mapas (linhas \"agregado\" não entram na correção).")
    L.append("")
    L.append("| Dataset | Regime | Mapa | Variante | pares | Δ convergência | p (adj) | Δ MAE med. | p (adj) | Δ rastreio med. (n) | p (adj) | Conclusão |")
    L.append("|---|---|---|---|---|---|---|---|---|---|---|---|")
    for t in tests:
        concl = "; ".join([
            f"convergência: {t['v_conv']}", f"MAE: {t['v_mae']}", f"rastreio: {t['v_track']}"])
        L.append(
            f"| {t['dataset']} | {t['regime']} | {t['map']} | {desc_label(t['variant'])} | {t['n_pairs']} | "
            f"{t['d_conv_pp']:+.0f} pp | {fmt_p(t['p_conv'])} ({fmt_p(t['p_conv_adj'])}) | "
            f"{fmt(t['d_mae_median'])} m | {fmt_p(t['p_mae'])} ({fmt_p(t['p_mae_adj'])}) | "
            f"{fmt(t['d_track_median'])} m ({t['n_both_conv']}) | {fmt_p(t['p_track'])} ({fmt_p(t['p_track_adj'])}) | "
            f"{concl} |")
    L.append("")
    return "\n".join(L)


def build_latex(groups):
    L = [r"\begin{table}[t]", r"\centering", r"\caption{abBRIEF vs. $\lambda$-BRIEF.}",
         r"\begin{tabular}{llllrrr}", r"\toprule",
         r"Dataset & Regime & Map & Descriptor & Conv. (\%) & Track. med. (m) & MAE (m) \\", r"\midrule"]
    for g in groups:
        map_tex = g["map"].replace("_", r"\_")
        desc_tex = desc_label(g["desc"]).replace("λ", r"$\lambda$").replace("α", r"$\alpha$")
        L.append(f"{g['dataset']} & {g['regime']} & {map_tex} & {desc_tex} & "
                 f"{fmt(100 * g['conv_rate'], 0)} & {fmt(g['track_median'])} & {fmt(g['mae_mean'])} " + r"\\")
    L += [r"\bottomrule", r"\end{tabular}", r"\end{table}"]
    return "\n".join(L)


def write_history(md_text, tag, path=HISTORICO_FILE):
    begin, end = f"<!-- {tag} BEGIN -->", f"<!-- {tag} END -->"
    block = f"{begin}\n### {tag}\n\n{md_text}\n{end}\n"
    content = open(path).read() if os.path.exists(path) else ""
    if begin in content and end in content:
        pre = content.split(begin)[0]
        post = content.split(end, 1)[1].lstrip("\n")
        content = pre + block + post
        action = "substituído"
    else:
        content = content.rstrip("\n") + "\n\n" + block
        action = "adicionado"
    with open(path, "w") as f:
        f.write(content)
    print(f"[HISTORICO] bloco {tag} {action} em {path}")


# --------------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description="Consolida o benchmark abBRIEF × λ-BRIEF",
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--benchmark_dir", required=True,
                    help="Pasta base (a mesma --output_base_dir do runner), ex.: "
                         "experimentos/2026-10-15_piloto_baro_vs_noalt")
    ap.add_argument("--dataset", default=None, help="Filtra um dataset")
    ap.add_argument("--map", default=None, help="Filtra um mapa")
    ap.add_argument("--regime", default=None, help="Filtra um regime (vanilla, final)")
    ap.add_argument("--gt_file", default=None,
                    help="Reavalia contra este arquivo de GT na pasta do dataset (ex.: traj_truth_sync.txt)")
    ap.add_argument("--gt_lag", default="0",
                    help="Lag inteiro do GT: '-2' para todos ou 'ADM0036=-2,UFRGS=3' por dataset")
    ap.add_argument("--conv_thresh", type=float, default=25.0, help="Limiar de convergência (m)")
    ap.add_argument("--conv_frames", type=int, default=10, help="Frames seguidos abaixo do limiar")
    ap.add_argument("--lost_thresh", type=float, default=50.0, help="Limiar de perda de rastro (m)")
    ap.add_argument("--keep_duplicates", action="store_true", help="Não descarta mapas duplicados")
    ap.add_argument("--no_plots", action="store_true")
    ap.add_argument("--out_dir", default=None, help="Padrão: <benchmark_dir>/summary_<gt>")
    ap.add_argument("--write_history", action="store_true",
                    help="Grava/substitui um bloco no HISTORICO_RESULTADOS.md (precisa de --tag)")
    ap.add_argument("--tag", default=None, help="Nome do bloco no histórico, ex.: BENCHMARK_FINAL_v1_A")
    ap.add_argument("--no_history", action="store_true", help="(compatibilidade; o padrão já é não gravar)")
    args = ap.parse_args()

    if args.write_history and not args.tag:
        ap.error("--write_history exige --tag")

    sys.path.insert(0, SCRIPT_DIR)
    import experimentos as exp
    args.benchmark_dir = exp.resolve(args.benchmark_dir)   # aceita os caminhos antigos (results_final/…)
    runs = discover(args.benchmark_dir)
    runs = [r for r in runs if (not args.dataset or r["dataset"] == args.dataset)
            and (not args.map or r["map"] == args.map)
            and (not args.regime or r["regime"] == args.regime)]
    if not runs:
        print(f"Nenhum errors.txt encontrado em {args.benchmark_dir}")
        sys.exit(0)

    lags = parse_lags(args.gt_lag)
    missing_gt = set()
    kept = []
    for r in runs:
        d = load_errors(r["path"])
        if d is None:
            continue
        if args.gt_file:
            ctx = run_context(r)
            frames, err = eval_gt_file(d, ctx, args.gt_file) if ctx else (None, None)
            if err is None:
                missing_gt.add(r["dataset"])
                continue
        else:
            frames, err = shift_gt(d, lags.get(r["dataset"], lags["*"]))
        if len(err) < args.conv_frames:
            continue
        r.update(run_metrics(err, args.conv_thresh, args.conv_frames, args.lost_thresh))
        r["frames"], r["err"] = frames, err
        r["hash"] = hashlib.md5(open(r["path"], "rb").read()).hexdigest()
        r["s_frame"] = mean_s_per_frame(r["seed_dir"])
        kept.append(r)
    runs = kept
    for ds in sorted(missing_gt):
        print(f"Aviso: {ds} sem {args.gt_file} (rode scripts/extract_altimeter.py) — execuções ignoradas.")

    dups = {} if args.keep_duplicates else find_duplicate_maps(runs)
    runs = [r for r in runs if (r["dataset"], r["map"]) not in dups]
    for (ds, mp), keep in dups.items():
        print(f"Aviso: {ds}/{mp} tem resultados idênticos a {ds}/{keep} — mapa duplicado descartado.")

    gt_tag = (os.path.splitext(args.gt_file)[0] if args.gt_file
              else "lag" + str(args.gt_lag).replace("=", "").replace(",", "_"))
    out_dir = args.out_dir or os.path.join(args.benchmark_dir, f"summary_{gt_tag}")
    plots_dir = os.path.join(out_dir, "plots")
    os.makedirs(plots_dir, exist_ok=True)

    # índice
    idx = {}
    for r in runs:
        idx.setdefault((r["dataset"], r["regime"]), {}).setdefault((r["map"], r["desc"]), []).append(r)

    def desc_order(ds):
        names = sorted(ds, key=lambda d: (d != BASELINE, d != "lambdaBRIEF", d))
        return names

    groups, tests = [], []
    for (ds, regime), data in sorted(idx.items()):
        maps = sorted({m for m, _ in data})
        descs = desc_order({d for _, d in data})
        for mp in maps:
            for d in descs:
                rs = sorted(data.get((mp, d), []), key=lambda r: r["seed"])
                if rs:
                    g = group_summary(rs)
                    g.update({"dataset": ds, "regime": regime, "map": mp, "desc": d})
                    groups.append(g)
        # testes por mapa + agregado
        for var in [d for d in descs if d != BASELINE]:
            fam = []
            for mp in maps:
                base, vr = data.get((mp, BASELINE)), data.get((mp, var))
                if base and vr:
                    t = paired_tests(base, vr)
                    if t:
                        t.update({"dataset": ds, "regime": regime, "map": mp, "variant": var})
                        fam.append(t)
            for key in ("p_conv", "p_mae", "p_track"):
                adj = holm([t[key] for t in fam])
                for t, a in zip(fam, adj):
                    t[key + "_adj"] = a
            tests.extend(fam)
            # agregado: empilha os pares mapa×seed (seed vira mapa:seed)
            base_all, var_all = [], []
            for mp in maps:
                for r in data.get((mp, BASELINE), []):
                    base_all.append({**r, "seed": f"{mp}:{r['seed']}"})
                for r in data.get((mp, var), []):
                    var_all.append({**r, "seed": f"{mp}:{r['seed']}"})
            t = paired_tests(base_all, var_all) if base_all and var_all else None
            if t and len(maps) > 1:
                t.update({"dataset": ds, "regime": regime, "map": "agregado", "variant": var,
                          "p_conv_adj": t["p_conv"], "p_mae_adj": t["p_mae"], "p_track_adj": t["p_track"]})
                tests.append(t)
        if not args.no_plots and maps:
            plot_grid(ds, regime, maps, descs, data, args.conv_thresh,
                      os.path.join(plots_dir, f"{ds}_{regime}_grade.png"))
            plot_track_cdf(ds, regime, descs, data, maps,
                           os.path.join(plots_dir, f"{ds}_{regime}_cdf_rastreio.png"))

    # gráficos comparativos gerais (não derrubam a consolidação se falharem)
    if not args.no_plots and runs:
        for fn, name in ((plot_summary_comparison, "comparacao_ab_vs_lambda.png"),
                         (plot_convergence_by_regime, "convergencia_por_frame.png")):
            try:
                fn(runs, args.conv_thresh, os.path.join(plots_dir, name))
            except Exception as e:
                print(f"Aviso: gráfico {name} falhou: {e}")

    # agregado de todos os datasets, por regime e variante
    by_regime = {}
    for r in runs:
        by_regime.setdefault((r["regime"], r["desc"]), []).append(
            {**r, "seed": f"{r['dataset']}:{r['map']}:{r['seed']}"})
    for (regime, var), vr in sorted(by_regime.items()):
        if var == BASELINE or (regime, BASELINE) not in by_regime:
            continue
        if len({r["dataset"] for r in vr}) < 2:
            continue
        t = paired_tests(by_regime[(regime, BASELINE)], vr)
        if t:
            t.update({"dataset": "TODOS", "regime": regime, "map": "agregado", "variant": var,
                      "p_conv_adj": t["p_conv"], "p_mae_adj": t["p_mae"], "p_track_adj": t["p_track"]})
            tests.append(t)

    for t in tests:
        t["v_conv"] = verdict(t["p_conv_adj"], t["d_conv_pp"], better_if_negative=False)
        t["v_mae"] = verdict(t["p_mae_adj"], t["d_mae_median"], better_if_negative=True)
        t["v_track"] = verdict(t["p_track_adj"], t["d_track_median"] if t["d_track_median"] is not None else 0,
                               better_if_negative=True)

    run_fields = ["dataset", "regime", "map", "desc", "seed", "n_frames", "converged", "ttf",
                  "track_median", "track_mean", "lost_frames", "mae", "median", "rmse", "final",
                  "final_ok", "frac_below", "s_frame", "path"]
    group_fields = ["dataset", "regime", "map", "desc", "n_runs", "conv_rate", "conv_lo", "conv_hi",
                    "ttf_median", "track_median", "track_lo", "track_hi", "track_mean", "lost_mean",
                    "mae_mean", "mae_median", "final_ok_rate", "eps_paper", "s_frame"]
    test_fields = ["dataset", "regime", "map", "variant", "n_pairs", "conv_base", "conv_var", "d_conv_pp",
                   "mcnemar_b_only_base", "mcnemar_c_only_var", "p_conv", "p_conv_adj", "d_mae_median",
                   "p_mae", "p_mae_adj", "n_both_conv", "d_track_median", "p_track", "p_track_adj",
                   "v_conv", "v_mae", "v_track"]
    write_csv(os.path.join(out_dir, "runs.csv"), sorted(runs, key=lambda r: (r["dataset"], r["regime"], r["map"], r["desc"], r["seed"])), run_fields)
    write_csv(os.path.join(out_dir, "groups.csv"), groups, group_fields)
    write_csv(os.path.join(out_dir, "tests.csv"), tests, test_fields)

    md = build_markdown(args, groups, tests, dups, len(runs))
    with open(os.path.join(out_dir, "summary.md"), "w") as f:
        f.write(md + "\n")
    with open(os.path.join(out_dir, "tables.tex"), "w") as f:
        f.write(build_latex(groups) + "\n")

    print(md)
    print(f"\nSaídas em {out_dir}")
    if args.write_history:
        write_history(md, args.tag)


if __name__ == "__main__":
    main()

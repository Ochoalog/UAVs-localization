#!/usr/bin/env python3
"""
plot_paper_grid.py — Generate a multi-panel comparison plot (like Figure 5 of the paper)
for all 5 maps of ADM0036 comparing abBRIEF vs lambdaBRIEF across 30 seeds.
"""

import os
import glob
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
import sys  # noqa: E402
sys.path.insert(0, SCRIPT_DIR)
import experimentos as exp  # noqa: E402
BENCHMARK_DIR = exp.resolve(os.path.join(MODERN_DIR, "results_benchmark", "ADM0036"))
OUTPUT_FILE = os.path.join(BENCHMARK_DIR, "comparacao_5_mapas_paper.png")

MAPS = [
    ("ADM_35_12_09_2018", "12/09/2018"),
    ("ADM_35_17_08_2018", "17/08/2018"),
    ("ADM_35_17_12_2018", "17/12/2018"),
    ("ADM_35_05_09_2018", "05/09/2018"),
    ("ADM_35_27_02_2017", "27/02/2017"),
]

def load_temporal_matrix(desc_dir):
    seed_files = sorted(glob.glob(os.path.join(desc_dir, "seed_*", "errors.txt")))
    runs = []
    frames = None
    for sf in seed_files:
        try:
            data = np.loadtxt(sf, skiprows=1)
            if len(data) > 0:
                runs.append(data[:, 7])
                if frames is None:
                    frames = data[:, 0]
        except Exception:
            continue
    if not runs:
        return None, None
    min_len = min(len(r) for r in runs)
    mat = np.array([r[:min_len] for r in runs])
    return frames[:min_len], mat

def main():
    fig, axes = plt.subplots(2, 3, figsize=(16, 9), dpi=200, sharex=True, sharey=True)
    axes_flat = axes.flatten()

    for idx, (map_name, label_date) in enumerate(MAPS):
        ax = axes_flat[idx]
        map_path = os.path.join(BENCHMARK_DIR, map_name)

        dir_a = os.path.join(map_path, "abBRIEF_regime_vanilla")
        dir_b = os.path.join(map_path, "lambdaBRIEF_regime_vanilla")

        frames_a, mat_a = load_temporal_matrix(dir_a)
        frames_b, mat_b = load_temporal_matrix(dir_b)

        if mat_a is not None:
            mean_a = np.mean(mat_a, axis=0)
            ci_a = 1.96 * np.std(mat_a, axis=0) / np.sqrt(len(mat_a))
            ax.plot(frames_a, mean_a, label="abBRIEF (α=0.0)", color="#d62728", lw=2)
            ax.fill_between(frames_a, np.maximum(0, mean_a - ci_a), mean_a + ci_a, color="#d62728", alpha=0.15)

        if mat_b is not None:
            mean_b = np.mean(mat_b, axis=0)
            ci_b = 1.96 * np.std(mat_b, axis=0) / np.sqrt(len(mat_b))
            ax.plot(frames_b, mean_b, label="λ-BRIEF (α=0.5, NDVI)", color="#1f77b4", lw=2)
            ax.fill_between(frames_b, np.maximum(0, mean_b - ci_b), mean_b + ci_b, color="#1f77b4", alpha=0.15)

        ax.axhline(25, color="gray", linestyle="--", alpha=0.6, lw=1)
        ax.set_title(f"({chr(97+idx)}) Mapa: {label_date}", fontsize=12, fontweight="bold")
        ax.set_ylim(0, 300)
        ax.grid(True, linestyle=":", alpha=0.6)
        if idx == 0:
            ax.legend(loc="upper right", frameon=True, fontsize=10)
        if idx in [0, 3]:
            ax.set_ylabel("Erro Médio de Posição (m)", fontsize=11)
        if idx in [2, 3, 4]:
            ax.set_xlabel("Frame da Trajetória", fontsize=11)

    # 6th panel: Overall CDF
    ax_cdf = axes_flat[5]
    all_a, all_b = [], []
    for map_name, _ in MAPS:
        _, mat_a = load_temporal_matrix(os.path.join(BENCHMARK_DIR, map_name, "abBRIEF_regime_vanilla"))
        _, mat_b = load_temporal_matrix(os.path.join(BENCHMARK_DIR, map_name, "lambdaBRIEF_regime_vanilla"))
        if mat_a is not None:
            all_a.append(mat_a.flatten())
        if mat_b is not None:
            all_b.append(mat_b.flatten())

    if all_a:
        flat_a = np.sort(np.concatenate(all_a))
        ax_cdf.plot(flat_a, np.arange(1, len(flat_a)+1)/len(flat_a), label="abBRIEF", color="#d62728", lw=2)
    if all_b:
        flat_b = np.sort(np.concatenate(all_b))
        ax_cdf.plot(flat_b, np.arange(1, len(flat_b)+1)/len(flat_b), label="λ-BRIEF", color="#1f77b4", lw=2)

    ax_cdf.axvline(25, color="gray", linestyle="--", alpha=0.6, lw=1)
    ax_cdf.set_title("(f) CDF Geral de Todos os Mapas", fontsize=12, fontweight="bold")
    ax_cdf.set_xlim(0, 200)
    ax_cdf.set_ylim(0, 1.02)
    ax_cdf.set_xlabel("Erro de Posição (m)", fontsize=11)
    ax_cdf.set_ylabel("Probabilidade P(Erro ≤ X)", fontsize=11)
    ax_cdf.grid(True, linestyle=":", alpha=0.6)
    ax_cdf.legend(loc="lower right", frameon=True, fontsize=10)

    plt.suptitle("Benchmark abBRIEF vs. λ-BRIEF — ADM2 (0036) — 30 Sementes / Regime Vanilla (Paper)", fontsize=14, fontweight="bold", y=0.98)
    plt.tight_layout()
    plt.savefig(OUTPUT_FILE)
    plt.close()
    print(f"[OK] Gráfico combinado do paper gerado em: {OUTPUT_FILE}")

if __name__ == "__main__":
    main()

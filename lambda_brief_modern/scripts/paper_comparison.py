#!/usr/bin/env python3
"""
paper_comparison.py — o teste do artigo λ-BRIEF (Figs. 4 e 5) com a engine moderna, 1 seed

O artigo compara, em cada voo e em cada mapa (ano), o abBRIEF com o λ-BRIEF: uma curva de erro
por instante para cada combinação. Este script faz exatamente isso, com UM comando:

  para cada voo (ADM1, ADM2, ADM3, UFRGS)
    para cada mapa do artigo (5 por voo)
      roda o abBRIEF uma vez e o λ-BRIEF uma vez (seed 1)

Cada curva do gráfico é uma execução da engine, então são 2 execuções por mapa
(4 voos × 5 mapas × 2 métodos = 40).
O artigo fez o mesmo, mas repetindo cada execução 30 vezes e tirando a média.

Protocolos
  --protocolo artigo  (padrão) igual ao artigo: filtro original (regime vanilla, odometria gravada
                      do legado), λ-BRIEF com α = 1 (comparação final do artigo), erro contra o
                      traj_truth.txt original.
  --protocolo novo    filtro em duas fases + barômetro real, α = 0,5, erro contra o GT
                      re-sincronizado (traj_truth_sync.txt).
  --regime / --lambda_alpha / --gt sobrescrevem o protocolo.

Voos (Tabela I do artigo) → pastas
  ADM1  = Datasets/ADM/0035  imagens 0103–0402 (300 frames)  mapas ADM_35
  ADM2  = Datasets/ADM/0036  imagens 0199–0394 (196 frames)  mapas ADM_35
  ADM3  = Datasets/ADM/0040  imagens 0200–0465 (266 frames)  mapas e GT do Zenodo (Datasets/ADM/0040_zenodo)
  UFRGS = Datasets/UFRGS VET/voo_100m  imagens 0390–0613 (224 frames)  mapas UFRGS_VET_V3

Mapas (legendas das Figs. 4 e 5)
  ADM   12-2018 ADM_35_17_12_2018 · 01-2014 ADM_35_27_01_2014 · 11-2016 ADM_35_24_11_2016 ·
        09-2017 ADM_35_08_09_2017 · 02-2014 ADM_35_02_02_2014       (Datasets/ADM/0039/Mapas)
  UFRGS 08-2011 V3_11_08_2011 · 06-2014 V3_14_06_2014 · 12-2002 V3_17_12_2002 ·
        04-2015 V3_02_04_2015 · 10-2012 V3_12_10_2012              (UFRGS VET/voo_100m/Mapa)
  As datas foram confirmadas pelos mapas do Zenodo do ADM3 (Maps.zip: 17/12/2018, 27/01/2014,
  24/11/2016, 08/09/2017, 02/02/2014), idênticos (md5) aos ADM_35_* do repositório.

Uso (de dentro de lambda_brief_modern/)
  python3 scripts/paper_comparison.py --dry_run        # lista o que vai rodar
  python3 scripts/paper_comparison.py                  # roda tudo e gera as figuras
  python3 scripts/paper_comparison.py --plot_only      # só refaz as figuras (da pasta mais recente)
  python3 scripts/paper_comparison.py --continuar      # termina/estende a pasta mais recente com o mesmo nome

Saída: experimentos/<AAAA-MM-DD>_artigo_<protocolo>_a<α>/ (registrada em experimentos/INDICE.md;
       use --out <pasta> para escolher outra)
  00_LEIA_ME.md                       o que é cada arquivo + mapas e datas usados
  compilado_artigo_ADM.png            como a Fig. 5 (ADM1, ADM2, ADM3)
  compilado_artigo_UFRGS.png          como a Fig. 4
  tabela.md / tabela.csv              números por voo e mapa
  <VOO>/<VOO>_compilado.png           todos os mapas do voo + miniatura de cada mapa
  <VOO>/<VOO>_mapa_<MM-AAAA>.png      abBRIEF × λ-BRIEF naquele mapa, com o mapa embaixo
  execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_1/   saída bruta da engine
"""

import argparse
import csv
import json
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
from matplotlib.gridspec import GridSpec

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import benchmark_runner as br  # noqa: E402
from consolidate_benchmark import (load_errors, run_context, eval_gt_file, run_metrics,  # noqa: E402
                                   SURFACE, INK, INK2, GRID, style_axes)

MODERN_DIR = os.path.dirname(SCRIPT_DIR)
import experimentos as exp  # noqa: E402
WS = os.path.dirname(MODERN_DIR)
ADM_FIG5_MAPS_DIR = os.path.join(WS, "Datasets", "ADM", "0039", "Mapas")

FLIGHTS = {
    "ADM1": {"ds": "ADM0035", "pasta": "Datasets/ADM/0035", "imagens": "0103–0402", "n": 300,
             "traj_file": os.path.join(WS, "Datasets", "ADM", "0035", "traj_comp.txt")},
    "ADM2": {"ds": "ADM0036", "pasta": "Datasets/ADM/0036", "imagens": "0199–0394", "n": 196, "traj_file": None},
    # ADM3: mapas e GT do Zenodo do artigo (4070013) em Datasets/ADM/0040_zenodo. O traj_truth.txt local
    # está em outro sistema de coordenadas (rotação de ~27° e escala ×1,21); o GT do Zenodo está no sistema
    # ADM_35, o mesmo dos mapas (md5 idênticos aos ADM_35_* do repositório). Altímetro: altimeter_baro.txt
    # gerado da altura do log de voo (GT_DAT/ADM3.kml).
    "ADM3": {"ds": "ADM0040", "pasta": "Datasets/ADM/0040", "imagens": "0200–0465", "n": 266, "traj_file": None,
             "maps_dir": os.path.join(WS, "Datasets", "ADM", "0040_zenodo", "Maps"), "map_prefix": "ADM_",
             "gt_file": os.path.join(WS, "Datasets", "ADM", "0040_zenodo", "GT_DAT", "ADM3_ground_truth.txt"),
             "cfg": {"pixel_to_meter": 0.188679245, "min_scale": 0.1, "max_scale": 3.0}},
    "UFRGS": {"ds": "UFRGS", "pasta": "Datasets/UFRGS VET/voo_100m", "imagens": "0390–0613", "n": 224,
              "traj_file": None},
}
ORDER = ["ADM1", "ADM2", "ADM3", "UFRGS"]

ADM_MAPS = [("12-2018", "ADM_35_17_12_2018.jpg"), ("01-2014", "ADM_35_27_01_2014.jpg"),
            ("11-2016", "ADM_35_24_11_2016.jpg"), ("09-2017", "ADM_35_08_09_2017.jpg"),
            ("02-2014", "ADM_35_02_02_2014.jpg")]
UFRGS_MAPS = [("08-2011", "UFRGS_VET_V3_11_08_2011.jpg"), ("06-2014", "UFRGS_VET_V3_14_06_2014.jpg"),
              ("12-2002", "UFRGS_VET_V3_17_12_2002.jpg"), ("04-2015", "UFRGS_VET_V3_02_04_2015.jpg"),
              ("10-2012", "UFRGS_VET_V3_12_10_2012.jpg")]

PROTOCOLS = {
    "artigo": {"regime": "vanilla", "lambda_alpha": 1.0, "gt": "original"},
    "novo": {"regime": "final_baro", "lambda_alpha": 0.5, "gt": "sync"},
}

# Cores: uma por mapa nos compilados (como no artigo); ab × λ nos gráficos por ano.
MAP_COLORS = ["#d62f2f", "#1f9d55", "#2a78d6", "#b23ab0", "#222222"]
AB_COLOR, LAMBDA_COLOR, GT_COLOR = "#eb6834", "#2a78d6", "#1a8f3a"
CONV_THRESH = 25.0


# --------------------------------------------------------------------------- configuração

def flight_maps(flight, args):
    if flight == "UFRGS":
        d = br.DATASET_CONFIGS["UFRGS"]["maps_dir"]
        return [(lbl, os.path.join(d, f)) for lbl, f in UFRGS_MAPS]
    maps = list(ADM_MAPS)
    maps[0] = ("12-2018", args.adm_map_12_2018)
    maps[1] = ("01-2014", args.adm_map_01_2014)
    info = FLIGHTS[flight]
    if info.get("maps_dir"):  # ADM3: mesmos mapas, com os nomes do Zenodo (ADM_dd_mm_aaaa.jpg)
        return [(lbl, os.path.join(info["maps_dir"], f.replace("ADM_35_", info["map_prefix"]))) for lbl, f in maps]
    return [(lbl, os.path.join(ADM_FIG5_MAPS_DIR, f)) for lbl, f in maps]


def ds_cfg_for(flight):
    cfg = dict(br.DATASET_CONFIGS[FLIGHTS[flight]["ds"]])
    cfg.update(FLIGHTS[flight].get("cfg", {}))
    if FLIGHTS[flight]["traj_file"]:
        cfg["traj_file"] = FLIGHTS[flight]["traj_file"]
    return cfg


def with_traj(cmd, cfg):
    return cmd + ["--traj", cfg["traj_file"]] if cfg.get("traj_file") else cmd


# A odometria fixa dos regimes final_* do ADM1 tem de ser gerada com o traj_comp.txt.
_orig_odom_cmd = br.build_odom_command
br.build_odom_command = lambda b, c, m, o: with_traj(_orig_odom_cmd(b, c, m, o), c)


def method_labels(args):
    lam = "lambdaBRIEF" if abs(args.lambda_alpha - 0.5) < 1e-12 else f"lambdaBRIEF_a{args.lambda_alpha:g}"
    return [("abBRIEF", 0.0), (lam, args.lambda_alpha)]


def build_tasks(args):
    tasks, notes = [], []
    for flight in args.flights:
        info = FLIGHTS[flight]
        if info.get("skip"):
            notes.append(f"{flight}: não roda — {info['skip']}.")
            continue
        cfg = ds_cfg_for(flight)
        if cfg.get("traj_file") and not os.path.exists(cfg["traj_file"]):
            notes.append(f"{flight}: falta {cfg['traj_file']}.")
            continue
        if info.get("gt_file") and not os.path.exists(info["gt_file"]):
            notes.append(f"{flight}: falta o GT {info['gt_file']}.")
            continue
        if args.regime == "final_baro" and not os.path.exists(os.path.join(cfg["dataset_dir"], "altimeter_baro.txt")):
            notes.append(f"{flight}: falta {cfg['dataset_dir']}/altimeter_baro.txt (necessário no regime final_baro).")
            continue
        odom = br.odom_path_for(info["ds"], args.odom_dir)
        for lbl, map_path in flight_maps(flight, args):
            if not os.path.exists(map_path):
                notes.append(f"{flight}: mapa {lbl} não encontrado ({map_path}).")
                continue
            map_name = os.path.splitext(os.path.basename(map_path))[0]
            for desc, alpha in method_labels(args):
                out_dir = os.path.join(args.out, "execucoes", flight, map_name,
                                       f"{desc}_regime_{args.regime}", f"seed_{args.seed}")
                cmd = br.build_command(args.bin, cfg, map_path, out_dir, alpha, args.regime, args.seed,
                                       args.particles, scale_prior_gain=1.0, final_odom=odom)
                cmd = with_traj(cmd, cfg)
                if info.get("gt_file"):
                    cmd += ["--gt", info["gt_file"]]
                tasks.append({"flight": flight, "ds": info["ds"], "label": lbl, "map_path": map_path,
                              "map_name": map_name, "desc": desc, "alpha": alpha, "out_dir": out_dir,
                              "cmd": cmd, "cfg": cfg})
    return tasks, notes


def ensure_odom(args):
    if not args.regime.startswith("final"):
        return True
    ok = True
    for flight in args.flights:
        if FLIGHTS[flight].get("skip"):
            continue
        cfg = ds_cfg_for(flight)
        first = flight_maps(flight, args)[0][1]
        cfg["maps_dir"] = os.path.dirname(first)
        ok &= br.make_odom(FLIGHTS[flight]["ds"], cfg, args.bin, args.odom_dir, os.path.basename(first),
                           False, args.dry_run)
    return ok


def run_task(t, omp_threads, force):
    errors = os.path.join(t["out_dir"], "errors.txt")
    if os.path.exists(errors) and os.path.getsize(errors) > 100 and not force:
        return t, "já existia", 0.0
    os.makedirs(t["out_dir"], exist_ok=True)
    with open(os.path.join(t["out_dir"], "cmd.txt"), "w") as f:
        json.dump(t["cmd"], f, indent=1)
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(omp_threads)
    t0 = time.time()
    with open(os.path.join(t["out_dir"], "run.log"), "w") as lf:
        rc = subprocess.run(t["cmd"], stdout=lf, stderr=subprocess.STDOUT, env=env).returncode
    return t, ("ok" if rc == 0 else f"FALHOU (exit {rc}), veja run.log"), time.time() - t0


# --------------------------------------------------------------------------- avaliação

_SYNC = {}


def evaluate(t, gt_mode):
    if FLIGHTS[t["flight"]].get("gt_file"):
        gt_mode = "original"  # ADM3: o erro do errors.txt já é contra o GT do Zenodo (não há GT re-sincronizado)
    path = os.path.join(t["out_dir"], "errors.txt")
    if not os.path.exists(path):
        return None
    d = load_errors(path)
    if d is None or len(d["err"]) < 10:
        return None
    if gt_mode == "original":
        frames, err = d["frame"], d["err"]
        gx, gy, keep = d["trueX"], d["trueY"], np.ones(len(frames), bool)
    else:
        ctx = run_context({"seed_dir": t["out_dir"], "dataset": t["ds"]})
        frames, err = eval_gt_file(d, ctx, "traj_truth_sync.txt")
        if frames is None:
            return None
        keep = np.isin(d["frame"], frames)
        p = os.path.join(ctx["dataset_dir"], "traj_truth_sync.txt")
        if p not in _SYNC:
            _SYNC[p] = np.array([[float(v) for v in l.split()[:2]] for l in open(p) if l.strip()])
        g = _SYNC[p][ctx["start_frame"] + frames]
        gx, gy = g[:, 0], g[:, 1]
    m = run_metrics(err, CONV_THRESH, 10, 50.0)
    return {"frames": frames, "err": err, "estX": d["estX"][keep], "estY": d["estY"][keep],
            "gtX": gx, "gtY": gy, "m": m}


def load_map(path, cache={}):
    if path not in cache:
        cache.clear()  # um mapa por vez na memória (4800 × 4800)
        cache[path] = mpimg.imread(path)
    return cache[path]


def crop_extent(img, xs, ys, pad):
    h, w = img.shape[:2]
    x0, x1 = int(max(0, np.nanmin(xs) - pad)), int(min(w, np.nanmax(xs) + pad))
    y0, y1 = int(max(0, np.nanmin(ys) - pad)), int(min(h, np.nanmax(ys) + pad))
    return x0, x1, y0, y1


def draw_map(ax, path, gtx, gty, pad=300, est=()):
    ax.set_xticks([])
    ax.set_yticks([])
    for s in ax.spines.values():
        s.set_color(GRID)
    if not os.path.exists(path):
        ax.text(0.5, 0.5, "mapa não encontrado", ha="center", va="center", transform=ax.transAxes, color=INK2)
        return
    img = load_map(path)
    x0, x1, y0, y1 = crop_extent(img, gtx, gty, pad)
    ax.imshow(img[y0:y1, x0:x1], extent=(x0, x1, y1, y0))
    for xs, ys, c, ls, lab in est:
        ax.plot(xs, ys, ls, color=c, lw=1.4, label=lab, alpha=0.95)
    ax.plot(gtx, gty, "-", color=GT_COLOR, lw=2.4, label="Ground truth (GPS)")
    ax.plot(gtx[0], gty[0], "o", color=GT_COLOR, mec="white", mew=1.5, ms=9)
    ax.set_xlim(x0, x1)
    ax.set_ylim(y1, y0)


# --------------------------------------------------------------------------- figuras

def fig_por_ano(flight, lbl, map_path, ab, lam, args, lam_name, path):
    fig = plt.figure(figsize=(10, 11.5), facecolor=SURFACE)
    gs = GridSpec(2, 1, height_ratios=[1, 1.35], hspace=0.16, top=0.95, figure=fig)
    ax = fig.add_subplot(gs[0])
    style_axes(ax)
    txt = []
    for r, c, ls, name in ((ab, AB_COLOR, ":", "abBRIEF"), (lam, LAMBDA_COLOR, "-", lam_name)):
        if r is None:
            txt.append(f"{name}: sem resultado")
            continue
        ax.plot(r["frames"], r["err"], ls, color=c, lw=2.0, label=name)
        t = r["m"]["ttf"]
        if t is not None:
            ax.axvline(r["frames"][t], color=c, ls="--", lw=0.9)
        txt.append(f"{name}: MAE {r['m']['mae']:.1f} m · convergiu no frame "
                   f"{r['frames'][t] if t is not None else '—'} · erro final {r['m']['final']:.1f} m")
    ax.axhline(CONV_THRESH, color=INK2, ls="--", lw=0.8, label=f"{CONV_THRESH:.0f} m")
    ax.set_ylim(bottom=0)
    ax.set_xlabel("Instant (frame)", color=INK2, fontsize=9)
    ax.set_ylabel("Error (m)", color=INK2, fontsize=9)
    ax.legend(fontsize=9, loc="upper right")
    ax.set_title(f"{flight} · mapa {lbl} — abBRIEF × {lam_name}", color=INK, fontsize=12)
    ax.text(0.5, 0.97, "\n".join(txt), transform=ax.transAxes, fontsize=8.5, color=INK, va="top", ha="center",
            bbox=dict(boxstyle="round", fc="white", ec=GRID, alpha=0.92))

    axm = fig.add_subplot(gs[1])
    ref = ab or lam
    if ref is not None:
        est = []
        if ab is not None:
            est.append((ab["estX"], ab["estY"], AB_COLOR, ":", "abBRIEF (estimativa)"))
        if lam is not None:
            est.append((lam["estX"], lam["estY"], LAMBDA_COLOR, "-", f"{lam_name} (estimativa)"))
        draw_map(axm, map_path, ref["gtX"], ref["gtY"], est=est)
        axm.legend(fontsize=8, loc="lower right", framealpha=0.9)
    axm.set_title(f"Mapa usado: {os.path.basename(map_path)} (recorte em torno da trajetória; ● = início)",
                  color=INK, fontsize=10)
    fig.suptitle(f"Seed {args.seed} · regime {args.regime} · GT {args.gt}", y=0.985, color=INK2, fontsize=9)
    fig.savefig(path, dpi=150, facecolor=SURFACE, bbox_inches="tight")
    plt.close(fig)


def plot_flight_errors(ax, flight, res, args, lam_name, legend=True):
    style_axes(ax)
    labels = [lbl for lbl, _ in flight_maps(flight, args)]
    drew = False
    for i, lbl in enumerate(labels):
        c = MAP_COLORS[i % len(MAP_COLORS)]
        for desc, ls, name in (("abBRIEF", ":", "abBRIEF"), ("lambda", "-", lam_name)):
            r = res.get((flight, lbl, desc))
            if r is None:
                continue
            ax.plot(r["frames"], r["err"], ls, color=c, lw=1.7, label=f"{lbl} {name}")
            drew = True
    if not drew:
        ax.text(0.5, 0.5, "sem execuções\n" + FLIGHTS[flight].get("skip", ""), ha="center", va="center",
                transform=ax.transAxes, color=INK2, fontsize=8, wrap=True)
    ax.set_title(f"{flight} flight", color=INK, fontsize=11)
    ax.set_xlabel("Instant", color=INK2, fontsize=9)
    ax.set_ylabel("Error (m)", color=INK2, fontsize=9)
    ax.set_ylim(bottom=0)
    if legend and drew:
        ax.legend(fontsize=7.5, ncol=2, loc="upper right", framealpha=0.9)
    return drew


def fig_compilado_voo(flight, res, args, lam_name, path):
    maps = flight_maps(flight, args)
    fig = plt.figure(figsize=(17, 10), facecolor=SURFACE)
    gs = GridSpec(2, len(maps), height_ratios=[1.25, 1], hspace=0.28, wspace=0.08, top=0.94, figure=fig)
    ax = fig.add_subplot(gs[0, :])
    plot_flight_errors(ax, flight, res, args, lam_name)
    ax.set_title(f"{flight}: abBRIEF (pontilhado) × {lam_name} (cheio), uma cor por mapa", color=INK, fontsize=12)
    for i, (lbl, mp) in enumerate(maps):
        axm = fig.add_subplot(gs[1, i])
        r = res.get((flight, lbl, "abBRIEF")) or res.get((flight, lbl, "lambda"))
        if r is not None:
            draw_map(axm, mp, r["gtX"], r["gtY"], pad=250)
        else:
            axm.set_axis_off()
        axm.set_title(f"{lbl}\n{os.path.basename(mp)}", color=MAP_COLORS[i % len(MAP_COLORS)], fontsize=10,
                      fontweight="bold")
    fig.suptitle(f"Seed {args.seed} · regime {args.regime} · GT {args.gt} · linha verde nos mapas = ground truth",
                 y=0.975, color=INK2, fontsize=9)
    fig.savefig(path, dpi=140, facecolor=SURFACE, bbox_inches="tight")
    plt.close(fig)


def fig_compilado_artigo(flights, res, args, lam_name, path, title):
    fig, axes = plt.subplots(1, len(flights), figsize=(max(6.3 * len(flights), 12), 4.8), facecolor=SURFACE,
                             squeeze=False)
    for ax, flight in zip(axes[0], flights):
        plot_flight_errors(ax, flight, res, args, lam_name, legend=False)
    h, lab = [], []
    for ax in axes[0]:
        for a, b in zip(*ax.get_legend_handles_labels()):
            if b not in lab:
                h.append(a)
                lab.append(b)
    if h:
        fig.legend(h, lab, loc="upper center", ncol=5, fontsize=8.5, frameon=False)
    fig.text(0.5, 0.01, title, ha="center", color=INK, fontsize=10)
    fig.tight_layout(rect=(0, 0.05, 1, 0.84))
    fig.savefig(path, dpi=160, facecolor=SURFACE)
    plt.close(fig)


# --------------------------------------------------------------------------- saída

def make_outputs(tasks, notes, args):
    lam_name = "λ-BRIEF" + ("" if abs(args.lambda_alpha - 0.5) < 1e-12 else f" (α={args.lambda_alpha:g})")
    res, paths = {}, {}
    for t in tasks:
        r = evaluate(t, args.gt)
        key_desc = "abBRIEF" if t["desc"] == "abBRIEF" else "lambda"
        paths[(t["flight"], t["label"])] = t["map_path"]
        if r is not None:
            res[(t["flight"], t["label"], key_desc)] = r

    made = []
    flights_run = [f for f in args.flights if not FLIGHTS[f].get("skip")]
    for flight in args.flights:
        fdir = os.path.join(args.out, flight)
        os.makedirs(fdir, exist_ok=True)
        if FLIGHTS[flight].get("skip"):
            with open(os.path.join(fdir, "LEIA_ME.md"), "w") as f:
                f.write(f"# {flight}\n\nNão foi rodado: {FLIGHTS[flight]['skip']}.\n")
            continue
        for lbl, mp in flight_maps(flight, args):
            ab = res.get((flight, lbl, "abBRIEF"))
            lam = res.get((flight, lbl, "lambda"))
            if ab is None and lam is None:
                continue
            p = os.path.join(fdir, f"{flight}_mapa_{lbl}.png")
            fig_por_ano(flight, lbl, mp, ab, lam, args, lam_name, p)
            made.append(p)
        if any((flight, lbl, d) in res for lbl, _ in flight_maps(flight, args) for d in ("abBRIEF", "lambda")):
            p = os.path.join(fdir, f"{flight}_compilado.png")
            fig_compilado_voo(flight, res, args, lam_name, p)
            made.append(p)

    adm = [f for f in ("ADM1", "ADM2", "ADM3") if f in args.flights]
    if adm:
        p = os.path.join(args.out, "compilado_artigo_ADM.png")
        fig_compilado_artigo(adm, res, args, lam_name, p,
                             f"Comparison between abBRIEF and {lam_name} for the Arroio do Meio flights "
                             f"(como a Fig. 5 · seed {args.seed} · {args.regime})")
        made.append(p)
    if "UFRGS" in args.flights:
        p = os.path.join(args.out, "compilado_artigo_UFRGS.png")
        fig_compilado_artigo(["UFRGS"], res, args, lam_name, p,
                             f"Comparison between abBRIEF and {lam_name} for the UFRGS flight "
                             f"(como a Fig. 4 · seed {args.seed} · {args.regime})")
        made.append(p)

    # tabela
    rows = []
    for flight in flights_run:
        for lbl, mp in flight_maps(flight, args):
            row = {"voo": flight, "mapa": lbl, "arquivo": os.path.basename(mp)}
            for tag, key in (("ab", "abBRIEF"), ("lambda", "lambda")):
                r = res.get((flight, lbl, key))
                m = r["m"] if r else {}
                ttf = m.get("ttf")
                row[f"{tag}_convergiu_frame"] = int(r["frames"][ttf]) if r and ttf is not None else None
                row[f"{tag}_mae_m"] = round(m["mae"], 2) if r else None
                row[f"{tag}_rastreio_med_m"] = round(m["track_median"], 2) if r and m.get("track_median") else None
                row[f"{tag}_final_m"] = round(m["final"], 2) if r else None
            rows.append(row)
    if rows:
        with open(os.path.join(args.out, "tabela.csv"), "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)

    def f1(v):
        return "—" if v is None else f"{v:.1f}"

    T = ["| Voo | Mapa | Arquivo | Convergiu (ab / λ) | MAE ab | MAE λ | Rastreio ab | Rastreio λ | Final ab | Final λ |",
         "|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        conv = " / ".join("não" if r[k] is None else str(r[k]) for k in ("ab_convergiu_frame", "lambda_convergiu_frame"))
        T.append(f"| {r['voo']} | {r['mapa']} | {r['arquivo']} | {conv} | {f1(r['ab_mae_m'])} | {f1(r['lambda_mae_m'])} | "
                 f"{f1(r['ab_rastreio_med_m'])} | {f1(r['lambda_rastreio_med_m'])} | {f1(r['ab_final_m'])} | "
                 f"{f1(r['lambda_final_m'])} |")
    with open(os.path.join(args.out, "tabela.md"), "w") as f:
        f.write(f"# abBRIEF × {lam_name} por voo e mapa\n\nProtocolo {args.protocolo} · regime {args.regime} · "
                f"seed {args.seed} · GT {args.gt}. Convergiu = primeiro frame com erro < 25 m por 10 frames "
                "seguidos. Rastreio = mediana do erro depois de convergir. Erros em metros.\n\n"
                + "\n".join(T) + "\n")

    write_readme(args, notes, rows, lam_name)
    print("\nArquivos gerados:")
    for p in made:
        print("  ", os.path.relpath(p, args.out))
    print(f"\nTudo em {args.out} (comece por 00_LEIA_ME.md)")


def write_readme(args, notes, rows, lam_name):
    L = [f"# Teste do artigo λ-BRIEF — abBRIEF × {lam_name}, 1 seed", "",
         f"Protocolo **{args.protocolo}**: regime `{args.regime}`, λ-BRIEF com α = {args.lambda_alpha:g}, "
         f"seed {args.seed}, erro contra o GT `{'traj_truth.txt (original)' if args.gt == 'original' else 'traj_truth_sync.txt'}`.",
         "", "Cada curva é uma execução da engine: em cada voo, cada mapa (ano) roda uma vez com o abBRIEF e uma "
         "vez com o λ-BRIEF. O artigo fez o mesmo com 30 repetições por curva (média); aqui é 1.", "",
         "## Onde olhar", "",
         "| Arquivo | O que mostra |", "|---|---|",
         "| `compilado_artigo_ADM.png` | Como a Fig. 5: ADM1, ADM2 e ADM3, uma cor por mapa, ab pontilhado × λ cheio |",
         "| `compilado_artigo_UFRGS.png` | Como a Fig. 4 |",
         "| `<VOO>/<VOO>_compilado.png` | O gráfico do voo com a miniatura de cada mapa embaixo, na mesma cor da curva |",
         "| `<VOO>/<VOO>_mapa_<MM-AAAA>.png` | Um mapa (ano): erro do ab × λ por frame em cima; o mapa com o GT e as duas trajetórias estimadas embaixo |",
         "| `tabela.md` / `tabela.csv` | MAE, frame de convergência, rastreio e erro final por voo e mapa |",
         "| `execucoes/` | Saída bruta da engine (errors.txt, traj_estimated.txt, cmd.txt, run.log) |", "",
         "## Voos e mapas usados", "",
         "| Voo | Pasta | Imagens | Frames | Mapa (legenda do artigo) | Arquivo | Encontrado |", "|---|---|---|---|---|---|---|"]
    for flight in args.flights:
        info = FLIGHTS[flight]
        for lbl, mp in flight_maps(flight, args):
            ok = "sim" if os.path.exists(mp) else "**não**"
            if info.get("skip"):
                ok = "não roda"
            L.append(f"| {flight} | `{info['pasta']}` | {info['imagens']} | {info['n']} | {lbl} | "
                     f"`{os.path.relpath(mp, WS)}` | {ok} |")
    L += ["", "Notas:", "",
          "- ADM3: mapas e GT do Zenodo do artigo (zenodo.org/record/4070013), em `Datasets/ADM/0040_zenodo`. "
          "O erro é contra `GT_DAT/ADM3_ground_truth.txt` (não há GT re-sincronizado para esse voo); a altura vem "
          "do log de voo (`GT_DAT/ADM3.kml`), em `Datasets/ADM/0040/altimeter_baro.txt`.",
          "- As cinco datas da Fig. 5 vêm do `Maps.zip` do Zenodo do ADM3: 17/12/2018, 27/01/2014, 24/11/2016, "
          "08/09/2017 e 02/02/2014 (arquivos idênticos aos ADM_35_* do repositório).",
          "- ADM1 usa `Datasets/ADM/0035/traj_comp.txt` (nomes de imagem corrigidos; o traj.txt original apontava "
          "para .png que não existem)."]
    if notes:
        L += [""] + [f"- {n}" for n in notes]
    with open(os.path.join(args.out, "00_LEIA_ME.md"), "w") as f:
        f.write("\n".join(L) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--protocolo", default="artigo", choices=list(PROTOCOLS))
    ap.add_argument("--flights", nargs="*", default=ORDER, choices=ORDER)
    ap.add_argument("--regime", default=None, choices=["vanilla", "final_baro", "final_noalt"])
    ap.add_argument("--lambda_alpha", type=float, default=None)
    ap.add_argument("--gt", default=None, choices=["original", "sync"])
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--particles", type=int, default=50000)
    ap.add_argument("--jobs", type=int, default=4, help="execuções em paralelo")
    ap.add_argument("--omp_threads", type=int, default=2, help="threads OpenMP por execução")
    ap.add_argument("--adm_map_12_2018", default="ADM_35_17_12_2018.jpg")
    ap.add_argument("--adm_map_01_2014", default="ADM_35_27_01_2014.jpg")
    ap.add_argument("--bin", default=br.DEFAULT_BIN)
    ap.add_argument("--odom_dir", default=br.DEFAULT_ODOM_DIR)
    ap.add_argument("--out", default=None,
                    help="pasta de saída (padrão: experimentos/<hoje>_<nome>, registrada no INDICE.md)")
    ap.add_argument("--nome", default=None, help="nome do experimento (padrão: artigo_<protocolo>_a<α>)")
    ap.add_argument("--continuar", action="store_true",
                    help="reaproveita a pasta mais recente experimentos/*_<nome> (ex.: terminar no outro dia)")
    ap.add_argument("--force", action="store_true", help="roda de novo mesmo se errors.txt já existir")
    ap.add_argument("--plot_only", action="store_true")
    ap.add_argument("--dry_run", action="store_true")
    args = ap.parse_args()

    p = PROTOCOLS[args.protocolo]
    args.regime = args.regime or p["regime"]
    args.lambda_alpha = p["lambda_alpha"] if args.lambda_alpha is None else args.lambda_alpha
    args.gt = args.gt or p["gt"]
    descricao = (f"Teste do artigo: abBRIEF × λ-BRIEF por voo e mapa (protocolo {args.protocolo}, "
                 f"regime {args.regime}, λ α={args.lambda_alpha:g}, seed {args.seed}, GT {args.gt})")
    if args.out is None:
        nome = args.nome or f"artigo_{args.protocolo}_a{args.lambda_alpha:g}"
        args.out = exp.pasta_experimento(nome, descricao, continuar=args.continuar or args.plot_only,
                                         dry_run=args.dry_run)
    else:
        args.out = exp.resolve(args.out)
        if not args.dry_run and os.path.abspath(args.out).startswith(exp.EXP_DIR + os.sep):
            exp.registrar(args.out, descricao)

    tasks, notes = build_tasks(args)
    print(f"Protocolo {args.protocolo}: regime {args.regime}, λ α = {args.lambda_alpha:g}, seed {args.seed}, "
          f"GT {args.gt}\nSaída: {args.out}\n")
    for n in notes:
        print("[aviso]", n)

    if args.dry_run:
        ensure_odom(args)
        for t in tasks:
            print(f"  {t['flight']:5s} mapa {t['label']}  {t['desc']:22s} {os.path.basename(t['map_path'])}")
        print(f"\n{len(tasks)} execuções (uma por curva). Exemplo de comando:\n  {' '.join(tasks[0]['cmd']) if tasks else '-'}")
        return

    if not args.plot_only:
        if not ensure_odom(args):
            print("Odometria fixa indisponível; veja as mensagens acima.")
            sys.exit(1)
        print(f"{len(tasks)} execuções, {args.jobs} em paralelo ...", flush=True)
        with ThreadPoolExecutor(max_workers=args.jobs) as ex:
            futs = [ex.submit(run_task, t, args.omp_threads, args.force) for t in tasks]
            for i, f in enumerate(as_completed(futs), 1):
                t, status, dt = f.result()
                print(f"[{i}/{len(tasks)}] {t['flight']} mapa {t['label']} {t['desc']}: {status} ({dt:.0f}s)",
                      flush=True)

    make_outputs(tasks, notes, args)


if __name__ == "__main__":
    main()

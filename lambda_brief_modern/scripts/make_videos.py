#!/usr/bin/env python3
"""
make_videos.py — Vídeos do λ-BRIEF a partir das execuções do benchmark

Para cada caso (voo, mapa, descritor, regime):
  1. escolhe a seed de MAE mediano entre as execuções já consolidadas (runs.csv), para o vídeo
     ser representativo e não o melhor caso; sem runs.csv, usa a seed 1;
  2. roda a engine de novo com o MESMO comando da execução (cmd.txt), acrescentando
     --save_particles e o GT re-sincronizado (traj_truth_sync.txt, --gt_offset 0) para o desenho;
     a trajetória estimada é a mesma da execução original (a exportação de partículas não usa o RNG);
  3. renderiza com scripts/render_video.py (câmera + mapa lado a lado) e, se houver ffmpeg,
     converte para H.264 (abre em qualquer player/navegador).

Uso
  python3 scripts/make_videos.py                                   # casos padrão, agora
  python3 scripts/make_videos.py --wait_log piloto.log --wait_marker "mapas extras"
  python3 scripts/make_videos.py --cases ADM0036:ADM_35_27_02_2017:lambdaBRIEF:final_baro
  python3 scripts/make_videos.py --dry_run

Saída: <bench>/videos/<voo>_<mapa>_<descritor>_<regime>_seed<N>.mp4 (+ .txt com as métricas).
O padrão de --bench é o piloto (experimentos/2026-10-15_piloto_baro_vs_noalt).
"""

import argparse
import csv
import json
import os
import shutil
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)
import experimentos as exp  # noqa: E402

DEFAULT_CASES = [
    "ADM0036:ADM_35_12_09_2018:lambdaBRIEF:final_baro",
    "UFRGS:UFRGS_VET_V3_18_09_2018:lambdaBRIEF:final_baro",
    "ADM0036:ADM_35_12_09_2018:lambdaBRIEF:final_noalt",
    "UFRGS:UFRGS_VET_V3_18_09_2018:lambdaBRIEF:final_noalt",
]


def arg_of(cmd, key):
    return cmd[cmd.index(key) + 1] if key in cmd else None


def set_arg(cmd, key, value):
    if key in cmd:
        cmd[cmd.index(key) + 1] = value
    else:
        cmd += [key, value]


def pick_seed(runs_csv, ds, mp, desc, regime):
    if not os.path.exists(runs_csv):
        return 1, "runs.csv não encontrado: seed 1"
    rows = [r for r in csv.DictReader(open(runs_csv))
            if r["dataset"] == ds and r["map"] == mp and r["desc"] == desc and r["regime"] == regime]
    if not rows:
        return 1, "caso ausente no runs.csv: seed 1"
    rows.sort(key=lambda r: float(r["mae"]))
    r = rows[len(rows) // 2]
    return int(r["seed"]), f"seed de MAE mediano entre {len(rows)} (MAE {float(r['mae']):.1f} m)"


def summarize(errors_path):
    vals = []
    with open(errors_path) as f:
        f.readline()
        for line in f:
            p = line.split()
            if len(p) >= 8:
                vals.append(float(p[7]))
    if not vals:
        return "sem erros gravados"
    conv = next((i for i in range(len(vals) - 9) if all(v < 25 for v in vals[i:i + 10])), None)
    return (f"frames {len(vals)} | MAE {sum(vals) / len(vals):.1f} m | erro final {vals[-1]:.1f} m | "
            f"convergiu no frame {conv if conv is not None else '—'}")


def main():
    ap = argparse.ArgumentParser(description="Vídeos do λ-BRIEF a partir do benchmark")
    ap.add_argument("--bench", default=os.path.join(MODERN_DIR, "results_final", "B_piloto"),
                    help="pasta do benchmark (caminhos antigos são achados no layout novo)")
    ap.add_argument("--summary", default="summary_traj_truth_sync", help="Pasta do consolidado dentro de --bench")
    ap.add_argument("--out", default=None, help="padrão: <bench>/videos")
    ap.add_argument("--cases", nargs="*", default=DEFAULT_CASES, help="voo:mapa:descritor:regime")
    ap.add_argument("--seed", type=int, default=None, help="Força uma seed (padrão: MAE mediano)")
    ap.add_argument("--layout", default="side_by_side", choices=["side_by_side", "map", "camera"])
    ap.add_argument("--export_particles", type=int, default=2000)
    ap.add_argument("--fps", type=float, default=6.0)
    ap.add_argument("--omp_threads", type=int, default=4)
    ap.add_argument("--wait_log", default=None, help="Espera este log conter --wait_marker antes de começar")
    ap.add_argument("--wait_marker", default="FIM")
    ap.add_argument("--dry_run", action="store_true")
    args = ap.parse_args()
    args.bench = exp.resolve(args.bench)
    args.out = args.out or os.path.join(args.bench, "videos")
    if not args.dry_run and os.path.abspath(args.bench).startswith(exp.EXP_DIR + os.sep):
        exp.registrar(args.bench, "benchmark com vídeos (make_videos.py)")   # anota o comando no COMANDO.txt do experimento

    if args.wait_log:
        print(f"[videos] aguardando '{args.wait_marker}' em {args.wait_log} ...", flush=True)
        while not (os.path.exists(args.wait_log) and args.wait_marker in open(args.wait_log, errors="replace").read()):
            time.sleep(60)
        time.sleep(30)  # dá tempo da consolidação gravar o runs.csv

    os.makedirs(args.out, exist_ok=True)
    runs_csv = os.path.join(args.bench, args.summary, "runs.csv")
    ffmpeg = shutil.which("ffmpeg")
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(args.omp_threads)
    made = []

    for case in args.cases:
        ds, mp, desc, regime = case.split(":")
        seed, why = (args.seed, "seed escolhida") if args.seed else pick_seed(runs_csv, ds, mp, desc, regime)
        src = os.path.join(args.bench, ds, mp, f"{desc}_regime_{regime}", f"seed_{seed}", "cmd.txt")
        tag = f"{ds}_{mp}_{desc}_{regime}_seed{seed}"
        print(f"\n[videos] {tag} — {why}", flush=True)
        if not os.path.exists(src):
            print(f"  pulando: {src} não existe (a execução ainda não rodou?)")
            continue
        cmd = [exp.resolve(c) for c in json.load(open(src))]   # ex.: results_final/odom -> insumos/odom
        run_dir = os.path.join(args.out, tag)
        set_arg(cmd, "--output_dir", run_dir)
        ds_dir = arg_of(cmd, "--dataset_dir")
        sync_gt = os.path.join(ds_dir, "traj_truth_sync.txt")
        if os.path.exists(sync_gt):
            set_arg(cmd, "--gt", sync_gt)
            set_arg(cmd, "--gt_offset", "0")
        set_arg(cmd, "--export_particles", str(args.export_particles))
        if "--save_particles" not in cmd:
            cmd.append("--save_particles")

        render = [sys.executable, os.path.join(SCRIPT_DIR, "render_video.py"),
                  "--results_dir", run_dir, "--map", arg_of(cmd, "--map"), "--dataset_dir", ds_dir,
                  "--output", os.path.join(args.out, tag + "_raw.mp4"), "--layout", args.layout,
                  "--fps", str(args.fps), "--pixel_to_meter", arg_of(cmd, "--pixel_to_meter"),
                  "--start_frame", arg_of(cmd, "--start_frame")]
        if arg_of(cmd, "--odom_file"):
            render += ["--odom_file", arg_of(cmd, "--odom_file")]
        if args.layout == "side_by_side":
            render += ["--video_width", "1920", "--video_height", "1080"]

        if args.dry_run:
            print("  engine:", " ".join(cmd))
            print("  render:", " ".join(render))
            continue

        os.makedirs(run_dir, exist_ok=True)
        t0 = time.time()
        with open(os.path.join(run_dir, "run.log"), "w") as lf:
            rc = subprocess.run(cmd, stdout=lf, stderr=subprocess.STDOUT, env=env).returncode
        if rc != 0:
            print(f"  engine FALHOU (exit {rc}), veja {run_dir}/run.log")
            continue
        print(f"  engine ok ({time.time() - t0:.0f}s): {summarize(os.path.join(run_dir, 'errors.txt'))}", flush=True)

        t0 = time.time()
        with open(os.path.join(run_dir, "render.log"), "w") as lf:
            rc = subprocess.run(render, stdout=lf, stderr=subprocess.STDOUT).returncode
        raw = os.path.join(args.out, tag + "_raw.mp4")
        if rc != 0 or not os.path.exists(raw):
            print(f"  render FALHOU (exit {rc}), veja {run_dir}/render.log")
            continue
        final = os.path.join(args.out, tag + ".mp4")
        if ffmpeg:
            r = subprocess.run([ffmpeg, "-y", "-loglevel", "error", "-i", raw, "-c:v", "libx264",
                                "-pix_fmt", "yuv420p", "-crf", "20", final])
            if r.returncode == 0:
                os.remove(raw)
            else:
                os.replace(raw, final)
        else:
            os.replace(raw, final)
        with open(os.path.join(args.out, tag + ".txt"), "w") as f:
            f.write(f"{tag}\n{why}\n{summarize(os.path.join(run_dir, 'errors.txt'))}\n"
                    f"GT desenhado: {'traj_truth_sync.txt' if os.path.exists(sync_gt) else 'traj_truth.txt'}\n")
        print(f"  vídeo: {final} ({time.time() - t0:.0f}s)", flush=True)
        made.append(final)

    print(f"\n[videos] {len(made)} vídeo(s) em {args.out}")


if __name__ == "__main__":
    main()

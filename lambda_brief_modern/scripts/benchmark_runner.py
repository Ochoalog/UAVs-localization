#!/usr/bin/env python3
"""
benchmark_runner.py — Execução automatizada do benchmark abBRIEF × λ-BRIEF

Roda simulações MCL pareadas (mesmas seeds para todos os descritores) por
dataset, mapa, descritor/α, regime e seed, seguindo o protocolo do AGENTS.md.

Regimes
-------
vanilla      Protocolo do artigo: sem priors, sem injeção, odometria gravada do legado.
final_baro   Filtro em duas fases + injeção adaptativa + --pos_noise 17 + ALTÍMETRO REAL
             (barômetro do log do DJI, <dataset>/altimeter_baro.txt gerado por
             scripts/extract_altimeter.py; z = h_baro/(f_px·GSD), ganho 1,0). Recomendado.
final_noalt  O mesmo, sem nenhum prior de escala: só câmera + VO. É o cenário que uma CNN de
             localização global precisaria resolver.
final_gtalt  O mesmo, com o altímetro EMULADO a partir do z do traj_truth.txt (ganho por dataset).
             Só como referência: no ADM0036 o z do GT está ~45 frames defasado e no UFRGS 19% abaixo
             da escala física (ver insumos/altimetro/). "final" é um alias deste regime.
Todos os final_* usam a odometria FIXA gerada pela VO nova (ratio + MAGSAC, 4000 SIFT), com dz = 1.
Gere uma vez por dataset com --make_odom. (Com --odom_file a engine ignora --vo_scale_deadband.)

Exemplos
--------
  # 0) altímetro real (uma vez; gera Datasets/*/altimeter_baro.txt e traj_truth_sync.txt)
  python3 scripts/extract_altimeter.py

  # 1) odometria fixa dos regimes final_* (uma vez por dataset)
  python3 scripts/benchmark_runner.py --make_odom --dataset ADM0036 ADM0035 UFRGS

  # 2) Benchmark A (artigo) com α = 0, 0.5 e 1.0
  #    (sem --output_base_dir a saída vai para experimentos/<hoje>_<nome>/ e entra no INDICE.md)
  python3 scripts/benchmark_runner.py --dataset ADM0035 --regime vanilla \\
      --alphas 0.5 1.0 --seeds 1..30 --nome A_artigo

  # 3) Duas fases com barômetro real e sem altímetro
  python3 scripts/benchmark_runner.py --dataset ADM0036 ADM0035 UFRGS --regime final_baro \\
      --seeds 1..30 --nome B_engine
  python3 scripts/benchmark_runner.py --dataset ADM0036 ADM0035 UFRGS --regime final_noalt \\
      --seeds 1..30 --nome B_engine --continuar      # mesma pasta, mesmo que seja outro dia

  # Reaproveitar o vanilla antigo do ADM2 só se ele for reproduzido bit a bit
  #   (A=experimentos/<data>_A_artigo; copie para ela experimentos/2026-10-01_benchmark_artigo_vanilla/ADM0036
  #    e apague dela o mapa duplicado ADM_35_05_09_2018)
  python3 scripts/benchmark_runner.py --dataset ADM0036 --regime vanilla --seeds 1..2 \\
      --nome A_artigo --continuar --adopt_existing --verify_seeds 1,2
  # se todas as linhas disserem VERIFIED, adote o resto:
  python3 scripts/benchmark_runner.py --dataset ADM0036 --regime vanilla --seeds 1..30 \\
      --nome A_artigo --continuar --adopt_existing

  # Ver os comandos sem rodar
  python3 scripts/benchmark_runner.py --dataset ADM0036 --regime final_baro --dry_run

Estrutura de saída (AGENTS.md). <output_base_dir> padrão: experimentos/<AAAA-MM-DD>_<nome>/
  <output_base_dir>/<dataset>/<mapa>/<descritor>_regime_<regime>/seed_<N>/
      errors.txt  traj_estimated.txt  run.log  cmd.txt  meta.json  [phase_log.txt]
Descritores: abBRIEF (α = 0), lambdaBRIEF (α = 0.5) e lambdaBRIEF_a<α> para outros α.
"""

import argparse
import datetime
import hashlib
import json
import os
import shutil
import socket
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
WORKSPACE_DIR = os.path.dirname(MODERN_DIR)
sys.path.insert(0, SCRIPT_DIR)
import experimentos as exp  # noqa: E402

DEFAULT_BIN = os.path.join(MODERN_DIR, "build", "lambda_brief_loc")
DEFAULT_ODOM_DIR = exp.odom_dir()   # insumos/odom (ou results_final/odom antes de mover as pastas)

ADM_MAPS_DIR = os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0036", "Mapas")

# Mapas do artigo para ADM1/ADM2. ADM_35_05_09_2018.jpg foi removido: é o MESMO
# arquivo que ADM_35_12_09_2018.jpg (md5 idêntico, resultados bit a bit iguais).
ADM_PAPER_MAPS = [
    "ADM_35_12_09_2018.jpg",
    "ADM_35_17_08_2018.jpg",
    "ADM_35_17_12_2018.jpg",
    "ADM_35_27_02_2017.jpg",
]
KNOWN_DUPLICATE_MAPS = {"ADM_35_05_09_2018.jpg": "ADM_35_12_09_2018.jpg"}

# start/end_frame = índices do traj.txt (fatia avaliada); gt_offset = linha do traj.txt
# que corresponde à linha 0 do traj_truth.txt. Conferidos em 15/10/2026 contra a
# contagem de linhas do GT e da odometria gravada (N_frames − 1 transições).
# scale_prior_gain: só para o regime final_gtalt (altímetro emulado do GT). None = não calibrado.
DATASET_CONFIGS = {
    "ADM0036": {  # ADM2 do artigo: imagens 0199–0394, 196 frames
        "dataset_dir": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0036"),
        "maps_dir": ADM_MAPS_DIR,
        "default_maps": ADM_PAPER_MAPS,
        "start_frame": 84,
        "end_frame": 279,
        "gt_offset": 84,
        "pixel_to_meter": 0.188679245,
        "min_scale": 0.1,
        "max_scale": 3.0,
        "scale_prior_gain": 0.87,
        "legacy_odom": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0036", "traj",
                                    "odom-190723-145134_sift_bruteforce.txt"),
    },
    "ADM0035": {  # ADM1 do artigo: imagens 0103–0402, 300 frames (AGENTS.md dizia 318: fim correto é 317)
        "dataset_dir": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0035"),
        # Os mapas do artigo (12/09, 17/08, 17/12 de 2018 e 27/02/2017) estão em 0036/Mapas,
        # não em 0035/Mapas; os dois voos usam o mesmo sistema de coordenadas ADM_35.
        "maps_dir": ADM_MAPS_DIR,
        "default_maps": ADM_PAPER_MAPS,
        "start_frame": 18,
        "end_frame": 317,
        "gt_offset": 18,
        "pixel_to_meter": 0.188679245,
        "min_scale": 0.1,
        "max_scale": 3.0,
        "scale_prior_gain": None,
        "legacy_odom": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0035", "traj",
                                    "odom-190717-170541.txt"),
    },
    "ADM0040": {  # ADM3 do artigo: imagens 0200–0465, 266 frames
        "dataset_dir": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0040"),
        # Datasets/ADM/0040 não tem pasta Mapas, e o traj_truth.txt local está em outro sistema
        # de coordenadas. Os mapas e o GT certos são os do Zenodo do artigo (4070013), em
        # Datasets/ADM/0040_zenodo; o paper_comparison*.py sobrescreve maps_dir, pixel_to_meter
        # (0,188679245) e min_scale (0,1) para usá-los. Altímetro: scripts/adm3_altimeter_from_kml.py.
        "maps_dir": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0040", "Mapas"),
        "default_maps": [],
        "start_frame": 55,
        "end_frame": 320,
        "gt_offset": 55,
        "pixel_to_meter": 0.239726027,
        "min_scale": 0.05,
        "max_scale": 3.0,
        "scale_prior_gain": None,
        "legacy_odom": os.path.join(WORKSPACE_DIR, "Datasets", "ADM", "0040", "traj",
                                    "odom-200122-132345.txt"),
    },
    "UFRGS": {  # UFRGS VET voo_100m: imagens 0390–0613, 224 frames
        "dataset_dir": os.path.join(WORKSPACE_DIR, "Datasets", "UFRGS VET", "voo_100m"),
        "maps_dir": os.path.join(WORKSPACE_DIR, "Datasets", "UFRGS VET", "voo_100m", "Mapa"),
        # 4 mapas dos experimentos do legado (UFRGS_VET_390_613_50K_apenas_beta_4_mapas).
        # O artigo usou 12; amplie com --maps quando a lista for recuperada.
        "default_maps": [
            "UFRGS_VET_V3_18_09_2018.jpg",
            "UFRGS_VET_V3_02_04_2015.jpg",
            "UFRGS_VET_V3_17_12_2002.jpg",
            "UFRGS_VET_V3_23_08_2016.jpg",
        ],
        "start_frame": 204,
        "end_frame": 427,
        "gt_offset": 204,
        "pixel_to_meter": 0.10,
        "min_scale": 0.5,
        "max_scale": 5.0,
        "scale_prior_gain": None,
        "legacy_odom": os.path.join(WORKSPACE_DIR, "Datasets", "UFRGS VET", "voo_100m", "traj",
                                    "odom-190219-144314.txt"),
    },
}

# Parâmetros comuns dos regimes final_* (duas fases). Ver docs/revisao_odometria.md, seção 3.1.
FINAL_REGIME_ARGS = [
    "--two_phase",
    "--inject_ratio", "0.05",
    "--inject_spread", "300",
    "--pos_noise", "17",
]
SCALE_PRIOR_SIGMA = "0.10"
FINAL_REGIMES = ("final_baro", "final_noalt", "final_gtalt")
REGIME_ALIASES = {"final": "final_gtalt"}
ALTIMETER_FILE = "altimeter_baro.txt"
# VO usada para gerar a odometria fixa do regime final.
NEW_VO_ARGS = [
    "--vo_matcher", "ratio",
    "--vo_robust", "magsac",
    "--vo_reproj", "2",
    "--sift_features", "4000",
    "--vo_scale_deadband", "1",
]
LAMBDA_ARGS = ["--vi", "NDVI", "--ms_filter", "bilateral", "--bf_d", "6", "--bf_sc", "5.0", "--bf_ss", "5.0"]


# --------------------------------------------------------------------------- utilidades

def parse_seeds(seed_str):
    seeds = []
    for part in seed_str.split(","):
        part = part.strip()
        if ".." in part:
            start, end = part.split("..")
            seeds.extend(range(int(start), int(end) + 1))
        elif "-" in part and not part.startswith("-"):
            start, end = part.split("-")
            seeds.extend(range(int(start), int(end) + 1))
        elif part:
            seeds.append(int(part))
    return sorted(set(seeds))


def descriptor_variants(descriptors, alphas):
    """Lista de (rótulo, α). abBRIEF = α 0; lambdaBRIEF = α 0.5; outros α ganham sufixo."""
    variants = []
    if "abBRIEF" in descriptors:
        variants.append(("abBRIEF", 0.0))
    if "lambdaBRIEF" in descriptors:
        for a in alphas:
            if a <= 0.0:
                raise ValueError("Use o descritor abBRIEF para α = 0 (não inclua 0 em --alphas).")
            label = "lambdaBRIEF" if abs(a - 0.5) < 1e-12 else f"lambdaBRIEF_a{a:g}"
            variants.append((label, a))
    return variants


def odom_path_for(dataset, odom_dir):
    return os.path.join(odom_dir, f"{dataset}_vo_ratio.txt")


def count_odom_lines(path):
    with open(path) as f:
        return sum(1 for line in f if line.strip())


def file_md5(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def git_info():
    try:
        rev = subprocess.run(["git", "-C", MODERN_DIR, "rev-parse", "HEAD"],
                             capture_output=True, text=True, timeout=10)
        dirty = subprocess.run(["git", "-C", MODERN_DIR, "status", "--porcelain", "--untracked-files=no"],
                               capture_output=True, text=True, timeout=10)
        if rev.returncode == 0:
            return {"commit": rev.stdout.strip(), "dirty": bool(dirty.stdout.strip())}
    except Exception:
        pass
    return {"commit": None, "dirty": None}


def comparable_cmd(cmd):
    """Comando sem o valor de --output_dir (a pasta pode mudar sem mudar a configuração).

    Caminhos das pastas antigas (ex.: results_final/odom) são trocados pelos novos
    (insumos/odom), para um cmd.txt antigo continuar batendo depois da reorganização."""
    out = [exp.canonical(c) for c in cmd]
    if "--output_dir" in out:
        i = out.index("--output_dir")
        if i + 1 < len(out):
            out[i + 1] = "<OUT>"
    return out


# --------------------------------------------------------------------------- comandos

def build_command(bin_path, ds_cfg, map_path, out_dir, alpha, regime, seed, num_particles,
                  scale_prior_gain=None, final_odom=None):
    cmd = [
        bin_path,
        "--map", map_path,
        "--dataset_dir", ds_cfg["dataset_dir"],
        "--output_dir", out_dir,
        "--start_frame", str(ds_cfg["start_frame"]),
        "--end_frame", str(ds_cfg["end_frame"]),
        "--gt_offset", str(ds_cfg["gt_offset"]),
        "--num_particles", str(num_particles),
        "--pairs", "256",
        "--quantize", "25",
        "--min_scale", str(ds_cfg["min_scale"]),
        "--max_scale", str(ds_cfg["max_scale"]),
        "--pixel_to_meter", str(ds_cfg["pixel_to_meter"]),
        "--color_mode", "LAB_NO_L",
        "--seed", str(seed),
        "--quiet",
    ]

    if alpha <= 0.0:
        cmd.extend(["--alpha", "0.0"])
    else:
        cmd.extend(["--alpha", f"{alpha:g}"] + LAMBDA_ARGS)

    if regime == "vanilla":
        cmd.extend(["--odom_file", ds_cfg["legacy_odom"]])
    elif regime in FINAL_REGIMES:
        cmd.extend(["--odom_file", final_odom])
        cmd.extend(FINAL_REGIME_ARGS)
        if regime == "final_baro":
            cmd.extend(["--scale_prior_sigma", SCALE_PRIOR_SIGMA,
                        "--altimeter_file", os.path.join(ds_cfg["dataset_dir"], ALTIMETER_FILE),
                        "--scale_prior_gain", f"{scale_prior_gain:g}"])
        elif regime == "final_gtalt":
            cmd.extend(["--scale_prior_sigma", SCALE_PRIOR_SIGMA,
                        "--scale_prior_gain", f"{scale_prior_gain:g}"])
    else:
        raise ValueError(f"Regime desconhecido: {regime}")
    return cmd


def build_odom_command(bin_path, ds_cfg, map_path, out_dir):
    return [
        bin_path,
        "--map", map_path,
        "--dataset_dir", ds_cfg["dataset_dir"],
        "--output_dir", out_dir,
        "--start_frame", str(ds_cfg["start_frame"]),
        "--end_frame", str(ds_cfg["end_frame"]),
        "--gt_offset", str(ds_cfg["gt_offset"]),
        "--num_particles", "1000",
        "--alpha", "0.0",
        "--quantize", "25",
        "--min_scale", str(ds_cfg["min_scale"]),
        "--max_scale", str(ds_cfg["max_scale"]),
        "--pixel_to_meter", str(ds_cfg["pixel_to_meter"]),
        "--seed", "1",
        "--quiet",
    ] + NEW_VO_ARGS


# --------------------------------------------------------------------------- odometria fixa

def make_odom(dataset, ds_cfg, bin_path, odom_dir, map_file, force, dry_run):
    """Gera <odom_dir>/<dataset>_vo_ratio.txt a partir do LB_DEBUG_ODOM da VO nova.

    Formato de saída = traj/odom-*.txt do legado: "dx dy dz dyaw reliable", uma linha por
    transição (N_frames − 1 linhas). dz é sempre gravado como 1: a escala da VO não mede a
    altitude, e com --odom_file a engine não aplica --vo_scale_deadband.
    """
    out_txt = odom_path_for(dataset, odom_dir)
    out_log = out_txt.replace(".txt", ".log")
    expected = ds_cfg["end_frame"] - ds_cfg["start_frame"]  # N_frames − 1

    if os.path.exists(out_txt) and not force:
        n = count_odom_lines(out_txt)
        status = "OK" if n == expected else f"ERRO: {n} linhas, esperado {expected}"
        print(f"[odom] {dataset}: já existe ({out_txt}) — {status}. Use --force para gerar de novo.")
        return n == expected

    map_path = os.path.join(ds_cfg["maps_dir"], map_file)
    if not os.path.exists(map_path):
        print(f"[odom] {dataset}: mapa não encontrado ({map_path}). Passe --maps/--maps_dir.")
        return False

    tmp_out = os.path.join(odom_dir, f"_tmp_{dataset}")
    cmd = build_odom_command(bin_path, ds_cfg, map_path, tmp_out)
    if dry_run:
        print(f"[DRY-RUN] LB_DEBUG_ODOM=1 {' '.join(cmd)} 2> {out_log}")
        return True

    os.makedirs(odom_dir, exist_ok=True)
    env = os.environ.copy()
    env["LB_DEBUG_ODOM"] = "1"
    print(f"[odom] {dataset}: gerando com a VO nova ({' '.join(NEW_VO_ARGS)}) ...", flush=True)
    t0 = time.time()
    with open(out_log, "w") as f_err:
        res = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=f_err, env=env)
    if res.returncode != 0:
        print(f"[odom] {dataset}: FALHOU (exit {res.returncode}). Veja {out_log}")
        return False

    steps = {}
    with open(out_log) as f:
        for line in f:
            if not line.startswith("ODOM "):
                continue
            p = line.split()
            frame = int(p[1])
            dx, dy, _dz, dyaw = (float(v) for v in p[2:6])
            steps[frame] = (dx, dy, dyaw, int(p[6]))
    frames = sorted(steps)
    if frames != list(range(1, expected + 1)):
        print(f"[odom] {dataset}: ERRO — o log tem {len(frames)} transições (esperado {expected}, "
              f"frames 1..{expected}). Veja {out_log}")
        return False

    with open(out_txt, "w") as f:
        for k in frames:
            dx, dy, dyaw, rel = steps[k]
            f.write(f"{dx:.6g} {dy:.6g} 1 {dyaw:.6g} {rel}\n")
    shutil.rmtree(tmp_out, ignore_errors=True)
    n_unrel = sum(1 for k in frames if steps[k][3] == 0)
    with open(out_txt + ".meta.json", "w") as f:
        json.dump({"dataset": dataset, "map": map_file, "cmd": ["LB_DEBUG_ODOM=1"] + cmd,
                   "transitions": len(frames), "unreliable": n_unrel,
                   "binary_md5": file_md5(bin_path), "git": git_info(),
                   "created": datetime.datetime.now().isoformat(timespec="seconds")}, f, indent=2)
    print(f"[odom] {dataset}: {len(frames)} transições ({n_unrel} não confiáveis) -> {out_txt} "
          f"({time.time() - t0:.0f}s)")
    return True


# --------------------------------------------------------------------------- execução

def run_single_simulation(task, omp_threads, dry_run, adopt_existing, verify):
    t0 = time.time()
    tag = f"[{task['dataset']}|{task['map_name']}|{task['desc']}|regime_{task['regime']}|seed_{task['seed']}]"
    out_dir = task["out_dir"]
    errors_file = os.path.join(out_dir, "errors.txt")
    cmd_file = os.path.join(out_dir, "cmd.txt")
    have_result = os.path.exists(errors_file) and os.path.getsize(errors_file) > 100

    if have_result and not task["force"]:
        if os.path.exists(cmd_file):
            with open(cmd_file) as f:
                old = json.load(f)
            if comparable_cmd(old) != comparable_cmd(task["cmd"]):
                print(f"[STALE] {tag} existe com OUTRA configuração ({cmd_file}). "
                      f"Use outra --output_base_dir ou --force.", flush=True)
                return {"task": task, "status": "STALE_CONFIG", "duration": 0.0}
            status = "SKIPPED_EXISTING"
        elif adopt_existing:
            status = "ADOPTED"
        else:
            print(f"[STALE] {tag} tem errors.txt sem cmd.txt (resultado antigo). "
                  f"Use --adopt_existing se ele foi gerado com esta configuração.", flush=True)
            return {"task": task, "status": "STALE_CONFIG", "duration": 0.0}

        if dry_run:
            return {"task": task, "status": status, "duration": 0.0}
        result = {"task": task, "status": status, "duration": 0.0}
        if verify and task["seed"] in verify:
            result = verify_existing(task, omp_threads, tag, status)
        # Só adota (grava cmd.txt/meta.json) se não houve verificação ou se ela passou.
        if status == "ADOPTED" and result["status"] != "MISMATCH":
            with open(cmd_file, "w") as f:
                json.dump(task["cmd"], f, indent=1)
            write_meta(task, omp_threads, 0.0, adopted=True)
        return result

    if dry_run:
        print(f"[DRY-RUN] {tag} -> {' '.join(task['cmd'])}")
        return {"task": task, "status": "DRY_RUN", "duration": 0.0}

    os.makedirs(out_dir, exist_ok=True)
    print(f"[START] {tag}...", flush=True)
    rc = run_engine(task["cmd"], os.path.join(out_dir, "run.log"), omp_threads)
    duration = time.time() - t0
    if rc == 0 and os.path.exists(errors_file) and os.path.getsize(errors_file) > 100:
        with open(cmd_file, "w") as f:
            json.dump(task["cmd"], f, indent=1)
        write_meta(task, omp_threads, duration, adopted=False)
        print(f"[DONE ] {tag} ({duration:.1f}s)", flush=True)
        return {"task": task, "status": "SUCCESS", "duration": duration}
    print(f"[FAIL ] {tag} (exit code {rc}, veja {os.path.join(out_dir, 'run.log')})", flush=True)
    return {"task": task, "status": "FAILED", "duration": duration}


def run_engine(cmd, log_file, omp_threads):
    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(omp_threads)
    with open(log_file, "w") as f_log:
        res = subprocess.run(cmd, stdout=f_log, stderr=subprocess.STDOUT, env=env)
    # Avisos da engine (ex.: --odom_file mais curto que a fatia, opção desconhecida)
    with open(log_file, errors="replace") as f:
        for line in f:
            if line.startswith("Warning"):
                print(f"        {line.strip()}", flush=True)
    return res.returncode


def verify_existing(task, omp_threads, tag, status):
    """Roda de novo numa pasta temporária e compara errors.txt bit a bit (teste de regressão)."""
    vdir = os.path.join(task["out_dir"], "_verify")
    os.makedirs(vdir, exist_ok=True)
    cmd = list(task["cmd"])
    cmd[cmd.index("--output_dir") + 1] = vdir
    t0 = time.time()
    rc = run_engine(cmd, os.path.join(vdir, "run.log"), omp_threads)
    a = os.path.join(task["out_dir"], "errors.txt")
    b = os.path.join(vdir, "errors.txt")
    same = rc == 0 and os.path.exists(b) and open(a, "rb").read() == open(b, "rb").read()
    verdict = "VERIFIED" if same else "MISMATCH"
    print(f"[{verdict}] {tag} ({time.time() - t0:.1f}s) — resultado existente "
          f"{'reproduzido bit a bit' if same else 'NÃO reproduzido: não reaproveite'}", flush=True)
    if same:
        shutil.rmtree(vdir, ignore_errors=True)
    return {"task": task, "status": verdict, "duration": time.time() - t0}


def write_meta(task, omp_threads, duration, adopted):
    meta = {
        "dataset": task["dataset"], "map": task["map_name"], "descriptor": task["desc"],
        "alpha": task["alpha"], "regime": task["regime"], "seed": task["seed"],
        "binary_md5": task["binary_md5"], "git": task["git"],
        "omp_threads": omp_threads, "duration_s": round(duration, 2),
        "host": socket.gethostname(), "adopted": adopted,
        "written": datetime.datetime.now().isoformat(timespec="seconds"),
    }
    with open(os.path.join(task["out_dir"], "meta.json"), "w") as f:
        json.dump(meta, f, indent=2)


# --------------------------------------------------------------------------- main

def main():
    parser = argparse.ArgumentParser(
        description="Benchmark abBRIEF × λ-BRIEF (regimes vanilla = artigo, final = melhor engine)",
        formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__.split("Exemplos")[1])
    parser.add_argument("--dataset", nargs="+", default=["ADM0036"], choices=list(DATASET_CONFIGS),
                        help="Dataset(s) (padrão: ADM0036)")
    parser.add_argument("--maps", nargs="*", default=None,
                        help="Arquivos de mapa (padrão: mapas do artigo). Só com um dataset")
    parser.add_argument("--maps_dir", default=None, help="Pasta dos mapas (sobrescreve a do dataset)")
    parser.add_argument("--descriptors", nargs="*", default=["abBRIEF", "lambdaBRIEF"],
                        choices=["abBRIEF", "lambdaBRIEF"], help="Descritores (padrão: os dois)")
    parser.add_argument("--alphas", nargs="*", type=float, default=[0.5],
                        help="Valores de α do λ-BRIEF (padrão: 0.5). Ex.: --alphas 0.5 1.0")
    parser.add_argument("--regime", default="vanilla",
                        choices=["vanilla", *FINAL_REGIMES, *REGIME_ALIASES],
                        help="vanilla | final_baro (barômetro real) | final_noalt (sem altímetro) | "
                             "final_gtalt (altímetro emulado do GT; alias: final)")
    parser.add_argument("--seeds", default="1..30", help="Ex.: '1..30' ou '1,2,42' (padrão: 1..30)")
    parser.add_argument("--num_particles", type=int, default=50000, help="Partículas (padrão: 50000)")
    parser.add_argument("--scale_prior_gain", type=float, default=None,
                        help="Ganho do prior de escala: final_baro (padrão 1,0, sensibilidade) ou "
                             "final_gtalt (sobrescreve o do dataset)")
    parser.add_argument("--workers", type=int, default=2, help="Execuções simultâneas (padrão: 2)")
    parser.add_argument("--omp_threads", type=int, default=4,
                        help="OMP_NUM_THREADS por execução (padrão: 4). Mantenha workers×threads ≤ núcleos")
    parser.add_argument("--output_base_dir", default=None,
                        help="Pasta base de saída (padrão: experimentos/<hoje>_<nome>, registrada no INDICE.md)")
    parser.add_argument("--nome", default=None,
                        help="Nome do experimento na pasta padrão (padrão: benchmark_<regime>_<datasets>)")
    parser.add_argument("--continuar", action="store_true",
                        help="Reaproveita a pasta mais recente experimentos/*_<nome> (continuar outro dia)")
    parser.add_argument("--odom_dir", default=DEFAULT_ODOM_DIR,
                        help="Pasta da odometria fixa do regime final (padrão: insumos/odom)")
    parser.add_argument("--bin", default=DEFAULT_BIN, help="Binário lambda_brief_loc")
    parser.add_argument("--make_odom", action="store_true",
                        help="Só gera a odometria fixa (VO nova, dz = 1) dos datasets e sai")
    parser.add_argument("--adopt_existing", action="store_true",
                        help="Aceita errors.txt antigos sem cmd.txt (ex.: results_benchmark copiado)")
    parser.add_argument("--verify_seeds", default=None,
                        help="Roda de novo estas seeds dos resultados existentes e compara bit a bit")
    parser.add_argument("--force", action="store_true", help="Roda de novo mesmo se errors.txt existir")
    parser.add_argument("--dry_run", action="store_true", help="Só imprime os comandos")
    args = parser.parse_args()

    if not args.dry_run and not os.path.exists(args.bin):
        print(f"Erro: binário {args.bin} não encontrado. Compile o lambda_brief_modern primeiro.")
        sys.exit(1)
    if args.maps and len(args.dataset) > 1:
        print("Erro: --maps só pode ser usado com um dataset por vez.")
        sys.exit(1)

    configs = {}
    for ds in args.dataset:
        cfg = dict(DATASET_CONFIGS[ds])
        if args.maps_dir:
            cfg["maps_dir"] = os.path.abspath(args.maps_dir)
        configs[ds] = cfg

    # ---- modo odometria
    if args.make_odom:
        ok = True
        for ds, cfg in configs.items():
            maps = args.maps if args.maps else cfg["default_maps"]
            if not maps:
                print(f"[odom] {ds}: sem mapas configurados. Passe --maps/--maps_dir.")
                ok = False
                continue
            ok &= make_odom(ds, cfg, args.bin, args.odom_dir, maps[0], args.force, args.dry_run)
        sys.exit(0 if ok else 1)

    # ---- validações dos regimes final_*
    if args.regime in REGIME_ALIASES:
        print(f"Aviso: --regime {args.regime} = {REGIME_ALIASES[args.regime]} (altímetro emulado do GT).")
        args.regime = REGIME_ALIASES[args.regime]
    if args.regime in FINAL_REGIMES:
        problems = []
        for ds, cfg in configs.items():
            if args.regime == "final_gtalt":
                gain = args.scale_prior_gain if args.scale_prior_gain is not None else cfg["scale_prior_gain"]
                if gain is None:
                    problems.append(f"{ds}: scale_prior_gain do altímetro emulado não calibrado; "
                                    f"passe --scale_prior_gain (ou use --regime final_baro).")
            else:
                gain = args.scale_prior_gain if args.scale_prior_gain is not None else 1.0
            cfg["_gain"] = gain
            if args.regime == "final_baro":
                alt = os.path.join(cfg["dataset_dir"], ALTIMETER_FILE)
                if not os.path.exists(alt) and not args.dry_run:
                    problems.append(f"{ds}: {alt} não existe. Rode antes: scripts/extract_altimeter.py "
                                    f"(o ADM0040 não tem log do DJI no repositório).")
            odom = odom_path_for(ds, args.odom_dir)
            expected = cfg["end_frame"] - cfg["start_frame"]
            if not os.path.exists(odom):
                if not args.dry_run:
                    problems.append(f"{ds}: odometria fixa {odom} não existe. Rode antes: "
                                    f"benchmark_runner.py --make_odom --dataset {ds}")
            elif count_odom_lines(odom) != expected:
                problems.append(f"{ds}: {odom} tem {count_odom_lines(odom)} linhas, esperado {expected}.")
            cfg["_final_odom"] = os.path.abspath(odom)
        if problems:
            print(f"Não dá para rodar o regime {args.regime}:\n  - " + "\n  - ".join(problems))
            sys.exit(1)
    else:
        for ds, cfg in configs.items():
            if not os.path.exists(cfg["legacy_odom"]):
                print(f"Erro: odometria do legado não encontrada: {cfg['legacy_odom']}")
                sys.exit(1)

    variants = descriptor_variants(args.descriptors, args.alphas)
    seeds_to_run = parse_seeds(args.seeds)

    descricao = (f"Benchmark abBRIEF × λ-BRIEF, regime {args.regime}, {', '.join(configs)}, "
                 f"seeds {args.seeds}" + (f", mapas {' '.join(args.maps)}" if args.maps else ""))
    if args.output_base_dir is None:
        nome = args.nome or f"benchmark_{args.regime}_{'-'.join(configs)}"
        args.output_base_dir = exp.pasta_experimento(nome, descricao, continuar=args.continuar,
                                                     dry_run=args.dry_run)
    elif not args.dry_run and os.path.abspath(args.output_base_dir).startswith(exp.EXP_DIR + os.sep):
        exp.registrar(args.output_base_dir, descricao)
    verify = set(parse_seeds(args.verify_seeds)) if args.verify_seeds else set()
    bin_md5 = file_md5(args.bin) if os.path.exists(args.bin) else None
    ginfo = git_info()

    tasks, n_frames = [], 0
    for ds, cfg in configs.items():
        maps_to_run = args.maps if args.maps else cfg["default_maps"]
        if not maps_to_run:
            print(f"Aviso: {ds} não tem mapas configurados (ver comentário em DATASET_CONFIGS). Pulando.")
            continue
        for map_file in maps_to_run:
            if map_file in KNOWN_DUPLICATE_MAPS:
                print(f"Aviso: {map_file} é cópia idêntica de {KNOWN_DUPLICATE_MAPS[map_file]}. Pulando.")
                continue
            map_path = os.path.join(cfg["maps_dir"], map_file)
            if not os.path.exists(map_path) and not args.dry_run:
                print(f"Aviso: mapa não encontrado: {map_path}. Pulando.")
                continue
            map_name = os.path.splitext(map_file)[0]
            for label, alpha in variants:
                for seed in seeds_to_run:
                    out_dir = os.path.join(args.output_base_dir, ds, map_name,
                                           f"{label}_regime_{args.regime}", f"seed_{seed}")
                    cmd = build_command(args.bin, cfg, map_path, out_dir, alpha, args.regime, seed,
                                        args.num_particles, cfg.get("_gain"), cfg.get("_final_odom"))
                    tasks.append({
                        "dataset": ds, "map_name": map_name, "desc": label, "alpha": alpha,
                        "regime": args.regime, "seed": seed, "out_dir": out_dir, "cmd": cmd,
                        "force": args.force, "binary_md5": bin_md5, "git": ginfo,
                    })
                    n_frames += cfg["end_frame"] - cfg["start_frame"] + 1

    print("=" * 74)
    print("  Benchmark abBRIEF × λ-BRIEF")
    print("=" * 74)
    print(f"Datasets     : {', '.join(configs)}")
    print(f"Descritores  : {', '.join(f'{l} (α={a:g})' for l, a in variants)}")
    print(f"Regime       : {args.regime}")
    print(f"Seeds ({len(seeds_to_run)})   : {seeds_to_run[0]}..{seeds_to_run[-1]}")
    print(f"Partículas   : {args.num_particles}")
    print(f"Workers      : {args.workers} (OMP_NUM_THREADS={args.omp_threads})")
    print(f"Binário      : {args.bin} (md5 {bin_md5}, git {ginfo['commit']}{' DIRTY' if ginfo['dirty'] else ''})")
    print(f"Saída        : {args.output_base_dir}")
    print(f"Tarefas      : {len(tasks)} ({n_frames} frames; ~{n_frames * 0.2 / 3600:.1f} h a 0,2 s/frame efetivo)")
    if ginfo["dirty"]:
        print("AVISO: a árvore do git tem mudanças não commitadas; o resultado não fica rastreável.")
    print("=" * 74)

    t_start = time.time()
    summary = {}
    if args.workers > 1 and not args.dry_run:
        with ThreadPoolExecutor(max_workers=args.workers) as ex:
            futs = [ex.submit(run_single_simulation, t, args.omp_threads, False,
                              args.adopt_existing, verify) for t in tasks]
            for fut in as_completed(futs):
                st = fut.result()["status"]
                summary[st] = summary.get(st, 0) + 1
    else:
        for t in tasks:
            st = run_single_simulation(t, args.omp_threads, args.dry_run, args.adopt_existing, verify)["status"]
            summary[st] = summary.get(st, 0) + 1

    print("\n" + "=" * 74)
    print(f"  Fim — {time.time() - t_start:.1f}s")
    for k, v in sorted(summary.items()):
        print(f"  {k:18s}: {v}")
    print("=" * 74)
    if summary.get("FAILED") or summary.get("STALE_CONFIG") or summary.get("MISMATCH"):
        sys.exit(2)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Batch Runner for lambda-BRIEF Modern across all ADM 0036 Maps
Runs simulation, generates evaluation plots, and renders videos for each map.
"""

import os
import sys
import glob
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
import numpy as np

WORKSPACE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN_PATH = os.path.join(WORKSPACE_DIR, "build", "lambda_brief_loc")
DATASET_DIR = "../Datasets/ADM/0036"
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import experimentos as exp  # noqa: E402
OUTPUT_BASE_DIR = None  # definido em main(): experimentos/<hoje>_todos_mapas_adm2

TARGET_MAPS = [
    # 5 Mapas do Paper:
    "ADM_35_12_09_2018.jpg",
    "ADM_35_17_08_2018.jpg",
    "ADM_35_17_12_2018.jpg",
    "ADM_35_05_09_2018.jpg",
    "ADM_35_27_02_2017.jpg",
    # Demais mapas históricos da mesma área:
    "ADM_35_13_06_2017.jpg",
    "ADM_35_31_07_2016.jpg",
    "ADM_35_23_07_2016.jpg",
    "ADM_35_18_11_2013.jpg",
    "ADM_35_19_09_2010.jpg",
    "ADM_35_28_02_2006.jpg",
]

def run_single_map(map_filename, worker_id=0):
    map_name = os.path.splitext(map_filename)[0]
    out_dir = os.path.join(OUTPUT_BASE_DIR, map_name)
    os.makedirs(out_dir, exist_ok=True)
    map_path = f"../Datasets/ADM/0036/Mapas/{map_filename}"
    log_path = os.path.join(out_dir, "run.log")

    errors_file = os.path.join(out_dir, "errors.txt")
    video_file = os.path.join(out_dir, "video.mp4")
    plot_file = os.path.join(out_dir, "evaluation.png")

    print(f"\n[Worker {worker_id}] >>> Iniciando mapa: {map_name}", flush=True)
    t0 = time.time()

    # Step 1: Run lambda_brief_loc if errors.txt does not exist
    if not (os.path.exists(errors_file) and os.path.getsize(errors_file) > 100):
        cmd_sim = [
            BIN_PATH,
            "--map", map_path,
            "--dataset_dir", DATASET_DIR,
            "--output_dir", out_dir,
            "--start_frame", "84",
            "--end_frame", "279",
            "--num_particles", "50000",
            "--pairs", "256",
            "--alpha", "0.5",
            "--vi", "NDVI",
            "--quantize", "25",
            "--ms_filter", "bilateral",
            "--bf_d", "6",
            "--bf_sc", "5",
            "--bf_ss", "5",
            "--pixel_to_meter", "0.188679245",
            "--scale_prior_sigma", "0.10",
            "--scale_prior_gain", "0.87",
            "--yaw_prior_sigma", "15",
            "--inject_ratio", "0.05",
            "--inject_spread", "300",
            "--vo_scale_deadband", "1",
            "--two_phase",
            "--seed", "1",
            "--save_particles",
            "--export_particles", "1000"
        ]
        
        env = os.environ.copy()
        env["OMP_NUM_THREADS"] = "4"
        
        with open(log_path, "w") as log_f:
            res = subprocess.run(cmd_sim, cwd=WORKSPACE_DIR, stdout=log_f, stderr=subprocess.STDOUT, env=env)
        
        if res.returncode != 0:
            print(f"[Worker {worker_id}] ERRO na simulação de {map_name} (código {res.returncode}). Veja {log_path}", flush=True)
            return {"map": map_name, "status": "FAIL_SIM", "time": time.time() - t0}
        print(f"[Worker {worker_id}] Simulação de {map_name} concluída em {time.time() - t0:.1f}s.", flush=True)
    else:
        print(f"[Worker {worker_id}] Resultados de {map_name} já existem; pulando simulação.", flush=True)

    # Step 2: Generate Evaluation Plot
    if not os.path.exists(plot_file):
        cmd_eval = [
            sys.executable,
            os.path.join(WORKSPACE_DIR, "scripts", "evaluate_results.py"),
            "--results_dir", out_dir,
            "--scale", "0.188679245",
            "--plot",
            "--output_plot", plot_file
        ]
        subprocess.run(cmd_eval, cwd=WORKSPACE_DIR, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"[Worker {worker_id}] Gráfico salvo: {plot_file}", flush=True)

    # Step 3: Render Video
    if not os.path.exists(video_file):
        cmd_vid = [
            sys.executable,
            os.path.join(WORKSPACE_DIR, "scripts", "render_video.py"),
            "--results_dir", out_dir,
            "--map", map_path,
            "--output", video_file,
            "--fps", "10",
            "--zoom_margin", "350"
        ]
        subprocess.run(cmd_vid, cwd=WORKSPACE_DIR, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"[Worker {worker_id}] Vídeo salvo: {video_file}", flush=True)

    elapsed = time.time() - t0
    print(f"[Worker {worker_id}] <<< Finalizado mapa: {map_name} (Total: {elapsed:.1f}s)", flush=True)

    # Read metrics
    metrics = {"map": map_name, "status": "OK", "time": elapsed}
    if os.path.exists(errors_file):
        try:
            arr = np.loadtxt(errors_file, skiprows=1)
            pos_err = arr[:, 7]
            yaw_err = arr[:, 8]
            metrics["mean_pos"] = float(np.mean(pos_err))
            metrics["median_pos"] = float(np.median(pos_err))
            metrics["rmse_pos"] = float(np.sqrt(np.mean(pos_err**2)))
            metrics["final_pos"] = float(pos_err[-1])
            metrics["mean_yaw"] = float(np.mean(yaw_err))
        except Exception as e:
            metrics["error_msg"] = str(e)

    return metrics

def main():
    global OUTPUT_BASE_DIR
    OUTPUT_BASE_DIR = exp.pasta_experimento("todos_mapas_adm2", "ADM2 (0036) em todos os mapas, batch_run_all_maps.py")
    os.makedirs(OUTPUT_BASE_DIR, exist_ok=True)
    print("=" * 70, flush=True)
    print(f"Executando bateria em {len(TARGET_MAPS)} mapas do ADM 0036 (2 workers paralelos)", flush=True)
    print(f"Diretório de saída: {OUTPUT_BASE_DIR}", flush=True)
    print("=" * 70, flush=True)

    start_all = time.time()
    results = []

    with ThreadPoolExecutor(max_workers=2) as executor:
        futures = {executor.submit(run_single_map, m, idx % 2 + 1): m for idx, m in enumerate(TARGET_MAPS)}
        for future in as_completed(futures):
            m = futures[future]
            try:
                data = future.result()
                results.append(data)
            except Exception as exc:
                print(f"Erro no processamento de {m}: {exc}", flush=True)

    map_order = {os.path.splitext(m)[0]: i for i, m in enumerate(TARGET_MAPS)}
    results.sort(key=lambda r: map_order.get(r["map"], 999))

    total_time = time.time() - start_all
    report_lines = [
        "# Resumo da Bateria em Todos os Mapas (ADM 0036)\n",
        f"**Data da Execução**: {time.strftime('%d/%m/%Y %H:%M:%S')}",
        f"**Tempo Total**: {total_time/60:.1f} min\n",
        "| Mapa | Erro Médio (m) | Mediana (m) | Erro Final (m) | RMSE (m) | Yaw Médio (°) | Status |",
        "| :--- | :---: | :---: | :---: | :---: | :---: | :---: |"
    ]

    print("\n" + "=" * 80, flush=True)
    print("                           RESUMO GERAL DOS MAPAS                             ", flush=True)
    print("=" * 80, flush=True)
    print(f"{'Mapa':<24} | {'Média (m)':<9} | {'Mediana (m)':<11} | {'Final (m)':<9} | {'RMSE (m)':<8} | {'Yaw (°)':<7}", flush=True)
    print("-" * 80, flush=True)

    for r in results:
        if "mean_pos" in r:
            line_str = f"{r['map']:<24} | {r['mean_pos']:>9.2f} | {r['median_pos']:>11.2f} | {r['final_pos']:>9.2f} | {r['rmse_pos']:>8.2f} | {r['mean_yaw']:>7.2f}"
            table_row = f"| `{r['map']}` | {r['mean_pos']:.2f} m | {r['median_pos']:.2f} m | {r['final_pos']:.2f} m | {r['rmse_pos']:.2f} m | {r['mean_yaw']:.2f}° | Concluído |"
        else:
            line_str = f"{r['map']:<24} | {'--':>9} | {'--':>11} | {'--':>9} | {'--':>8} | {'--':>7}"
            table_row = f"| `{r['map']}` | -- | -- | -- | -- | -- | Falha |"
        print(line_str, flush=True)
        report_lines.append(table_row)

    report_lines.append(f"\n*Todos os vídeos (`video.mp4`) e gráficos (`evaluation.png`) foram gerados em `{OUTPUT_BASE_DIR}/<mapa>/`.*")

    summary_file = os.path.join(OUTPUT_BASE_DIR, "resumo_todos_mapas.md")
    with open(summary_file, "w") as f:
        f.write("\n".join(report_lines) + "\n")

    print("=" * 80, flush=True)
    print(f"Relatório consolidado salvo em: {summary_file}", flush=True)

if __name__ == "__main__":
    main()

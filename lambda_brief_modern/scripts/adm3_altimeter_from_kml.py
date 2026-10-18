#!/usr/bin/env python3
"""
adm3_altimeter_from_kml.py — altímetro do ADM3 (Datasets/ADM/0040) a partir do log de voo do Zenodo.

O ADM3 não tem o log DJI em formato SQL no repositório (o scripts/extract_altimeter.py não serve para ele).
O Zenodo do artigo (record 4070013, GT_DAT.zip) traz:
  - GT_DAT/ADM3.kml               trilha do log (Airdata, ~2 Hz): lon, lat, altitude absoluta; SEM horário
  - GT_DAT/ADM3_ground_truth.txt  GT por imagem (0200–0465), em pixels dos mapas ADM_35

Como o KML não tem horário, cada imagem é casada com um ponto da trilha pela POSIÇÃO:
  1. lon/lat do KML → UTM 22S → pixel do mapa ADM_35 (afim de sentinel2/map_georef.json)
  2. casamento monotônico (programação dinâmica): a imagem i usa o ponto k_i, com k_i não decrescente,
     minimizando a soma das distâncias GT_i ↔ KML_k (a trilha passa duas vezes perto de alguns pontos;
     a monotonicidade evita trocar a ida pela volta)
  3. h = altitude do ponto − altitude do 1º ponto (decolagem); z = k·h, k = 0,005379/m
     (a mesma conversão física dos voos ADM1/ADM2: f_px = 1029 px, GSD ADM_35 = 0,181 m/px)

Saída: Datasets/ADM/0040/altimeter_baro.txt, lido pela engine com --altimeter_file
  linhas "traj_idx z h_rel_m kml_idx"; traj_idx = 55 + i (offset do GT no traj.txt do 0040)

Uso (de dentro de lambda_brief_modern/):
  python3 scripts/adm3_altimeter_from_kml.py            # gera o arquivo
  python3 scripts/adm3_altimeter_from_kml.py --check    # só mostra o resumo, não grava
"""

import argparse
import json
import math
import os
import re
import sys

import numpy as np

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
WS = os.path.dirname(MODERN_DIR)
K_PER_M = 0.005379        # z = K·h (ADM, mapas ADM_35)
TRAJ_OFFSET = 55          # linha do traj.txt do 0040 que corresponde à imagem 0200 (1ª do GT)


def lonlat_to_utm(lon, lat, zone=22, south=True):
    """Transversa de Mercator (WGS84), sem dependências externas. Erro < 1 mm na zona."""
    a, f = 6378137.0, 1 / 298.257223563
    e2 = f * (2 - f)
    ep2 = e2 / (1 - e2)
    k0 = 0.9996
    lon0 = math.radians((zone - 1) * 6 - 180 + 3)
    lat, lon = np.radians(lat), np.radians(lon)
    N = a / np.sqrt(1 - e2 * np.sin(lat) ** 2)
    T = np.tan(lat) ** 2
    C = ep2 * np.cos(lat) ** 2
    A = np.cos(lat) * (lon - lon0)
    M = a * ((1 - e2 / 4 - 3 * e2 ** 2 / 64 - 5 * e2 ** 3 / 256) * lat
             - (3 * e2 / 8 + 3 * e2 ** 2 / 32 + 45 * e2 ** 3 / 1024) * np.sin(2 * lat)
             + (15 * e2 ** 2 / 256 + 45 * e2 ** 3 / 1024) * np.sin(4 * lat)
             - (35 * e2 ** 3 / 3072) * np.sin(6 * lat))
    E = k0 * N * (A + (1 - T + C) * A ** 3 / 6 + (5 - 18 * T + T ** 2 + 72 * C - 58 * ep2) * A ** 5 / 120) + 500000.0
    Nn = k0 * (M + N * np.tan(lat) * (A ** 2 / 2 + (5 - T + 9 * C + 4 * C ** 2) * A ** 4 / 24
                                       + (61 - 58 * T + T ** 2 + 600 * C - 330 * ep2) * A ** 6 / 720))
    if south:
        Nn = Nn + 10000000.0
    return E, Nn


def read_kml(path):
    txt = open(path, encoding="utf-8", errors="replace").read()
    blocks = re.findall(r"<coordinates>(.*?)</coordinates>", txt, flags=re.S)
    # o KML tem um <Point> (decolagem) e a <LineString> da trilha: usa só o bloco mais longo (a trilha)
    pts = []
    for b in sorted(blocks, key=lambda b: len(b.split()), reverse=True)[:1]:
        for tok in b.split():
            p = tok.split(",")
            if len(p) >= 3:
                pts.append((float(p[0]), float(p[1]), float(p[2])))
    if not pts:
        sys.exit(f"Nenhuma coordenada em {path}")
    return np.array(pts)


def utm_to_px(E, N, A):
    """Inverte o afim px→UTM: [E,N] = A[:, :2]·[x,y] + A[:, 2]."""
    M = np.array(A)[:, :2]
    t = np.array(A)[:, 2]
    xy = np.linalg.solve(M, np.vstack([E - t[0], N - t[1]]))
    return xy[0], xy[1]


def monotonic_match(D):
    """D[i,k] = custo de casar a imagem i com o ponto k. Retorna k_i não decrescente de custo mínimo."""
    n, m = D.shape
    cost = np.empty((n, m))
    arg = np.zeros((n, m), dtype=int)
    cost[0] = D[0]
    for i in range(1, n):
        best = np.minimum.accumulate(cost[i - 1])                     # melhor custo até k (k' ≤ k)
        bidx = np.zeros(m, dtype=int)
        cur = 0
        for k in range(m):                                           # argmin acumulado
            if cost[i - 1, k] <= cost[i - 1, cur]:
                cur = k
            bidx[k] = cur
        cost[i] = D[i] + best
        arg[i] = bidx
    ks = np.zeros(n, dtype=int)
    ks[-1] = int(np.argmin(cost[-1]))
    for i in range(n - 1, 0, -1):
        ks[i - 1] = arg[i, ks[i]]
    return ks


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--zenodo", default=os.path.join(WS, "Datasets", "ADM", "0040_zenodo"))
    ap.add_argument("--georef", default=os.path.join(WS, "sentinel2", "map_georef.json"))
    ap.add_argument("--out", default=os.path.join(WS, "Datasets", "ADM", "0040", "altimeter_baro.txt"))
    ap.add_argument("--check", action="store_true", help="só mostra o resumo, sem gravar")
    args = ap.parse_args()

    kml = read_kml(os.path.join(args.zenodo, "GT_DAT", "ADM3.kml"))
    gt = np.array([[float(v) for v in l.split()[:3]]
                   for l in open(os.path.join(args.zenodo, "GT_DAT", "ADM3_ground_truth.txt")) if l.strip()])
    A = json.load(open(args.georef))["maps"]["ADM_35"]["px_to_utm"]

    E, N = lonlat_to_utm(kml[:, 0], kml[:, 1])
    kx, ky = utm_to_px(E, N, A)
    D = np.hypot(gt[:, 0:1] - kx[None, :], gt[:, 1:2] - ky[None, :])
    ks = monotonic_match(D)
    dist = D[np.arange(len(ks)), ks]
    h = kml[ks, 2] - kml[0, 2]
    z = K_PER_M * h
    corr = np.corrcoef(z, gt[:, 2])[0, 1]
    ratio = np.median(gt[:, 2] / np.where(z > 0, z, np.nan))

    print(f"KML: {len(kml)} pontos · GT: {len(gt)} imagens")
    print(f"distância GT ↔ ponto casado: mediana {np.median(dist):.1f} px, p95 {np.percentile(dist, 95):.1f} px, "
          f"máx {dist.max():.1f} px (1 px = 0,181 m)")
    print(f"altura relativa: {h.min():.1f}–{h.max():.1f} m · z = {z.min():.3f}–{z.max():.3f}")
    print(f"coerência com o z do GT do Zenodo: correlação {corr:.3f}, z_GT/z = {ratio:.3f} (mediana)")
    if args.check:
        return

    with open(args.out, "w") as f:
        f.write("# ADM3 (Datasets/ADM/0040): altura relativa à decolagem do log de voo (Zenodo 4070013, "
                "GT_DAT/ADM3.kml, Airdata, 2 Hz),\n")
        f.write(f"# casada com cada imagem pela posição (ADM3_ground_truth.txt × trilha do KML, casamento monotônico, "
                f"mediana {np.median(dist):.0f} px).\n")
        f.write(f"# z = k * h, k = {K_PER_M}/m (mesma conversão física dos voos ADM1/ADM2: f_px = 1029 px, "
                "GSD ADM_35 = 0.181 m/px).\n")
        f.write("# O KML não tem horário; por isso a sincronização usa a posição GPS (só offline, como no ADM1/ADM2).\n")
        f.write("# Gerado por scripts/adm3_altimeter_from_kml.py\n")
        f.write("# traj_idx z h_rel_m kml_idx\n")
        for i, (zz, hh, kk) in enumerate(zip(z, h, ks)):
            f.write(f"{TRAJ_OFFSET + i} {zz:.6f} {hh:.2f} {kk}\n")
    print(f"gravado: {args.out}")


if __name__ == "__main__":
    main()

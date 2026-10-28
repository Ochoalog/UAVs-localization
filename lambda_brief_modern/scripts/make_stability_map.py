#!/usr/bin/env python3
"""
make_stability_map.py — raster de peso por pixel do mapa para a v3 (--stability_map da engine)

A v3 pondera cada par de pixels do descritor pelo quanto a região do mapa onde ele cai costuma mudar,
medido no Sentinel-2 (S2). Este script gera esse peso como uma imagem 8-bit do MESMO tamanho do mapa
(255 = peso 1), que a engine lê com --stability_map. Ver docs/MANUAL_TESTES_ARTIGO.md, seção 10.

Duas variantes:

  A  estabilidade  desvio-padrão do NDVI do S2 em cada célula de 10 m ao longo do tempo (padrão: todas as
                   cenas aproveitáveis, 2018–2026). Peso 1 até --lo de desvio, caindo linearmente até --wmin
                   a partir de --hi. Não depende da data do mapa: um raster serve para os 5 mapas ADM_35.
  B  mudança       |ΔNDVI| entre o S2 perto da data do mapa (lida do nome do arquivo, dd_mm_aaaa) e o S2 perto
                   da data do voo (--data_voo). Peso 1 até --dlo, caindo até --wmin a partir de --dhi.
                   Só existe para mapas com cenas S2 a até --tol dias (no repositório: a partir de 08/2018).

Entradas (relativas à raiz do repositório):
  Datasets/Sentinel2/ADM/index.csv, scenes/*.npz   cenas S2 (refl uint16 ×1e4, scl), grade de 10 m em EPSG:32722
  sentinel2/map_georef.json                        afim pixel do mapa → UTM (chaves ADM_35, ADM_V2, ADM_V3)

Uso (de dentro de lambda_brief_modern/)
  python3 scripts/make_stability_map.py --modo A
  python3 scripts/make_stability_map.py --modo A --janela 2018-08-01:2019-05-31 --nome ADM_35_A_2018-2019
  python3 scripts/make_stability_map.py --modo B --mapa ADM_35_17_12_2018.jpg --data_voo 2019-04-18

Saída: insumos/estabilidade/<nome>.png (+ <nome>.json com os parâmetros e estatísticas, <nome>_preview.png)
  Nomes padrão: ADM_35_A.png ; ADM_35_B_<aaaa-mm-dd do mapa>_<aaaa-mm-dd do voo>.png
"""

import argparse
import csv
import datetime as dt
import json
import os
import re
import sys

import numpy as np
import cv2

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
WS = os.path.dirname(MODERN_DIR)
S2_DIR = os.path.join(WS, "Datasets", "Sentinel2", "ADM")
GEOREF = os.path.join(WS, "sentinel2", "map_georef.json")
OUT_DIR = os.path.join(MODERN_DIR, "insumos", "estabilidade")
BAD_SCL = [0, 1, 3, 8, 9, 10]   # sem dado, saturado, sombra de nuvem, nuvem média/alta, cirrus


# --------------------------------------------------------------------------- S2

def s2_grid():
    g = json.load(open(os.path.join(S2_DIR, "grid.json")))
    t = g["transform"]
    return t[2], t[5], t[0], g["width"], g["height"]   # X0, Y0, R, W, H


def load_ndvi_by_date(tiles=None, janela=None):
    """{data: NDVI (H×W, nan onde inválido)}, juntando os tiles da mesma data (média dos válidos)."""
    rows = list(csv.DictReader(open(os.path.join(S2_DIR, "index.csv"))))
    by_date = {}
    for r in rows:
        if r["kept"] != "True":
            continue
        if tiles and not any(f"T22{t}" in r["id"] for t in tiles):
            continue
        d = r["date"]
        if janela and not (janela[0] <= d <= janela[1]):
            continue
        f = os.path.join(S2_DIR, "scenes", f"{d}_{r['id']}.npz")
        if not os.path.exists(f):
            print(f"[aviso] cena listada mas ausente: {os.path.basename(f)}")
            continue
        z = np.load(f)
        b = dict(zip([str(x) for x in z["bands"]], z["refl"].astype(np.float32) / 1e4))
        nir, red = b["B08"], b["B04"]
        bad = np.isin(z["scl"], BAD_SCL) | (nir + red <= 0)
        nd = (nir - red) / np.maximum(nir + red, 1e-6)
        nd[bad] = np.nan
        by_date.setdefault(d, []).append(nd)
    out = {}
    for d, L in sorted(by_date.items()):
        with np.errstate(all="ignore"):
            out[d] = np.nanmean(np.stack(L), axis=0) if len(L) > 1 else L[0]
    return out


def ramp(v, lo, hi, wmin):
    """1 até lo, 1 − (v − lo)/(hi − lo) entre lo e hi, limitado por baixo a wmin (= protótipo de 27/10);
    nan → 1 (sem dado = sem penalidade)."""
    w = np.clip(1.0 - (v - lo) / (hi - lo), wmin, 1.0)
    w[~np.isfinite(v)] = 1.0
    return w


def composite(ndvi, center, tol):
    """Mediana dos NDVI das datas a até tol dias de center; (raster, datas usadas)."""
    c = dt.date.fromisoformat(center)
    ds = [d for d in ndvi if abs((dt.date.fromisoformat(d) - c).days) <= tol]
    if not ds:
        return None, []
    with np.errstate(all="ignore"):
        return np.nanmedian(np.stack([ndvi[d] for d in ds]), axis=0), ds


# --------------------------------------------------------------------------- mapa

def map_date(name):
    m = re.search(r"(\d{2})_(\d{2})_(\d{4})", os.path.basename(name))
    if not m:
        sys.exit(f"Não achei a data dd_mm_aaaa no nome do mapa: {name}")
    return f"{m.group(3)}-{m.group(2)}-{m.group(1)}"


def to_map(cell_w, georef_key, size, block=400):
    """Raster S2 (H×W, peso 0..1) → imagem uint8 do tamanho do mapa, célula mais próxima (floor), 255 fora da grade."""
    X0, Y0, R, SW, SH = s2_grid()
    ce, cn = json.load(open(GEOREF))["maps"][georef_key]["px_to_utm"]
    Wm, Hm = size
    out = np.empty((Hm, Wm), np.uint8)
    xs = np.arange(Wm, dtype=np.float64)
    w8 = np.round(np.clip(cell_w, 0, 1) * 255).astype(np.uint8)
    for y0 in range(0, Hm, block):
        ys = np.arange(y0, min(Hm, y0 + block), dtype=np.float64)[:, None]
        E = ce[0] * xs[None, :] + ce[1] * ys + ce[2]
        N = cn[0] * xs[None, :] + cn[1] * ys + cn[2]
        c = np.floor((E - X0) / R).astype(np.int64)
        r = np.floor((Y0 - N) / R).astype(np.int64)
        inside = (c >= 0) & (c < SW) & (r >= 0) & (r < SH)
        blk = np.full(c.shape, 255, np.uint8)
        blk[inside] = w8[r[inside], c[inside]]
        out[y0:y0 + blk.shape[0]] = blk
    return out


def preview(wmap, map_path, path):
    small = cv2.resize(wmap, (800, 800), interpolation=cv2.INTER_NEAREST)
    col = cv2.applyColorMap(255 - small, cv2.COLORMAP_INFERNO)   # escuro = peso alto (estável)
    if map_path and os.path.exists(map_path):
        m = cv2.resize(cv2.imread(map_path), (800, 800), interpolation=cv2.INTER_AREA)
        col = np.hstack([m, cv2.addWeighted(m, 0.45, col, 0.55, 0)])
    cv2.imwrite(path, col)


def find_map(name):
    if os.path.exists(name):
        return name
    for d in ("Datasets/ADM/0039/Mapas", "Datasets/ADM/0036/Mapas", "Datasets/ADM/0040_zenodo/Maps"):
        p = os.path.join(WS, d, os.path.basename(name))
        if os.path.exists(p):
            return p
    return None


# --------------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--modo", required=True, choices=["A", "B"])
    ap.add_argument("--georef", default="ADM_35", help="chave em sentinel2/map_georef.json (padrão ADM_35)")
    ap.add_argument("--mapa", default="ADM_35_17_12_2018.jpg",
                    help="mapa de referência: dá o tamanho do raster, a data (modo B) e o fundo do preview")
    ap.add_argument("--tiles", nargs="*", default=None, help="ex.: JDN (padrão: todos os tiles)")
    ap.add_argument("--janela", default=None, help="modo A: aaaa-mm-dd:aaaa-mm-dd (padrão: todas as datas)")
    ap.add_argument("--min_obs", type=int, default=8, help="modo A: mínimo de datas válidas na célula (senão peso 1)")
    ap.add_argument("--lo", type=float, default=0.08, help="modo A: desvio até o qual o peso é 1")
    ap.add_argument("--hi", type=float, default=0.22, help="modo A: desvio a partir do qual o peso é --wmin")
    ap.add_argument("--data_voo", default="2019-04-18", help="modo B: data do voo (aaaa-mm-dd)")
    ap.add_argument("--tol", type=int, default=20, help="modo B: janela ± dias dos compostos")
    ap.add_argument("--dlo", type=float, default=0.10, help="modo B: |ΔNDVI| até o qual o peso é 1")
    ap.add_argument("--dhi", type=float, default=0.35, help="modo B: |ΔNDVI| a partir do qual o peso é --wmin")
    ap.add_argument("--wmin", type=float, default=0.05, help="peso mínimo (padrão 0,05)")
    ap.add_argument("--nome", default=None, help="nome do arquivo de saída, sem extensão")
    ap.add_argument("--out_dir", default=OUT_DIR)
    ap.add_argument("--salvar_celulas", action="store_true", help="salva também o raster S2 (.npy) de peso e da estatística")
    args = ap.parse_args()

    map_path = find_map(args.mapa)
    if map_path is None:
        sys.exit(f"Mapa não encontrado: {args.mapa}")
    h, w = cv2.imread(map_path, cv2.IMREAD_GRAYSCALE).shape
    janela = tuple(args.janela.split(":")) if args.janela else None
    ndvi = load_ndvi_by_date(args.tiles, janela if args.modo == "A" else None)
    if not ndvi:
        sys.exit("Nenhuma cena S2 aproveitável com esses filtros.")
    meta = {"modo": args.modo, "georef": args.georef, "mapa_referencia": os.path.relpath(map_path, WS),
            "tamanho": [w, h], "wmin": args.wmin, "tiles": args.tiles or "todos",
            "gerado_em": dt.datetime.now().isoformat(timespec="seconds"),
            "comando": " ".join(["python3", "scripts/make_stability_map.py"] + sys.argv[1:])}

    if args.modo == "A":
        stack = np.stack(list(ndvi.values()))
        n_obs = np.isfinite(stack).sum(0)
        with np.errstate(all="ignore"):
            stat = np.nanstd(stack, axis=0)
        stat[n_obs < args.min_obs] = np.nan
        cell_w = ramp(stat, args.lo, args.hi, args.wmin)
        nome = args.nome or "ADM_35_A"
        meta.update({"datas": list(ndvi), "n_datas": len(ndvi), "janela": args.janela or "todas",
                     "lo": args.lo, "hi": args.hi, "min_obs": args.min_obs,
                     "celulas_sem_dado": int(np.isnan(stat).sum())})
        print(f"Modo A: {len(ndvi)} datas ({min(ndvi)} a {max(ndvi)}), desvio do NDVI: mediana "
              f"{np.nanmedian(stat):.3f}, células com peso < 0,5: {np.mean(cell_w < 0.5):.0%}")
    else:
        dm = map_date(map_path)
        before, ds_m = composite(ndvi, dm, args.tol)
        after, ds_v = composite(ndvi, args.data_voo, args.tol)
        if before is None:
            sys.exit(f"Sem cena S2 a até {args.tol} dias da data do mapa ({dm}); a cobertura no repositório começa "
                     f"em {min(ndvi)}. A variante B não existe para este mapa.")
        if after is None:
            sys.exit(f"Sem cena S2 a até {args.tol} dias da data do voo ({args.data_voo}).")
        stat = np.abs(after - before)
        cell_w = ramp(stat, args.dlo, args.dhi, args.wmin)
        nome = args.nome or f"ADM_35_B_{dm}_{args.data_voo}"
        meta.update({"data_mapa": dm, "data_voo": args.data_voo, "tol_dias": args.tol, "datas_mapa": ds_m,
                     "datas_voo": ds_v, "dlo": args.dlo, "dhi": args.dhi,
                     "celulas_sem_dado": int(np.isnan(stat).sum())})
        print(f"Modo B: mapa {dm} ({len(ds_m)} cenas) × voo {args.data_voo} ({len(ds_v)} cenas); |ΔNDVI| mediana "
              f"{np.nanmedian(stat):.3f}, células com peso < 0,5: {np.mean(cell_w < 0.5):.0%}")

    wmap = to_map(cell_w, args.georef, (w, h))
    os.makedirs(args.out_dir, exist_ok=True)
    png = os.path.join(args.out_dir, nome + ".png")
    cv2.imwrite(png, wmap)
    meta.update({"peso_medio_mapa": round(float(wmap.mean()) / 255, 4),
                 "frac_mapa_peso_menor_0_5": round(float((wmap < 128).mean()), 4)})
    json.dump(meta, open(os.path.join(args.out_dir, nome + ".json"), "w"), indent=1, ensure_ascii=False)
    preview(wmap, map_path, os.path.join(args.out_dir, nome + "_preview.png"))
    if args.salvar_celulas:
        np.save(os.path.join(args.out_dir, nome + "_celulas_peso.npy"), cell_w.astype(np.float32))
        np.save(os.path.join(args.out_dir, nome + "_celulas_estat.npy"), stat.astype(np.float32))
    print(f"Peso médio no mapa {meta['peso_medio_mapa']:.2f}; {meta['frac_mapa_peso_menor_0_5']:.0%} do mapa com peso "
          f"< 0,5.\nSalvo: {os.path.relpath(png, MODERN_DIR)} (+ .json, _preview.png)")


if __name__ == "__main__":
    main()

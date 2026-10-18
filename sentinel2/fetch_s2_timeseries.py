#!/usr/bin/env python3
"""
fetch_s2_timeseries.py — baixa recortes Sentinel-2 L2A (reflectância de superfície)
sobre as áreas dos voos ADM (Arroio do Meio) e UFRGS, numa grade fixa de 10 m,
para analisar mudanças entre as datas dos mapas, a data dos voos e as enchentes
de 2023/2024 no Vale do Taquari.

Fonte: catálogo STAC público da Element84 (Earth Search), sem login.
  - coleção preferida: sentinel-2-c1-l2a (Collection 1, reprocessada, consistente)
  - fallback:          sentinel-2-l2a

Instalação (uma vez):
    pip install pystac-client rasterio numpy matplotlib

Uso (a partir da raiz do repositório "UAVs Localization"):
    python sentinel2/fetch_s2_timeseries.py                 # modo "key" (rápido, ~50-150 cenas)
    python sentinel2/fetch_s2_timeseries.py --mode full     # série completa 2015-hoje (vários GB de tráfego)
    python sentinel2/fetch_s2_timeseries.py --aoi ADM       # só Arroio do Meio
    python sentinel2/fetch_s2_timeseries.py --aoi ADM --collection sentinel-2-l2a   # completa 2016-2017

Saída (padrão: Datasets/Sentinel2/):
    <AOI>/scenes/<data>_<id>.npz   refl (uint16, reflectância*10000) + scl + metadados da grade
    <AOI>/quicklooks/<data>.png    RGB realçado
    <AOI>/index.csv                uma linha por cena: data, nuvem na AOI, NDVI/NDWI médios ...
    <AOI>/grid.json                CRS, transform e dimensões da grade fixa
    <AOI>/key_dates_panel.png      painel RGB/NDVI nas datas-chave (mapas, voo, enchentes)

Nada aqui altera os datasets existentes; só cria Datasets/Sentinel2/.
"""
from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import os
import re
import sys
import traceback
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import numpy as np

try:
    import rasterio
    from rasterio.enums import Resampling
    from rasterio.transform import from_origin
    from rasterio.vrt import WarpedVRT
    from rasterio.warp import transform_bounds
    from pystac_client import Client
except ImportError as e:  # pragma: no cover
    sys.exit(f"Dependência faltando ({e}). Rode: pip install pystac-client rasterio numpy matplotlib")

# ----------------------------------------------------------------------------
# Configuração
# ----------------------------------------------------------------------------
STAC_URL = "https://earth-search.aws.element84.com/v1"
COLLECTIONS = ["sentinel-2-c1-l2a", "sentinel-2-l2a"]
TARGET_CRS = "EPSG:32722"  # UTM 22S (Arroio do Meio e Porto Alegre)
RES = 10.0

# Bandas lidas (chaves de asset do Earth Search v1) -> nome curto salvo
BANDS = {
    "blue": "B02", "green": "B03", "red": "B04", "nir": "B08",
    "rededge1": "B05", "swir16": "B11",
}
SCL_BAD = {0, 1, 3, 8, 9, 10}  # nodata, saturado, sombra de nuvem, nuvem média/alta, cirrus

# AOIs em lon/lat (oeste, sul, leste, norte). ADM = união dos mapas ADM_35, V2 e V3
# (calculada dos arquivos pixel_to_coord) + ~500 m de margem. UFRGS = controle (não alagou).
AOIS = {
    "ADM":   (-51.9600, -29.4160, -51.9270, -29.3870),
    "UFRGS": (-51.1350, -30.0845, -51.1170, -30.0670),
}

FLIGHTS = {"ADM": "2019-04-18", "UFRGS": "2018-05-05"}

# Janelas de eventos (inclusivas). Ajuste se quiser.
EVENT_WINDOWS = {
    "pre_2023":       ("2023-06-01", "2023-08-31"),
    "post_2023_09":   ("2023-09-06", "2023-10-31"),
    "post_2023_11":   ("2023-11-20", "2024-01-31"),
    "pre_2024_05":    ("2024-02-01", "2024-04-25"),
    "post_2024_05":   ("2024-05-04", "2024-07-31"),
    "recovery":       ("2024-08-01", dt.date.today().isoformat()),
}

S2_START = dt.date(2015, 7, 1)
MAP_DIRS = {
    "ADM": ["Datasets/ADM/0036/Mapas", "Datasets/ADM/0035/Mapas"],
    "UFRGS": ["Datasets/UFRGS VET/voo_100m/Mapa"],
}

# GDAL: leitura anônima e eficiente de COGs via HTTP
os.environ.setdefault("AWS_NO_SIGN_REQUEST", "YES")
os.environ.setdefault("GDAL_DISABLE_READDIR_ON_OPEN", "EMPTY_DIR")
os.environ.setdefault("CPL_VSIL_CURL_ALLOWED_EXTENSIONS", ".tif,.TIF,.tiff")
os.environ.setdefault("GDAL_HTTP_MULTIPLEX", "YES")
os.environ.setdefault("GDAL_HTTP_MAX_RETRY", "4")
os.environ.setdefault("GDAL_HTTP_RETRY_DELAY", "2")
os.environ.setdefault("VSI_CACHE", "TRUE")


# ----------------------------------------------------------------------------
# Utilidades
# ----------------------------------------------------------------------------
def parse_map_dates(repo: Path, aoi: str) -> list[dt.date]:
    """Extrai datas DD_MM_YYYY dos nomes dos mapas de satélite do repositório."""
    dates = set()
    for d in MAP_DIRS.get(aoi, []):
        p = repo / d
        if not p.is_dir():
            continue
        for f in p.iterdir():
            m = re.search(r"(\d{2})_(\d{2})_(\d{4})", f.name)
            if m:
                try:
                    dates.add(dt.date(int(m[3]), int(m[2]), int(m[1])))
                except ValueError:
                    pass
    return sorted(dates)


def build_grid(bbox_ll: tuple[float, float, float, float]) -> dict:
    """Grade fixa em UTM 22S, alinhada a múltiplos de 10 m."""
    w, s, e, n = transform_bounds("EPSG:4326", TARGET_CRS, *bbox_ll, densify_pts=21)
    w, s = np.floor(w / RES) * RES, np.floor(s / RES) * RES
    e, n = np.ceil(e / RES) * RES, np.ceil(n / RES) * RES
    width, height = int(round((e - w) / RES)), int(round((n - s) / RES))
    transform = from_origin(w, n, RES, RES)
    return {"crs": TARGET_CRS, "transform": list(transform)[:6], "width": width, "height": height,
            "bounds_utm": [w, s, e, n], "bbox_lonlat": list(bbox_ll)}


def key_windows(aoi: str, map_dates: list[dt.date], half_days: int) -> list[tuple[str, dt.date, dt.date]]:
    wins = []
    for d in map_dates:
        if d >= S2_START:
            wins.append((f"map_{d.isoformat()}", d - dt.timedelta(days=half_days), d + dt.timedelta(days=half_days)))
    f = dt.date.fromisoformat(FLIGHTS[aoi])
    wins.append((f"flight_{f.isoformat()}", f - dt.timedelta(days=half_days), f + dt.timedelta(days=half_days)))
    for name, (a, b) in EVENT_WINDOWS.items():
        wins.append((name, dt.date.fromisoformat(a), dt.date.fromisoformat(b)))
    return wins


def merge_ranges(wins):
    rs = sorted((a, b) for _, a, b in wins)
    out = []
    for a, b in rs:
        if out and a <= out[-1][1] + dt.timedelta(days=1):
            out[-1] = (out[-1][0], max(out[-1][1], b))
        else:
            out.append((a, b))
    return out


def scale_offset(item, asset) -> tuple[float, float]:
    """Converte DN -> reflectância. Usa raster:bands quando presente; senão heurística do baseline 04.00."""
    rb = asset.extra_fields.get("raster:bands") or []
    if rb and ("scale" in rb[0] or "offset" in rb[0]):
        return float(rb[0].get("scale", 1e-4)), float(rb[0].get("offset", 0.0))
    props = item.properties
    if props.get("earthsearch:boa_offset_applied") is True:
        return 1e-4, 0.0
    try:
        pb = float(props.get("s2:processing_baseline", "0"))
    except ValueError:
        pb = 0.0
    return 1e-4, (-0.1 if pb >= 4.0 else 0.0)


def read_on_grid(href: str, grid: dict, resampling) -> np.ndarray:
    tr = rasterio.Affine(*grid["transform"])
    with rasterio.open(href) as src:
        with WarpedVRT(src, crs=grid["crs"], transform=tr, width=grid["width"], height=grid["height"],
                       resampling=resampling, src_nodata=0, nodata=0) as vrt:
            return vrt.read(1)


def process_item(item, grid: dict, out_dir: Path, max_bad: float) -> dict | None:
    date = item.datetime.date().isoformat() if item.datetime else item.properties["datetime"][:10]
    scl = read_on_grid(item.assets["scl"].href, grid, Resampling.nearest)
    bad = np.isin(scl, list(SCL_BAD))
    bad_frac = float(bad.mean())
    row = {"date": date, "id": item.id, "bad_frac_aoi": round(bad_frac, 4),
           "eo_cloud_cover_tile": item.properties.get("eo:cloud_cover"),
           "processing_baseline": item.properties.get("s2:processing_baseline"), "kept": False}
    if bad_frac > max_bad:
        return row
    refl = np.zeros((len(BANDS), grid["height"], grid["width"]), np.uint16)
    for i, (key, short) in enumerate(BANDS.items()):
        a = item.assets[key]
        dn = read_on_grid(a.href, grid, Resampling.bilinear).astype(np.float32)
        sc, off = scale_offset(item, a)
        r = np.where(dn > 0, dn * sc + off, 0)
        refl[i] = np.clip(np.round(r * 10000), 0, 65535).astype(np.uint16)
    b = {s: refl[i].astype(np.float32) / 1e4 for i, s in enumerate(BANDS.values())}
    good = (~bad) & (b["B08"] > 0)
    eps = 1e-6
    ndvi = (b["B08"] - b["B04"]) / (b["B08"] + b["B04"] + eps)
    ndwi = (b["B03"] - b["B08"]) / (b["B03"] + b["B08"] + eps)        # McFeeters
    mndwi = (b["B03"] - b["B11"]) / (b["B03"] + b["B11"] + eps)       # Xu
    row.update(kept=True,
               ndvi_mean=round(float(ndvi[good].mean()), 4) if good.any() else None,
               ndwi_mean=round(float(ndwi[good].mean()), 4) if good.any() else None,
               water_frac_mndwi=round(float((mndwi[good] > 0).mean()), 4) if good.any() else None,
               brightness_mean=round(float(((b["B02"] + b["B03"] + b["B04"]) / 3)[good].mean()), 4) if good.any() else None)
    tag = f"{date}_{item.id}"
    np.savez_compressed(out_dir / "scenes" / f"{tag}.npz", refl=refl, scl=scl, bands=np.array(list(BANDS.values())),
                        date=date, item_id=item.id, grid=json.dumps(grid))
    save_quicklook(refl, out_dir / "quicklooks" / f"{date}.png")
    return row


def save_quicklook(refl: np.ndarray, path: Path):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    rgb = np.stack([refl[2], refl[1], refl[0]], -1).astype(np.float32) / 1e4
    rgb = np.clip(rgb / 0.25, 0, 1) ** (1 / 1.6)
    plt.imsave(path, rgb)


def nearest_clear(rows: list[dict], start: dt.date, end: dt.date, center: dt.date | None):
    cand = [r for r in rows if r.get("kept") and start <= dt.date.fromisoformat(r["date"]) <= end]
    if not cand:
        return None
    if center is None:
        return min(cand, key=lambda r: r["bad_frac_aoi"])
    return min(cand, key=lambda r: (abs((dt.date.fromisoformat(r["date"]) - center).days), r["bad_frac_aoi"]))


def key_panel(aoi: str, rows: list[dict], wins, out_dir: Path):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    picks = []
    for name, a, b in wins:
        center = a + (b - a) / 2 if (name.startswith("map_") or name.startswith("flight_")) else None
        r = nearest_clear(rows, a, b, center)
        if r:
            picks.append((name, r))
    if not picks:
        return
    n = len(picks)
    fig, ax = plt.subplots(2, n, figsize=(2.6 * n, 5.6), squeeze=False)
    for j, (name, r) in enumerate(picks):
        f = next((out_dir / "scenes").glob(f"{r['date']}_{r['id']}.npz"))
        d = np.load(f)
        refl = d["refl"].astype(np.float32) / 1e4
        rgb = np.clip(np.stack([refl[2], refl[1], refl[0]], -1) / 0.25, 0, 1) ** (1 / 1.6)
        ndvi = (refl[3] - refl[2]) / (refl[3] + refl[2] + 1e-6)
        ax[0, j].imshow(rgb); ax[0, j].set_title(f"{name}\n{r['date']}", fontsize=7)
        ax[1, j].imshow(ndvi, vmin=-0.3, vmax=0.9, cmap="RdYlGn"); ax[1, j].set_title("NDVI", fontsize=7)
        for k in (0, 1):
            ax[k, j].axis("off")
    fig.suptitle(f"Sentinel-2 L2A — {aoi} — datas-chave", fontsize=10)
    fig.tight_layout()
    fig.savefig(out_dir / "key_dates_panel.png", dpi=130)
    plt.close(fig)
    with open(out_dir / "key_dates.json", "w") as fh:
        json.dump({name: r for name, r in picks}, fh, indent=2, default=str)


# ----------------------------------------------------------------------------
def run_aoi(aoi: str, args, client, collection: str, repo: Path):
    out_dir = Path(args.out) / aoi
    (out_dir / "scenes").mkdir(parents=True, exist_ok=True)
    (out_dir / "quicklooks").mkdir(parents=True, exist_ok=True)
    grid = build_grid(AOIS[aoi])
    (out_dir / "grid.json").write_text(json.dumps(grid, indent=2))
    map_dates = parse_map_dates(repo, aoi)
    wins = key_windows(aoi, map_dates, args.half_window)
    if args.mode == "full":
        ranges = [(S2_START, dt.date.today())]
    else:
        ranges = merge_ranges(wins)
    print(f"[{aoi}] grade {grid['width']}x{grid['height']} px @10 m | mapas com data: {len(map_dates)} "
          f"| {len(ranges)} intervalo(s) de busca")

    items = {}
    for a, b in ranges:
        search = client.search(collections=[collection], bbox=AOIS[aoi],
                               datetime=f"{a.isoformat()}/{b.isoformat()}",
                               query={"eo:cloud_cover": {"lt": args.max_tile_cloud}})
        for it in search.items():
            items[it.id] = it
    print(f"[{aoi}] {len(items)} itens no catálogo (nuvem no tile < {args.max_tile_cloud}%)")

    done = {p.stem.split("_", 1)[1] for p in (out_dir / "scenes").glob("*.npz")}
    rows, errors = [], 0
    with ThreadPoolExecutor(max_workers=args.workers) as ex:
        futs = {ex.submit(process_item, it, grid, out_dir, args.max_bad): it
                for it in items.values() if it.id not in done}
        for k, fu in enumerate(as_completed(futs), 1):
            it = futs[fu]
            try:
                r = fu.result()
                if r:
                    rows.append(r)
                    flag = "OK " if r["kept"] else "-- "
                    print(f"  [{k}/{len(futs)}] {flag}{r['date']} nuvem_AOI={r['bad_frac_aoi']:.2f} {it.id}")
            except Exception:
                errors += 1
                print(f"  [{k}/{len(futs)}] ERRO {it.id}\n{traceback.format_exc(limit=1)}")

    # índice (acrescenta ao existente) e deduplicação por data (mantém a menos nublada)
    idx = out_dir / "index.csv"
    old = list(csv.DictReader(open(idx))) if idx.exists() else []
    for r in old:
        r["kept"] = r.get("kept") in ("True", True)
        r["bad_frac_aoi"] = float(r["bad_frac_aoi"])
    allrows = {r["id"]: r for r in old}
    allrows.update({r["id"]: r for r in rows})
    allrows = sorted(allrows.values(), key=lambda r: (r["date"], r["bad_frac_aoi"]))
    fields = ["date", "id", "kept", "bad_frac_aoi", "eo_cloud_cover_tile", "processing_baseline",
              "ndvi_mean", "ndwi_mean", "water_frac_mndwi", "brightness_mean"]
    with open(idx, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fields, extrasaction="ignore")
        w.writeheader()
        w.writerows(allrows)
    kept = [r for r in allrows if r["kept"]]
    print(f"[{aoi}] cenas aproveitáveis: {len(kept)} | erros: {errors} | índice: {idx}")
    key_panel(aoi, allrows, wins, out_dir)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", default=".", help="raiz do repositório 'UAVs Localization' (para ler datas dos mapas)")
    ap.add_argument("--out", default="Datasets/Sentinel2", help="pasta de saída")
    ap.add_argument("--aoi", choices=list(AOIS) + ["all"], default="all")
    ap.add_argument("--mode", choices=["key", "full"], default="key",
                    help="key = janelas em torno de mapas/voo/enchentes; full = 2015-hoje inteiro")
    ap.add_argument("--half-window", type=int, default=30, help="meia-janela (dias) em torno das datas dos mapas/voo")
    ap.add_argument("--max-tile-cloud", type=float, default=60.0, help="filtro de nuvem do tile inteiro (%%)")
    ap.add_argument("--max-bad", type=float, default=0.20, help="fração máxima de pixels ruins (nuvem/sombra) na AOI")
    ap.add_argument("--workers", type=int, default=6)
    ap.add_argument("--collection", default=None,
                    help="força a coleção STAC (ex.: sentinel-2-l2a, que tem cenas de 2016-2017 ausentes na c1)")
    args = ap.parse_args()

    repo = Path(args.repo).resolve()
    client = Client.open(STAC_URL)
    available = {c.id for c in client.get_collections()}
    prefs = [args.collection] if args.collection else COLLECTIONS
    collection = next((c for c in prefs if c in available), None)
    if not collection:
        sys.exit(f"Nenhuma das coleções {COLLECTIONS} encontrada em {STAC_URL}")
    print(f"Coleção: {collection}")
    for aoi in (AOIS if args.aoi == "all" else [args.aoi]):
        run_aoi(aoi, args, client, collection, repo)
    print("Pronto.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
extract_altimeter.py — Altímetro barométrico real por frame, a partir dos logs do DJI Matrice 100

Para cada voo com log disponível, gera  <dataset_dir>/altimeter_baro.txt  com uma linha por
imagem do traj.txt:

    traj_idx  z_baro  h_baro_m  h_rel_m  h_gps_m  t_img_s  img_num  lat  lon

  z_baro    escala do mapa (px do mapa por px do frame) prevista pelo barômetro:
            z = h_baro / (f_px · GSD_mapa)   — modelo pinhole, sem nenhum ajuste à trajetória
  h_baro_m  IMU_ATTI(0):barometer:Smooth menos a média em solo antes da decolagem (m)
  h_rel_m   General:relativeHeight do DJI (fusão da controladora; é a origem do z do traj_truth)
  h_gps_m   GPS(0):heightMSL menos a média em solo (só diagnóstico; não é usado no filtro)

Também gera  <dataset_dir>/traj_truth_sync.txt  ("x y z yaw", uma linha por linha do traj.txt):
o GPS do log no instante exato de cada imagem, projetado no mapa pelos pontos de controle, z do
barômetro e yaw da IMU na convenção do traj_truth. Serve para AVALIAR (consolidate_benchmark.py
--gt_file traj_truth_sync.txt), não entra no filtro.

A engine lê as duas primeiras colunas com  --altimeter_file  (o prior de escala passa a ser
centrado em scale_prior_gain · z_baro, em vez de scale_prior_gain · z do GT).

Sincronização imagem ↔ log (sem usar o GT)
  1. tempo do log  = ajuste linear de GPS:dateTimeStamp contra o nº da linha (o CSV é reamostrado
     a taxa fixa; a coluna offsetTime tem valores corrompidos pela conversão de vírgula decimal);
  2. tempo da imagem = ajuste linear do horário no nome do arquivo (IMG_aammdd_HHMMSS_nnnn)
     contra o nº da imagem (fotos a intervalo fixo; tira a quantização de 1 s);
  3. atraso Δ entre os dois relógios = o que minimiza a distância entre a posição GPS da Sequoia
     (EXIF, só imagens com fix novo) e a posição do log interpolada em t_img + Δ. Sem fixes
     novos (0039: o GPS da Sequoia ficou congelado), herda o Δ dos outros voos do mesmo dia.
  O GPS só é usado aqui, offline, para alinhar relógios — o filtro não recebe posição.

Constante da câmera: Sequoia monocromática, foco 3,98 mm, pixel 3,75 µm => 1061,3 px em 1280 px
de largura; os frames registrados (RGBres) têm W px => f_px = 1061,3 · W / 1280.
GSD do mapa: ajuste afim dos pontos de controle (pixel_to_coord_*). Confere com a escala medida
à mão em Datasets/ADM/0035/escala (0,3308 a 62 m) e com o mínimo da verossimilhança (0,87·z_GT).

Diagnósticos (insumos/altimetro/<voo>_report.md e .png)
  - Δ, erro de posição depois da sincronização, barômetro × GPS (RMS);
  - atraso do traj_truth.txt: compara a posição do log projetada no mapa com o GT para
    lags −15..+15 (o melhor lag é o que o --gt_offset deveria corrigir);
  - razão k_GT / k_pinhole (o ganho que o prior emulado precisava).

Uso
  python3 scripts/extract_altimeter.py                       # todos os voos com log
  python3 scripts/extract_altimeter.py --dataset ADM0036 UFRGS
  python3 scripts/extract_altimeter.py --dataset ADM0036 --dry_run   # só diagnóstico, não grava
"""

import argparse
import datetime as dt
import math
import os
import re
import sys

import numpy as np
import pandas as pd

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
WORKSPACE_DIR = os.path.dirname(MODERN_DIR)
DATASETS_DIR = os.path.join(WORKSPACE_DIR, "Datasets")
sys.path.insert(0, SCRIPT_DIR)
import experimentos as exp  # noqa: E402
REPORT_DIR = exp.altimetro_dir()   # insumos/altimetro (ou results_final/altimeter antes de mover)

SEQUOIA_F_PX_1280 = 3.98e-3 / 3.75e-6  # 1061,3 px

ADM_CTRL = "ADM/0035/pixel_to_coord_ADM_MATRICE100_SEQUOIA"
DATASETS = {
    "UFRGS": dict(dir="UFRGS VET/voo_100m", log="FLY044_5hz_completo.csv",
                  exif="outGpsImu_NIR_principais_tags.csv", gt_offset=204, frame_w=1234,
                  ctrl="UFRGS VET/voo_100m/pixel_to_coord_UFRGS_VET", ctrl_section="V3"),
    "ADM0035": dict(dir="ADM/0035", log="Banco/csv/20hz/FLY050_20hz.csv",
                    exif="Banco/outGpsImu_nir_principais_tags.csv", gt_offset=18, frame_w=1241,
                    ctrl=ADM_CTRL, ctrl_section=None, same_day="ADM_2019-04-18"),
    "ADM0036": dict(dir="ADM/0036", log="Banco/036_5hz/FLY052_5hz.csv",
                    exif="Banco/outGpsImu_GRE_principais_tags.csv", gt_offset=84, frame_w=1241,
                    ctrl=ADM_CTRL, ctrl_section=None, same_day="ADM_2019-04-18"),
    "ADM0039": dict(dir="ADM/0039", log="Banco/csv_banco/FLY053_5hz.csv",
                    exif="Banco/outGpsImu_GRE_principais_tags.csv", gt_offset=0, frame_w=1241,
                    ctrl=ADM_CTRL, ctrl_section=None, same_day="ADM_2019-04-18"),
    # ADM3: as imagens de 0040 são das 17:23 (relógio da câmera); o FLY053 termina antes disso e
    # o log do voo (provavelmente FLY054) não está no repositório.
    "ADM0040": None,
}
EARTH_N, EARTH_E = 110540.0, 111320.0
SYNC_BY_DAY = {}  # Δ (relógio da câmera -> log) dos voos sincronizados, por dia


# --------------------------------------------------------------------------- utilidades

def to_num(s):
    return pd.to_numeric(s, errors="coerce")


def arr(s):
    """Cópia gravável (o pandas devolve arrays somente leitura com copy-on-write)."""
    return np.array(s, dtype=float, copy=True)


def hampel(x, win=11, thresh=5.0):
    """Remove picos (valores corrompidos pela conversão de vírgula) e interpola."""
    s = pd.Series(x, dtype=float)
    med = s.rolling(win, center=True, min_periods=3).median()
    bad = (s - med).abs() > thresh
    s[bad] = np.nan
    return arr(s.interpolate(limit_direction="both")), int(bad.sum())


def robust_line(x, y, tol, iters=5):
    m = np.isfinite(x) & np.isfinite(y)
    for _ in range(iters):
        a, b = np.polyfit(x[m], y[m], 1)
        r = y - (a * x + b)
        m2 = m & (np.abs(r) < tol)
        if m2.sum() == m.sum():
            break
        m = m2
    r = y[m] - (a * x[m] + b)
    return a, b, float(np.std(r)), int(m.sum())


def enu(lat, lon, lat0, lon0):
    return ((lon - lon0) * EARTH_E * math.cos(math.radians(lat0)), (lat - lat0) * EARTH_N)


def parse_ctrl(path, section):
    txt = open(path, encoding="utf-8", errors="replace").read()
    if section:
        txt = txt.split(section, 1)[1]
    pts = []
    for line in txt.splitlines():
        m = re.match(r"\s*(\d+),\s*(\d+)\s+(\d+)[,.](\d+)[,.]([\d.]+)\s*S\s+(\d+)[,.](\d+)[,.]([\d.]+)\s*W", line)
        if m:
            x, y, a, b, c, d, e, f = m.groups()
            pts.append((int(x), int(y), -(int(a) + int(b) / 60 + float(c) / 3600),
                        -(int(d) + int(e) / 60 + float(f) / 3600)))
    return np.array(pts)


class Georef:
    """Afim pixel <-> (E, N) local a partir dos pontos de controle."""

    def __init__(self, pts):
        self.lat0, self.lon0 = pts[:, 2].mean(), pts[:, 3].mean()
        E, N = enu(pts[:, 2], pts[:, 3], self.lat0, self.lon0)
        X = np.c_[pts[:, 0], pts[:, 1], np.ones(len(pts))]
        self.ce = np.linalg.lstsq(X, E, rcond=None)[0]
        self.cn = np.linalg.lstsq(X, N, rcond=None)[0]
        J = np.array([[self.ce[0], self.ce[1]], [self.cn[0], self.cn[1]]])
        self.J_inv = np.linalg.inv(J)
        self.gsd = float(np.sqrt(abs(np.linalg.det(J))))
        res = np.hypot(X @ self.ce - E, X @ self.cn - N)
        self.resid = float(np.sqrt(np.mean(res ** 2)))

    def to_px(self, lat, lon):
        E, N = enu(np.asarray(lat), np.asarray(lon), self.lat0, self.lon0)
        v = np.vstack([E - self.ce[2], N - self.cn[2]])
        p = self.J_inv @ v
        return p[0], p[1]


# --------------------------------------------------------------------------- leitura

def load_log(path):
    cols = ["GPS:dateTimeStamp", "IMU_ATTI(0):barometer:Smooth", "General:relativeHeight",
            "GPS(0):heightMSL", "GPS(0):Lat", "GPS(0):Long", "IMU_ATTI(0):numSats", "IMU_ATTI(0):yaw"]
    d = pd.read_csv(path, usecols=lambda c: c in cols, dtype=str, low_memory=False)
    n = len(d)
    row = np.arange(n, dtype=float)
    ts = pd.to_datetime(d["GPS:dateTimeStamp"], errors="coerce", utc=True)
    epoch = arr((ts - pd.Timestamp("1970-01-01", tz="UTC")).dt.total_seconds())
    # dateTimeStamp tem resolução de 1 s (truncado): soma 0,5 s antes do ajuste
    a, b, sd, used = robust_line(row, epoch + 0.5, tol=1.5)
    t = a * row + b

    lat, lon = arr(to_num(d["GPS(0):Lat"])), arr(to_num(d["GPS(0):Long"]))
    lat0, lon0 = np.nanmedian(lat), np.nanmedian(lon)
    ok = (np.abs(lat - lat0) < 0.05) & (np.abs(lon - lon0) < 0.05)
    lat[~ok], lon[~ok] = np.nan, np.nan
    lat = arr(pd.Series(lat).interpolate(limit_direction="both"))
    lon = arr(pd.Series(lon).interpolate(limit_direction="both"))

    baro_raw = arr(to_num(d["IMU_ATTI(0):barometer:Smooth"]))
    rel_raw = arr(to_num(d["General:relativeHeight"]))
    gps_raw = arr(to_num(d["GPS(0):heightMSL"]))
    baro_raw[np.abs(baro_raw) > 2000] = np.nan
    gps_raw[np.abs(gps_raw) > 5000] = np.nan
    yaw = arr(to_num(d["IMU_ATTI(0):yaw"]))
    yaw[np.abs(yaw) > 360] = np.nan
    ys = arr(pd.Series(np.sin(np.radians(yaw))).interpolate(limit_direction="both"))
    yc = arr(pd.Series(np.cos(np.radians(yaw))).interpolate(limit_direction="both"))
    baro, n_bad_b = hampel(baro_raw)
    rel, _ = hampel(rel_raw)
    gps, _ = hampel(gps_raw)

    # solo antes da decolagem: últimos 20 s antes de a altura relativa passar de 3 m
    flying = np.where(rel > 3.0)[0]
    if len(flying) == 0:
        raise RuntimeError("o log não tem trecho em voo")
    i_to = flying[0]
    i_land = flying[-1]
    rate = 1.0 / a
    g0 = max(0, i_to - int(20 * rate))
    baro0 = np.nanmedian(baro[g0:i_to]) if i_to > g0 else baro[i_to]
    gps0 = np.nanmedian(gps[g0:i_to]) if i_to > g0 else gps[i_to]
    return {
        "t": t, "rate": rate, "time_fit_sd": sd, "time_rows_used": used,
        "lat": lat, "lon": lon, "h_baro": baro - baro0, "h_rel": rel, "h_gps": gps - gps0,
        "n_rows": n, "baro_spikes": n_bad_b, "i_takeoff": i_to, "i_land": i_land,
        "yaw_sin": ys, "yaw_cos": yc,
    }


def load_traj(path):
    rows = []
    for idx, line in enumerate(l for l in open(path) if l.strip()):
        m = re.search(r"_(\d{4})_RGB", line)
        rows.append((idx, int(m.group(1)) if m else -1))
    return rows


def load_camera(path, img_nums):
    """Relógio da câmera (nome do arquivo IMG_yymmdd_HHMMSS_nnnn) e fixes GPS novos do EXIF.

    O horário do nome do arquivo existe para todas as imagens; o ajuste linear contra o nº da
    imagem tira a quantização de 1 s. O GPSTimeStamp do EXIF falha em parte das imagens (ADM2)
    ou fica congelado o voo inteiro (0039), então só entram na sincronização as imagens cujo
    fix GPS é novo (timestamp diferente do da imagem anterior).
    """
    d = pd.read_csv(path, usecols=["SourceFile", "GPSTimeStamp", "GPSLatitude", "GPSLongitude"], dtype=str)
    parts = d["SourceFile"].str.extract(r"IMG_(\d{6})_(\d{6})_(\d{4})_[A-Z]{3}\.TIF")
    rows = {}
    for (dd, hms, n), ts, la, lo in zip(parts.itertuples(index=False), d["GPSTimeStamp"],
                                        d["GPSLatitude"], d["GPSLongitude"]):
        if not isinstance(n, str):
            continue
        base = dt.datetime.strptime(dd, "%y%m%d").replace(tzinfo=dt.timezone.utc).timestamp()
        t_file = base + int(hms[:2]) * 3600 + int(hms[2:4]) * 60 + int(hms[4:6])
        rows[int(n)] = (t_file, ts if isinstance(ts, str) else None, to_num(pd.Series([la]))[0],
                        to_num(pd.Series([lo]))[0])
    nums = np.array(sorted(img_nums), dtype=float)
    t_file = np.array([rows[int(k)][0] if int(k) in rows else np.nan for k in nums])
    a, b, sd, used = robust_line(nums, t_file + 0.5, tol=1.5)
    lat = np.full(len(nums), np.nan)
    lon = np.full(len(nums), np.nan)
    prev = None
    for i, k in enumerate(nums):
        r = rows.get(int(k))
        if r is None or r[1] is None:
            continue
        if r[1] != prev:
            lat[i], lon[i] = r[2], r[3]
        prev = r[1]
    return {"num": nums, "t": a * nums + b, "period": a, "fit_sd": sd, "used": used,
            "lat": lat, "lon": lon, "n_fresh": int(np.isfinite(lat).sum())}


def load_gt(path):
    return np.array([[float(v) for v in l.split()[:4]] for l in open(path) if l.strip()])


# --------------------------------------------------------------------------- sincronização

def fit_clock_offset(img, log):
    """Δ (s) tal que a posição do log em t_img + Δ bate com a posição GPS da Sequoia."""
    lat0, lon0 = np.nanmedian(log["lat"]), np.nanmedian(log["lon"])
    LE, LN = enu(log["lat"], log["lon"], lat0, lon0)
    ok = np.isfinite(img["lat"]) & np.isfinite(img["lon"])
    IE, IN = enu(img["lat"][ok], img["lon"][ok], lat0, lon0)
    ti = img["t"][ok]
    # só imagens com o drone em voo (na decolagem a posição quase não muda e não informa)
    t_fly0, t_fly1 = log["t"][log["i_takeoff"]], log["t"][log["i_land"]]

    def cost(delta):
        tq = ti + delta
        inside = (tq > t_fly0) & (tq < t_fly1)
        if inside.sum() < 30:
            return np.inf, inside.sum()
        e = np.interp(tq[inside], log["t"], LE) - IE[inside]
        n = np.interp(tq[inside], log["t"], LN) - IN[inside]
        return float(np.median(np.hypot(e, n))), int(inside.sum())

    lo = log["t"][0] - ti.max()
    hi = log["t"][-1] - ti.min()
    grid = np.arange(lo, hi, 1.0)
    costs = np.array([cost(x)[0] for x in grid])
    best = grid[np.argmin(costs)]
    fine = np.arange(best - 2.0, best + 2.0, 0.02)
    fc = np.array([cost(x)[0] for x in fine])
    delta = float(fine[np.argmin(fc)])
    med, n_used = cost(delta)
    return delta, med, n_used


def lag_check(gt, gt_offset, traj_idx, px, py, h_rel, max_lag=15):
    """Posição do log (projetada no mapa) × traj_truth.txt para cada lag (GT row = idx − offset + lag)."""
    out = []
    for L in range(-max_lag, max_lag + 1):
        g = traj_idx - gt_offset + L
        ok = (g >= 0) & (g < len(gt)) & np.isfinite(px)
        if ok.sum() < 20:
            continue
        gi = g[ok].astype(int)
        dpos = np.hypot(px[ok] - gt[gi, 0], py[ok] - gt[gi, 1])
        zr = gt[gi, 2] / np.where(h_rel[ok] > 5, h_rel[ok], np.nan)
        out.append({"lag": L, "n": int(ok.sum()), "pos_med_px": float(np.median(dpos)),
                    "pos_p90_px": float(np.percentile(dpos, 90)),
                    "k_gt": float(np.nanmedian(zr))})
    return out


# --------------------------------------------------------------------------- principal

def process(name, cfg, args):
    ds_dir = os.path.join(DATASETS_DIR, cfg["dir"])
    print(f"\n=== {name} ({cfg['dir']})")
    log = load_log(os.path.join(ds_dir, cfg["log"]))
    traj = load_traj(os.path.join(ds_dir, "traj.txt"))
    traj_idx = np.array([i for i, _ in traj])
    img_num = np.array([n for _, n in traj])
    img = load_camera(os.path.join(ds_dir, cfg["exif"]), img_num)
    if img["n_fresh"] >= 30:
        delta, pos_err, n_sync = fit_clock_offset(img, log)
        sync_mode = "posição GPS da Sequoia"
    elif cfg.get("same_day") and SYNC_BY_DAY.get(cfg["same_day"]):
        ds = SYNC_BY_DAY[cfg["same_day"]]
        delta, pos_err, n_sync = float(np.mean(ds)), float("nan"), 0
        sync_mode = (f"herdada dos outros voos do mesmo dia (EXIF sem fix GPS novo); "
                     f"Δ deles: {', '.join(f'{x:.2f}' for x in ds)} s")
    else:
        raise RuntimeError("EXIF sem fixes GPS novos e nenhum voo do mesmo dia para herdar o relógio")
    if cfg.get("same_day") and img["n_fresh"] >= 30:
        SYNC_BY_DAY.setdefault(cfg["same_day"], []).append(delta)

    t_img = img["t"] + delta  # mesma ordem de img_num (ordenado); traj também é crescente
    order = np.argsort(img_num)
    t_traj = np.empty(len(traj))
    t_traj[order] = t_img
    h_baro = np.interp(t_traj, log["t"], log["h_baro"])
    h_rel = np.interp(t_traj, log["t"], log["h_rel"])
    h_gps = np.interp(t_traj, log["t"], log["h_gps"])
    lat = np.interp(t_traj, log["t"], log["lat"])
    lon = np.interp(t_traj, log["t"], log["lon"])
    inside = (t_traj >= log["t"][0]) & (t_traj <= log["t"][-1])

    geo = Georef(parse_ctrl(os.path.join(DATASETS_DIR, cfg["ctrl"]), cfg["ctrl_section"]))
    f_px = SEQUOIA_F_PX_1280 * cfg["frame_w"] / 1280.0
    k = 1.0 / (f_px * geo.gsd)
    z_baro = k * h_baro

    fly = log["h_rel"] > 20
    db = log["h_baro"][fly] - log["h_gps"][fly]
    baro_gps_rms = float(np.sqrt(np.mean((db - np.median(db)) ** 2)))
    baro_rel_rms = float(np.sqrt(np.mean((log["h_baro"][fly] - log["h_rel"][fly]) ** 2)))

    yaw_log = np.degrees(np.arctan2(np.interp(t_traj, log["t"], log["yaw_sin"]),
                                    np.interp(t_traj, log["t"], log["yaw_cos"])))
    px, py = geo.to_px(lat, lon)
    gt_path = os.path.join(ds_dir, "traj_truth.txt")
    lags, best, yaw_off, yaw_spread, sync_vs_gt = [], None, None, None, None
    if os.path.exists(gt_path):
        gt = load_gt(gt_path)
        lags = lag_check(gt, cfg["gt_offset"], traj_idx, px, py, h_rel)
        if lags:
            best = min(lags, key=lambda r: r["pos_med_px"])
            # convenção de yaw do mapa: offset constante entre o yaw do traj_truth e o da IMU
            g = traj_idx - cfg["gt_offset"] + best["lag"]
            ok = (g >= 0) & (g < len(gt))
            dyaw = np.radians(gt[g[ok].astype(int), 3] - yaw_log[ok])
            yaw_off = float(np.degrees(np.arctan2(np.median(np.sin(dyaw)), np.median(np.cos(dyaw)))))
            res = np.degrees(np.angle(np.exp(1j * (dyaw - np.radians(yaw_off)))))
            yaw_spread = float(np.median(np.abs(res)))
            sync_vs_gt = best["pos_med_px"]

    print(f"  log: {log['n_rows']} linhas a {log['rate']:.2f} Hz (ajuste do relógio σ={log['time_fit_sd']:.2f} s), "
          f"{log['baro_spikes']} picos de barômetro removidos")
    print(f"  câmera: período {img['period']:.3f} s/imagem (σ={img['fit_sd']:.2f} s), "
          f"{img['n_fresh']} imagens com fix GPS novo no EXIF")
    print(f"  sincronização ({sync_mode}): Δ = {delta:.2f} s ({delta / 3600:.3f} h); "
          f"erro mediano de posição {pos_err:.1f} m em {n_sync} imagens")
    print(f"  barômetro × GPS em voo: {baro_gps_rms:.2f} m RMS (sem viés); barômetro × relativeHeight: {baro_rel_rms:.2f} m RMS")
    print(f"  mapa: GSD {geo.gsd:.4f} m/px (resíduo dos pontos de controle {geo.resid:.2f} m); "
          f"f_px {f_px:.1f}; k = {k:.6f} /m")
    if yaw_off is not None:
        print(f"  yaw: offset mapa−IMU {yaw_off:.1f}° (desvio mediano {yaw_spread:.1f}°)")
    if best:
        print(f"  traj_truth.txt: melhor lag {best['lag']:+d} (posição {best['pos_med_px']:.1f} px mediana); "
              f"lag 0 = {next(r for r in lags if r['lag'] == 0)['pos_med_px']:.1f} px; "
              f"k_GT/k = {best['k_gt'] / k:.3f}")
    n_out = int((~inside).sum())
    if n_out:
        print(f"  aviso: {n_out} imagens fora do intervalo do log (h extrapolado como constante)")

    report = {
        "name": name, "log": cfg["log"], "rows": log["n_rows"], "rate": log["rate"], "delta": delta,
        "pos_err": pos_err, "n_sync": n_sync, "baro_gps_rms": baro_gps_rms, "baro_rel_rms": baro_rel_rms,
        "gsd": geo.gsd, "ctrl_resid": geo.resid, "f_px": f_px, "k": k, "lags": lags, "best": best,
        "period": img["period"], "n_out": n_out, "sync_mode": sync_mode,
        "yaw_off": yaw_off, "yaw_spread": yaw_spread,
    }

    if args.dry_run:
        return report
    out_path = os.path.join(ds_dir, "altimeter_baro.txt")
    with open(out_path, "w") as f:
        f.write(f"# altimeter_baro.txt — scripts/extract_altimeter.py, {dt.datetime.now():%Y-%m-%d %H:%M}\n")
        f.write(f"# log {cfg['log']} (IMU_ATTI(0):barometer:Smooth − solo antes da decolagem)\n")
        f.write(f"# sincronização ({sync_mode}): Δ = {delta:.2f} s, erro mediano de posição {pos_err:.1f} m\n")
        f.write(f"# z = h_baro / (f_px·GSD) = {k:.6f}·h_baro  (f_px {f_px:.1f}, GSD {geo.gsd:.4f} m/px)\n")
        f.write("# traj_idx z_baro h_baro_m h_rel_m h_gps_m t_img_s img_num lat lon\n")
        for i in range(len(traj)):
            f.write(f"{traj_idx[i]} {z_baro[i]:.6f} {h_baro[i]:.3f} {h_rel[i]:.3f} {h_gps[i]:.3f} "
                    f"{t_traj[i] - t_traj[0]:.3f} {img_num[i]} {lat[i]:.8f} {lon[i]:.8f}\n")
    print(f"  -> {out_path}")
    # GT re-sincronizado: posição do log no instante exato de cada imagem, projetada no mapa.
    # Uma linha por linha do traj.txt (use --gt_offset 0 na engine, ou o consolidador com --gt_file).
    if yaw_off is not None:
        sync_path = os.path.join(ds_dir, "traj_truth_sync.txt")
        yaw_map = (yaw_log + yaw_off + 180.0) % 360.0 - 180.0
        with open(sync_path, "w") as f:
            for i in range(len(traj)):
                f.write(f"{px[i]:.1f} {py[i]:.1f} {z_baro[i]:.6f} {yaw_map[i]:.3f}\n")
        print(f"  -> {sync_path} (GT re-sincronizado; linha i = linha i do traj.txt)")
        report["sync_gt"] = sync_path
    report["out"] = out_path
    report["series"] = (traj_idx, h_baro, h_rel, h_gps)
    return report


def write_report(rep, gt_path, gt_offset):
    os.makedirs(REPORT_DIR, exist_ok=True)
    L = [f"# Altímetro barométrico — {rep['name']}", "",
         f"- Log: `{rep['log']}` ({rep['rows']} linhas, {rep['rate']:.2f} Hz)",
         f"- Sincronização imagem → log ({rep['sync_mode']}): Δ = {rep['delta']:.2f} s; erro mediano de posição {rep['pos_err']:.1f} m "
         f"({rep['n_sync']} imagens); período da câmera {rep['period']:.3f} s",
         f"- Barômetro × GPS em voo: {rep['baro_gps_rms']:.2f} m RMS; barômetro × relativeHeight: {rep['baro_rel_rms']:.2f} m RMS",
         f"- Mapa: GSD {rep['gsd']:.4f} m/px (pontos de controle, resíduo {rep['ctrl_resid']:.2f} m); "
         f"f_px {rep['f_px']:.1f}; **k = {rep['k']:.6f} /m**"]
    if rep.get("yaw_off") is not None:
        L.append(f"- Yaw: offset mapa − IMU {rep['yaw_off']:.1f}° (desvio mediano {rep['yaw_spread']:.1f}°)")
    if rep.get("sync_gt"):
        L.append(f"- GT re-sincronizado: `{os.path.basename(rep['sync_gt'])}` (uma linha por linha do traj.txt)")
    L.append("")
    if rep["lags"]:
        L += ["## Alinhamento do traj_truth.txt (posição do log projetada no mapa)", "",
              "| lag (GT row = idx − offset + lag) | n | posição mediana (px) | p90 (px) | k_GT / k |",
              "|---|---|---|---|---|"]
        for r in rep["lags"]:
            mark = " **←**" if rep["best"] and r["lag"] == rep["best"]["lag"] else ""
            L.append(f"| {r['lag']:+d}{mark} | {r['n']} | {r['pos_med_px']:.1f} | {r['pos_p90_px']:.1f} | "
                     f"{r['k_gt'] / rep['k']:.3f} |")
        L.append("")
    path = os.path.join(REPORT_DIR, f"{rep['name']}_report.md")
    with open(path, "w") as f:
        f.write("\n".join(L) + "\n")

    if "series" in rep:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        idx, hb, hr, hg = rep["series"]
        fig, ax = plt.subplots(figsize=(9, 3.8), dpi=140)
        fig.patch.set_facecolor("#fcfcfb"); ax.set_facecolor("#fcfcfb")
        ax.plot(idx, hb, color="#2a78d6", lw=2, label="barômetro (usado no filtro)")
        ax.plot(idx, hg, color="#eb6834", lw=1.2, label="GPS (só diagnóstico)")
        if os.path.exists(gt_path):
            gt = load_gt(gt_path)
            lag = rep["best"]["lag"] if rep["best"] else 0
            g = idx - gt_offset + lag
            ok = (g >= 0) & (g < len(gt))
            ax.plot(idx[ok], gt[g[ok].astype(int), 2] / rep["k"], color="#1baf7a", lw=1.2, ls="--",
                    label=f"z do traj_truth / k (lag {lag:+d})")
        for s in ("top", "right"):
            ax.spines[s].set_visible(False)
        ax.grid(True, color="#e4e3df", lw=0.6)
        ax.set_xlabel("índice no traj.txt", fontsize=8, color="#52514e")
        ax.set_ylabel("altura sobre a decolagem (m)", fontsize=8, color="#52514e")
        ax.set_title(f"{rep['name']}: altímetro por frame", fontsize=10)
        ax.legend(frameon=False, fontsize=8)
        fig.tight_layout()
        fig.savefig(os.path.join(REPORT_DIR, f"{rep['name']}_altimetro.png"), facecolor="#fcfcfb")
        plt.close(fig)
    return path


def main():
    ap = argparse.ArgumentParser(description="Gera altimeter_baro.txt a partir dos logs do DJI")
    ap.add_argument("--dataset", nargs="*", default=None, choices=list(DATASETS),
                    help="Voos (padrão: todos com log)")
    ap.add_argument("--dry_run", action="store_true", help="Só imprime os diagnósticos")
    args = ap.parse_args()

    names = args.dataset or [k for k, v in DATASETS.items() if v]
    ok = True
    for name in names:
        cfg = DATASETS[name]
        if cfg is None:
            print(f"\n=== {name}: sem log do DJI no repositório (o voo ADM3 não está no FLY053). Pulando.")
            continue
        try:
            rep = process(name, cfg, args)
            if not args.dry_run:
                p = write_report(rep, os.path.join(DATASETS_DIR, cfg["dir"], "traj_truth.txt"), cfg["gt_offset"])
                print(f"  relatório: {p}")
        except Exception as e:  # um voo com problema não impede os outros
            ok = False
            print(f"  ERRO em {name}: {e}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()

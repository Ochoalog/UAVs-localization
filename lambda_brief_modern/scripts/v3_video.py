#!/usr/bin/env python3
"""
v3_video.py — vídeos para entender a v3: o que o drone vê, o que o Sentinel-2 vê e onde caem os pares

Roda a engine em UM voo × mapa com --save_pairs (pares, máscara de amostragem e NDVI de cada frame;
não muda o resultado) e --save_particles, e monta um vídeo 1920×1080 por método:

  ┌──────────────────────┬──────────────────────┬──────────────────────┐
  │ 1 Drone RGB          │ 2 Drone NDVI         │ 3 Mapa RGB           │
  │ (quantizado, como o  │ + pontos sorteados   │ recorte em volta do  │
  │ descritor vê) + os   │ + máscara de         │ GT: pegadas GT/estim.│
  │ 256 pares            │ amostragem (canto)   │ + pares projetados   │
  ├──────────────────────┼──────────────────────┼──────────────────────┤
  │ 4 Sentinel-2 RGB     │ 5 Sentinel-2 NDVI    │ 6 Peso S2-A (o que a │
  │ do dia do voo (10 m) │ do dia do voo        │ v3 usa) + partículas │
  │ + pegadas + pares    │ + pegadas + pares    │ + visão geral (canto)│
  ├──────────────────────┴──────────────────────┴──────────────────────┤
  │ erro por frame (log) · peso médio dos pares · fase do filtro       │
  └────────────────────────────────────────────────────────────────────┘

Cores dos pares (em todos os painéis): peso do raster S2-A no ponto do mapa onde o par cai na pose do GT —
verde = estável (peso 1), amarelo = meio, vermelho = muda muito (peso mínimo). Nos painéis do mapa,
a ponta do par é branca quando o bit concorda com o drone na pose do GT e magenta quando discorda.
O peso aparece mesmo no abBRIEF puro (que não o usa), para comparar onde os pares caem.

Uso (de dentro de lambda_brief_modern/)
  # ab e ab+S2A no ADM2 · 01-2014, seed 1, protocolo novo (~4 min de engine + ~2 min de vídeo)
  python3 scripts/v3_video.py --voo ADM2 --mapa 01-2014
  # só um trecho do vídeo (a engine roda o voo inteiro; o filtro depende do passado)
  python3 scripts/v3_video.py --voo ADM2 --mapa 01-2014 --frames 80:120
  # peso suave, e também o λ
  python3 scripts/v3_video.py --voo ADM3 --mapa 01-2014 --metodos ab ab_s2a lambda --wmin 0.5
  # só renderizar uma execução que já tem pairs_log.csv
  python3 scripts/v3_video.py --run_dir <pasta da execução> --map <mapa.jpg> --stability_map <png>

Saída: experimentos/<AAAA-MM-DD>_v3_video_<VOO>_<mapa>[_wmin…]/
  video_<método>.mp4        o vídeo (6 fps por padrão)
  quadros/<método>/*.png    um quadro a cada --png_cada frames, para slides
  area_sentinel.png         a área inteira: mapa RGB, S2 RGB e NDVI do dia do voo, peso S2-A e o GT
  execucoes/...             saída da engine (pairs_log.csv, frames_log.csv, amostragem/, particles_log.csv)
"""

import argparse
import copy
import csv
import datetime as dt
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import warnings
from concurrent.futures import ThreadPoolExecutor

import numpy as np
import cv2
import matplotlib
matplotlib.use("Agg")
from matplotlib import cm, font_manager
from PIL import Image, ImageDraw, ImageFont

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
WS = os.path.dirname(MODERN_DIR)
S2_DIR = os.path.join(WS, "Datasets", "Sentinel2", "ADM")
GEOREF = os.path.join(WS, "sentinel2", "map_georef.json")
BAD_SCL = [0, 1, 3, 8, 9, 10]

VW, VH = 1920, 1080
PW, PH = 640, 452          # painel
HEAD = 28                  # faixa de título de cada painel
STRIP = VH - 2 * (PH + HEAD)
LAYER_SCALE = 0.5          # camadas S2 e peso guardadas a 1/2 da resolução do mapa
# vista "voo": mapa grande à esquerda + 6 painéis pequenos (2 colunas × 3 linhas) à direita
SW, SH, SHEAD = 402, 284, 22
BIG_W, BIG_H = VW - 2 * SW, VH - STRIP - HEAD
METODOS = {  # chave -> (descrição na engine, com peso S2?, nome no vídeo)
    "ab": ("abBRIEF", False, "abBRIEF"),
    "ab_s2a": ("abBRIEF", True, "abBRIEF + peso S2-A"),
    "lambda": ("lambda", False, "λ-BRIEF"),
    "lambda_s2a": ("lambda", True, "λ-BRIEF + peso S2-A"),
}

_FONT = font_manager.findfont("DejaVu Sans")
_FONTB = font_manager.findfont(font_manager.FontProperties(family="DejaVu Sans", weight="bold"))


def font(size, bold=False):
    return ImageFont.truetype(_FONTB if bold else _FONT, size)


# --------------------------------------------------------------------------- cores

_RYG = (cm.get_cmap("RdYlGn") if hasattr(cm, "get_cmap") else matplotlib.colormaps["RdYlGn"])
_LUT_W = (np.array([_RYG(i / 255.0)[:3] for i in range(256)])[:, ::-1] * 255).astype(np.uint8)  # BGR


def wcolor(w):
    """Peso 0..1 -> cor BGR (vermelho -> amarelo -> verde)."""
    i = int(np.clip(w, 0, 1) * 255)
    return tuple(int(c) for c in _LUT_W[i])


def colorize(gray8, cmap):
    """uint8 -> BGR por um colormap do OpenCV ou do matplotlib ('RdYlGn')."""
    if cmap == "RdYlGn":
        return _LUT_W[gray8]
    return cv2.applyColorMap(gray8, cmap)


# --------------------------------------------------------------------------- Sentinel-2

def s2_scene(date):
    """RGB (uint8, esticado) e NDVI (float, nan inválido) do S2 nessa data, juntando os tiles."""
    rows = [r for r in csv.DictReader(open(os.path.join(S2_DIR, "index.csv"))) if r["date"] == date]
    if not rows:
        return None, None, None
    rgbs, ndvis = [], []
    for r in rows:
        f = os.path.join(S2_DIR, "scenes", f"{r['date']}_{r['id']}.npz")
        if not os.path.exists(f):
            continue
        z = np.load(f)
        b = dict(zip([str(x) for x in z["bands"]], z["refl"].astype(np.float32) / 1e4))
        bad = np.isin(z["scl"], BAD_SCL) | (b["B04"] <= 0)
        rgb = np.stack([b["B02"], b["B03"], b["B04"]], -1)   # BGR
        rgb[bad] = np.nan
        nd = (b["B08"] - b["B04"]) / np.maximum(b["B08"] + b["B04"], 1e-6)
        nd[bad] = np.nan
        rgbs.append(rgb)
        ndvis.append(nd)
    if not rgbs:
        return None, None, None
    with np.errstate(all="ignore"), warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        rgb = np.nanmean(np.stack(rgbs), 0)
        nd = np.nanmean(np.stack(ndvis), 0)
    lo, hi = np.nanpercentile(rgb, 2), np.nanpercentile(rgb, 98)
    rgb8 = np.clip((rgb - lo) / (hi - lo), 0, 1) ** (1 / 1.4)
    rgb8 = np.nan_to_num(rgb8 * 255, nan=0).astype(np.uint8)
    return rgb8, nd, [r["id"] for r in rows]


def s2_to_map(cell_img, georef_key, map_size, scale=LAYER_SCALE, fill=0):
    """Imagem na grade S2 (H×W[×C]) -> imagem na grade do mapa reduzida por `scale` (célula mais próxima)."""
    g = json.load(open(os.path.join(S2_DIR, "grid.json")))
    X0, Y0, R = g["transform"][2], g["transform"][5], g["transform"][0]
    ce, cn = json.load(open(GEOREF))["maps"][georef_key]["px_to_utm"]
    Wm, Hm = int(map_size[0] * scale), int(map_size[1] * scale)
    xs = (np.arange(Wm) + 0.5) / scale
    out = np.full((Hm, Wm) + cell_img.shape[2:], fill, cell_img.dtype)
    for y0 in range(0, Hm, 256):
        ys = ((np.arange(y0, min(Hm, y0 + 256)) + 0.5) / scale)[:, None]
        E = ce[0] * xs[None, :] + ce[1] * ys + ce[2]
        N = cn[0] * xs[None, :] + cn[1] * ys + cn[2]
        c = np.floor((E - X0) / R).astype(int)
        r = np.floor((Y0 - N) / R).astype(int)
        ok = (c >= 0) & (c < cell_img.shape[1]) & (r >= 0) & (r < cell_img.shape[0])
        blk = out[y0:y0 + c.shape[0]]
        blk[ok] = cell_img[r[ok], c[ok]]
    return out


# --------------------------------------------------------------------------- dados da execução

def load_run(run_dir):
    fr = {}
    for r in csv.DictReader(open(os.path.join(run_dir, "frames_log.csv"))):
        f = int(r["frame"])
        g = None
        if r["gt_x"]:
            zr = r.get("gt_z_ref") or r["est_z"]
            g = (float(r["gt_x"]), float(r["gt_y"]), float(r["gt_yaw_deg"]), float(zr))
        fr[f] = {"rgb": r["rgb"], "nir": r["nir"], "red": r["red"],
                 "est": (float(r["est_x"]), float(r["est_y"]), float(r["est_z"]), float(r["est_yaw_deg"])),
                 "gt": g, "d_est": float(r["d_est"]), "d_gt": float(r["d_gt"]),
                 "mean_w_est": float(r["mean_w_est"]), "phase": r["phase"]}
    P = np.loadtxt(os.path.join(run_dir, "pairs_log.csv"), delimiter=",", skiprows=1)
    pairs = {int(f): P[P[:, 0] == f] for f in np.unique(P[:, 0])}
    parts = {}
    pp = os.path.join(run_dir, "particles_log.csv")
    if os.path.exists(pp):
        Q = np.loadtxt(pp, delimiter=",", skiprows=1, usecols=(0, 2, 3))
        idx = np.r_[0, np.flatnonzero(np.diff(Q[:, 0])) + 1, len(Q)]
        for a, b in zip(idx[:-1], idx[1:]):
            parts[int(Q[a, 0])] = Q[a:b, 1:3]
    err = {}
    ep = os.path.join(run_dir, "errors.txt")
    if os.path.exists(ep):
        for line in open(ep):
            v = line.split()
            if v and v[0].isdigit():
                err[int(v[0])] = float(v[7])
    cmd = []
    cp = os.path.join(run_dir, "cmd.txt")
    if os.path.exists(cp):
        try:
            cmd = json.load(open(cp))
        except ValueError:
            cmd = []
    return fr, pairs, parts, err, cmd


def cmd_value(cmd, flag, default=None):
    return cmd[cmd.index(flag) + 1] if flag in cmd and cmd.index(flag) + 1 < len(cmd) else default


def quantize(img, levels):
    if levels <= 0:
        return img
    L = np.float32(levels)
    step = float(np.float32(255.0) / L)
    lut = np.array([int(np.floor(float(np.float32(v) * L) / 255.0) * step) for v in range(256)], np.uint8)
    return cv2.LUT(img, lut)


def project(pts, pose_xy_yaw_z, W, H):
    """Pontos da imagem do drone -> mapa, com a mesma fórmula da engine (Descriptor::pairDetails)."""
    x, y, yaw_deg, z = pose_xy_yaw_z
    th = -np.radians(yaw_deg) - np.pi / 2
    px = (pts[:, 0] - W // 2) * z
    py = (pts[:, 1] - H // 2) * z
    return np.c_[px * np.cos(th) + py * np.sin(th) + x, -px * np.sin(th) + py * np.cos(th) + y]


# --------------------------------------------------------------------------- desenho

class Crop:
    """Janela do mapa (coordenadas do mapa) desenhada num painel PW×PH."""

    def __init__(self, cx, cy, half_w):
        self.half_w = half_w
        self.half_h = half_w * PH / PW
        self.x0, self.y0 = cx - self.half_w, cy - self.half_h
        self.s = PW / (2 * self.half_w)

    def to_px(self, xy):
        xy = np.atleast_2d(xy)
        return np.c_[(xy[:, 0] - self.x0) * self.s, (xy[:, 1] - self.y0) * self.s]

    def cut(self, img, img_scale=1.0, border=0):
        """Recorte de `img` (na escala img_scale do mapa) redimensionado para o painel."""
        M = np.array([[self.s / img_scale, 0, -self.x0 * self.s], [0, self.s / img_scale, -self.y0 * self.s]])
        interp = cv2.INTER_NEAREST if self.s / img_scale > 1.5 else cv2.INTER_AREA
        return cv2.warpAffine(img, M, (PW, PH), flags=interp, borderMode=cv2.BORDER_CONSTANT, borderValue=border)


def draw_footprint(img, crop, pose, W, H, color, thick=2):
    c = np.array([[0, 0], [W, 0], [W, H], [0, H]], float)
    p = crop.to_px(project(c, pose, W, H)).astype(np.int32)
    cv2.polylines(img, [p], True, color, thick, cv2.LINE_AA)
    # seta da "frente" do drone (borda de cima da imagem)
    top = crop.to_px(project(np.array([[W / 2, 0]], float), pose, W, H))[0].astype(int)
    ctr = crop.to_px(project(np.array([[W / 2, H / 2]], float), pose, W, H))[0].astype(int)
    cv2.arrowedLine(img, tuple(ctr), tuple(top), color, thick, cv2.LINE_AA, tipLength=0.15)


def draw_pairs_map(img, crop, m1, m2, w, e, thick=1):
    a, b = crop.to_px(m1).astype(int), crop.to_px(m2).astype(int)
    for i in np.argsort(-w):     # instáveis por cima
        cv2.line(img, tuple(a[i]), tuple(b[i]), wcolor(w[i]), thick, cv2.LINE_AA)
    for i in range(len(a)):
        col = (255, 255, 255) if e[i] == 0 else (255, 0, 255) if e[i] > 0 else (90, 90, 90)
        cv2.circle(img, tuple(a[i]), 2, col, -1, cv2.LINE_AA)
        cv2.circle(img, tuple(b[i]), 2, col, -1, cv2.LINE_AA)


def warp_to_drone(layer, sc, pose, W, H, interp):
    """Camada do mapa (na escala sc) reamostrada na imagem W×H do drone para a pose (x, y, yaw°, z)."""
    x, y, yaw, z = pose
    th = -np.radians(yaw) - np.pi / 2
    c, s = np.cos(th), np.sin(th)
    A = np.array([[z * c, z * s, x - (W // 2) * z * c - (H // 2) * z * s],
                  [-z * s, z * c, y + (W // 2) * z * s - (H // 2) * z * c]]) * sc
    return cv2.warpAffine(layer, A, (W, H), flags=cv2.WARP_INVERSE_MAP | interp,
                          borderMode=cv2.BORDER_CONSTANT, borderValue=(30, 30, 30))


def draw_pairs_drone(img, A, B, w, e):
    for i in np.argsort(-w):
        cv2.line(img, tuple(A[i]), tuple(B[i]), wcolor(w[i]), 1, cv2.LINE_AA)
    for i in range(len(A)):
        col = (255, 255, 255) if e[i] == 0 else (255, 0, 255) if e[i] > 0 else (90, 90, 90)
        cv2.circle(img, tuple(A[i]), 2, col, -1, cv2.LINE_AA)
        cv2.circle(img, tuple(B[i]), 2, col, -1, cv2.LINE_AA)


def fit(img, w, h):
    """Redimensiona mantendo a proporção, com faixas pretas."""
    s = min(w / img.shape[1], h / img.shape[0])
    r = cv2.resize(img, (int(img.shape[1] * s), int(img.shape[0] * s)), interpolation=cv2.INTER_AREA)
    out = np.zeros((h, w, 3), np.uint8)
    y, x = (h - r.shape[0]) // 2, (w - r.shape[1]) // 2
    out[y:y + r.shape[0], x:x + r.shape[1]] = r
    return out, s, x, y


# --------------------------------------------------------------------------- renderização

class Renderer:
    def __init__(self, map_path, stab_path, s2_date, georef, notes):
        self.map = cv2.imread(map_path)
        if self.map is None:
            sys.exit(f"Mapa não encontrado: {map_path}")
        self.map_name = os.path.basename(map_path)
        Hm, Wm = self.map.shape[:2]
        self.map_small = cv2.resize(self.map, None, fx=LAYER_SCALE, fy=LAYER_SCALE, interpolation=cv2.INTER_AREA)
        self.W = None
        if stab_path and os.path.exists(stab_path):
            w8 = cv2.imread(stab_path, cv2.IMREAD_GRAYSCALE)
            self.wraw = w8
            self.wlayer = colorize(cv2.resize(w8, None, fx=LAYER_SCALE, fy=LAYER_SCALE,
                                              interpolation=cv2.INTER_NEAREST), "RdYlGn")
            self.stab_name = os.path.basename(stab_path)
        else:
            self.wraw, self.wlayer, self.stab_name = None, None, None
            notes.append("sem raster de peso: painel 6 e cores dos pares ficam neutros")
        rgb8, nd, ids = s2_scene(s2_date)
        self.s2_date = s2_date
        if rgb8 is None:
            notes.append(f"sem cena S2 em {s2_date}: painéis 4 e 5 vazios")
            self.s2rgb = self.s2ndvi = None
        else:
            self.s2rgb = s2_to_map(rgb8, georef, (Wm, Hm))
            nd8 = np.nan_to_num(np.clip((nd + 0.2) / 1.2, 0, 1) * 255, nan=0).astype(np.uint8)
            nd_bgr = colorize(nd8, "RdYlGn")
            nd_bgr[~np.isfinite(nd)] = (40, 40, 40)
            self.s2ndvi = s2_to_map(nd_bgr, georef, (Wm, Hm))
            self.s2_ids = ids
        # visão geral (canto do painel 6)
        self.ov_s = 230 / max(Wm, Hm)
        base = self.wlayer if self.wlayer is not None else self.map_small
        self.overview = cv2.resize(base, (int(Wm * self.ov_s), int(Hm * self.ov_s)), interpolation=cv2.INTER_AREA)

    def weight_at(self, xy):
        if self.wraw is None:
            return np.ones(len(xy))
        x = np.clip(np.round(xy[:, 0]).astype(int), 0, self.wraw.shape[1] - 1)
        y = np.clip(np.round(xy[:, 1]).astype(int), 0, self.wraw.shape[0] - 1)
        out = self.wraw[y, x] / 255.0
        inside = (xy[:, 0] >= 0) & (xy[:, 0] < self.wraw.shape[1]) & (xy[:, 1] >= 0) & (xy[:, 1] < self.wraw.shape[0])
        return np.where(inside, out, 1.0)

    def area_figure(self, gt_xy, path):
        """Área inteira: mapa, S2 RGB, S2 NDVI, peso — com a trajetória do GT."""
        x0, y0 = gt_xy.min(0) - 700
        x1, y1 = gt_xy.max(0) + 700
        cx, cy, hw = (x0 + x1) / 2, (y0 + y1) / 2, max(x1 - x0, (y1 - y0) * PW / PH) / 2
        crop = Crop(cx, cy, hw)
        tiles = [("Mapa RGB (" + self.map_name + ")", crop.cut(self.map))]
        if self.s2rgb is not None:
            tiles += [(f"Sentinel-2 RGB {self.s2_date} (10 m)", crop.cut(self.s2rgb, LAYER_SCALE)),
                      (f"Sentinel-2 NDVI {self.s2_date}", crop.cut(self.s2ndvi, LAYER_SCALE))]
        if self.wlayer is not None:
            tiles.append((f"Peso S2-A ({self.stab_name})", crop.cut(self.wlayer, LAYER_SCALE)))
        canvas = np.full((PH + HEAD, PW * len(tiles), 3), 22, np.uint8)
        for k, (t, im) in enumerate(tiles):
            p = crop.to_px(gt_xy).astype(np.int32)
            cv2.polylines(im, [p], False, (255, 255, 255), 3, cv2.LINE_AA)
            cv2.polylines(im, [p], False, (40, 200, 40), 2, cv2.LINE_AA)
            cv2.circle(im, tuple(p[0]), 6, (255, 255, 255), -1)
            canvas[HEAD:, k * PW:(k + 1) * PW] = im
        pil = Image.fromarray(cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB))
        d = ImageDraw.Draw(pil)
        for k, (t, _) in enumerate(tiles):
            d.text((k * PW + 8, 5), t, font=font(16, True), fill=(235, 235, 235))
        pil.save(path)

    def render(self, run_dir, metodo_nome, frames, out_prefix, png_dir, png_cada, fps, notes,
               vistas=("voo", "drone", "mapa"), fundo="mapa"):
        """Gera <out_prefix>_<vista>.mp4: voo (mapa grande com as partículas convergindo + painéis),
        drone (tudo na orientação da câmera) e mapa (recortes norte para cima)."""
        fr, pairs, parts, err, cmd = load_run(run_dir)
        levels = int(cmd_value(cmd, "--quantize", 25))
        alpha = float(cmd_value(cmd, "--alpha", 0.0))
        usa_peso = "--stability_map" in cmd
        all_f = sorted(fr)
        if frames:
            all_f = [f for f in all_f if frames[0] <= f <= frames[1]]
        if not all_f:
            sys.exit("Nenhum frame no intervalo pedido.")
        zs = np.array([fr[f]["est"][2] for f in sorted(fr)])
        img0 = cv2.imread(fr[all_f[0]]["rgb"])
        W, H = img0.shape[1], img0.shape[0]
        half_w = 0.75 * W * float(np.median(zs))          # janela fixa: ~1,5 pegada de largura
        gt_track = np.array([fr[f]["gt"][:2] if fr[f]["gt"] else fr[f]["est"][:2] for f in sorted(fr)])
        est_track = np.array([fr[f]["est"][:2] for f in sorted(fr)])
        frames_sorted = sorted(fr)
        errs = np.array([err.get(f, np.nan) for f in frames_sorted])
        mw = np.array([fr[f]["mean_w_est"] for f in frames_sorted])
        strip_bg = self.strip_background(frames_sorted, errs, mw, fr, usa_peso)

        tmp = {v: tempfile.mkdtemp(prefix=f"v3video_{v}_") for v in vistas}
        px2m = float(cmd_value(cmd, "--pixel_to_meter", 0.188679245))
        boxes = self.flight_boxes(fr, parts, frames_sorted, W, float(np.median(zs)))
        big_bg = self.map if fundo == "mapa" or self.wlayer is None else cv2.addWeighted(
            self.map, 0.45, cv2.resize(self.wlayer, (self.map.shape[1], self.map.shape[0]),
                                       interpolation=cv2.INTER_NEAREST), 0.55, 0)
        os.makedirs(png_dir, exist_ok=True)
        t0 = time.time()
        for n, f in enumerate(all_f):
            F = fr[f]
            P = pairs.get(f)
            if P is None:
                continue
            est = F["est"]
            gt = F["gt"]
            pose_est = (est[0], est[1], est[3], est[2])
            pose_gt = (gt[0], gt[1], gt[2], gt[3]) if gt else pose_est
            ab = P[:, 2:6]
            m1g, m2g, eg = P[:, 12:14], P[:, 14:16], P[:, 16]
            wg = np.minimum(self.weight_at(m1g), self.weight_at(m2g))
            # 1 — drone RGB quantizado + pares
            rgb = quantize(cv2.imread(F["rgb"]), levels)
            p1, s1, ox, oy = fit(rgb, PW, PH)
            A = (ab[:, 0:2] * s1 + [ox, oy]).astype(int)
            B = (ab[:, 2:4] * s1 + [ox, oy]).astype(int)
            for i in np.argsort(-wg):
                cv2.line(p1, tuple(A[i]), tuple(B[i]), wcolor(wg[i]), 1, cv2.LINE_AA)
            for i in range(len(A)):
                cv2.circle(p1, tuple(A[i]), 2, wcolor(wg[i]), -1, cv2.LINE_AA)
                cv2.circle(p1, tuple(B[i]), 2, wcolor(wg[i]), -1, cv2.LINE_AA)
            # 2 — NDVI do drone + pontos + máscara de amostragem
            nd = cv2.imread(os.path.join(run_dir, "amostragem", f"ndvi_{f:04d}.png"), cv2.IMREAD_GRAYSCALE)
            mk = cv2.imread(os.path.join(run_dir, "amostragem", f"mask_{f:04d}.png"), cv2.IMREAD_GRAYSCALE)
            if nd is not None:
                ndv = np.clip((nd.astype(np.float32) - 127.5) / 127.5, -1, 1)          # NDVI em [-1, 1]
                nd8 = np.clip((ndv + 0.2) / 1.2 * 255, 0, 255).astype(np.uint8)
                p2, s2, ox2, oy2 = fit(colorize(nd8, "RdYlGn"), PW, PH)
                s2 *= 0.5   # o png está em meia resolução
            else:
                p2, s2, ox2, oy2 = np.zeros((PH, PW, 3), np.uint8), PW / W, 0, 0
            pts = np.r_[ab[:, 0:2], ab[:, 2:4]]
            for q in (pts * s2 + [ox2, oy2]).astype(int):
                cv2.circle(p2, tuple(q), 2, (255, 255, 255), -1, cv2.LINE_AA)
                cv2.circle(p2, tuple(q), 2, (0, 0, 0), 1, cv2.LINE_AA)
            if mk is not None:
                ins = cv2.resize(cv2.applyColorMap(mk, cv2.COLORMAP_INFERNO), (200, int(200 * mk.shape[0] / mk.shape[1])))
                p2[6:6 + ins.shape[0], PW - 206:PW - 6] = ins
                cv2.rectangle(p2, (PW - 207, 5), (PW - 5, 6 + ins.shape[0]), (230, 230, 230), 1)
            # janela do mapa em volta do GT
            cx, cy = (gt[0], gt[1]) if gt else (est[0], est[1])
            crop = Crop(cx, cy, half_w)
            panels = []
            for layer, sc in ((self.map, 1.0), (self.s2rgb, LAYER_SCALE), (self.s2ndvi, LAYER_SCALE),
                              (self.wlayer, LAYER_SCALE)):
                if layer is None:
                    panels.append(np.full((PH, PW, 3), 30, np.uint8))
                else:
                    panels.append(crop.cut(layer, sc, border=(30, 30, 30)))
            p3, p4, p5, p6 = panels
            for im in (p3, p4, p5):
                draw_pairs_map(im, crop, m1g, m2g, wg, eg)
                if gt:
                    draw_footprint(im, crop, pose_gt, W, H, (60, 220, 60))
                draw_footprint(im, crop, pose_est, W, H, (0, 170, 255))
            # 6 — peso + partículas + visão geral
            if f in parts:
                for q in crop.to_px(parts[f]).astype(int):
                    if 0 <= q[0] < PW and 0 <= q[1] < PH:
                        cv2.circle(p6, tuple(q), 1, (255, 255, 255), -1)
            if gt:
                draw_footprint(p6, crop, pose_gt, W, H, (60, 220, 60))
            draw_footprint(p6, crop, pose_est, W, H, (0, 170, 255))
            ov = self.overview.copy()
            k = frames_sorted.index(f)
            cv2.polylines(ov, [(gt_track[:k + 1] * self.ov_s).astype(np.int32)], False, (60, 220, 60), 1, cv2.LINE_AA)
            cv2.polylines(ov, [(est_track[:k + 1] * self.ov_s).astype(np.int32)], False, (0, 170, 255), 1, cv2.LINE_AA)
            if f in parts:
                for q in (parts[f] * self.ov_s).astype(int):
                    if 0 <= q[0] < ov.shape[1] and 0 <= q[1] < ov.shape[0]:
                        ov[q[1], q[0]] = (255, 255, 255)
            r0 = (np.array([crop.x0, crop.y0]) * self.ov_s).astype(int)
            r1 = (np.array([crop.x0 + 2 * crop.half_w, crop.y0 + 2 * crop.half_h]) * self.ov_s).astype(int)
            cv2.rectangle(ov, tuple(r0), tuple(r1), (255, 255, 255), 1)
            p6[PH - ov.shape[0] - 6:PH - 6, PW - ov.shape[1] - 6:PW - 6] = ov
            cv2.rectangle(p6, (PW - ov.shape[1] - 7, PH - ov.shape[0] - 7), (PW - 5, PH - 5), (230, 230, 230), 1)
            # vista do drone: mapa, S2 e peso reamostrados na pose do GT, na mesma orientação da imagem
            pose_v = pose_gt if gt else pose_est
            dv = []
            for layer, sc, interp in ((self.map, 1.0, cv2.INTER_LINEAR), (self.s2rgb, LAYER_SCALE, cv2.INTER_NEAREST),
                                      (self.s2ndvi, LAYER_SCALE, cv2.INTER_NEAREST),
                                      (self.wlayer, LAYER_SCALE, cv2.INTER_NEAREST)):
                if layer is None:
                    dv.append(np.full((PH, PW, 3), 30, np.uint8))
                    continue
                im = warp_to_drone(layer, sc, pose_v, W, H, interp)
                im, _, _, _ = fit(im, PW, PH)
                draw_pairs_drone(im, A, B, wg, eg)
                dv.append(im)
            v_map, v_s2rgb, v_s2nd, v_w = dv
            k = frames_sorted.index(f)
            strip = strip_bg.copy()
            xk = self.strip_x(k, len(frames_sorted))
            cv2.line(strip, (xk, 6), (xk, STRIP - 22), (255, 255, 255), 1)
            e_now = err.get(f)
            frac_inst = float(np.mean(wg < 0.3))
            info = (f"{metodo_nome}  ·  frame {f}  ·  fase {'rastreio' if F['phase'] == 'T' else 'busca global'}"
                    + (f"  ·  erro {e_now:.1f} m" if e_now is not None else "")
                    + f"  ·  peso médio dos pares (GT) {wg.mean():.2f}, {100 * frac_inst:.0f}% instáveis (w<0,3)"
                    + f"  ·  d(GT) {F['d_gt']:.3f}  d(estimativa) {F['d_est']:.3f}")
            peso_tag = " (usado)" if usa_peso else " (só exibido)"
            t_rgb = f"Drone RGB (quantizado {levels} níveis) + {len(P)} pares"
            t_nd = f"Drone NDVI + pontos sorteados · canto: máscara (α={alpha:g})"
            views = {
                "mapa": ([p1, p2, p3, p4, p5, p6],
                         [f"1 · {t_rgb}", f"2 · {t_nd}", f"3 · Mapa {self.map_name} · pares na pose do GT",
                          f"4 · Sentinel-2 RGB {self.s2_date} (célula de 10 m)", f"5 · Sentinel-2 NDVI {self.s2_date}",
                          f"6 · Peso S2-A{peso_tag} · partículas · visão geral"],
                         "pegada: verde = GT, laranja = estimativa  ·  par: cor = peso S2-A no mapa (verde estável → "
                         "vermelho instável)  ·  ponta branca = bit concorda com o drone na pose do GT, magenta = discorda"),
                "drone": ([p1, v_map, v_s2rgb, p2, v_s2nd, v_w],
                          [f"1 · {t_rgb}", f"2 · Mapa {self.map_name} visto da pose do GT",
                           f"3 · Sentinel-2 RGB {self.s2_date} visto da pose do GT", f"4 · {t_nd}",
                           f"5 · Sentinel-2 NDVI {self.s2_date} visto da pose do GT",
                           f"6 · Peso S2-A{peso_tag} visto da pose do GT"],
                          "tudo na orientação da câmera: mapa, Sentinel-2 e peso reamostrados na pose do GT  ·  par: cor = "
                          "peso S2-A (verde estável → vermelho instável)  ·  ponta branca/magenta = bit concorda/discorda"),
            }
            if "voo" in vistas:
                big = self.flight_panel(big_bg, boxes[f], k, frames_sorted, gt_track, est_track, parts.get(f),
                                        pose_gt if gt else None, pose_est, W, H, px2m)
                small = [cv2.resize(im, (SW, SH), interpolation=cv2.INTER_AREA)
                         for im in (p1, p2, v_map, v_s2rgb, v_s2nd, v_w)]
                canvas = np.full((VH, VW, 3), 18, np.uint8)
                canvas[HEAD:HEAD + BIG_H, :BIG_W] = big
                for j, im in enumerate(small):
                    r, c = divmod(j, 2)
                    y = SHEAD + r * (SH + SHEAD)
                    canvas[y:y + SH, BIG_W + c * SW:BIG_W + (c + 1) * SW] = im
                canvas[VH - STRIP:, :] = strip
                pil = Image.fromarray(cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB))
                d = ImageDraw.Draw(pil)
                ttl = (f"Voo · {self.map_name}" + (" + peso S2-A" if fundo == "peso" else "")
                       + f" · verde = GT, laranja = estimativa, amarelo = {len(parts.get(f, []))} partículas")
                fs = 16
                while fs > 11 and d.textlength(ttl, font=font(fs, True)) > BIG_W - 16:
                    fs -= 1
                d.text((8, 5), ttl, font=font(fs, True), fill=(235, 235, 235))
                small_t = ["Drone RGB + pares", "Drone NDVI + pontos + máscara", "Mapa visto da pose do GT",
                           f"Sentinel-2 RGB {self.s2_date}", f"Sentinel-2 NDVI {self.s2_date}", f"Peso S2-A{peso_tag}"]
                for j, t in enumerate(small_t):
                    r, c = divmod(j, 2)
                    d.text((BIG_W + c * SW + 6, r * (SH + SHEAD) + 3), t, font=font(13, True), fill=(235, 235, 235))
                d.text((10, VH - STRIP + 2), info, font=font(16, True), fill=(255, 255, 255))
                d.text((10, VH - 22), "esquerda: o voo e a nuvem de partículas (zoom acompanha a nuvem)  ·  direita: "
                       "o que o drone vê e o mapa / Sentinel-2 / peso na pose do GT  ·  par: verde estável → vermelho "
                       "instável; ponta branca/magenta = bit concorda/discorda", font=font(13), fill=(200, 200, 200))
                views["voo"] = None
                frame_img = cv2.cvtColor(np.array(pil), cv2.COLOR_RGB2BGR)
                cv2.imwrite(os.path.join(tmp["voo"], f"{n:05d}.png"), frame_img)
                if png_cada and (n % png_cada == 0):
                    os.makedirs(os.path.join(png_dir, "voo"), exist_ok=True)
                    cv2.imwrite(os.path.join(png_dir, "voo", f"frame_{f:04d}.png"), frame_img)
            for vname, val in views.items():
                if vname not in vistas or val is None:
                    continue
                grid, titles, legend = val
                canvas = np.full((VH, VW, 3), 18, np.uint8)
                for j, im in enumerate(grid):
                    r, c = divmod(j, 3)
                    canvas[HEAD + r * (PH + HEAD):HEAD + r * (PH + HEAD) + PH, c * PW:(c + 1) * PW] = im
                canvas[VH - STRIP:, :] = strip
                pil = Image.fromarray(cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB))
                d = ImageDraw.Draw(pil)
                for j, t in enumerate(titles):
                    r, c = divmod(j, 3)
                    d.text((c * PW + 8, r * (PH + HEAD) + 5), t, font=font(15, True), fill=(235, 235, 235))
                d.text((10, VH - STRIP + 2), info, font=font(16, True), fill=(255, 255, 255))
                d.text((10, VH - 22), legend, font=font(13), fill=(200, 200, 200))
                frame_img = cv2.cvtColor(np.array(pil), cv2.COLOR_RGB2BGR)
                cv2.imwrite(os.path.join(tmp[vname], f"{n:05d}.png"), frame_img)
                if png_cada and (n % png_cada == 0):
                    os.makedirs(os.path.join(png_dir, vname), exist_ok=True)
                    cv2.imwrite(os.path.join(png_dir, vname, f"frame_{f:04d}.png"), frame_img)
            if n % 20 == 0:
                print(f"  [{metodo_nome}] quadro {n + 1}/{len(all_f)} ({time.time() - t0:.0f}s)", flush=True)
        outs = []
        for vname, d in tmp.items():
            out = f"{out_prefix}_{vname}.mp4"
            encode(d, out, fps)
            shutil.rmtree(d, ignore_errors=True)
            outs.append(out)
        return outs

    def flight_boxes(self, fr, parts, frames_sorted, W, med_z):
        """Janela (cx, cy, meia largura) da vista do voo em cada frame: cobre a nuvem (percentis 2–98),
        o GT e a estimativa, com suavização entre frames."""
        aspect = BIG_W / BIG_H
        Hm, Wm = self.map.shape[:2]
        min_hw, max_hw = 0.9 * W * med_z, 0.55 * max(Wm, Hm * aspect)
        out, c, hw = {}, None, None
        for f in frames_sorted:
            pts = [np.array([fr[f]["est"][:2]])]
            if fr[f]["gt"]:
                pts.append(np.array([fr[f]["gt"][:2]]))
            if f in parts and len(parts[f]):
                q = parts[f]
                pts.append(np.percentile(q, [2, 98], axis=0))
            P = np.vstack(pts)
            lo, hi = P.min(0), P.max(0)
            tc = (lo + hi) / 2
            thw = np.clip(max((hi[0] - lo[0]) / 2, (hi[1] - lo[1]) / 2 * aspect) * 1.25 + 0.3 * min_hw, min_hw, max_hw)
            if c is None:
                c, hw = tc, thw
            else:
                c = 0.75 * c + 0.25 * tc
                hw = float(np.exp(0.8 * np.log(hw) + 0.2 * np.log(thw)))
            out[f] = (float(c[0]), float(c[1]), hw)
        return out

    def flight_panel(self, bg, box, k, frames_sorted, gt_track, est_track, pts, pose_gt, pose_est, W, H, px2m):
        cx, cy, hw = box
        hh = hw * BIG_H / BIG_W
        sc = BIG_W / (2 * hw)
        M = np.array([[sc, 0, -(cx - hw) * sc], [0, sc, -(cy - hh) * sc]])
        img = cv2.warpAffine(bg, M, (BIG_W, BIG_H), flags=cv2.INTER_AREA if sc < 1 else cv2.INTER_LINEAR,
                             borderMode=cv2.BORDER_CONSTANT, borderValue=(30, 30, 30))
        to_px = lambda xy: np.c_[(xy[:, 0] - (cx - hw)) * sc, (xy[:, 1] - (cy - hh)) * sc]
        if pts is not None and len(pts):
            q = to_px(pts).astype(int)
            ok = (q[:, 0] >= 0) & (q[:, 0] < BIG_W - 1) & (q[:, 1] >= 0) & (q[:, 1] < BIG_H - 1)
            q = q[ok]
            for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
                img[q[:, 1] + dy, q[:, 0] + dx] = (0, 235, 255)
        cv2.polylines(img, [to_px(gt_track[:k + 1]).astype(np.int32)], False, (60, 220, 60), 2, cv2.LINE_AA)
        cv2.polylines(img, [to_px(est_track[:k + 1]).astype(np.int32)], False, (0, 170, 255), 2, cv2.LINE_AA)
        crop = Crop(cx, cy, hw)
        crop.half_h, crop.y0, crop.s = hh, cy - hh, sc
        if pose_gt:
            draw_footprint(img, crop, pose_gt, W, H, (60, 220, 60))
        draw_footprint(img, crop, pose_est, W, H, (0, 170, 255))
        # barra de escala
        for m in (10, 20, 50, 100, 200, 500, 1000):
            Lpx = m / px2m * sc
            if Lpx >= 110:
                break
        y = BIG_H - 22
        cv2.rectangle(img, (14, y - 18), (int(30 + Lpx), y + 10), (20, 20, 20), -1)
        cv2.line(img, (20, y), (int(20 + Lpx), y), (255, 255, 255), 3)
        cv2.putText(img, f"{m} m", (int(26 + Lpx / 2) - 18, y - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1,
                    cv2.LINE_AA)
        return img

    # faixa de baixo: erro (log) e peso médio por frame
    def strip_x(self, k, n):
        return int(60 + (VW - 80) * k / max(1, n - 1))

    def strip_background(self, frames, errs, mw, fr, usa_peso):
        s = np.full((STRIP, VW, 3), 26, np.uint8)
        top, bot = 26, STRIP - 24
        n = len(frames)
        # fases
        for k, f in enumerate(frames):
            if fr[f]["phase"] == "G":
                x = self.strip_x(k, n)
                cv2.line(s, (x, top), (x, bot), (55, 40, 40), 2)
        def ylog(v):
            v = np.clip(v, 1, 1000)
            return int(bot - (np.log10(v) / 3) * (bot - top))
        for v, lab in ((1, "1 m"), (10, "10 m"), (25, "25"), (100, "100 m"), (1000, "1 km")):
            y = ylog(v)
            cv2.line(s, (60, y), (VW - 20, y), (60, 60, 60) if v != 25 else (90, 90, 140), 1)
            cv2.putText(s, lab, (6, y + 4), cv2.FONT_HERSHEY_SIMPLEX, 0.38, (170, 170, 170), 1, cv2.LINE_AA)
        pts = [(self.strip_x(k, n), ylog(e)) for k, e in enumerate(errs) if np.isfinite(e)]
        if len(pts) > 1:
            cv2.polylines(s, [np.array(pts, np.int32)], False, (255, 200, 90), 2, cv2.LINE_AA)
        if usa_peso:
            pw = [(self.strip_x(k, n), int(bot - w * (bot - top))) for k, w in enumerate(mw)]
            cv2.polylines(s, [np.array(pw, np.int32)], False, (120, 220, 120), 1, cv2.LINE_AA)
        cv2.putText(s, "erro (log, azul)" + ("  peso medio dos pares na estimativa (verde, 0-1)" if usa_peso else "")
                    + "  fundo vermelho = busca global", (VW - 760, top + 12), cv2.FONT_HERSHEY_SIMPLEX, 0.42,
                    (190, 190, 190), 1, cv2.LINE_AA)
        return s


def encode(frames_dir, out_mp4, fps):
    os.makedirs(os.path.dirname(os.path.abspath(out_mp4)), exist_ok=True)
    if shutil.which("ffmpeg"):
        r = subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", str(fps), "-i",
                            os.path.join(frames_dir, "%05d.png"), "-c:v", "libx264", "-pix_fmt", "yuv420p",
                            "-crf", "20", out_mp4])
        if r.returncode == 0:
            return
    files = sorted(os.listdir(frames_dir))
    vw = cv2.VideoWriter(out_mp4, cv2.VideoWriter_fourcc(*"mp4v"), fps, (VW, VH))
    for fn in files:
        vw.write(cv2.imread(os.path.join(frames_dir, fn)))
    vw.release()


# --------------------------------------------------------------------------- execução da engine

def run_engine(args, notes):
    import paper_comparison as pc
    import benchmark_runner as br
    import experimentos as exp
    import v3_comparison as v3

    p = pc.PROTOCOLS[args.protocolo]
    a = argparse.Namespace(**vars(args))
    a.regime = args.regime or p["regime"]
    a.gt = p["gt"]
    a.lambda_alpha = args.lambda_alpha
    a.flights = [args.voo]
    a.particles = args.particles
    a.bin = args.bin or br.DEFAULT_BIN
    a.odom_dir = args.odom_dir or br.DEFAULT_ODOM_DIR
    a.adm_map_12_2018, a.adm_map_01_2014 = "ADM_35_17_12_2018.jpg", "ADM_35_27_01_2014.jpg"
    a.dry_run = False
    a.pesos, a.formulas, a._sem_b = ["A"], ["normalized"], {}
    a.data_voo = "2019-04-18"
    _todos = getattr(pc, "_flight_maps_orig", pc.flight_maps)
    pc._flight_maps_orig = _todos
    pc.flight_maps = lambda f, aa: [m for m in _todos(f, aa) if m[0] == args.mapa]
    nome = f"v3_video_{args.voo}_{args.mapa}" + v3.rampa_tag(a)
    a.out = args.out or exp.pasta_experimento(
        nome, f"v3: vídeos (drone RGB/NDVI, mapa, Sentinel-2 RGB/NDVI, peso S2-A, pares e partículas) "
              f"de {args.voo} · {args.mapa}, seed {args.seed}, métodos {', '.join(args.metodos)}",
        continuar=args.continuar)
    a.seed = args.seed
    base, n_s = pc.build_tasks(a)
    notes += n_s
    if not base:
        sys.exit("Nenhuma execução montada; veja os avisos acima.")
    if not pc.ensure_odom(a):
        sys.exit("Odometria fixa indisponível.")
    stab = v3.raster_for("A", base[0]["map_path"], a, notes, dry_run=False)
    tasks = []
    for key in args.metodos:
        desc, s2, _ = METODOS[key]
        t = next((t for t in base if (t["desc"] == "abBRIEF") == (desc == "abBRIEF")), None)
        if t is None:
            continue
        t = dict(t)
        label = t["desc"] + ("_S2A" if s2 else "")
        t["out_dir"] = t["out_dir"].replace(f"{os.sep}{t['desc']}_regime_", f"{os.sep}{label}_regime_")
        cmd = list(t["cmd"])
        cmd[cmd.index("--output_dir") + 1] = t["out_dir"]
        cmd += ["--save_pairs", "--save_particles", "--export_particles", str(args.export_particles)]
        if s2:
            cmd += ["--stability_map", stab, "--stability_formula", "normalized"]
        t["cmd"], t["key"] = cmd, key
        tasks.append(t)
    print(f"Saída: {a.out}\n{len(tasks)} execuções da engine em paralelo (~200 s cada) ...", flush=True)
    for t in tasks:   # refaz se a pasta não tem os logs do vídeo
        if not os.path.exists(os.path.join(t["out_dir"], "pairs_log.csv")):
            shutil.rmtree(t["out_dir"], ignore_errors=True)
    with ThreadPoolExecutor(max_workers=len(tasks)) as ex:
        for t, status, dtime in ex.map(lambda t: pc.run_task(t, args.omp_threads, False), tasks):
            print(f"  {t['key']}: {status} ({dtime:.0f}s)", flush=True)
    return a.out, tasks, stab


# --------------------------------------------------------------------------- main

def parse_frames(s):
    if not s:
        return None
    a, b = s.split(":")
    return int(a), int(b)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--voo", choices=["ADM1", "ADM2", "ADM3"], help="voo (modo que roda a engine)")
    ap.add_argument("--mapa", default="01-2014", choices=["12-2018", "01-2014", "11-2016", "09-2017", "02-2014"])
    ap.add_argument("--metodos", nargs="+", default=["ab", "ab_s2a"], choices=list(METODOS))
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--protocolo", default="novo", choices=["novo", "artigo"])
    ap.add_argument("--regime", default=None)
    ap.add_argument("--lambda_alpha", type=float, default=0.5)
    ap.add_argument("--wmin", type=float, default=0.05)
    ap.add_argument("--lo", type=float, default=0.08)
    ap.add_argument("--hi", type=float, default=0.22)
    ap.add_argument("--particles", type=int, default=50000)
    ap.add_argument("--export_particles", type=int, default=2000)
    ap.add_argument("--omp_threads", type=int, default=2)
    ap.add_argument("--bin", default=None)
    ap.add_argument("--odom_dir", default=None)
    ap.add_argument("--out", default=None)
    ap.add_argument("--continuar", action="store_true", help="reaproveita a pasta de hoje com o mesmo nome")
    ap.add_argument("--frames", default=None, help="trecho do vídeo, ex.: 80:120 (padrão: o voo todo)")
    ap.add_argument("--fps", type=float, default=6.0)
    ap.add_argument("--png_cada", type=int, default=10, help="salva um quadro PNG a cada N frames (0 = nenhum)")
    ap.add_argument("--s2_data", default="2019-04-18", help="cena do Sentinel-2 dos painéis 4 e 5 (dia do voo)")
    ap.add_argument("--vistas", nargs="+", default=["voo", "drone", "mapa"], choices=["voo", "drone", "mapa"],
                    help="quais vídeos gerar (padrão: os três)")
    ap.add_argument("--fundo", default="mapa", choices=["mapa", "peso"],
                    help="fundo do mapa grande da vista 'voo': o mapa RGB ou o mapa misturado com o peso S2-A")
    ap.add_argument("--georef", default="ADM_35")
    # modo só renderização
    ap.add_argument("--run_dir", default=None, help="renderiza só esta execução (já rodada com --save_pairs)")
    ap.add_argument("--map", default=None, help="com --run_dir: o mapa usado")
    ap.add_argument("--stability_map", default=None, help="com --run_dir: o raster de peso a exibir")
    args = ap.parse_args()
    frames = parse_frames(args.frames)
    notes = []

    if args.run_dir:
        if not args.map:
            ap.error("--run_dir precisa de --map")
        out_dir = os.path.dirname(os.path.abspath(args.run_dir.rstrip("/")))
        r = Renderer(args.map, args.stability_map, args.s2_data, args.georef, notes)
        nome = os.path.basename(os.path.abspath(args.run_dir.rstrip("/")))
        fr, *_ = load_run(args.run_dir)
        gt = np.array([fr[f]["gt"][:2] for f in sorted(fr) if fr[f]["gt"]])
        if len(gt):
            r.area_figure(gt, os.path.join(out_dir, "area_sentinel.png"))
        outs = r.render(args.run_dir, nome, frames, os.path.join(out_dir, f"video_{nome}"),
                        os.path.join(out_dir, "quadros", nome), args.png_cada, args.fps, notes,
                        vistas=args.vistas, fundo=args.fundo)
        print("Vídeos:\n  " + "\n  ".join(outs))
        for n in notes:
            print("[aviso]", n)
        return

    if not args.voo:
        ap.error("informe --voo (ou --run_dir)")
    out, tasks, stab = run_engine(args, notes)
    r = Renderer(tasks[0]["map_path"], stab, args.s2_data, args.georef, notes)
    fr, *_ = load_run(tasks[0]["out_dir"])
    gt = np.array([fr[f]["gt"][:2] for f in sorted(fr) if fr[f]["gt"]])
    if len(gt):
        r.area_figure(gt, os.path.join(out, "area_sentinel.png"))
    for t in tasks:
        if not os.path.exists(os.path.join(t["out_dir"], "pairs_log.csv")):
            notes.append(f"{t['key']}: sem pairs_log.csv (a execução falhou? veja {t['out_dir']}/run.log)")
            continue
        print(f"Renderizando {t['key']} ...", flush=True)
        r.render(t["out_dir"], METODOS[t["key"]][2], frames, os.path.join(out, f"video_{t['key']}"),
                 os.path.join(out, "quadros", t["key"]), args.png_cada, args.fps, notes,
                 vistas=args.vistas, fundo=args.fundo)
    with open(os.path.join(out, "00_LEIA_ME.md"), "w") as f:
        f.write(f"# Vídeos v3 — {args.voo} · {args.mapa}, seed {args.seed}\n\n"
                f"Gerado por `{' '.join(['python3', 'scripts/v3_video.py'] + sys.argv[1:])}`.\n\n"
                "| Arquivo | O que é |\n|---|---|\n"
                + "".join(f"| `video_{t['key']}_voo.mp4` | {METODOS[t['key']][2]}: o voo no mapa com a nuvem de "
                          f"partículas convergindo + os 6 painéis ao lado |\n"
                          f"| `video_{t['key']}_drone.mp4` | {METODOS[t['key']][2]}: drone, mapa, S2 e peso na "
                          f"orientação da câmera (pose do GT) |\n"
                          f"| `video_{t['key']}_mapa.mp4` | {METODOS[t['key']][2]}: mapa, S2 e peso norte para cima, "
                          f"com pegadas e partículas |\n" for t in tasks)
                + "| `quadros/<método>/` | um quadro a cada N frames |\n"
                  "| `area_sentinel.png` | área inteira: mapa, S2 RGB e NDVI do dia do voo, peso S2-A, GT |\n\n"
                  "Painéis: 1 drone RGB (quantizado) + pares · 2 drone NDVI + pontos + máscara de amostragem · "
                  "3 mapa · 4 S2 RGB · 5 S2 NDVI · 6 peso S2-A + partículas. Pares coloridos pelo peso S2-A "
                  "no ponto do mapa (pose do GT); ponta branca/magenta = bit concorda/discorda do drone.\n"
                + ("\nAvisos:\n\n" + "".join(f"- {n}\n" for n in notes) if notes else ""))
    print(f"\nTudo em {out}")
    for n in notes:
        print("[aviso]", n)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
v3_comparison.py — abBRIEF e λ-BRIEF com e sem o peso de estabilidade do Sentinel-2 (v3)

A v3 pondera cada par de pixels pela estabilidade da região do mapa onde ele cai (--stability_map da
engine; raster de scripts/make_stability_map.py). Hipótese: o abBRIEF com o peso (ab+S2) chega perto do λ
no ADM2, onde a lavoura muda, sem a perda do λ no ADM3. Ver docs/MANUAL_TESTES_ARTIGO.md, seção 10.

Em cada voo ADM × mapa do artigo e para cada seed:

  ab          --alpha 0                                        (copiado do teste do artigo, se existir)
  λ           --alpha <α> + NDVI                               (idem)
  ab+S2A      --alpha 0   --stability_map insumos/estabilidade/ADM_35_A.png
  λ+S2A       --alpha <α> --stability_map insumos/estabilidade/ADM_35_A.png
  [--formulas coin]  as mesmas com --stability_formula coin   (ablação: par instável vale uma moeda)
  [--pesos A B]      ab+S2B / λ+S2B só nos mapas com cena S2 perto da data (no artigo: só o 12-2018)

Os rasters que faltam são gerados com make_stability_map.py (parâmetros padrão).

Uso (de dentro de lambda_brief_modern/)
  python3 scripts/v3_comparison.py --seeds 1 --dry_run
  nohup python3 scripts/v3_comparison.py --seeds 1 > v3.log 2>&1 &              # 30 execuções novas
  nohup python3 scripts/v3_comparison.py --seeds 1..5 > v3_5seeds.log 2>&1 &
  python3 scripts/v3_comparison.py --seeds 1 --formulas normalized coin --metodos_s2 ab
  python3 scripts/v3_comparison.py --seeds 1 --pesos A B
  python3 scripts/v3_comparison.py --seeds 1 --plot_only                          # refaz tabelas e figura
  # teste rápido (~4 min): só o ab+S2A, ADM2 e ADM3, 2 mapas cada = 4 execuções novas
  python3 scripts/v3_comparison.py --seeds 1 --flights ADM2 ADM3 --mapas 12-2018 01-2014 --metodos_s2 ab
  # peso mais suave: par instável vale no mínimo 0,5 (gera insumos/estabilidade/ADM_35_A_wmin0.5.png)
  python3 scripts/v3_comparison.py --seeds 1 --flights ADM2 ADM3 --mapas 12-2018 01-2014 --metodos_s2 ab --wmin 0.5

Saída: experimentos/<AAAA-MM-DD>_v3_<protocolo>_a<α>[_<N>seeds]/ (registrada no INDICE.md)
  00_LEIA_ME.md   o que foi rodado, resumo por voo e como ler
  tabela.md       MAE e convergência por mapa e por voo; ganhos contra o ab e o λ; Wilcoxon pareado
  tabela.csv      cada execução (voo, mapa, método, seed)
  compilado.png   erro por frame: uma linha por voo, uma coluna por mapa, uma curva por método
  execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_<N>/
"""

import argparse
import copy
import csv
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import paper_comparison as pc  # noqa: E402
import paper_comparison_seeds as pcs  # noqa: E402
import benchmark_runner as br  # noqa: E402
import experimentos as exp  # noqa: E402
from consolidate_benchmark import SURFACE, INK, INK2, GRID, style_axes  # noqa: E402

try:
    from scipy.stats import wilcoxon
except ImportError:  # a tabela sai sem o p-valor
    wilcoxon = None

ADM = ["ADM1", "ADM2", "ADM3"]
STAB_DIR = os.path.join(pc.MODERN_DIR, "insumos", "estabilidade")
MAKE = os.path.join(SCRIPT_DIR, "make_stability_map.py")
S2_COLOR = {"ab": "#c0392b", "lambda": "#123f8c"}   # tons mais escuros das cores do ab e do λ


# --------------------------------------------------------------------------- métodos

def variants(args):
    """[(sufixo, peso, fórmula)] das versões com S2, na ordem das tabelas."""
    out = []
    for p in args.pesos:
        for f in args.formulas:
            out.append((f"S2{p}" + ("" if f == "normalized" else f"_{f}"), p, f))
    return out


def metodos(args):
    """Lista de (chave, nome, cor, estilo, nome curto)."""
    lam = "λ" + ("" if abs(args.lambda_alpha - 0.5) < 1e-12 else f" (α={args.lambda_alpha:g})")
    m = [("abBRIEF", "abBRIEF", pc.AB_COLOR, ":", "ab"), ("lambda", f"λ-BRIEF{lam[1:]}", pc.LAMBDA_COLOR, "-", "λ")]
    styles = ["-", "--", "-.", (0, (5, 2, 1, 2))]
    for i, (suf, p, f) in enumerate(variants(args)):
        tag = f"+S2{p}" + ("" if f == "normalized" else f" {f}")
        if "ab" in args.metodos_s2:
            m.append((f"abBRIEF_{suf}", f"abBRIEF{tag}", S2_COLOR["ab"], styles[i % 4], f"ab{tag}"))
        if "lambda" in args.metodos_s2:
            m.append((f"lambda_{suf}", f"{lam}{tag}", S2_COLOR["lambda"], styles[i % 4], f"{lam}{tag}"))
    return m


def rampa_tag(args):
    """Sufixo do raster e da pasta quando a rampa do peso não é a padrão (lo 0,08, hi 0,22, wmin 0,05)."""
    t = ""
    if abs(args.lo - 0.08) > 1e-12 or abs(args.hi - 0.22) > 1e-12:
        t += f"_lo{args.lo:g}_hi{args.hi:g}"
    if abs(args.wmin - 0.05) > 1e-12:
        t += f"_wmin{args.wmin:g}"
    return t


def raster_for(peso, map_path, args, notes, dry_run):
    """Caminho do raster (gerando-o se faltar) ou None se não existe para esse mapa."""
    rampa = rampa_tag(args)
    if peso == "A":
        name = "ADM_35_A" + rampa
        extra = ["--modo", "A", "--lo", f"{args.lo:g}", "--hi", f"{args.hi:g}", "--wmin", f"{args.wmin:g}"]
    else:
        dm = pc_map_date(map_path)
        name = f"ADM_35_B_{dm}_{args.data_voo}" + rampa
        extra = ["--modo", "B", "--mapa", map_path, "--data_voo", args.data_voo, "--wmin", f"{args.wmin:g}"]
    png = os.path.join(STAB_DIR, name + ".png")
    if os.path.exists(png):
        return png
    if args._sem_b.get(name):   # a variante B já falhou para este mapa
        return None
    cmd = [sys.executable, MAKE] + extra + ["--nome", name]
    if dry_run:
        if name not in args._sem_b:
            args._sem_b[name] = False
            print("[gera]", " ".join(cmd))
        return png
    print("[gera]", " ".join(cmd), flush=True)
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    if r.returncode != 0 or not os.path.exists(png):
        msg = (r.stderr or r.stdout).strip().splitlines()[-1:] or ["erro"]
        if peso == "B":
            args._sem_b[name] = True
            notes.append(f"S2B sem raster para {os.path.basename(map_path)}: {msg[0]}")
            return None
        sys.exit(f"Falhou ao gerar {png}: {msg[0]}")
    return png


def pc_map_date(map_path):
    import re
    m = re.search(r"(\d{2})_(\d{2})_(\d{4})", os.path.basename(map_path))
    return f"{m.group(3)}-{m.group(2)}-{m.group(1)}" if m else "sem_data"


def build_all_tasks(args, dry_run):
    tasks, notes = [], []
    args._sem_b = {}
    for s in args.seeds:
        a = copy.copy(args)
        a.seed = s
        base, n_s = pc.build_tasks(a)
        for n in n_s:
            if n not in notes:
                notes.append(n)
        for t in base:
            t["seed"] = s
            kind = "ab" if t["desc"] == "abBRIEF" else "lambda"
            t["key"] = "abBRIEF" if kind == "ab" else "lambda"
            tasks.append(t)
            if kind not in args.metodos_s2:
                continue
            for suf, peso, formula in variants(args):
                png = raster_for(peso, t["map_path"], args, notes, dry_run)
                if png is None:
                    continue
                label = f"{t['desc']}_{suf}"
                nt = dict(t)
                nt["desc"], nt["key"] = label, f"{t['key']}_{suf}"
                nt["out_dir"] = t["out_dir"].replace(f"{os.sep}{t['desc']}_regime_", f"{os.sep}{label}_regime_")
                cmd = list(t["cmd"])
                cmd[cmd.index("--output_dir") + 1] = nt["out_dir"]
                nt["cmd"] = cmd + ["--stability_map", png, "--stability_formula", formula]
                tasks.append(nt)
    return tasks, notes


def source_dir_for(args):
    if args.sem_reaproveitar:
        return None
    if args.reaproveitar:
        p = exp.resolve(args.reaproveitar)
        return p if os.path.isdir(p) else None
    base = f"artigo_{args.protocolo}_a{args.lambda_alpha:g}"
    return exp._ultima(f"{base}_{args.n_seeds}seeds") if args.n_seeds > 1 else exp._ultima(base) or \
        exp._ultima(f"{base}_5seeds")


# --------------------------------------------------------------------------- resultados

def collect(tasks, args):
    runs, rows = {}, []
    for t in tasks:
        r = pc.evaluate(t, args.gt)
        if r is None:
            continue
        runs.setdefault((t["flight"], t["label"], t["key"]), []).append((t["seed"], r))
        rows.append({"voo": t["flight"], "mapa": t["label"], "metodo": t["key"], "seed": t["seed"],
                     "mae_m": round(r["m"]["mae"], 2),
                     "frame_conv": (int(r["frames"][r["m"]["ttf"]]) if r["m"]["ttf"] is not None else ""),
                     "rastreio_m": (round(r["m"]["track_median"], 2) if r["m"]["track_median"] is not None else ""),
                     "final_m": round(r["m"]["final"], 2)})
    agg = {k: pcs.aggregate(sorted(v, key=lambda x: x[0])) for k, v in runs.items()}
    return agg, rows


def fmt(v, nd=1, sign=False):
    if v is None:
        return "—"
    return f"{v:+.{nd}f}" if sign else f"{v:.{nd}f}"


def paired(agg, flights, args, k_new, k_ref):
    """MAE pareado (mapa, seed) entre dois métodos: (n, mediana da diferença ref − novo, p de Wilcoxon)."""
    d = []
    for flight in flights:
        for lbl, _ in pc.flight_maps(flight, args):
            a, b = agg.get((flight, lbl, k_new)), agg.get((flight, lbl, k_ref))
            if a is None or b is None:
                continue
            for s, v in a["mae_by_seed"].items():
                if s in b["mae_by_seed"]:
                    d.append(b["mae_by_seed"][s] - v)
    if not d:
        return 0, None, None
    p = None
    if wilcoxon is not None and len(d) >= 5 and any(abs(x) > 1e-9 for x in d):
        try:
            p = float(wilcoxon(d).pvalue)
        except ValueError:
            p = None
    return len(d), float(np.median(d)), p


def write_tables(agg, rows, args, out):
    met = metodos(args)
    keys = [m[0] for m in met]
    names = {m[0]: m[4] for m in met}
    flights = [f for f in args.flights if any(k[0] == f for k in agg)]
    with open(os.path.join(out, "tabela.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["voo", "mapa", "metodo", "seed", "mae_m", "frame_conv", "rastreio_m",
                                          "final_m"])
        w.writeheader()
        w.writerows(rows)

    L = [f"# v3: peso de estabilidade do Sentinel-2 — {args.n_seeds} seed(s) ({args.seeds_str})", "",
         f"Protocolo {args.protocolo} · regime {args.regime} · λ com α = {args.lambda_alpha:g} · GT {args.gt} "
         "(ADM3: GT do Zenodo). Erros em metros.", "",
         "- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma).",
         "- **Conv.** = execuções em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame "
         "mediano de convergência.",
         "- **Ganho** = MAE do método de referência − MAE do método com S2: positivo = o S2 ajudou.",
         "- **Nomes:** " + "; ".join(f"{m[4]} = {m[1]}" for m in met) + ".", ""]

    def mae(x):
        if x is None:
            return "—"
        return f"{x['mae_mean']:.1f}" + (f" ± {x['mae_sd']:.1f}" if x["n"] > 1 else "")

    def conv(x):
        if x is None:
            return "—"
        return f"{x['n_conv']}/{x['n']}" + (f" ({x['conv_med']:.0f})" if x["conv_med"] is not None else "")

    # por mapa
    L += ["## Por mapa", "",
          "| Voo | Mapa | " + " | ".join(f"MAE {names[k]}" for k in keys) + " | "
          + " | ".join(f"Conv. {names[k]}" for k in keys) + " |",
          "|" + "---|" * (2 + 2 * len(keys))]
    for flight in flights:
        for lbl, _ in pc.flight_maps(flight, args):
            g = {k: agg.get((flight, lbl, k)) for k in keys}
            if not any(g.values()):
                continue
            L.append(f"| {flight} | {lbl} | " + " | ".join(mae(g[k]) for k in keys) + " | "
                     + " | ".join(conv(g[k]) for k in keys) + " |")

    # por voo
    L += ["", "## Por voo (média dos mapas)", "",
          "| Voo | " + " | ".join(f"MAE {names[k]}" for k in keys) + " | "
          + " | ".join(f"Conv. {names[k]}" for k in keys) + " |",
          "|" + "---|" * (1 + 2 * len(keys))]
    resumo = {}
    for flight in flights:
        m, c = {}, {}
        for k in keys:
            gs = [agg[(flight, lbl, k)] for lbl, _ in pc.flight_maps(flight, args) if (flight, lbl, k) in agg]
            m[k] = float(np.mean([g["mae_mean"] for g in gs])) if gs else None
            c[k] = (sum(g["n_conv"] for g in gs), sum(g["n"] for g in gs)) if gs else None
        resumo[flight] = (m, c)
        L.append(f"| {flight} | " + " | ".join(fmt(m[k]) for k in keys) + " | "
                 + " | ".join(f"{c[k][0]}/{c[k][1]}" if c[k] else "—" for k in keys) + " |")

    # ganhos e Wilcoxon
    L += ["", "## Ganho do S2 (MAE pareado por mapa e seed)", "",
          "Mediana de (MAE da referência − MAE com S2) e p de Wilcoxon bilateral. Com 1 seed são 5 pares por voo "
          "(o menor p possível é 0,06): leia como indício.", "",
          "| Comparação | " + " | ".join(flights) + " | Todos os voos |", "|" + "---|" * (len(flights) + 2)]
    for suf, _, _ in variants(args):
        comps = []
        if "ab" in args.metodos_s2:
            comps += [(f"abBRIEF_{suf}", "abBRIEF"), (f"abBRIEF_{suf}", "lambda")]
        if "lambda" in args.metodos_s2:
            comps += [(f"lambda_{suf}", "lambda")]
        for k_new, k_ref in comps:
            if k_new not in names:
                continue
            cells = []
            for fl in flights + [flights]:
                n, med, p = paired(agg, fl if isinstance(fl, list) else [fl], args, k_new, k_ref)
                cells.append("—" if n == 0 else f"{med:+.1f} m (n={n}" + (f", p={p:.2f})" if p is not None else ")"))
            L.append(f"| {names[k_new]} vs {names[k_ref]} | " + " | ".join(cells) + " |")

    # critério do artigo
    ab_s2 = next((k for k in keys if k.startswith("abBRIEF_S2")), None)
    if ab_s2:
        L += ["", f"## Critério mínimo: {names[ab_s2]} converge pelo menos tanto quanto o melhor de ab e λ", "",
              "| Voo | Conv. ab | Conv. λ | Conv. " + names[ab_s2] + " | Atende? |", "|---|---|---|---|---|"]
        for flight in flights:
            m, c = resumo[flight]
            if not (c.get("abBRIEF") and c.get("lambda") and c.get(ab_s2)):
                continue
            ok = c[ab_s2][0] >= max(c["abBRIEF"][0], c["lambda"][0])
            L.append(f"| {flight} | {c['abBRIEF'][0]}/{c['abBRIEF'][1]} | {c['lambda'][0]}/{c['lambda'][1]} | "
                     f"{c[ab_s2][0]}/{c[ab_s2][1]} | {'sim' if ok else 'não'} |")
    with open(os.path.join(out, "tabela.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    return L


def fig_compilado(agg, args, out):
    met = metodos(args)
    flights = [f for f in args.flights if any(k[0] == f for k in agg)]
    if not flights:
        return
    ncol = max(len(pc.flight_maps(f, args)) for f in flights)
    fig, axes = plt.subplots(len(flights), ncol, figsize=(4.4 * ncol, 3.6 * len(flights) + 0.8), facecolor=SURFACE,
                             squeeze=False)
    for i, flight in enumerate(flights):
        for j, (lbl, _) in enumerate(pc.flight_maps(flight, args)):
            ax = axes[i][j]
            style_axes(ax)
            for key, name, color, ls, _ in met:
                g = agg.get((flight, lbl, key))
                if g is None:
                    continue
                ax.plot(g["frames"], g["mean"], linestyle=ls, color=color, lw=1.6,
                        label=f"{name}: {g['mae_mean']:.0f} m")
                if g["n"] > 1:
                    ax.fill_between(g["frames"], g["lo"], g["hi"], color=color, alpha=0.08, lw=0)
            ax.set_yscale("log")
            ax.axhline(pc.CONV_THRESH, color=GRID, lw=1, zorder=0)
            ax.set_title(f"{flight} · {lbl}", color=INK, fontsize=11)
            if j == 0:
                ax.set_ylabel("erro (m)", color=INK2)
            if i == len(flights) - 1:
                ax.set_xlabel("frame", color=INK2)
            ax.legend(fontsize=7.5, frameon=False, loc="upper right")
    fig.suptitle(f"v3: com e sem o peso de estabilidade do Sentinel-2 — protocolo {args.protocolo}, "
                 f"{args.n_seeds} seed(s); na legenda, o MAE", color=INK, fontsize=13)
    fig.tight_layout(rect=(0, 0, 1, 0.97))
    fig.savefig(os.path.join(out, "compilado.png"), dpi=110, facecolor=SURFACE)
    plt.close(fig)


def write_readme(args, notes, n_reused, src, resumo):
    met = metodos(args)
    L = ["# v3: abBRIEF e λ-BRIEF com o peso de estabilidade do Sentinel-2", "",
         f"Protocolo **{args.protocolo}** (regime `{args.regime}`), λ com α = {args.lambda_alpha:g}, "
         f"seeds {args.seeds_str}, GT {args.gt} (ADM3: GT do Zenodo).", "",
         "**Pergunta:** dar menos peso aos pares que caem onde o mapa costuma mudar (desvio do NDVI do S2 ao longo "
         "dos anos) traz o abBRIEF para perto do λ no ADM2 sem perder no ADM3?", "",
         "| Método | Opções da engine |", "|---|---|",
         "| abBRIEF | `--alpha 0` |", f"| λ-BRIEF | `--alpha {args.lambda_alpha:g}` + NDVI |"]
    for suf, p, f in variants(args):
        r = (f"insumos/estabilidade/ADM_35_A{rampa_tag(args)}.png" if p == "A"
             else f"insumos/estabilidade/ADM_35_B_<mapa>_<voo>{rampa_tag(args)}.png")
        for kind in args.metodos_s2:
            base = "--alpha 0" if kind == "ab" else f"--alpha {args.lambda_alpha:g}"
            nome = next(m[1] for m in met if m[0] == ("abBRIEF" if kind == "ab" else "lambda") + "_" + suf)
            L.append(f"| {nome} | `{base} --stability_map {r} --stability_formula {f}` |")
    L += ["", "Arquivos: `tabela.md` (por mapa, por voo, ganhos com Wilcoxon e o critério mínimo), `tabela.csv`, "
          "`compilado.png`, `execucoes/`. Os parâmetros de cada raster estão no `.json` ao lado dele, em "
          "`insumos/estabilidade/`.", ""]
    if n_reused:
        L += [f"{n_reused} execuções do abBRIEF e do λ foram copiadas de `{os.path.relpath(src, pc.MODERN_DIR)}` "
              "(comando idêntico; a engine é determinística para a mesma seed).", ""]
    L += ["## Resumo por voo", ""] + resumo
    if notes:
        L += ["", "Avisos:", ""] + [f"- {n}" for n in notes]
    with open(os.path.join(args.out, "00_LEIA_ME.md"), "w") as f:
        f.write("\n".join(L) + "\n")


# --------------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--protocolo", default="novo", choices=list(pc.PROTOCOLS))
    ap.add_argument("--seeds", default="1", help="ex.: 1, 1..5 (padrão 1)")
    ap.add_argument("--pesos", nargs="+", default=["A"], choices=["A", "B"], help="variantes do peso (padrão A)")
    ap.add_argument("--formulas", nargs="+", default=["normalized"], choices=["normalized", "coin"])
    ap.add_argument("--metodos_s2", nargs="+", default=["ab", "lambda"], choices=["ab", "lambda"],
                    help="a quais métodos aplicar o peso (padrão: os dois)")
    ap.add_argument("--data_voo", default="2019-04-18", help="data do voo para a variante B")
    ap.add_argument("--wmin", type=float, default=0.05,
                    help="piso do peso no raster (padrão 0,05). Ex.: 0.5 = par instável vale metade")
    ap.add_argument("--lo", type=float, default=0.08, help="variante A: desvio até o qual o peso é 1 (padrão 0,08)")
    ap.add_argument("--hi", type=float, default=0.22, help="variante A: desvio em que o peso chega ao piso (padrão 0,22)")
    ap.add_argument("--flights", nargs="*", default=ADM, choices=ADM)
    ap.add_argument("--mapas", nargs="+", default=None, choices=[m[0] for m in pc.ADM_MAPS],
                    help="só estes mapas (ex.: 12-2018 01-2014); padrão: os 5. Para um teste rápido")
    ap.add_argument("--regime", default=None, choices=["vanilla", "final_baro", "final_noalt"])
    ap.add_argument("--lambda_alpha", type=float, default=0.5, help="α do λ (padrão 0,5)")
    ap.add_argument("--gt", default=None, choices=["original", "sync"])
    ap.add_argument("--particles", type=int, default=50000)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--omp_threads", type=int, default=2)
    ap.add_argument("--adm_map_12_2018", default="ADM_35_17_12_2018.jpg")
    ap.add_argument("--adm_map_01_2014", default="ADM_35_27_01_2014.jpg")
    ap.add_argument("--bin", default=br.DEFAULT_BIN)
    ap.add_argument("--odom_dir", default=br.DEFAULT_ODOM_DIR)
    ap.add_argument("--out", default=None)
    ap.add_argument("--nome", default=None, help="nome do experimento (padrão: v3_<protocolo>_a<α>)")
    ap.add_argument("--continuar", action="store_true")
    ap.add_argument("--reaproveitar", default=None,
                    help="teste do artigo de onde copiar ab e λ (padrão: o mais recente com o mesmo protocolo e α)")
    ap.add_argument("--sem_reaproveitar", action="store_true")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--plot_only", action="store_true")
    ap.add_argument("--dry_run", action="store_true")
    args = ap.parse_args()

    p = pc.PROTOCOLS[args.protocolo]
    args.regime = args.regime or p["regime"]
    args.gt = args.gt or p["gt"]
    args.seeds = br.parse_seeds(args.seeds)
    if not args.seeds:
        ap.error("--seeds vazio")
    args.n_seeds = len(args.seeds)
    s = args.seeds
    args.seeds_str = (f"{s[0]}..{s[-1]}" if s == list(range(s[0], s[-1] + 1)) and len(s) > 1
                      else ",".join(map(str, s)))
    args.seed = s[0]

    suf = "" if args.pesos == ["A"] else "_" + "".join(args.pesos)
    suf += rampa_tag(args)
    suf += "" if args.formulas == ["normalized"] else "_" + "_".join(args.formulas)
    if args.mapas:
        _todos = pc.flight_maps
        pc.flight_maps = lambda f, a: [m for m in _todos(f, a) if m[0] in args.mapas]
        suf += "_rapido"
    descricao = (f"v3: ab e λ com e sem o peso de estabilidade do Sentinel-2 (--stability_map; pesos "
                 f"{'+'.join(args.pesos)}, fórmulas {'+'.join(args.formulas)}), ADM1–3 × 5 mapas, "
                 f"{args.n_seeds} seed(s) ({args.seeds_str}); protocolo {args.protocolo}, λ α={args.lambda_alpha:g}. "
                 "Comece por `00_LEIA_ME.md`")
    if args.out is None:
        nome = args.nome or (f"v3_{args.protocolo}_a{args.lambda_alpha:g}{suf}"
                             + (f"_{args.n_seeds}seeds" if args.n_seeds > 1 else ""))
        args.out = exp.pasta_experimento(nome, descricao, continuar=args.continuar or args.plot_only,
                                         dry_run=args.dry_run)
    else:
        args.out = exp.resolve(args.out)
        if not args.dry_run and os.path.abspath(args.out).startswith(exp.EXP_DIR + os.sep):
            exp.registrar(args.out, descricao)

    tasks, notes = build_all_tasks(args, dry_run=args.dry_run or args.plot_only)
    src = source_dir_for(args)
    print(f"Protocolo {args.protocolo}: regime {args.regime}, λ α = {args.lambda_alpha:g}, seeds {args.seeds_str}, "
          f"pesos {'+'.join(args.pesos)}, fórmulas {'+'.join(args.formulas)}\nSaída: {args.out}")
    if src:
        print(f"Reaproveita o abBRIEF e o λ de: {src}")
    for n in notes:
        print("[aviso]", n)

    if args.dry_run:
        pc.ensure_odom(args)
        n_reuse = pcs.reuse_existing(tasks, src, args.out, dry_run=True)
        todo = [t for t in tasks if not os.path.exists(os.path.join(t["out_dir"], "errors.txt"))]
        n_run = max(0, len(todo) - n_reuse)
        print(f"\n{len(tasks)} execuções no total; {len(tasks) - len(todo)} já na pasta, {n_reuse} seriam copiadas, "
              f"{n_run} rodariam (~{n_run * 200 / args.jobs / 60:.0f} min com --jobs {args.jobs}).")
        ex = next((t for t in tasks if "_S2" in t["key"]), None)
        if ex:
            print("Exemplo (com S2):\n  " + " ".join(ex["cmd"]))
        return

    n_reused = 0
    if not args.plot_only:
        if not pc.ensure_odom(args):
            print("Odometria fixa indisponível; veja as mensagens acima.")
            sys.exit(1)
        n_reused = pcs.reuse_existing(tasks, src, args.out, dry_run=False)
        if n_reused:
            print(f"{n_reused} execuções copiadas (mesmo comando).", flush=True)
        print(f"{len(tasks)} execuções, {args.jobs} em paralelo (as que já existem são puladas) ...", flush=True)
        t0 = time.time()
        with ThreadPoolExecutor(max_workers=args.jobs) as ex:
            futs = [ex.submit(pc.run_task, t, args.omp_threads, args.force) for t in tasks]
            for i, f in enumerate(as_completed(futs), 1):
                t, status, dtime = f.result()
                print(f"[{i}/{len(tasks)}] seed {t['seed']} {t['flight']} mapa {t['label']} {t['desc']}: "
                      f"{status} ({dtime:.0f}s; total {(time.time() - t0) / 60:.0f} min)", flush=True)
    else:
        n_reused = sum(os.path.exists(os.path.join(t["out_dir"], "REAPROVEITADO_DE.txt")) for t in tasks)

    agg, rows = collect(tasks, args)
    L = write_tables(agg, rows, args, args.out)
    fig_compilado(agg, args, args.out)
    i = L.index("## Por voo (média dos mapas)")
    j = next((k for k in range(i + 1, len(L)) if L[k].startswith("## ")), len(L))
    write_readme(args, notes, n_reused, src, L[i + 2:j])
    print(f"\nTudo em {args.out} (comece por 00_LEIA_ME.md)")


if __name__ == "__main__":
    main()

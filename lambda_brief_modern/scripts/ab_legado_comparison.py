#!/usr/bin/env python3
"""
ab_legado_comparison.py — quanto do ganho do λ-BRIEF no artigo vinha do baseline abBRIEF do legado

No código legado (phir2frameworkv2), o abBRIEF não era o λ-BRIEF com α = 0: ele usava um padrão de 256
pares FIXO e quase UNIFORME (BriefHeuristic.cpp:1132–1154; cv::RNG com a semente padrão e a variância
passada como desvio-padrão), enquanto o λ concentrava os pares no centro, re-sorteava a cada frame e usava
o NDVI. Ver docs/achados_codigo_legado.md.

Este script roda, em cada voo × mapa do artigo e para cada seed:

  abBRIEF legado   --alpha 0 --pair_pattern legacy_ab   (o abBRIEF do artigo)
  abBRIEF          --alpha 0                            (o abBRIEF da engine: gaussiana central, re-sorteado)
  λ-BRIEF          --alpha <α> + NDVI

e separa a diferença "abBRIEF legado → λ" (a comparação do artigo) em duas partes:

  baseline = MAE(ab legado) − MAE(ab)     o que vinha só de trocar o baseline
  NDVI     = MAE(ab) − MAE(λ)             o que é da vegetação

Com --variantes uniform roulette_fixed entram também:
  abBRIEF uniforme  --pair_pattern uniform         pares uniformes, re-sorteados a cada frame
  abBRIEF fixo      --pair_pattern roulette_fixed  gaussiana central, sorteada uma vez e mantida
e a diferença do baseline é quebrada em "re-sortear" (legado → uniforme) e "centralizar" (uniforme → ab).

As execuções do abBRIEF e do λ que já existem num teste do artigo com o mesmo protocolo, α e seeds
(experimentos/*_artigo_<protocolo>_a<α>[_<N>seeds]) são copiadas, desde que o comando seja idêntico; só as
variantes novas rodam.

Uso (de dentro de lambda_brief_modern/)
  # 1 seed, protocolo novo, α = 1: reaproveita o ab e o λ de 17/10 e roda só as 20 do ab legado
  python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 --dry_run
  nohup python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 > ab_legado.log 2>&1 &

  # protocolo do artigo (sem altímetro, odometria do legado), 1 seed: 60 execuções
  python3 scripts/ab_legado_comparison.py --protocolo artigo --seeds 1

  # decomposição completa (re-sorteio × centro × NDVI)
  python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 \\
          --variantes legacy_ab uniform roulette_fixed

Saída: experimentos/<AAAA-MM-DD>_ab_legado_<protocolo>_a<α>[_<N>seeds]/ (registrada no INDICE.md)
  00_LEIA_ME.md          o que foi rodado e como ler
  tabela.md              MAE, convergência e decomposição por voo e por mapa
  tabela.csv             MAE de cada execução (voo, mapa, método, seed)
  compilado.png          erro por frame: uma linha por voo, uma coluna por mapa, uma curva por método
  execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_<N>/   saída bruta da engine
"""

import argparse
import copy
import csv
import os
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

ORDER = pc.ORDER
# variante -> (rótulo da pasta, nome nas figuras, cor, estilo, nome curto nas tabelas)
VARIANTES = {
    "legacy_ab": ("abBRIEF_legado", "abBRIEF legado (fixo, uniforme)", "#6b6f76", "--", "ab legado"),
    "uniform": ("abBRIEF_uniforme", "abBRIEF uniforme (re-sorteado)", "#b23ab0", "-.", "ab uniforme"),
    "roulette_fixed": ("abBRIEF_fixo", "abBRIEF central fixo", "#1f9d55", (0, (5, 2, 1, 2)), "ab central fixo"),
}
AB = ("abBRIEF", "abBRIEF (engine)", pc.AB_COLOR, ":", "ab")


def lam_name(args):
    return "λ-BRIEF" + ("" if abs(args.lambda_alpha - 0.5) < 1e-12 else f" (α={args.lambda_alpha:g})")


def metodos(args):
    """Lista (chave, nome, cor, estilo, nome curto) na ordem da decomposição: legado → … → ab → λ."""
    m = [VARIANTES[v] for v in ("legacy_ab", "uniform", "roulette_fixed") if v in args.variantes]
    m.append(AB)
    m.append(("lambda", lam_name(args), pc.LAMBDA_COLOR, "-", "λ"))
    return m


# --------------------------------------------------------------------------- execuções

def build_all_tasks(args):
    tasks, notes = [], []
    for s in args.seeds:
        a = copy.copy(args)
        a.seed = s
        base, n_s = pc.build_tasks(a)
        for n in n_s:
            if n not in notes:
                notes.append(n)
        for t in base:
            t["seed"] = s
            t["key"] = "abBRIEF" if t["desc"] == "abBRIEF" else "lambda"
            tasks.append(t)
            if t["desc"] != "abBRIEF":
                continue
            for v in args.variantes:
                label = VARIANTES[v][0]
                nt = dict(t)
                nt["desc"], nt["key"] = label, label
                nt["out_dir"] = t["out_dir"].replace(f"{os.sep}abBRIEF_regime_", f"{os.sep}{label}_regime_")
                cmd = list(t["cmd"])
                cmd[cmd.index("--output_dir") + 1] = nt["out_dir"]
                nt["cmd"] = cmd + ["--pair_pattern", v]
                tasks.append(nt)
    return tasks, notes


def source_dir_for(args):
    if args.sem_reaproveitar:
        return None
    if args.reaproveitar:
        p = exp.resolve(args.reaproveitar)
        return p if os.path.isdir(p) else None
    base = f"artigo_{args.protocolo}_a{args.lambda_alpha:g}"
    return exp._ultima(base if args.n_seeds == 1 else f"{base}_{args.n_seeds}seeds") or exp._ultima(base)


# --------------------------------------------------------------------------- resultados

def collect(tasks, args):
    """agg[(voo, mapa, chave)] = pcs.aggregate(...) e linhas por execução para o CSV."""
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


def fmt(v, nd=1):
    return "—" if v is None else f"{v:.{nd}f}"


def write_tables(agg, rows, args, out):
    met = metodos(args)
    keys = [m[0] for m in met]
    with open(os.path.join(out, "tabela.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["voo", "mapa", "metodo", "seed", "mae_m", "frame_conv", "rastreio_m",
                                          "final_m"])
        w.writeheader()
        w.writerows(rows)

    L = [f"# abBRIEF legado × abBRIEF × {lam_name(args)} — {args.n_seeds} seed(s) ({args.seeds_str})", "",
         f"Protocolo {args.protocolo} · regime {args.regime} · GT {args.gt} (ADM3: GT do Zenodo). Erros em metros.",
         "",
         "- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma seed).",
         "- **Conv.** = seeds em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame "
         "mediano de convergência.",
         "- **Baseline** = MAE(ab legado) − MAE(ab): o que muda só por trocar o abBRIEF do legado pelo da engine.",
         "- **NDVI** = MAE(ab) − MAE(λ): o efeito da vegetação.",
         "- **Baseline / total** = que fração da diferença ab legado → λ (a comparação do artigo) vem do baseline. "
         "Só faz sentido quando a diferença total é positiva e grande; com 1 seed, leia como indício.",
         "- **Nomes:** " + "; ".join(f"{m[4]} = {m[1]}" for m in met) + ".", ""]
    names = {m[0]: m[4] for m in met}

    # por mapa
    head = "| Voo | Mapa | " + " | ".join(f"MAE {names[k]}" for k in keys) + " | " + \
           " | ".join(f"Conv. {names[k]}" for k in keys) + " | Baseline | NDVI |"
    L += ["## Por mapa", "", head, "|" + "---|" * (2 + 2 * len(keys) + 2)]
    for flight in args.flights:
        for lbl, _ in pc.flight_maps(flight, args):
            g = {k: agg.get((flight, lbl, k)) for k in keys}
            if not any(g.values()):
                continue

            def mae(x):
                if x is None:
                    return "—"
                return f"{x['mae_mean']:.1f}" + (f" ± {x['mae_sd']:.1f}" if x["n"] > 1 else "")

            def conv(x):
                if x is None:
                    return "—"
                return f"{x['n_conv']}/{x['n']}" + (f" ({x['conv_med']:.0f})" if x["conv_med"] is not None else "")
            leg, ab, lam = g.get("abBRIEF_legado"), g.get("abBRIEF"), g.get("lambda")
            d_base = leg["mae_mean"] - ab["mae_mean"] if leg and ab else None
            d_ndvi = ab["mae_mean"] - lam["mae_mean"] if ab and lam else None
            L.append(f"| {flight} | {lbl} | " + " | ".join(mae(g[k]) for k in keys) + " | " +
                     " | ".join(conv(g[k]) for k in keys) + f" | {fmt(d_base)} | {fmt(d_ndvi)} |")

    # por voo
    L += ["", "## Por voo (média dos 5 mapas)", ""]
    cols = ["Voo"] + [f"MAE {names[k]}" for k in keys] + ["ab legado → λ (artigo)", "Baseline", "NDVI",
                                                            "Baseline / total"]
    if "uniform" in args.variantes:
        cols.insert(len(keys) + 2, "Re-sortear (legado → uniforme)")
        cols.insert(len(keys) + 3, "Centralizar (uniforme → ab)")
    L += ["| " + " | ".join(cols) + " |", "|" + "---|" * len(cols)]
    for flight in args.flights:
        m = {}
        for k in keys:
            v = [agg[(flight, lbl, k)]["mae_mean"] for lbl, _ in pc.flight_maps(flight, args)
                 if (flight, lbl, k) in agg]
            m[k] = float(np.mean(v)) if v else None
        if m.get("abBRIEF") is None or m.get("lambda") is None:
            continue
        leg = m.get("abBRIEF_legado")
        tot = leg - m["lambda"] if leg is not None else None
        base = leg - m["abBRIEF"] if leg is not None else None
        ndvi = m["abBRIEF"] - m["lambda"]
        frac = f"{100 * base / tot:.0f}%" if tot is not None and tot > 1.0 else "—"
        cells = [flight] + [fmt(m[k]) for k in keys] + [fmt(tot), fmt(base), fmt(ndvi), frac]
        if "uniform" in args.variantes:
            uni = m.get("abBRIEF_uniforme")
            cells.insert(len(keys) + 2, fmt(leg - uni if leg is not None and uni is not None else None))
            cells.insert(len(keys) + 3, fmt(uni - m["abBRIEF"] if uni is not None else None))
        L.append("| " + " | ".join(cells) + " |")
    L += ["", "Valores positivos = o método da direita erra menos. Ex.: Baseline 20 = o abBRIEF da engine erra "
          "20 m a menos que o abBRIEF do legado."]
    with open(os.path.join(out, "tabela.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    return L


def fig_compilado(agg, args, out):
    met = metodos(args)
    flights = [f for f in args.flights if any(k[0] == f for k in agg)]
    if not flights:
        return
    fig, axes = plt.subplots(len(flights), 5, figsize=(22, 3.6 * len(flights) + 0.8), facecolor=SURFACE,
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
    fig.suptitle(f"abBRIEF do legado × abBRIEF da engine × {lam_name(args)} — protocolo {args.protocolo}, "
                 f"{args.n_seeds} seed(s); na legenda, o MAE", color=INK, fontsize=13)
    fig.tight_layout(rect=(0, 0, 1, 0.97))
    fig.savefig(os.path.join(out, "compilado.png"), dpi=110, facecolor=SURFACE)
    plt.close(fig)


def write_readme(args, notes, n_reused, src, resumo):
    L = [f"# abBRIEF do legado × abBRIEF da engine × {lam_name(args)}", "",
         f"Protocolo **{args.protocolo}** (regime `{args.regime}`), λ com α = {args.lambda_alpha:g}, "
         f"seeds {args.seeds_str}, GT {args.gt} (ADM3: GT do Zenodo).", "",
         "**Pergunta:** quanto da vantagem do λ no artigo vinha do baseline? No legado, o abBRIEF usava um padrão "
         "de 256 pares fixo e quase uniforme (`phir2frameworkv2/BriefHeuristic.cpp:1132–1154`); aqui ele é "
         "reproduzido com `--pair_pattern legacy_ab` (mesmo `cv::RNG` na semente padrão, mesmas expressões). "
         "Detalhes em `docs/achados_codigo_legado.md`.", "",
         "| Método | Opções da engine |", "|---|---|",
         "| abBRIEF legado | `--alpha 0 --pair_pattern legacy_ab` |"]
    if "uniform" in args.variantes:
        L.append("| abBRIEF uniforme | `--alpha 0 --pair_pattern uniform` |")
    if "roulette_fixed" in args.variantes:
        L.append("| abBRIEF central fixo | `--alpha 0 --pair_pattern roulette_fixed` |")
    L += ["| abBRIEF (engine) | `--alpha 0` |", f"| {lam_name(args)} | `--alpha {args.lambda_alpha:g}` + NDVI |", "",
          "Arquivos: `tabela.md` (por mapa e por voo, com a decomposição), `tabela.csv` (cada execução), "
          "`compilado.png` (erro por frame), `execucoes/` (saída bruta).", ""]
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
    ap.add_argument("--protocolo", default="artigo", choices=list(pc.PROTOCOLS))
    ap.add_argument("--seeds", default="1", help="ex.: 1, 1..5 (padrão 1)")
    ap.add_argument("--variantes", nargs="+", default=["legacy_ab"], choices=list(VARIANTES),
                    help="padrões de pares do abBRIEF a comparar (padrão: legacy_ab)")
    ap.add_argument("--flights", nargs="*", default=ORDER, choices=ORDER)
    ap.add_argument("--regime", default=None, choices=["vanilla", "final_baro", "final_noalt"])
    ap.add_argument("--lambda_alpha", type=float, default=None)
    ap.add_argument("--gt", default=None, choices=["original", "sync"])
    ap.add_argument("--particles", type=int, default=50000)
    ap.add_argument("--jobs", type=int, default=4, help="execuções em paralelo")
    ap.add_argument("--omp_threads", type=int, default=2, help="threads OpenMP por execução")
    ap.add_argument("--adm_map_12_2018", default="ADM_35_17_12_2018.jpg")
    ap.add_argument("--adm_map_01_2014", default="ADM_35_27_01_2014.jpg")
    ap.add_argument("--bin", default=br.DEFAULT_BIN)
    ap.add_argument("--odom_dir", default=br.DEFAULT_ODOM_DIR)
    ap.add_argument("--out", default=None, help="pasta de saída (padrão: experimentos/<hoje>_<nome>)")
    ap.add_argument("--nome", default=None, help="nome do experimento (padrão: ab_legado_<protocolo>_a<α>)")
    ap.add_argument("--continuar", action="store_true", help="reaproveita a pasta mais recente com o mesmo nome")
    ap.add_argument("--reaproveitar", default=None,
                    help="pasta de um teste do artigo de onde copiar o abBRIEF e o λ (padrão: o mais recente "
                         "com o mesmo protocolo, α e número de seeds)")
    ap.add_argument("--sem_reaproveitar", action="store_true", help="roda tudo, sem copiar nada")
    ap.add_argument("--force", action="store_true", help="roda de novo mesmo se errors.txt já existir")
    ap.add_argument("--plot_only", action="store_true")
    ap.add_argument("--dry_run", action="store_true")
    args = ap.parse_args()

    p = pc.PROTOCOLS[args.protocolo]
    args.regime = args.regime or p["regime"]
    args.lambda_alpha = p["lambda_alpha"] if args.lambda_alpha is None else args.lambda_alpha
    args.gt = args.gt or p["gt"]
    args.seeds = br.parse_seeds(args.seeds)
    if not args.seeds:
        ap.error("--seeds vazio")
    args.n_seeds = len(args.seeds)
    s = args.seeds
    args.seeds_str = (f"{s[0]}..{s[-1]}" if s == list(range(s[0], s[-1] + 1)) and len(s) > 1
                      else ",".join(map(str, s)))
    args.seed = s[0]
    if "legacy_ab" not in args.variantes:
        args.variantes = ["legacy_ab"] + args.variantes

    descricao = (f"abBRIEF do legado (--pair_pattern legacy_ab) × abBRIEF × λ-BRIEF por voo e mapa, "
                 f"{args.n_seeds} seed(s) ({args.seeds_str}); protocolo {args.protocolo}, regime {args.regime}, "
                 f"λ α={args.lambda_alpha:g}. Comece por `00_LEIA_ME.md`")
    if args.out is None:
        nome = args.nome or (f"ab_legado_{args.protocolo}_a{args.lambda_alpha:g}"
                             + (f"_{args.n_seeds}seeds" if args.n_seeds > 1 else ""))
        args.out = exp.pasta_experimento(nome, descricao, continuar=args.continuar or args.plot_only,
                                         dry_run=args.dry_run)
    else:
        args.out = exp.resolve(args.out)
        if not args.dry_run and os.path.abspath(args.out).startswith(exp.EXP_DIR + os.sep):
            exp.registrar(args.out, descricao)

    tasks, notes = build_all_tasks(args)
    src = source_dir_for(args)
    print(f"Protocolo {args.protocolo}: regime {args.regime}, λ α = {args.lambda_alpha:g}, seeds {args.seeds_str}, "
          f"variantes {', '.join(args.variantes)}\nSaída: {args.out}")
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
        leg = next((t for t in tasks if t["key"] == "abBRIEF_legado"), None)
        if leg:
            print("Exemplo (abBRIEF legado):\n  " + " ".join(leg["cmd"]))
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
                t, status, dt = f.result()
                print(f"[{i}/{len(tasks)}] seed {t['seed']} {t['flight']} mapa {t['label']} {t['desc']}: "
                      f"{status} ({dt:.0f}s; total {(time.time() - t0) / 60:.0f} min)", flush=True)
    else:
        n_reused = sum(os.path.exists(os.path.join(t["out_dir"], "REAPROVEITADO_DE.txt")) for t in tasks)

    agg, rows = collect(tasks, args)
    L = write_tables(agg, rows, args, args.out)
    fig_compilado(agg, args, args.out)
    i = L.index("## Por voo (média dos 5 mapas)")
    write_readme(args, notes, n_reused, src, L[i + 2:])
    print(f"\nTudo em {args.out} (comece por 00_LEIA_ME.md)")


if __name__ == "__main__":
    main()

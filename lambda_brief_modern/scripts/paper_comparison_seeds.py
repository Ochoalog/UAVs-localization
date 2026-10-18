#!/usr/bin/env python3
"""
paper_comparison_seeds.py — o teste do artigo λ-BRIEF (Figs. 4 e 5) com VÁRIAS seeds e a média no fim

É o paper_comparison.py (que continua existindo, com 1 seed) repetido para N seeds:

  para cada seed (padrão 1..5)
    para cada voo (ADM1, ADM2, ADM3, UFRGS)
      para cada mapa do artigo (5 por voo)
        roda o abBRIEF e o λ-BRIEF

e no fim junta tudo como o artigo, que tira a média de várias execuções por curva:
  - curva de erro MÉDIA por frame de cada método, com a faixa entre a melhor e a pior seed;
  - taxa de convergência (k/N), frame mediano de convergência, MAE médio ± desvio, erro final;
  - em quantas seeds o λ teve MAE menor que o abBRIEF (as seeds são pareadas: mesma seed nos dois).

Reaproveitamento: as execuções que já existem no teste de 1 seed (experimentos/*_artigo_<protocolo>_a<α>)
são copiadas em vez de rodadas de novo, desde que o comando seja o mesmo (mesmo regime, α, mapa,
partículas, odometria e seed). Desligue com --sem_reaproveitar.

Uso (de dentro de lambda_brief_modern/)
  python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --dry_run
  nohup python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 > seeds.log 2>&1 &
  python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --plot_only   # só refaz as figuras
  python3 scripts/paper_comparison_seeds.py ... --continuar      # termina a pasta mais recente (outro dia)
  python3 scripts/paper_comparison_seeds.py ... --seeds 1..30    # o protocolo completo do artigo (γ = 30)

Saída: experimentos/<AAAA-MM-DD>_artigo_<protocolo>_a<α>_<N>seeds/ (registrada no INDICE.md)
  00_LEIA_ME.md                         o que é cada arquivo
  compilado_artigo_ADM.png              como a Fig. 5, com a curva MÉDIA de cada mapa
  compilado_artigo_UFRGS.png            como a Fig. 4, idem
  tabela.md / tabela.csv                taxa de convergência, MAE médio ± dp, λ < ab em k/N seeds…
  <VOO>/<VOO>_compilado.png             curvas médias do voo + miniatura de cada mapa
  <VOO>/<VOO>_mapa_<MM-AAAA>.png        média ± faixa (min–max) de cada método, seeds em traço fino;
                                        embaixo o mapa com o GT e as trajetórias da seed de MAE mediano
  <VOO>/seeds/<VOO>_mapa_<MM-AAAA>_seed<N>.png   a figura de cada seed sozinha (igual à do teste de 1 seed)
  execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_<N>/   saída bruta da engine
"""

import argparse
import copy
import csv
import json
import os
import shutil
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import paper_comparison as pc  # noqa: E402  (voos, mapas, comandos e figuras do teste de 1 seed)
import benchmark_runner as br  # noqa: E402
import experimentos as exp  # noqa: E402
from consolidate_benchmark import SURFACE, INK, INK2, GRID, style_axes  # noqa: E402

FLIGHTS, ORDER = pc.FLIGHTS, pc.ORDER
AB_COLOR, LAMBDA_COLOR, CONV_THRESH = pc.AB_COLOR, pc.LAMBDA_COLOR, pc.CONV_THRESH


# --------------------------------------------------------------------------- execuções

def build_all_tasks(args):
    """Tarefas de todas as seeds, com a seed por fora (se parar no meio, as primeiras seeds ficam completas)."""
    tasks, notes = [], []
    for s in args.seeds:
        a = copy.copy(args)
        a.seed = s
        t_s, n_s = pc.build_tasks(a)
        for t in t_s:
            t["seed"] = s
        tasks += t_s
        for n in n_s:
            if n not in notes:
                notes.append(n)
    return tasks, notes


def source_dir_for(args):
    """Pasta do teste de 1 seed com o mesmo protocolo/α (de onde as execuções existentes são copiadas)."""
    if args.sem_reaproveitar:
        return None
    if args.reaproveitar:
        p = exp.resolve(args.reaproveitar)
        return p if os.path.isdir(p) else None
    return exp._ultima(f"artigo_{args.protocolo}_a{args.lambda_alpha:g}")


def reuse_existing(tasks, src, out, dry_run):
    """Copia execuções já feitas (mesmo comando, só muda a pasta) da pasta src para a nova."""
    if not src or os.path.abspath(src) == os.path.abspath(out):
        return 0
    n = 0
    for t in tasks:
        if os.path.exists(os.path.join(t["out_dir"], "errors.txt")):
            continue
        rel = os.path.relpath(t["out_dir"], out)
        sdir = os.path.join(src, rel)
        s_err, s_cmd = os.path.join(sdir, "errors.txt"), os.path.join(sdir, "cmd.txt")
        if not (os.path.exists(s_err) and os.path.getsize(s_err) > 100 and os.path.exists(s_cmd)):
            continue
        try:
            old = json.load(open(s_cmd))
        except (OSError, ValueError):
            continue
        if br.comparable_cmd(old) != br.comparable_cmd(t["cmd"]):
            continue
        n += 1
        if dry_run:
            continue
        shutil.copytree(sdir, t["out_dir"], dirs_exist_ok=True)
        with open(os.path.join(t["out_dir"], "cmd.txt"), "w") as f:
            json.dump(t["cmd"], f, indent=1)
        with open(os.path.join(t["out_dir"], "REAPROVEITADO_DE.txt"), "w") as f:
            f.write(f"Copiado de {sdir}\n(mesmo comando; a engine é determinística para a mesma seed)\n")
    return n


# --------------------------------------------------------------------------- agregação

def aggregate(runs):
    """runs: lista de (seed, resultado do pc.evaluate). Devolve curvas e estatísticas entre seeds."""
    frames = sorted(set().union(*[set(np.asarray(r["frames"]).tolist()) for _, r in runs]))
    idx = {f: i for i, f in enumerate(frames)}
    M = np.full((len(runs), len(frames)), np.nan)
    for k, (_, r) in enumerate(runs):
        M[k, [idx[f] for f in np.asarray(r["frames"]).tolist()]] = r["err"]
    with np.errstate(all="ignore"):
        mean, lo, hi = np.nanmean(M, 0), np.nanmin(M, 0), np.nanmax(M, 0)
    conv = [int(r["frames"][r["m"]["ttf"]]) for _, r in runs if r["m"]["ttf"] is not None]
    mae = np.array([r["m"]["mae"] for _, r in runs])
    track = [r["m"]["track_median"] for _, r in runs if r["m"]["track_median"] is not None]
    final = np.array([r["m"]["final"] for _, r in runs])
    order = sorted(range(len(runs)), key=lambda k: runs[k][1]["m"]["mae"])
    rep_seed, rep = runs[order[(len(order) - 1) // 2]]
    mean_res = dict(rep)  # gtX/gtY do representante (o GT é o mesmo em todas as seeds)
    mean_res.update({"frames": np.array(frames), "err": mean})
    return {"frames": np.array(frames), "mean": mean, "lo": lo, "hi": hi, "M": M, "runs": runs,
            "n": len(runs), "n_conv": len(conv), "conv_med": float(np.median(conv)) if conv else None,
            "mae_mean": float(mae.mean()), "mae_sd": float(mae.std(ddof=1)) if len(mae) > 1 else 0.0,
            "mae_by_seed": {s: r["m"]["mae"] for s, r in runs},
            "track_mean": float(np.mean(track)) if track else None,
            "final_mean": float(final.mean()), "final_ok": int(np.sum(final < CONV_THRESH)),
            "rep_seed": rep_seed, "rep": rep, "mean_res": mean_res}


def gt_label(flight, args):
    return "GT do Zenodo" if FLIGHTS[flight].get("gt_file") else f"GT {args.gt}"


# --------------------------------------------------------------------------- figuras

def fig_media_ano(flight, lbl, map_path, A, L, args, lam_name, path):
    fig = plt.figure(figsize=(10, 13), facecolor=SURFACE)
    gs = GridSpec(3, 1, height_ratios=[0.16, 1.05, 1.3], hspace=0.22, top=0.95, figure=fig)
    ax_t = fig.add_subplot(gs[0])   # caixa de números em cima, fora do gráfico (não cobre as curvas)
    ax_t.axis("off")
    ax = fig.add_subplot(gs[1])
    style_axes(ax)
    txt = []
    for g, c, ls, name in ((A, AB_COLOR, ":", "abBRIEF"), (L, LAMBDA_COLOR, "-", lam_name)):
        if g is None:
            txt.append(f"{name}: sem resultado")
            continue
        for k in range(g["n"]):
            ax.plot(g["frames"], g["M"][k], "-", color=c, lw=0.6, alpha=0.28)
        ax.fill_between(g["frames"], g["lo"], g["hi"], color=c, alpha=0.10, lw=0)
        ax.plot(g["frames"], g["mean"], ls, color=c, lw=2.4, label=f"{name} (média de {g['n']} seeds)")
        cm = "—" if g["conv_med"] is None else f"{g['conv_med']:.0f}"
        txt.append(f"{name}: convergiu {g['n_conv']}/{g['n']} (frame mediano {cm}) · "
                   f"MAE {g['mae_mean']:.1f} ± {g['mae_sd']:.1f} m · erro final médio {g['final_mean']:.1f} m")
    if A is not None and L is not None:
        common = sorted(set(A["mae_by_seed"]) & set(L["mae_by_seed"]))
        wins = sum(L["mae_by_seed"][s] < A["mae_by_seed"][s] for s in common)
        txt.append(f"λ com MAE menor que o abBRIEF em {wins}/{len(common)} seeds (mesma seed nos dois)")
    ax.axhline(CONV_THRESH, color=INK2, ls="--", lw=0.8, label=f"{CONV_THRESH:.0f} m")
    ax.set_ylim(bottom=0)
    ax.set_xlabel("Instant (frame)", color=INK2, fontsize=9)
    ax.set_ylabel("Error (m)", color=INK2, fontsize=9)
    ax.legend(fontsize=8.5, loc="upper right")
    ax_t.set_title(f"{flight} · mapa {lbl} — abBRIEF × {lam_name}", color=INK, fontsize=12)
    ax_t.text(0.5, 0.5, "\n".join(txt), transform=ax_t.transAxes, fontsize=8.6, color=INK, va="center",
              ha="center", bbox=dict(boxstyle="round", fc="white", ec=GRID, alpha=0.95))

    axm = fig.add_subplot(gs[2])
    ref = A or L
    if ref is not None:
        est = []
        if A is not None:
            est.append((A["rep"]["estX"], A["rep"]["estY"], AB_COLOR, ":",
                        f"abBRIEF (seed {A['rep_seed']}, MAE mediano)"))
        if L is not None:
            est.append((L["rep"]["estX"], L["rep"]["estY"], LAMBDA_COLOR, "-",
                        f"{lam_name} (seed {L['rep_seed']}, MAE mediano)"))
        pc.draw_map(axm, map_path, ref["rep"]["gtX"], ref["rep"]["gtY"], est=est)
        axm.legend(fontsize=8, loc="lower right", framealpha=0.9)
    axm.set_title(f"Mapa usado: {os.path.basename(map_path)} (recorte; ● = início; trajetória da seed típica)",
                  color=INK, fontsize=10)
    fig.suptitle(f"Média de {args.n_seeds} seeds ({args.seeds_str}) · traço fino = cada seed · faixa = melhor–pior · "
                 f"regime {args.regime} · {gt_label(flight, args)}", y=0.985, color=INK2, fontsize=9)
    fig.savefig(path, dpi=150, facecolor=SURFACE, bbox_inches="tight")
    plt.close(fig)


# --------------------------------------------------------------------------- saída

def make_outputs(tasks, notes, args, n_reused, src):
    lam_name = "λ-BRIEF" + ("" if abs(args.lambda_alpha - 0.5) < 1e-12 else f" (α={args.lambda_alpha:g})")
    per = {}   # (voo, mapa, método) -> [(seed, resultado)]
    for t in tasks:
        r = pc.evaluate(t, args.gt)
        if r is None:
            continue
        key = (t["flight"], t["label"], "abBRIEF" if t["desc"] == "abBRIEF" else "lambda")
        per.setdefault(key, []).append((t["seed"], r))
    agg = {k: aggregate(sorted(v, key=lambda x: x[0])) for k, v in per.items()}
    mean_res = {k: g["mean_res"] for k, g in agg.items()}

    made = []
    for flight in args.flights:
        if FLIGHTS[flight].get("skip"):
            continue
        fdir = os.path.join(args.out, flight)
        os.makedirs(fdir, exist_ok=True)
        a_f = copy.copy(args)
        a_f.seed = f"{args.seeds_str} (média)"
        a_f.gt = gt_label(flight, args).replace("GT ", "")
        for lbl, mp in pc.flight_maps(flight, args):
            A, L = agg.get((flight, lbl, "abBRIEF")), agg.get((flight, lbl, "lambda"))
            if A is None and L is None:
                continue
            p = os.path.join(fdir, f"{flight}_mapa_{lbl}.png")
            fig_media_ano(flight, lbl, mp, A, L, args, lam_name, p)
            made.append(p)
            if not args.sem_figuras_seed:  # mesma figura do teste de 1 seed, para cada seed (mapa carregado 1 vez)
                sdir = os.path.join(fdir, "seeds")
                os.makedirs(sdir, exist_ok=True)
                ab_s = dict(per.get((flight, lbl, "abBRIEF"), []))
                la_s = dict(per.get((flight, lbl, "lambda"), []))
                for s in args.seeds:
                    if s not in ab_s and s not in la_s:
                        continue
                    a_s = copy.copy(args)
                    a_s.seed, a_s.gt = s, a_f.gt
                    pc.fig_por_ano(flight, lbl, mp, ab_s.get(s), la_s.get(s), a_s, lam_name,
                                   os.path.join(sdir, f"{flight}_mapa_{lbl}_seed{s}.png"))
        if any((flight, lbl, d) in agg for lbl, _ in pc.flight_maps(flight, args) for d in ("abBRIEF", "lambda")):
            p = os.path.join(fdir, f"{flight}_compilado.png")
            pc.fig_compilado_voo(flight, mean_res, a_f, lam_name, p)
            made.append(p)

    a_c = copy.copy(args)
    a_c.seed = f"{args.seeds_str} (média)"
    adm = [f for f in ("ADM1", "ADM2", "ADM3") if f in args.flights]
    if adm:
        p = os.path.join(args.out, "compilado_artigo_ADM.png")
        pc.fig_compilado_artigo(adm, mean_res, a_c, lam_name, p,
                                f"Comparison between abBRIEF and {lam_name} for the Arroio do Meio flights "
                                f"(como a Fig. 5 · média de {args.n_seeds} seeds · {args.regime})")
        made.append(p)
    if "UFRGS" in args.flights:
        p = os.path.join(args.out, "compilado_artigo_UFRGS.png")
        pc.fig_compilado_artigo(["UFRGS"], mean_res, a_c, lam_name, p,
                                f"Comparison between abBRIEF and {lam_name} for the UFRGS flight "
                                f"(como a Fig. 4 · média de {args.n_seeds} seeds · {args.regime})")
        made.append(p)

    rows, resumo = write_tables(agg, args, lam_name)
    write_readme(args, notes, lam_name, resumo, n_reused, src)
    print("\nArquivos gerados:")
    for p in made:
        print("  ", os.path.relpath(p, args.out))
    if not args.sem_figuras_seed:
        print("   <VOO>/seeds/  (uma figura por seed)")
    print(f"\nTudo em {args.out} (comece por 00_LEIA_ME.md)")


def write_tables(agg, args, lam_name):
    def f1(v):
        return "—" if v is None else f"{v:.1f}"

    rows, resumo = [], []
    try:
        from scipy.stats import wilcoxon
    except ImportError:
        wilcoxon = None
    for flight in args.flights:
        if FLIGHTS[flight].get("skip"):
            continue
        pares, mapas_lam = [], 0
        n_mapas = 0
        for lbl, mp in pc.flight_maps(flight, args):
            A, L = agg.get((flight, lbl, "abBRIEF")), agg.get((flight, lbl, "lambda"))
            row = {"voo": flight, "mapa": lbl, "arquivo": os.path.basename(mp)}
            for tag, g in (("ab", A), ("lambda", L)):
                row[f"{tag}_seeds"] = g["n"] if g else 0
                row[f"{tag}_convergiu"] = g["n_conv"] if g else None
                row[f"{tag}_conv_frame_mediano"] = g["conv_med"] if g else None
                row[f"{tag}_mae_media_m"] = round(g["mae_mean"], 2) if g else None
                row[f"{tag}_mae_dp_m"] = round(g["mae_sd"], 2) if g else None
                row[f"{tag}_rastreio_medio_m"] = round(g["track_mean"], 2) if g and g["track_mean"] else None
                row[f"{tag}_final_medio_m"] = round(g["final_mean"], 2) if g else None
                row[f"{tag}_final_abaixo_25m"] = g["final_ok"] if g else None
                row[f"{tag}_mae_por_seed"] = (" ".join(f"{s}:{v:.1f}" for s, v in g["mae_by_seed"].items())
                                              if g else "")
            if A and L:
                common = sorted(set(A["mae_by_seed"]) & set(L["mae_by_seed"]))
                row["lambda_vence_seeds"] = sum(L["mae_by_seed"][s] < A["mae_by_seed"][s] for s in common)
                row["seeds_pareadas"] = len(common)
                pares += [(A["mae_by_seed"][s], L["mae_by_seed"][s]) for s in common]
                n_mapas += 1
                mapas_lam += L["mae_mean"] < A["mae_mean"]
            else:
                row["lambda_vence_seeds"], row["seeds_pareadas"] = None, 0
            rows.append(row)
        if pares:
            a, l = np.array(pares).T
            p_txt = ""
            if wilcoxon is not None and len(pares) >= 6 and np.any(a != l):
                p_txt = f"; Wilcoxon pareado p = {wilcoxon(l, a).pvalue:.3f}"
            resumo.append(f"- **{flight}**: λ com MAE médio menor em {mapas_lam}/{n_mapas} mapas; λ venceu "
                          f"{int(np.sum(l < a))}/{len(pares)} pares (mapa × seed); MAE médio ab {a.mean():.1f} m × "
                          f"λ {l.mean():.1f} m{p_txt}.")

    if rows:
        with open(os.path.join(args.out, "tabela.csv"), "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)

    T = ["| Voo | Mapa | Convergiu ab | Convergiu λ | Frame conv. (ab / λ) | MAE ab | MAE λ | λ < ab | "
         "Rastreio ab | Rastreio λ | Final ab | Final λ |",
         "|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        def conv(tag):
            return "—" if r[f"{tag}_convergiu"] is None else f"{r[f'{tag}_convergiu']}/{r[f'{tag}_seeds']}"

        def mae(tag):
            return "—" if r[f"{tag}_mae_media_m"] is None else f"{r[f'{tag}_mae_media_m']:.1f} ± {r[f'{tag}_mae_dp_m']:.1f}"
        vence = "—" if r["lambda_vence_seeds"] is None else f"{r['lambda_vence_seeds']}/{r['seeds_pareadas']}"
        T.append(f"| {r['voo']} | {r['mapa']} | {conv('ab')} | {conv('lambda')} | "
                 f"{f1(r['ab_conv_frame_mediano'])} / {f1(r['lambda_conv_frame_mediano'])} | {mae('ab')} | "
                 f"{mae('lambda')} | {vence} | {f1(r['ab_rastreio_medio_m'])} | {f1(r['lambda_rastreio_medio_m'])} | "
                 f"{f1(r['ab_final_medio_m'])} | {f1(r['lambda_final_medio_m'])} |")
    with open(os.path.join(args.out, "tabela.md"), "w") as f:
        f.write(f"# abBRIEF × {lam_name} por voo e mapa — média de {args.n_seeds} seeds ({args.seeds_str})\n\n"
                f"Protocolo {args.protocolo} · regime {args.regime} · GT {args.gt} (ADM3: GT do Zenodo).\n\n"
                "- **Convergiu** = seeds em que o erro ficou < 25 m por 10 frames seguidos (k/N).\n"
                "- **Frame conv.** = mediana do frame de convergência entre as seeds que convergiram.\n"
                "- **MAE** = média ± desvio-padrão entre seeds do erro médio do voo.\n"
                "- **λ < ab** = em quantas seeds o λ teve MAE menor (mesma seed nos dois métodos).\n"
                "- **Rastreio** = média, entre seeds, da mediana do erro depois de convergir.\n"
                "- **Final** = média do erro no último frame. Erros em metros.\n\n"
                + "\n".join(T) + "\n\n## Resumo por voo\n\n" + "\n".join(resumo) + "\n")
    return rows, resumo


def write_readme(args, notes, lam_name, resumo, n_reused, src):
    L = [f"# Teste do artigo λ-BRIEF — abBRIEF × {lam_name}, média de {args.n_seeds} seeds", "",
         f"Protocolo **{args.protocolo}**: regime `{args.regime}`, λ-BRIEF com α = {args.lambda_alpha:g}, "
         f"seeds {args.seeds_str}, erro contra o GT `{'traj_truth.txt (original)' if args.gt == 'original' else 'traj_truth_sync.txt'}` "
         "(ADM3: GT do Zenodo).", "",
         "Mesmo teste do `paper_comparison.py` (cada voo × cada mapa do artigo × abBRIEF e λ-BRIEF), repetido "
         f"para {args.n_seeds} seeds. As curvas dos compilados são a **média** das seeds, como no artigo "
         "(que usa 30).", ""]
    if n_reused:
        L += [f"{n_reused} execuções foram copiadas de `{os.path.relpath(src, exp.MODERN_DIR)}` (mesmo comando, "
              "mesma seed) em vez de rodadas de novo; cada uma tem um `REAPROVEITADO_DE.txt`.", ""]
    L += ["## Onde olhar", "",
          "| Arquivo | O que mostra |", "|---|---|",
          "| `tabela.md` | Por voo e mapa: convergiu k/N, MAE médio ± dp, em quantas seeds o λ ganhou; resumo por voo no fim |",
          "| `compilado_artigo_ADM.png` / `compilado_artigo_UFRGS.png` | Como as Figs. 5 e 4, com a curva média de cada mapa |",
          "| `<VOO>/<VOO>_mapa_<MM-AAAA>.png` | Um mapa (ano): média de cada método (linha grossa), cada seed (traço fino) e a faixa melhor–pior; embaixo, o mapa com as trajetórias da seed de MAE mediano |",
          "| `<VOO>/<VOO>_compilado.png` | Curvas médias do voo com a miniatura de cada mapa |",
          "| `<VOO>/seeds/` | A figura de cada seed sozinha |",
          "| `tabela.csv` | Tudo em números, incluindo o MAE de cada seed |",
          "| `execucoes/` | Saída bruta da engine |", "",
          "## Resumo por voo", ""] + resumo
    if notes:
        L += ["", "## Avisos", ""] + [f"- {n}" for n in notes]
    L += ["", "Voos, mapas e datas: os mesmos do teste de 1 seed (ver o `00_LEIA_ME.md` daquela pasta e "
          "`docs/voos_e_mapas_do_artigo.md`)."]
    with open(os.path.join(args.out, "00_LEIA_ME.md"), "w") as f:
        f.write("\n".join(L) + "\n")


# --------------------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--protocolo", default="artigo", choices=list(pc.PROTOCOLS))
    ap.add_argument("--seeds", default="1..5", help="ex.: 1..5, 1..30 ou 1,2,7 (padrão 1..5)")
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
    ap.add_argument("--nome", default=None, help="nome do experimento (padrão: artigo_<protocolo>_a<α>_<N>seeds)")
    ap.add_argument("--continuar", action="store_true", help="reaproveita a pasta mais recente com o mesmo nome")
    ap.add_argument("--reaproveitar", default=None,
                    help="pasta de onde copiar execuções já feitas (padrão: o teste de 1 seed mais recente)")
    ap.add_argument("--sem_reaproveitar", action="store_true", help="roda tudo, sem copiar nada")
    ap.add_argument("--sem_figuras_seed", action="store_true", help="não gera <VOO>/seeds/ (mais rápido)")
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

    descricao = (f"Teste do artigo, média de {args.n_seeds} seeds ({args.seeds_str}): abBRIEF × λ-BRIEF por voo "
                 f"e mapa (protocolo {args.protocolo}, regime {args.regime}, λ α={args.lambda_alpha:g}, GT {args.gt})")
    if args.out is None:
        nome = args.nome or f"artigo_{args.protocolo}_a{args.lambda_alpha:g}_{args.n_seeds}seeds"
        args.out = exp.pasta_experimento(nome, descricao, continuar=args.continuar or args.plot_only,
                                         dry_run=args.dry_run)
    else:
        args.out = exp.resolve(args.out)
        if not args.dry_run and os.path.abspath(args.out).startswith(exp.EXP_DIR + os.sep):
            exp.registrar(args.out, descricao)

    tasks, notes = build_all_tasks(args)
    src = source_dir_for(args)
    print(f"Protocolo {args.protocolo}: regime {args.regime}, λ α = {args.lambda_alpha:g}, "
          f"seeds {args.seeds_str} ({args.n_seeds}), GT {args.gt}\nSaída: {args.out}")
    if src:
        print(f"Reaproveita execuções já feitas de: {src}")
    print()
    for n in notes:
        print("[aviso]", n)

    if args.dry_run:
        pc.ensure_odom(args)
        n_reuse = reuse_existing(tasks, src, args.out, dry_run=True)
        todo = [t for t in tasks if not os.path.exists(os.path.join(t["out_dir"], "errors.txt"))]
        n_run = max(0, len(todo) - n_reuse)
        print(f"{len(tasks)} execuções no total ({len(tasks) // args.n_seeds} por seed); "
              f"{len(tasks) - len(todo)} já na pasta, {n_reuse} seriam copiadas, {n_run} rodariam "
              f"(~{n_run * 200 / args.jobs / 3600:.1f} h com --jobs {args.jobs}).")
        return

    n_reused = 0
    if not args.plot_only:
        if not pc.ensure_odom(args):
            print("Odometria fixa indisponível; veja as mensagens acima.")
            sys.exit(1)
        n_reused = reuse_existing(tasks, src, args.out, dry_run=False)
        if n_reused:
            print(f"{n_reused} execuções copiadas da pasta de 1 seed (mesmo comando).", flush=True)
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

    make_outputs(tasks, notes, args, n_reused, src)


if __name__ == "__main__":
    main()

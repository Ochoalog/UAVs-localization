#!/usr/bin/env bash
# Avaliação do filtro em duas fases (--two_phase) no ADM/0036, voo completo.
# Rode a partir de lambda_brief_modern/:   bash scripts/eval_two_phase.sh [seeds...]
# Cada execução leva ~4 min; o padrão (3 configs x 5 seeds) leva ~1 h.
# Resultados: analise_2fases/<config>_s<seed>/ (errors.txt, phase_log.txt, run.log)
set -euo pipefail

SEEDS=("${@:-1 7 42 123 2024}")
read -r -a SEEDS <<< "${SEEDS[*]}"

BIN=./build/lambda_brief_loc
OUT=analise_2fases
mkdir -p "$OUT"

BASE=(--map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg"
      --dataset_dir "../Datasets/ADM/0036" --start_frame 84 --end_frame 279
      --num_particles 50000 --pairs 256 --alpha 0.5 --vi NDVI --color_mode LAB_NO_L
      --min_scale 0.1 --max_scale 3.0 --quantize 25
      --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 --pixel_to_meter 0.188679245)

# Priors/injeção da "config nova + bússola" (CFG2 da análise do fim do voo)
GLOBAL_OPTS=(--scale_prior_sigma 0.10 --scale_prior_gain 0.87 --yaw_prior_sigma 15
             --inject_ratio 0.05 --inject_spread 300 --vo_scale_deadband 1)

run() {  # run <nome> <seed> [opções extras...]
    local name=$1 seed=$2; shift 2
    local dir="$OUT/${name}_s${seed}"
    if [[ -f "$dir/errors.txt" ]]; then
        echo "[skip] $dir já existe"; return
    fi
    mkdir -p "$dir"
    echo "[run] $name seed $seed"
    "$BIN" "${BASE[@]}" --output_dir "$dir" --seed "$seed" "$@" > "$dir/run.log" 2>&1
    grep -E "Mean Position|Final Position|\[Phase\]" "$dir/run.log" | sed "s/^/    /" || true
}

for s in "${SEEDS[@]}"; do
    run artigo   "$s"                                  # config. do artigo (referência de rastreio)
    run cfg2     "$s" "${GLOBAL_OPTS[@]}"              # priors/injeção o voo todo
    run 2fases   "$s" "${GLOBAL_OPTS[@]}" --two_phase  # priors/injeção só até convergir
done

# Métricas e gráficos (usa scripts/analise_fim.py da análise anterior)
for cfg in artigo cfg2 2fases; do
    args=()
    for s in "${SEEDS[@]}"; do args+=(--run "$OUT/${cfg}_s${s}:${cfg} seed${s}"); done
    python3 scripts/analise_fim.py "${args[@]}" --group-name "2F_${cfg}" --outdir "$OUT/plots" \
        --corr-gt "../Datasets/ADM/0036/traj_truth_corr.txt" --table-out "$OUT/tabela.md"
done

# Resumo: média ± desvio entre seeds e quando cada execução entrou em TRACKING
python3 - "$OUT" "${SEEDS[@]}" <<'EOF'
import sys, numpy as np
out, seeds = sys.argv[1], sys.argv[2:]
print("\nconfig   | média (m)      | mediana (m)    | final (m)       | últimos 10 (m)  | frames<30m | entrou em TRACKING")
for cfg in ["artigo", "cfg2", "2fases"]:
    M, Md, F, L, B, T = [], [], [], [], [], []
    for s in seeds:
        e = np.loadtxt(f"{out}/{cfg}_s{s}/errors.txt", skiprows=1)[:, 7]
        M.append(e.mean()); Md.append(np.median(e)); F.append(e[-1]); L.append(e[-10:].mean()); B.append((e < 30).sum())
        try:
            ph = [l.split() for l in open(f"{out}/{cfg}_s{s}/phase_log.txt").readlines()[1:]]
            t = next((int(p[0]) for p in ph if p[1] == "TRACKING"), None)
            T.append(str(t) if t is not None else "nunca")
        except FileNotFoundError:
            T.append("-")
    f = lambda v: f"{np.mean(v):6.1f} ± {np.std(v):5.1f}"
    print(f"{cfg:8s} | {f(M)} | {f(Md)} | {f(F)}  | {f(L)}  | {np.mean(B):5.1f}      | {', '.join(T)}")
EOF
echo "Tabelas: $OUT/tabela.md   Gráficos: $OUT/plots/"

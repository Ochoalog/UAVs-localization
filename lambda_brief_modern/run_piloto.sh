#!/usr/bin/env bash
cd "/storage/UAVs Localization/lambda_brief_modern"
W="--workers 2 --omp_threads 4"   # ajuste conforme o nproc
# Saída: experimentos/<hoje>_piloto/ (registrada no experimentos/INDICE.md). Para continuar um
# piloto de outro dia, rode com OUT=experimentos/<data>_piloto ./run_piloto.sh
OUT=${OUT:-$(python3 scripts/experimentos.py nova piloto "Piloto abBRIEF × λ-BRIEF, final_baro e final_noalt, 5 seeds" ./run_piloto.sh)}
echo "== saída: $OUT"
consolidate() { python3 scripts/consolidate_benchmark.py --benchmark_dir "$OUT" --gt_file traj_truth_sync.txt > "$OUT/consolidate.log" 2>&1; }

echo "== $(date) build"
cmake --build build -j && ctest --test-dir build || { echo "BUILD FALHOU"; exit 1; }

echo "== $(date) odometria (ADM0036 e UFRGS já existem: só confere)"
python3 scripts/benchmark_runner.py --make_odom --dataset ADM0036 UFRGS

echo "== $(date) ADM0036 (2 mapas)"
for R in final_baro final_noalt; do
  python3 scripts/benchmark_runner.py --dataset ADM0036 --regime $R --seeds 1..5 $W \
    --maps ADM_35_12_09_2018.jpg ADM_35_27_02_2017.jpg --output_base_dir "$OUT"
done
consolidate

echo "== $(date) UFRGS (2 mapas)"
for R in final_baro final_noalt; do
  python3 scripts/benchmark_runner.py --dataset UFRGS --regime $R --seeds 1..5 $W \
    --maps UFRGS_VET_V3_18_09_2018.jpg UFRGS_VET_V3_02_04_2015.jpg --output_base_dir "$OUT"
done
consolidate

echo "== $(date) ADM0036 (mapas extras, se der tempo)"
for R in final_baro final_noalt; do
  python3 scripts/benchmark_runner.py --dataset ADM0036 --regime $R --seeds 1..5 $W \
    --maps ADM_35_17_08_2018.jpg ADM_35_17_12_2018.jpg --output_base_dir "$OUT"
done
consolidate
echo "== $(date) FIM"

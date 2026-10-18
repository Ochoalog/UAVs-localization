# Benchmark abBRIEF × λ-BRIEF — consolidado

- Pasta: `/storage/UAVs Localization/lambda_brief_modern/results_final/B_piloto`
- Gerado em 15/10/2026 14:15; 120 execuções
- Ground truth: `traj_truth_sync.txt` (re-sincronizado pelos timestamps)
- Convergência: erro < 25 m por 10 frames seguidos; perda de rastro: erro > 50 m após convergir

## Por grupo

| Dataset | Regime | Mapa | Descritor | n | Convergência (IC 95%) | TTF med. | Rastreio med. (IC 95%) | Perdas | MAE méd. / med. | Final < limiar | ε artigo | s/frame |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ADM0036 | final_baro | ADM_35_12_09_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 16 | 12.0 (11.6–12.2) m | 0.0 | 30.0 / 29.3 m | 100% | 34.0 m | 0.27 |
| ADM0036 | final_baro | ADM_35_12_09_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 17 | 13.3 (11.7–13.7) m | 0.0 | 30.1 / 28.3 m | 100% | 35.5 m | 0.32 |
| ADM0036 | final_baro | ADM_35_17_08_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 11 | 5.9 (5.9–6.1) m | 0.0 | 26.4 / 21.5 m | 100% | 31.7 m | 0.32 |
| ADM0036 | final_baro | ADM_35_17_08_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 14 | 7.1 (7.0–7.3) m | 0.0 | 27.4 / 25.9 m | 100% | 31.1 m | 0.45 |
| ADM0036 | final_baro | ADM_35_17_12_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 16 | 7.8 (7.6–8.0) m | 0.0 | 30.4 / 32.1 m | 100% | 34.2 m | 0.41 |
| ADM0036 | final_baro | ADM_35_17_12_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 17 | 7.9 (7.4–8.2) m | 0.6 | 30.3 / 32.2 m | 100% | 33.8 m | 0.41 |
| ADM0036 | final_baro | ADM_35_27_02_2017 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 16 | 9.4 (8.1–11.1) m | 0.0 | 26.4 / 27.8 m | 100% | 30.7 m | 0.28 |
| ADM0036 | final_baro | ADM_35_27_02_2017 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 16 | 10.7 (10.0–11.4) m | 0.0 | 25.0 / 28.2 m | 100% | 27.1 m | 0.31 |
| ADM0036 | final_noalt | ADM_35_12_09_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 117 | 11.4 (7.6–12.1) m | 0.0 | 117.0 / 126.7 m | 100% | 157.9 m | 0.29 |
| ADM0036 | final_noalt | ADM_35_12_09_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 25 | 13.4 (12.1–13.8) m | 0.0 | 40.8 / 34.7 m | 100% | 50.1 m | 0.32 |
| ADM0036 | final_noalt | ADM_35_17_08_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 21 | 6.1 (5.8–6.5) m | 0.0 | 36.3 / 30.9 m | 100% | 45.2 m | 0.28 |
| ADM0036 | final_noalt | ADM_35_17_08_2018 | λ-BRIEF (α=0.5) | 5 | 80% (38%–96%) | 18 | 7.2 (7.0–7.4) m | 0.0 | 110.0 / 31.6 m | 80% | 202.4 m | 0.32 |
| ADM0036 | final_noalt | ADM_35_17_12_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 21 | 7.8 (5.4–8.1) m | 3.2 | 81.7 / 33.5 m | 100% | 114.3 m | 0.29 |
| ADM0036 | final_noalt | ADM_35_17_12_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 25 | 8.2 (7.5–8.4) m | 0.4 | 47.1 / 47.8 m | 100% | 50.9 m | 0.31 |
| ADM0036 | final_noalt | ADM_35_27_02_2017 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 36 | 11.0 (5.5–11.5) m | 1.2 | 53.3 / 44.8 m | 100% | 65.1 m | 0.29 |
| ADM0036 | final_noalt | ADM_35_27_02_2017 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 25 | 11.6 (10.4–12.4) m | 0.0 | 42.8 / 41.8 m | 100% | 50.3 m | 0.34 |
| UFRGS | final_baro | UFRGS_VET_V3_02_04_2015 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 14 | 11.0 (10.7–11.3) m | 0.0 | 14.3 / 14.5 m | 100% | 14.6 m | 0.31 |
| UFRGS | final_baro | UFRGS_VET_V3_02_04_2015 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 12 | 11.8 (11.7–12.0) m | 0.0 | 12.7 / 12.7 m | 100% | 12.9 m | 0.35 |
| UFRGS | final_baro | UFRGS_VET_V3_18_09_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 9 | 10.8 (10.7–11.2) m | 0.0 | 10.6 / 10.4 m | 100% | 10.8 m | 0.32 |
| UFRGS | final_baro | UFRGS_VET_V3_18_09_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 10 | 11.2 (10.9–11.5) m | 0.0 | 11.6 / 11.5 m | 100% | 11.8 m | 0.37 |
| UFRGS | final_noalt | UFRGS_VET_V3_02_04_2015 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 22 | 11.3 (11.1–11.8) m | 0.0 | 16.3 / 16.6 m | 100% | 16.9 m | 0.29 |
| UFRGS | final_noalt | UFRGS_VET_V3_02_04_2015 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 25 | 12.1 (12.1–12.3) m | 0.0 | 17.8 / 17.0 m | 100% | 18.6 m | 0.32 |
| UFRGS | final_noalt | UFRGS_VET_V3_18_09_2018 | abBRIEF (α=0) | 5 | 100% (57%–100%) | 10 | 11.1 (10.5–11.6) m | 0.0 | 10.6 / 10.5 m | 100% | 10.7 m | 0.31 |
| UFRGS | final_noalt | UFRGS_VET_V3_18_09_2018 | λ-BRIEF (α=0.5) | 5 | 100% (57%–100%) | 12 | 11.3 (11.0–11.8) m | 0.0 | 12.1 / 11.6 m | 100% | 12.7 m | 0.34 |

## Testes pareados contra o abBRIEF

Δ = variante − abBRIEF. p ajustado por Holm sobre os mapas (linhas "agregado" não entram na correção).

| Dataset | Regime | Mapa | Variante | pares | Δ convergência | p (adj) | Δ MAE med. | p (adj) | Δ rastreio med. (n) | p (adj) | Conclusão |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ADM0036 | final_baro | ADM_35_12_09_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | -1.0 m | — (—) | 1.2 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_baro | ADM_35_17_08_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 2.4 m | — (—) | 1.1 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_baro | ADM_35_17_12_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 0.1 m | — (—) | 0.2 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_baro | ADM_35_27_02_2017 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 1.1 m | — (—) | 1.3 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_baro | agregado | λ-BRIEF (α=0.5) | 20 | +0 pp | 1.000 (1.000) | -0.2 m | 0.927 (0.927) | 1.0 m (20) | 4.8e-04 (4.8e-04) | convergência: sem diferença significativa; MAE: sem diferença significativa; rastreio: λ pior |
| ADM0036 | final_noalt | ADM_35_12_09_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | -50.7 m | — (—) | 1.3 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_noalt | ADM_35_17_08_2018 | λ-BRIEF (α=0.5) | 5 | -20 pp | 1.000 (1.000) | 1.9 m | — (—) | 1.1 m (4) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_noalt | ADM_35_17_12_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 2.6 m | — (—) | 0.4 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_noalt | ADM_35_27_02_2017 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | -3.0 m | — (—) | 1.4 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| ADM0036 | final_noalt | agregado | λ-BRIEF (α=0.5) | 20 | -5 pp | 1.000 (1.000) | -3.0 m | 0.143 (0.143) | 1.2 m (19) | 2.7e-05 (2.7e-05) | convergência: sem diferença significativa; MAE: sem diferença significativa; rastreio: λ pior |
| UFRGS | final_baro | UFRGS_VET_V3_02_04_2015 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | -1.7 m | — (—) | 0.8 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| UFRGS | final_baro | UFRGS_VET_V3_18_09_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 0.8 m | — (—) | 0.4 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| UFRGS | final_baro | agregado | λ-BRIEF (α=0.5) | 10 | +0 pp | 1.000 (1.000) | -0.1 m | 0.432 (0.432) | 0.6 m (10) | 0.002 (0.002) | convergência: sem diferença significativa; MAE: sem diferença significativa; rastreio: λ pior |
| UFRGS | final_noalt | UFRGS_VET_V3_02_04_2015 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 1.7 m | — (—) | 0.8 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| UFRGS | final_noalt | UFRGS_VET_V3_18_09_2018 | λ-BRIEF (α=0.5) | 5 | +0 pp | 1.000 (1.000) | 1.1 m | — (—) | 0.3 m (5) | — (—) | convergência: sem diferença significativa; MAE: n insuficiente; rastreio: n insuficiente |
| UFRGS | final_noalt | agregado | λ-BRIEF (α=0.5) | 10 | +0 pp | 1.000 (1.000) | 1.2 m | 0.037 (0.037) | 0.3 m (10) | 0.002 (0.002) | convergência: sem diferença significativa; MAE: λ pior; rastreio: λ pior |
| TODOS | final_baro | agregado | λ-BRIEF (α=0.5) | 30 | +0 pp | 1.000 (1.000) | -0.2 m | 0.839 (0.839) | 0.7 m (30) | 2.8e-06 (2.8e-06) | convergência: sem diferença significativa; MAE: sem diferença significativa; rastreio: λ pior |
| TODOS | final_noalt | agregado | λ-BRIEF (α=0.5) | 30 | -3 pp | 1.000 (1.000) | 0.5 m | 0.556 (0.556) | 0.6 m (29) | 5.2e-08 (5.2e-08) | convergência: sem diferença significativa; MAE: sem diferença significativa; rastreio: λ pior |


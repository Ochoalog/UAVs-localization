# Índice dos experimentos

Uma linha por experimento, o mais novo no topo. A data é a do início do experimento.
Os scripts acrescentam a linha sozinhos (ver `scripts/experimentos.py`). Cada pasta nova
tem um `COMANDO.txt` com os comandos que escreveram nela. Embaixo da tabela há um
**resumo de cada teste** (o que foi feito e o resultado principal).

Como rodar de novo os testes do artigo: [`../docs/MANUAL_TESTES_ARTIGO.md`](../docs/MANUAL_TESTES_ARTIGO.md).

As entradas reutilizadas por vários experimentos não são resultados e ficam em `../insumos/`:

- `insumos/odom/`: odometria fixa da VO nova.
- `insumos/altimetro/`: relatórios do altímetro real.

| Data | Pasta | O que é | Comando |
|---|---|---|---|
| 2026-10-27 | [2026-10-27_ab_legado_novo_a1](2026-10-27_ab_legado_novo_a1/) | abBRIEF do legado (--pair_pattern legacy_ab) × abBRIEF × λ-BRIEF por voo e mapa, 1 seed(s) (1); protocolo novo, regime final_baro, λ α=1. Comece por `00_LEIA_ME.md` | `python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 --reaproveitar experimentos/2026-10-17_artigo_novo_a1` |
| 2026-10-17 | [2026-10-17_artigo_novo_a0.5_5seeds](2026-10-17_artigo_novo_a0.5_5seeds/) | Teste do artigo com λ α = 0,5, média de 5 seeds (1..5): 100 execuções do λ; as 100 do abBRIEF foram copiadas do teste α = 1. final_baro, GT sync (ADM3: GT do Zenodo) | `python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1..5 --nome artigo_novo_a0.5_5seeds --reaproveitar ...` |
| 2026-10-17 | [2026-10-17_artigo_novo_a0.5](2026-10-17_artigo_novo_a0.5/) | Teste do artigo com λ α = 0,5, seed 1 (só o λ rodou; o abBRIEF foi copiado do teste α = 1). final_baro. Comece por `00_LEIA_ME.md` | `python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1 --nome artigo_novo_a0.5 --reaproveitar experiment...` |
| 2026-10-17 | [2026-10-17_artigo_novo_a1_5seeds](2026-10-17_artigo_novo_a1_5seeds/) | Teste do artigo com λ α = 1, média de 5 seeds (1..5): 200 execuções (4 voos × 5 mapas × 2 métodos × 5 seeds). final_baro, GT sync (ADM3: GT do Zenodo) | `python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --sem_reaproveitar` |
| 2026-10-17 | [2026-10-17_artigo_novo_a1](2026-10-17_artigo_novo_a1/) | Teste do artigo: abBRIEF × λ-BRIEF (α = 1) por voo (ADM1, ADM2, ADM3, UFRGS) e por mapa das Figs. 4–5; final_baro, seed 1. Comece por `00_LEIA_ME.md` | `python3 scripts/paper_comparison.py --protocolo novo --lambda_alpha 1.0` |
| 2026-10-15 | [2026-10-15_apresentacao_piloto](2026-10-15_apresentacao_piloto/) | Pasta da apresentação de 15/10 (só cópias): roteiro, testes, vídeos e evaluations do piloto. Comece por `00_ROTEIRO.md` | (montada à mão) |
| 2026-10-15 | [2026-10-15_piloto_baro_vs_noalt](2026-10-15_piloto_baro_vs_noalt/) | Piloto abBRIEF × λ-BRIEF (α = 0,5), final_baro e final_noalt, 5 seeds, ADM2 (4 mapas) e UFRGS (2 mapas). Tabelas em `summary_traj_truth_sync/`, vídeos em `videos/` | `./run_piloto.sh` |
| 2026-10-13 | [2026-10-13_odometria_vo_mapa_17-08](2026-10-13_odometria_vo_mapa_17-08/) | VO nova no ADM2, mapa 17/08/2018: evaluation, odometria e vídeos | — |
| 2026-10-12 | [2026-10-12_odometria_vo_variantes](2026-10-12_odometria_vo_variantes/) | Revisão da odometria: V0 (legado), V1 (VO nova), V2 (VO nova + ruído 17), V3 (legado + ruído 17), 3 seeds cada. Resumo em `resumo.txt` | — |
| 2026-10-01 | [2026-10-01_benchmark_artigo_vanilla](2026-10-01_benchmark_artigo_vanilla/) | Benchmark abBRIEF × λ-BRIEF (α = 0,5), protocolo do artigo (vanilla), ADM2, 5 mapas × 30 seeds. Figura `ADM0036/comparacao_5_mapas_paper.png` | `python3 scripts/benchmark_runner.py --regime vanilla` |
| 2026-10-01 | [2026-10-01_benchmark_12-09_parcial](2026-10-01_benchmark_12-09_parcial/) | Versão parcial e mais antiga do benchmark vanilla, só no mapa 12/09/2018 (abBRIEF 12 seeds, λ 2) | — |
| 2026-09-28 | [2026-09-28_todos_mapas_adm2](2026-09-28_todos_mapas_adm2/) | Duas fases no ADM2 em 11 mapas (2006–2018), seed 1. Resumo em `resumo_todos_mapas.md` | `python3 scripts/batch_run_all_maps.py` |
| 2026-09-28 | [2026-09-28_duas_fases_seed1](2026-09-28_duas_fases_seed1/) | Primeiro teste do filtro em duas fases (ADM2, seed 1): erro final 15,5 m | — |
| 2026-09-27 | [2026-09-27_analise_fim_E1-E5](2026-09-27_analise_fim_E1-E5/) | Diagnóstico do fim do voo do ADM2, séries E1–E5 (seeds, fatores, rastreio isolado). Ver `metrics_tables.md` e `plots/` | `python3 scripts/analise_fim.py` |
| 2026-09-27 | [2026-09-27_adm2_prior_escala](2026-09-27_adm2_prior_escala/) | Prior de escala (altímetro emulado) sem injeção: diverge (348 m) | — |
| 2026-09-27 | [2026-09-27_adm2_seed42_video](2026-09-27_adm2_seed42_video/) | Reprodução do artigo no ADM2 com seed 42 (erro final 15,3 m) + `video_convergencia.mp4` | — |
| 2026-09-17 | [2026-09-17_ufrgs_primeiro_teste](2026-09-17_ufrgs_primeiro_teste/) | Primeiro teste no UFRGS com a engine moderna (erro final 140 m) | — |
| 2026-09-14 | [2026-09-14_voo_completo_02](2026-09-14_voo_completo_02/) | ADM2, configuração do artigo, seed 1: diverge (426 m). Repetição do _01 | — |
| 2026-09-12 | [2026-09-12_voo_completo_01](2026-09-12_voo_completo_01/) | ADM2, configuração do artigo, seed 1: diverge (426 m) | — |

---

## Resumo dos testes

Uma explicação curta de cada experimento da tabela, do mais novo para o mais antigo. Detalhes, comandos completos e conclusões estão em `../HISTORICO_RESULTADOS.md`; as diferenças entre o legado e a engine moderna estão em `../docs/achados_codigo_legado.md`.

**Termos usados**

- **ab** = abBRIEF (α = 0); **λ** = λ-BRIEF.
- **MAE** = erro médio do voo, em metros, 2D.
- **final_baro** = filtro em duas fases + injeção adaptativa + barômetro real + odometria fixa da VO nova.
- **vanilla** = filtro do artigo: sem altímetro, sem injeção, odometria gravada do legado.
- Nos testes do artigo, "λ venceu k/25" conta os pares mapa × seed (5 mapas × 5 seeds) em que o λ teve MAE menor; o p é do Wilcoxon pareado.

### Testes do artigo (Figs. 4 e 5), protocolo novo, 17/10/2026

Os quatro rodam o abBRIEF e o λ-BRIEF em 4 voos (ADM1, ADM2, ADM3, UFRGS) × 5 mapas do artigo, regime final_baro. Nos quatro, **todas as execuções convergiram** (erro < 25 m por 10 frames seguidos). Como rodar: `../docs/MANUAL_TESTES_ARTIGO.md`.

MAE médio por voo (média dos 5 mapas e das seeds), ab → λ:

| Voo | α = 1, 1 seed | α = 0,5, 1 seed | α = 1, 5 seeds | α = 0,5, 5 seeds |
|---|---|---|---|---|
| ADM1 | 55,3 → 40,1 | 55,3 → 43,2 | 40,8 → **27,7** (λ venceu 18/25, p = 0,002) | 40,8 → **32,9** (18/25, p = 0,04) |
| ADM2 | 47,4 → 41,7 | 47,4 → 37,9 | 49,8 → 44,8 (11/25, p = 0,79) | 49,8 → 38,4 (16/25, p = 0,11) |
| ADM3 | 20,9 → 47,5 | 20,9 → 23,2 | 23,6 → **37,7** (2/25, p < 0,001) | 23,6 → **28,5** (2/25, p < 0,001) |
| UFRGS | 29,6 → 17,9 | 29,6 → 17,6 | 29,3 → **15,7** (18/25, p = 0,007) | 29,3 → **14,9** (18/25, p = 0,001) |

**2026-10-17_artigo_novo_a1** (α = 1, seed 1; `paper_comparison.py`)
- Primeira reprodução das Figs. 4 e 5 com a engine moderna: 40 execuções.
- O λ ganha com folga onde o ab demora a convergir: mapa 11-2016 no ADM1 e no ADM2, mapa 06-2014 no UFRGS.
- No ADM3, o λ se perde na descida (frames ~130–200) nos mapas 01-2014 e 09-2017; o ab se recupera.

**2026-10-17_artigo_novo_a1_5seeds** (α = 1, seeds 1..5; `paper_comparison_seeds.py`)
- Mesmo teste com 5 seeds e média. A seed 1 reproduziu bit a bit o teste de 1 seed.
- Confirma: o λ é melhor no ADM1 e no UFRGS, empata no ADM2 e é pior no ADM3.
- O rastreio depois de convergir é parecido nos dois (~8–15 m). A diferença vem do tempo até convergir e das perdas.

**2026-10-17_artigo_novo_a0.5** (α = 0,5, seed 1)
- Só o λ rodou; o ab foi copiado do teste α = 1, porque o ab não usa o α.
- Com 1 seed, o α = 0,5 parecia resolver o ADM3 (MAE 47,5 → 23,2 m; o 09-2017 terminou a 4,8 m em vez de 285 m).

**2026-10-17_artigo_novo_a0.5_5seeds** (α = 0,5, seeds 1..5)
- Com 5 seeds, o α = 0,5 **reduz, mas não elimina**, a perda do ADM3 (λ 37,7 → 28,5 m; ab 23,6 m). No 09-2017, uma seed ainda perde o drone no fim.
- Melhora o ADM2 (λ 44,8 → 38,4 m), mantém o UFRGS e fica um pouco pior que o α = 1 no ADM1 (27,7 → 32,9 m).
- Leitura: o α = 0,5 é o melhor equilíbrio entre os voos, mas no ADM3 (descida forte) a máscara de vegetação ainda descarta informação útil.

### Piloto e apresentação

**2026-10-15_piloto_baro_vs_noalt**
- 120 execuções: duas fases com barômetro real (final_baro) × sem altímetro (final_noalt), ab × λ (α = 0,5), 5 seeds, ADM2 (4 mapas) e UFRGS (2 mapas).
- Com barômetro, os dois convergem 100% e o MAE é igual (ADM2 ~28 m, UFRGS ~12 m).
- Sem altímetro, ainda converge (95–100%), mas demora mais e o MAE do ADM2 dobra (ab 72 m, λ 60 m).
- No mapa 12/09 sem altímetro, o λ achou o drone no frame 25, contra o 117 do ab.

**2026-10-15_apresentacao_piloto**
- Só cópias (roteiro, tabelas, vídeos) para a apresentação de 15/10. Não é um experimento novo.

### Odometria visual

**2026-10-13_odometria_vo_mapa_17-08**
- A VO nova aplicada no ADM2 com o mapa 17/08/2018: evaluation, odometria e vídeos.

**2026-10-12_odometria_vo_variantes**
- VO legada × VO nova (ratio test + MAGSAC, 4000 SIFT), com e sem `--pos_noise 17`; ADM2, ab, 3 seeds.
- Só a combinação VO nova + `--pos_noise 17` melhora o MAE (33,8 → 28,9 m) e reduz o tempo por frame em ~40%.
- Achados paralelos: o GT do ADM2 está ~2 frames atrasado, e o z do GT no pouso não bate com as imagens.

### Benchmark do protocolo do artigo

**2026-10-01_benchmark_artigo_vanilla**
- ab × λ (α = 0,5) no regime vanilla, ADM2, 30 seeds independentes por mapa.
- O λ converge mais vezes em todos os mapas (por exemplo 80% × 53% no 12/09) e por isso tem MAE médio menor.
- A mediana é igual nos dois (10–16 m). Quando os dois convergem, eles erram igual.

**2026-10-01_benchmark_12-09_parcial**
- Versão parcial e mais antiga do benchmark vanilla, só no mapa 12/09/2018. Substituída pela anterior.

### Desenvolvimento do filtro (ADM2)

**2026-09-28_todos_mapas_adm2**
- Filtro em duas fases (seed 1) em 11 mapas do ADM2, de 2006 a 2018.
- Localiza nos mapas de vários anos (resumo em `resumo_todos_mapas.md`).

**2026-09-28_duas_fases_seed1**
- Primeiro teste do `--two_phase`: priors e injeção só até convergir, depois rastreio como no artigo.
- Erro final de 15,5 m com a seed 1, que antes divergia (364 m).

**2026-09-27_analise_fim_E1-E5**
- 29 execuções para entender por que o filtro perdia o drone no pouso do ADM2.
- Priors e injeção contínua aceleram a convergência, mas atrapalham o rastreio fino na descida e na curva. Motivou as duas fases.

**2026-09-27_adm2_prior_escala**
- Prior de escala (altímetro emulado) sem injeção: o Neff cai para ~230 no 1º frame e o filtro diverge (349 m).
- Conclusão: o prior precisa vir junto com a injeção de partículas.

**2026-09-27_adm2_seed42_video**
- Configuração do artigo com a seed 42: converge no frame 125 e termina a 15,3 m. Gerou o vídeo de convergência.
- Mostrou que, na configuração do artigo, convergir ou não depende da seed.

### Primeiros testes da engine moderna

**2026-09-17_ufrgs_primeiro_teste**
- Primeiro teste no UFRGS: converge no meio do voo, mas deriva no fim (erro final 140 m).

**2026-09-12_voo_completo_01** e **2026-09-14_voo_completo_02**
- ADM2 na configuração do artigo, seed 1: diverge (426 m).
- Primeira evidência de que a convergência do legado dependia das repetições encadeadas: o `MCL::restart` herdava o z da execução anterior.

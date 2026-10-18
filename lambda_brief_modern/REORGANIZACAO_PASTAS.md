# Reorganização das pastas de resultados (17/10/2026)

> **Concluída em 17/10/2026.** Todas as pastas da tabela abaixo já foram movidas, e `results_final/` e `resultados/` foram apagadas. A tabela fica como referência: os `cmd.txt` antigos ainda citam os caminhos velhos, e os scripts os traduzem.

**Antes**, havia cinco pastas de resultados (`resultados/`, `results_benchmark/`, `results_final/`, `results_odometria_vo/` e `results_odometria_vo_mapas/`), além de logs soltos.

**Agora**, o layout é este:

```
lambda_brief_modern/
├── experimentos/
│   ├── INDICE.md                  ← comece aqui: uma linha por experimento, o mais novo no topo
│   └── AAAA-MM-DD_<nome>/         ← uma pasta por experimento
└── insumos/
    ├── odom/                      ← odometria fixa da VO nova
    └── altimetro/                 ← relatórios do altímetro real
```

## O que foi movido

Cada pasta foi movida para o caminho novo, dentro de `lambda_brief_modern/`.

| De | Para |
|---|---|
| `results_final/B_piloto` | `experimentos/2026-10-15_piloto_baro_vs_noalt` |
| `results_final/videos` | `experimentos/2026-10-15_piloto_baro_vs_noalt/videos` |
| `results_final/apresentacao_15_10` | `experimentos/2026-10-15_apresentacao_piloto` |
| `results_final/C_artigo_novo_a1` | `experimentos/2026-10-17_artigo_novo_a1` |
| `results_final/odom` | `insumos/odom` |
| `results_final/altimeter` | `insumos/altimetro` |
| `resultados/results_voo_completo_01` | `experimentos/2026-09-12_voo_completo_01` |
| `resultados/results_voo_completo_02` | `experimentos/2026-09-14_voo_completo_02` |
| `resultados/results_ufrgs` | `experimentos/2026-09-17_ufrgs_primeiro_teste` |
| `resultados/results_vis_adm0036` | `experimentos/2026-09-27_adm2_seed42_video` |
| `resultados/results_adm0036_alt` | `experimentos/2026-09-27_adm2_prior_escala` |
| `resultados/analise_fim` | `experimentos/2026-09-27_analise_fim_E1-E5` |
| `resultados/results_2fases_s1` | `experimentos/2026-09-28_duas_fases_seed1` |
| `resultados/results_todos_mapas` | `experimentos/2026-09-28_todos_mapas_adm2` |
| `resultados/results_benchmark` | `experimentos/2026-10-01_benchmark_12-09_parcial` |
| `results_benchmark` | `experimentos/2026-10-01_benchmark_artigo_vanilla` |
| `results_odometria_vo` | `experimentos/2026-10-12_odometria_vo_variantes` |
| `results_odometria_vo_mapas` | `experimentos/2026-10-13_odometria_vo_mapa_17-08` |
| `piloto.log` | `experimentos/2026-10-15_piloto_baro_vs_noalt/piloto.log` |
| `videos.log` | `experimentos/2026-10-15_piloto_baro_vs_noalt/videos/videos.log` |
| `artigo.log`, `artigo_novo_a1.log`, `artigo_novo_a1_parte2.log` | `experimentos/2026-10-17_artigo_novo_a1/` |

Os scripts continuam aceitando o caminho antigo: `experimentos.resolve()` traduz para o novo.

## O que mudou nos scripts

Sem `--out` ou `--output_base_dir`, cada script cria `experimentos/<hoje>_<nome>/` e põe a linha no `INDICE.md`.

| Script | Pasta padrão | Para continuar outro dia |
|---|---|---|
| `paper_comparison.py` | `<hoje>_artigo_<protocolo>_a<α>` (ex.: `artigo_novo_a1`) | `--continuar`. O `--plot_only` já usa a pasta mais recente |
| `paper_comparison_seeds.py` (novo) | `<hoje>_artigo_<protocolo>_a<α>_<N>seeds` (ex.: `artigo_novo_a1_5seeds`) | `--continuar`. O `--plot_only` já usa a pasta mais recente |
| `benchmark_runner.py` | `<hoje>_benchmark_<regime>_<datasets>`, ou `--nome X` | `--nome X --continuar` |
| `run_piloto.sh` | `<hoje>_piloto` | `OUT=experimentos/<data>_piloto ./run_piloto.sh` |
| `make_videos.py` | `<bench>/videos/` (o padrão do bench é o piloto) | — |
| `batch_run_all_maps.py` | `<hoje>_todos_mapas_adm2` | — |

Os scripts que escrevem nos insumos mudaram assim:

- `benchmark_runner.py --make_odom` grava em `insumos/odom`.
- `extract_altimeter.py` grava em `insumos/altimetro`.

Os caminhos antigos que estão nos `cmd.txt` (`results_final/odom/...`) são traduzidos para os novos. Assim, o benchmark não acusa `STALE` e o `make_videos.py` continua achando a odometria.

O `consolidate_benchmark.py` agora pede `--benchmark_dir`, porque não existe mais uma pasta padrão.

Comandos úteis:

```
python3 scripts/experimentos.py listar                     # índice no terminal
python3 scripts/experimentos.py onde results_final/B_piloto # para onde foi uma pasta antiga
python3 scripts/experimentos.py nova teste_x "descrição"   # cria e registra uma pasta à mão
```

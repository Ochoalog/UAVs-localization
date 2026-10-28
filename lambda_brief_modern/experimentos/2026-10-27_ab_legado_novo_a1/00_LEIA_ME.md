# abBRIEF do legado × abBRIEF da engine × λ-BRIEF (α=1)

Protocolo **novo** (regime `final_baro`), λ com α = 1, seeds 1, GT sync (ADM3: GT do Zenodo).

**Pergunta:** quanto da vantagem do λ no artigo vinha do baseline? No legado, o abBRIEF usava um padrão de 256 pares fixo e quase uniforme (`phir2frameworkv2/BriefHeuristic.cpp:1132–1154`); aqui ele é reproduzido com `--pair_pattern legacy_ab` (mesmo `cv::RNG` na semente padrão, mesmas expressões). Detalhes em `docs/achados_codigo_legado.md`.

| Método | Opções da engine |
|---|---|
| abBRIEF legado | `--alpha 0 --pair_pattern legacy_ab` |
| abBRIEF (engine) | `--alpha 0` |
| λ-BRIEF (α=1) | `--alpha 1` + NDVI |

Arquivos: `tabela.md` (por mapa e por voo, com a decomposição), `tabela.csv` (cada execução), `compilado.png` (erro por frame), `execucoes/` (saída bruta).

40 execuções do abBRIEF e do λ foram copiadas de `experimentos/2026-10-17_artigo_novo_a1` (comando idêntico; a engine é determinística para a mesma seed).

## Resumo por voo

| Voo | MAE ab legado | MAE ab | MAE λ | ab legado → λ (artigo) | Baseline | NDVI | Baseline / total |
|---|---|---|---|---|---|---|---|
| ADM1 | 55.7 | 55.3 | 40.1 | 15.6 | 0.4 | 15.2 | 2% |
| ADM2 | 74.1 | 47.4 | 41.7 | 32.4 | 26.7 | 5.7 | 82% |
| ADM3 | 35.4 | 20.9 | 47.5 | -12.1 | 14.5 | -26.6 | — |
| UFRGS | 23.7 | 29.6 | 17.9 | 5.8 | -5.9 | 11.7 | -102% |

Valores positivos = o método da direita erra menos. Ex.: Baseline 20 = o abBRIEF da engine erra 20 m a menos que o abBRIEF do legado.

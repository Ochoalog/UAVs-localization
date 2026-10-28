# v3: abBRIEF e λ-BRIEF com o peso de estabilidade do Sentinel-2

Protocolo **novo** (regime `final_baro`), λ com α = 0.5, seeds 1, GT sync (ADM3: GT do Zenodo).

**Pergunta:** dar menos peso aos pares que caem onde o mapa costuma mudar (desvio do NDVI do S2 ao longo dos anos) traz o abBRIEF para perto do λ no ADM2 sem perder no ADM3?

| Método | Opções da engine |
|---|---|
| abBRIEF | `--alpha 0` |
| λ-BRIEF | `--alpha 0.5` + NDVI |
| abBRIEF+S2A | `--alpha 0 --stability_map insumos/estabilidade/ADM_35_A.png --stability_formula normalized` |

Arquivos: `tabela.md` (por mapa, por voo, ganhos com Wilcoxon e o critério mínimo), `tabela.csv`, `compilado.png`, `execucoes/`. Os parâmetros de cada raster estão no `.json` ao lado dele, em `insumos/estabilidade/`.

8 execuções do abBRIEF e do λ foram copiadas de `experimentos/2026-10-17_artigo_novo_a0.5` (comando idêntico; a engine é determinística para a mesma seed).

## Resumo por voo

| Voo | MAE ab | MAE λ | MAE ab+S2A | Conv. ab | Conv. λ | Conv. ab+S2A |
|---|---|---|---|---|---|---|
| ADM2 | 41.7 | 38.5 | 53.0 | 2/2 | 2/2 | 2/2 |
| ADM3 | 19.9 | 22.9 | 43.7 | 2/2 | 2/2 | 2/2 |


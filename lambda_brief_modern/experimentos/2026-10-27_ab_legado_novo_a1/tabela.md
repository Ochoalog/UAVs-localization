# abBRIEF legado × abBRIEF × λ-BRIEF (α=1) — 1 seed(s) (1)

Protocolo novo · regime final_baro · GT sync (ADM3: GT do Zenodo). Erros em metros.

- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma seed).
- **Conv.** = seeds em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame mediano de convergência.
- **Baseline** = MAE(ab legado) − MAE(ab): o que muda só por trocar o abBRIEF do legado pelo da engine.
- **NDVI** = MAE(ab) − MAE(λ): o efeito da vegetação.
- **Baseline / total** = que fração da diferença ab legado → λ (a comparação do artigo) vem do baseline. Só faz sentido quando a diferença total é positiva e grande; com 1 seed, leia como indício.
- **Nomes:** ab legado = abBRIEF legado (fixo, uniforme); ab = abBRIEF (engine); λ = λ-BRIEF (α=1).

## Por mapa

| Voo | Mapa | MAE ab legado | MAE ab | MAE λ | Conv. ab legado | Conv. ab | Conv. λ | Baseline | NDVI |
|---|---|---|---|---|---|---|---|---|---|
| ADM1 | 12-2018 | 31.2 | 31.5 | 21.9 | 1/1 (30) | 1/1 (33) | 1/1 (21) | -0.4 | 9.6 |
| ADM1 | 01-2014 | 67.5 | 27.7 | 26.7 | 1/1 (56) | 1/1 (43) | 1/1 (33) | 39.7 | 1.0 |
| ADM1 | 11-2016 | 51.7 | 127.9 | 54.1 | 1/1 (50) | 1/1 (104) | 1/1 (59) | -76.2 | 73.7 |
| ADM1 | 09-2017 | 85.7 | 65.5 | 66.6 | 1/1 (85) | 1/1 (64) | 1/1 (55) | 20.2 | -1.1 |
| ADM1 | 02-2014 | 42.4 | 23.8 | 31.1 | 1/1 (45) | 1/1 (28) | 1/1 (41) | 18.5 | -7.2 |
| ADM2 | 12-2018 | 41.4 | 32.1 | 28.4 | 1/1 (26) | 1/1 (17) | 1/1 (16) | 9.3 | 3.7 |
| ADM2 | 01-2014 | 44.2 | 51.3 | 52.8 | 1/1 (29) | 1/1 (35) | 1/1 (31) | -7.1 | -1.5 |
| ADM2 | 11-2016 | 113.5 | 95.5 | 35.9 | 1/1 (26) | 1/1 (21) | 1/1 (23) | 18.0 | 59.6 |
| ADM2 | 09-2017 | 121.8 | 31.1 | 48.9 | 1/1 (70) | 1/1 (20) | 1/1 (30) | 90.6 | -17.7 |
| ADM2 | 02-2014 | 49.7 | 26.7 | 42.4 | 1/1 (33) | 1/1 (20) | 1/1 (27) | 22.9 | -15.7 |
| ADM3 | 12-2018 | 23.2 | 19.4 | 24.1 | 1/1 (4) | 1/1 (4) | 1/1 (4) | 3.8 | -4.7 |
| ADM3 | 01-2014 | 77.2 | 20.4 | 69.9 | 1/1 (6) | 1/1 (5) | 1/1 (6) | 56.9 | -49.6 |
| ADM3 | 11-2016 | 24.6 | 20.8 | 28.2 | 1/1 (7) | 1/1 (4) | 1/1 (5) | 3.8 | -7.4 |
| ADM3 | 09-2017 | 26.6 | 23.3 | 92.3 | 1/1 (5) | 1/1 (5) | 1/1 (5) | 3.3 | -69.1 |
| ADM3 | 02-2014 | 25.6 | 20.6 | 23.0 | 1/1 (6) | 1/1 (4) | 1/1 (5) | 5.0 | -2.4 |
| UFRGS | 08-2011 | 38.3 | 25.4 | 15.5 | 1/1 (20) | 1/1 (0) | 1/1 (19) | 12.9 | 9.9 |
| UFRGS | 06-2014 | 22.8 | 61.9 | 11.8 | 1/1 (13) | 1/1 (14) | 1/1 (9) | -39.2 | 50.1 |
| UFRGS | 12-2002 | 27.2 | 28.0 | 30.9 | 1/1 (41) | 1/1 (0) | 1/1 (38) | -0.8 | -2.9 |
| UFRGS | 04-2015 | 12.7 | 15.1 | 13.0 | 1/1 (13) | 1/1 (18) | 1/1 (12) | -2.4 | 2.2 |
| UFRGS | 10-2012 | 17.6 | 17.7 | 18.4 | 1/1 (15) | 1/1 (23) | 1/1 (22) | -0.1 | -0.7 |

## Por voo (média dos 5 mapas)

| Voo | MAE ab legado | MAE ab | MAE λ | ab legado → λ (artigo) | Baseline | NDVI | Baseline / total |
|---|---|---|---|---|---|---|---|
| ADM1 | 55.7 | 55.3 | 40.1 | 15.6 | 0.4 | 15.2 | 2% |
| ADM2 | 74.1 | 47.4 | 41.7 | 32.4 | 26.7 | 5.7 | 82% |
| ADM3 | 35.4 | 20.9 | 47.5 | -12.1 | 14.5 | -26.6 | — |
| UFRGS | 23.7 | 29.6 | 17.9 | 5.8 | -5.9 | 11.7 | -102% |

Valores positivos = o método da direita erra menos. Ex.: Baseline 20 = o abBRIEF da engine erra 20 m a menos que o abBRIEF do legado.

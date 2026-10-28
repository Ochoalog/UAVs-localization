# v3: peso de estabilidade do Sentinel-2 — 1 seed(s) (1)

Protocolo novo · regime final_baro · λ com α = 0.5 · GT sync (ADM3: GT do Zenodo). Erros em metros.

- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma).
- **Conv.** = execuções em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame mediano de convergência.
- **Ganho** = MAE do método de referência − MAE do método com S2: positivo = o S2 ajudou.
- **Nomes:** ab = abBRIEF; λ = λ-BRIEF; ab+S2A coin = abBRIEF+S2A coin.

## Por mapa

| Voo | Mapa | MAE ab | MAE λ | MAE ab+S2A coin | Conv. ab | Conv. λ | Conv. ab+S2A coin |
|---|---|---|---|---|---|---|---|
| ADM2 | 12-2018 | 32.1 | 32.2 | 166.4 | 1/1 (17) | 1/1 (17) | 1/1 (123) |
| ADM2 | 01-2014 | 51.3 | 44.8 | 192.0 | 1/1 (35) | 1/1 (26) | 1/1 (121) |
| ADM3 | 12-2018 | 19.4 | 21.8 | 55.7 | 1/1 (4) | 1/1 (4) | 1/1 (4) |
| ADM3 | 01-2014 | 20.4 | 24.0 | 74.1 | 1/1 (5) | 1/1 (5) | 1/1 (6) |

## Por voo (média dos mapas)

| Voo | MAE ab | MAE λ | MAE ab+S2A coin | Conv. ab | Conv. λ | Conv. ab+S2A coin |
|---|---|---|---|---|---|---|
| ADM2 | 41.7 | 38.5 | 179.2 | 2/2 | 2/2 | 2/2 |
| ADM3 | 19.9 | 22.9 | 64.9 | 2/2 | 2/2 | 2/2 |

## Ganho do S2 (MAE pareado por mapa e seed)

Mediana de (MAE da referência − MAE com S2) e p de Wilcoxon bilateral. Com 1 seed são 5 pares por voo (o menor p possível é 0,06): leia como indício.

| Comparação | ADM2 | ADM3 | Todos os voos |
|---|---|---|---|
| ab+S2A coin vs ab | -137.5 m (n=2) | -45.0 m (n=2) | -94.0 m (n=4) |
| ab+S2A coin vs λ | -140.7 m (n=2) | -42.0 m (n=2) | -92.1 m (n=4) |

## Critério mínimo: ab+S2A coin converge pelo menos tanto quanto o melhor de ab e λ

| Voo | Conv. ab | Conv. λ | Conv. ab+S2A coin | Atende? |
|---|---|---|---|---|
| ADM2 | 2/2 | 2/2 | 2/2 | sim |
| ADM3 | 2/2 | 2/2 | 2/2 | sim |

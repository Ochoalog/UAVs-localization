# v3: peso de estabilidade do Sentinel-2 — 1 seed(s) (1)

Protocolo novo · regime final_baro · λ com α = 0.5 · GT sync (ADM3: GT do Zenodo). Erros em metros.

- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma).
- **Conv.** = execuções em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame mediano de convergência.
- **Ganho** = MAE do método de referência − MAE do método com S2: positivo = o S2 ajudou.
- **Nomes:** ab = abBRIEF; λ = λ-BRIEF; ab+S2A = abBRIEF+S2A.

## Por mapa

| Voo | Mapa | MAE ab | MAE λ | MAE ab+S2A | Conv. ab | Conv. λ | Conv. ab+S2A |
|---|---|---|---|---|---|---|---|
| ADM2 | 12-2018 | 32.1 | 32.2 | 45.1 | 1/1 (17) | 1/1 (17) | 1/1 (21) |
| ADM2 | 01-2014 | 51.3 | 44.8 | 60.9 | 1/1 (35) | 1/1 (26) | 1/1 (39) |
| ADM3 | 12-2018 | 19.4 | 21.8 | 20.1 | 1/1 (4) | 1/1 (4) | 1/1 (4) |
| ADM3 | 01-2014 | 20.4 | 24.0 | 67.3 | 1/1 (5) | 1/1 (5) | 1/1 (5) |

## Por voo (média dos mapas)

| Voo | MAE ab | MAE λ | MAE ab+S2A | Conv. ab | Conv. λ | Conv. ab+S2A |
|---|---|---|---|---|---|---|
| ADM2 | 41.7 | 38.5 | 53.0 | 2/2 | 2/2 | 2/2 |
| ADM3 | 19.9 | 22.9 | 43.7 | 2/2 | 2/2 | 2/2 |

## Ganho do S2 (MAE pareado por mapa e seed)

Mediana de (MAE da referência − MAE com S2) e p de Wilcoxon bilateral. Com 1 seed são 5 pares por voo (o menor p possível é 0,06): leia como indício.

| Comparação | ADM2 | ADM3 | Todos os voos |
|---|---|---|---|
| ab+S2A vs ab | -11.3 m (n=2) | -23.8 m (n=2) | -11.3 m (n=4) |
| ab+S2A vs λ | -14.5 m (n=2) | -20.8 m (n=2) | -14.5 m (n=4) |

## Critério mínimo: ab+S2A converge pelo menos tanto quanto o melhor de ab e λ

| Voo | Conv. ab | Conv. λ | Conv. ab+S2A | Atende? |
|---|---|---|---|---|
| ADM2 | 2/2 | 2/2 | 2/2 | sim |
| ADM3 | 2/2 | 2/2 | 2/2 | sim |

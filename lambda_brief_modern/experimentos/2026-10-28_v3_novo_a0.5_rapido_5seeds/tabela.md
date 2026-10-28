# v3: peso de estabilidade do Sentinel-2 — 5 seed(s) (1..5)

Protocolo novo · regime final_baro · λ com α = 0.5 · GT sync (ADM3: GT do Zenodo). Erros em metros.

- **MAE** = erro médio do voo (média entre seeds ± desvio, quando há mais de uma).
- **Conv.** = execuções em que o erro ficou < 25 m por 10 frames seguidos; entre parênteses, o frame mediano de convergência.
- **Ganho** = MAE do método de referência − MAE do método com S2: positivo = o S2 ajudou.
- **Nomes:** ab = abBRIEF; λ = λ-BRIEF; ab+S2A = abBRIEF+S2A.

## Por mapa

| Voo | Mapa | MAE ab | MAE λ | MAE ab+S2A | Conv. ab | Conv. λ | Conv. ab+S2A |
|---|---|---|---|---|---|---|---|
| ADM2 | 12-2018 | 30.4 ± 11.3 | 30.3 ± 11.3 | 45.9 ± 42.0 | 5/5 (16) | 5/5 (17) | 5/5 (18) |
| ADM2 | 01-2014 | 57.6 ± 30.7 | 45.0 ± 11.9 | 50.7 ± 18.7 | 5/5 (37) | 5/5 (25) | 5/5 (39) |
| ADM3 | 12-2018 | 21.5 ± 3.0 | 23.4 ± 1.5 | 22.0 ± 2.8 | 5/5 (7) | 5/5 (8) | 5/5 (8) |
| ADM3 | 01-2014 | 23.5 ± 3.0 | 26.7 ± 3.7 | 65.9 ± 6.0 | 5/5 (10) | 5/5 (9) | 5/5 (10) |

## Por voo (média dos mapas)

| Voo | MAE ab | MAE λ | MAE ab+S2A | Conv. ab | Conv. λ | Conv. ab+S2A |
|---|---|---|---|---|---|---|
| ADM2 | 44.0 | 37.6 | 48.3 | 10/10 | 10/10 | 10/10 |
| ADM3 | 22.5 | 25.0 | 43.9 | 10/10 | 10/10 | 10/10 |

## Ganho do S2 (MAE pareado por mapa e seed)

Mediana de (MAE da referência − MAE com S2) e p de Wilcoxon bilateral. Com 1 seed são 5 pares por voo (o menor p possível é 0,06): leia como indício.

| Comparação | ADM2 | ADM3 | Todos os voos |
|---|---|---|---|
| ab+S2A vs ab | +0.4 m (n=10, p=1.00) | -16.8 m (n=10, p=0.00) | -0.8 m (n=20, p=0.05) |
| ab+S2A vs λ | +0.5 m (n=10, p=0.56) | -14.9 m (n=10, p=0.11) | -0.7 m (n=20, p=0.09) |

## Critério mínimo: ab+S2A converge pelo menos tanto quanto o melhor de ab e λ

| Voo | Conv. ab | Conv. λ | Conv. ab+S2A | Atende? |
|---|---|---|---|---|
| ADM2 | 10/10 | 10/10 | 10/10 | sim |
| ADM3 | 10/10 | 10/10 | 10/10 | sim |

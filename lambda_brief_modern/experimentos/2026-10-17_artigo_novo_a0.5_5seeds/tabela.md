# abBRIEF × λ-BRIEF por voo e mapa — média de 5 seeds (1..5)

Protocolo novo · regime final_baro · GT sync (ADM3: GT do Zenodo).

- **Convergiu** = seeds em que o erro ficou < 25 m por 10 frames seguidos (k/N).
- **Frame conv.** = mediana do frame de convergência entre as seeds que convergiram.
- **MAE** = média ± desvio-padrão entre seeds do erro médio do voo.
- **λ < ab** = em quantas seeds o λ teve MAE menor (mesma seed nos dois métodos).
- **Rastreio** = média, entre seeds, da mediana do erro depois de convergir.
- **Final** = média do erro no último frame. Erros em metros.

| Voo | Mapa | Convergiu ab | Convergiu λ | Frame conv. (ab / λ) | MAE ab | MAE λ | λ < ab | Rastreio ab | Rastreio λ | Final ab | Final λ |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ADM1 | 12-2018 | 5/5 | 5/5 | 28.0 / 18.0 | 26.8 ± 14.5 | 19.2 ± 4.7 | 4/5 | 9.3 | 8.0 | 7.0 | 6.5 |
| ADM1 | 01-2014 | 5/5 | 5/5 | 25.0 / 21.0 | 19.0 ± 5.2 | 17.7 ± 4.8 | 4/5 | 9.0 | 8.8 | 5.3 | 4.7 |
| ADM1 | 11-2016 | 5/5 | 5/5 | 46.0 / 28.0 | 60.8 ± 45.0 | 30.9 ± 22.1 | 5/5 | 12.4 | 9.6 | 4.1 | 4.5 |
| ADM1 | 09-2017 | 5/5 | 5/5 | 68.0 / 83.0 | 72.1 ± 14.4 | 71.6 ± 36.6 | 2/5 | 15.8 | 15.4 | 3.8 | 4.1 |
| ADM1 | 02-2014 | 5/5 | 5/5 | 25.0 / 35.0 | 25.5 ± 14.1 | 24.9 ± 11.6 | 3/5 | 11.2 | 11.6 | 7.5 | 6.6 |
| ADM2 | 12-2018 | 5/5 | 5/5 | 16.0 / 17.0 | 30.4 ± 11.3 | 30.3 ± 11.2 | 2/5 | 7.8 | 7.8 | 3.9 | 4.0 |
| ADM2 | 01-2014 | 5/5 | 5/5 | 37.0 / 25.0 | 57.5 ± 30.7 | 45.0 ± 11.9 | 3/5 | 9.3 | 10.7 | 5.6 | 5.4 |
| ADM2 | 11-2016 | 5/5 | 5/5 | 21.0 / 18.0 | 92.3 ± 45.3 | 30.7 ± 11.3 | 5/5 | 10.3 | 9.8 | 6.0 | 6.0 |
| ADM2 | 09-2017 | 5/5 | 5/5 | 20.0 / 23.0 | 30.7 ± 11.7 | 51.7 ± 53.8 | 2/5 | 9.7 | 8.7 | 8.7 | 8.8 |
| ADM2 | 02-2014 | 5/5 | 5/5 | 20.0 / 18.0 | 38.2 ± 20.9 | 34.4 ± 12.7 | 4/5 | 10.0 | 9.9 | 7.5 | 7.4 |
| ADM3 | 12-2018 | 5/5 | 5/5 | 7.0 / 8.0 | 21.4 ± 3.0 | 23.4 ± 1.5 | 1/5 | 14.1 | 13.9 | 6.7 | 6.1 |
| ADM3 | 01-2014 | 5/5 | 5/5 | 10.0 / 9.0 | 23.5 ± 3.0 | 26.7 ± 3.7 | 0/5 | 12.5 | 12.6 | 8.2 | 9.0 |
| ADM3 | 11-2016 | 5/5 | 5/5 | 9.0 / 7.0 | 22.1 ± 0.9 | 24.9 ± 2.9 | 0/5 | 13.5 | 13.6 | 7.8 | 7.7 |
| ADM3 | 09-2017 | 5/5 | 5/5 | 9.0 / 9.0 | 27.3 ± 5.5 | 41.5 ± 28.8 | 1/5 | 12.7 | 17.4 | 3.8 | 62.3 |
| ADM3 | 02-2014 | 5/5 | 5/5 | 8.0 / 8.0 | 23.7 ± 3.2 | 25.8 ± 3.1 | 0/5 | 14.0 | 14.3 | 7.0 | 9.0 |
| UFRGS | 08-2011 | 5/5 | 5/5 | 6.0 / 14.0 | 23.3 ± 8.7 | 12.7 ± 0.7 | 4/5 | 13.4 | 11.7 | 9.9 | 10.1 |
| UFRGS | 06-2014 | 5/5 | 5/5 | 17.0 / 10.0 | 63.6 ± 9.9 | 11.7 ± 0.7 | 5/5 | 37.8 | 10.9 | 8.6 | 8.5 |
| UFRGS | 12-2002 | 5/5 | 5/5 | 37.0 / 36.0 | 29.6 ± 6.7 | 19.9 ± 5.5 | 4/5 | 13.1 | 12.4 | 62.8 | 34.4 |
| UFRGS | 04-2015 | 5/5 | 5/5 | 14.0 / 12.0 | 14.3 ± 0.7 | 12.7 ± 0.5 | 5/5 | 11.0 | 11.8 | 8.9 | 8.4 |
| UFRGS | 10-2012 | 5/5 | 5/5 | 23.0 / 22.0 | 15.5 ± 2.1 | 17.5 ± 2.9 | 0/5 | 10.6 | 11.4 | 10.9 | 10.9 |

## Resumo por voo

- **ADM1**: λ com MAE médio menor em 5/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 40.8 m × λ 32.9 m; Wilcoxon pareado p = 0.039.
- **ADM2**: λ com MAE médio menor em 4/5 mapas; λ venceu 16/25 pares (mapa × seed); MAE médio ab 49.8 m × λ 38.4 m; Wilcoxon pareado p = 0.107.
- **ADM3**: λ com MAE médio menor em 0/5 mapas; λ venceu 2/25 pares (mapa × seed); MAE médio ab 23.6 m × λ 28.5 m; Wilcoxon pareado p = 0.000.
- **UFRGS**: λ com MAE médio menor em 4/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 29.3 m × λ 14.9 m; Wilcoxon pareado p = 0.001.

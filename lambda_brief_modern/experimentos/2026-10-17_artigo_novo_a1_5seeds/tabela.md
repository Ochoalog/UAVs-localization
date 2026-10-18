# abBRIEF × λ-BRIEF (α=1) por voo e mapa — média de 5 seeds (1..5)

Protocolo novo · regime final_baro · GT sync (ADM3: GT do Zenodo).

- **Convergiu** = seeds em que o erro ficou < 25 m por 10 frames seguidos (k/N).
- **Frame conv.** = mediana do frame de convergência entre as seeds que convergiram.
- **MAE** = média ± desvio-padrão entre seeds do erro médio do voo.
- **λ < ab** = em quantas seeds o λ teve MAE menor (mesma seed nos dois métodos).
- **Rastreio** = média, entre seeds, da mediana do erro depois de convergir.
- **Final** = média do erro no último frame. Erros em metros.

| Voo | Mapa | Convergiu ab | Convergiu λ | Frame conv. (ab / λ) | MAE ab | MAE λ | λ < ab | Rastreio ab | Rastreio λ | Final ab | Final λ |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ADM1 | 12-2018 | 5/5 | 5/5 | 28.0 / 16.0 | 26.8 ± 14.5 | 20.2 ± 7.8 | 4/5 | 9.3 | 8.3 | 7.0 | 6.2 |
| ADM1 | 01-2014 | 5/5 | 5/5 | 25.0 / 17.0 | 19.0 ± 5.2 | 18.2 ± 5.1 | 3/5 | 9.0 | 9.1 | 5.3 | 4.5 |
| ADM1 | 11-2016 | 5/5 | 5/5 | 46.0 / 19.0 | 60.8 ± 45.0 | 26.9 ± 17.8 | 5/5 | 12.4 | 9.9 | 4.1 | 4.8 |
| ADM1 | 09-2017 | 5/5 | 5/5 | 68.0 / 49.0 | 72.1 ± 14.4 | 48.2 ± 22.6 | 4/5 | 15.8 | 12.9 | 3.8 | 3.5 |
| ADM1 | 02-2014 | 5/5 | 5/5 | 25.0 / 29.0 | 25.5 ± 14.1 | 24.8 ± 8.1 | 2/5 | 11.2 | 11.4 | 7.5 | 6.1 |
| ADM2 | 12-2018 | 5/5 | 5/5 | 16.0 / 16.0 | 30.4 ± 11.3 | 30.9 ± 13.0 | 2/5 | 7.8 | 8.0 | 3.9 | 4.3 |
| ADM2 | 01-2014 | 5/5 | 5/5 | 37.0 / 27.0 | 57.5 ± 30.7 | 51.6 ± 14.1 | 1/5 | 9.3 | 10.9 | 5.6 | 5.0 |
| ADM2 | 11-2016 | 5/5 | 5/5 | 21.0 / 22.0 | 92.3 ± 45.3 | 48.0 ± 41.0 | 4/5 | 10.3 | 10.1 | 6.0 | 5.8 |
| ADM2 | 09-2017 | 5/5 | 5/5 | 20.0 / 28.0 | 30.7 ± 11.7 | 61.3 ± 59.2 | 1/5 | 9.7 | 8.5 | 8.7 | 9.2 |
| ADM2 | 02-2014 | 5/5 | 5/5 | 20.0 / 20.0 | 38.2 ± 20.9 | 32.1 ± 10.3 | 3/5 | 10.0 | 11.1 | 7.5 | 6.7 |
| ADM3 | 12-2018 | 5/5 | 5/5 | 7.0 / 11.0 | 21.4 ± 3.0 | 26.6 ± 4.3 | 0/5 | 14.1 | 14.1 | 6.7 | 5.7 |
| ADM3 | 01-2014 | 5/5 | 5/5 | 10.0 / 10.0 | 23.5 ± 3.0 | 54.0 ± 24.8 | 1/5 | 12.5 | 13.2 | 8.2 | 9.7 |
| ADM3 | 11-2016 | 5/5 | 5/5 | 9.0 / 9.0 | 22.1 ± 0.9 | 25.5 ± 2.2 | 0/5 | 13.5 | 13.9 | 7.8 | 7.9 |
| ADM3 | 09-2017 | 5/5 | 5/5 | 9.0 / 10.0 | 27.3 ± 5.5 | 54.0 ± 33.7 | 1/5 | 12.7 | 23.8 | 3.8 | 118.9 |
| ADM3 | 02-2014 | 5/5 | 5/5 | 8.0 / 8.0 | 23.7 ± 3.2 | 28.6 ± 7.7 | 0/5 | 14.0 | 14.4 | 7.0 | 10.4 |
| UFRGS | 08-2011 | 5/5 | 5/5 | 6.0 / 13.0 | 23.3 ± 8.7 | 13.3 ± 1.7 | 4/5 | 13.4 | 12.2 | 9.9 | 9.4 |
| UFRGS | 06-2014 | 5/5 | 5/5 | 17.0 / 10.0 | 63.6 ± 9.9 | 13.1 ± 1.5 | 5/5 | 37.8 | 11.6 | 8.6 | 8.4 |
| UFRGS | 12-2002 | 5/5 | 5/5 | 37.0 / 30.0 | 29.6 ± 6.7 | 20.5 ± 6.1 | 4/5 | 13.1 | 12.0 | 62.8 | 25.5 |
| UFRGS | 04-2015 | 5/5 | 5/5 | 14.0 / 12.0 | 14.3 ± 0.7 | 13.1 ± 0.4 | 5/5 | 11.0 | 12.7 | 8.9 | 8.3 |
| UFRGS | 10-2012 | 5/5 | 5/5 | 23.0 / 21.0 | 15.5 ± 2.1 | 18.6 ± 1.7 | 0/5 | 10.6 | 11.5 | 10.9 | 10.7 |

## Resumo por voo

- **ADM1**: λ com MAE médio menor em 5/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 40.8 m × λ 27.7 m; Wilcoxon pareado p = 0.002.
- **ADM2**: λ com MAE médio menor em 3/5 mapas; λ venceu 11/25 pares (mapa × seed); MAE médio ab 49.8 m × λ 44.8 m; Wilcoxon pareado p = 0.791.
- **ADM3**: λ com MAE médio menor em 0/5 mapas; λ venceu 2/25 pares (mapa × seed); MAE médio ab 23.6 m × λ 37.7 m; Wilcoxon pareado p = 0.000.
- **UFRGS**: λ com MAE médio menor em 4/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 29.3 m × λ 15.7 m; Wilcoxon pareado p = 0.007.

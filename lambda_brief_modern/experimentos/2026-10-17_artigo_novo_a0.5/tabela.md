# abBRIEF × λ-BRIEF por voo e mapa — média de 1 seeds (1)

Protocolo novo · regime final_baro · GT sync (ADM3: GT do Zenodo).

- **Convergiu** = seeds em que o erro ficou < 25 m por 10 frames seguidos (k/N).
- **Frame conv.** = mediana do frame de convergência entre as seeds que convergiram.
- **MAE** = média ± desvio-padrão entre seeds do erro médio do voo.
- **λ < ab** = em quantas seeds o λ teve MAE menor (mesma seed nos dois métodos).
- **Rastreio** = média, entre seeds, da mediana do erro depois de convergir.
- **Final** = média do erro no último frame. Erros em metros.

| Voo | Mapa | Convergiu ab | Convergiu λ | Frame conv. (ab / λ) | MAE ab | MAE λ | λ < ab | Rastreio ab | Rastreio λ | Final ab | Final λ |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ADM1 | 12-2018 | 1/1 | 1/1 | 33.0 / 25.0 | 31.5 ± 0.0 | 25.7 ± 0.0 | 1/1 | 9.4 | 8.3 | 7.6 | 6.3 |
| ADM1 | 01-2014 | 1/1 | 1/1 | 43.0 / 45.0 | 27.7 ± 0.0 | 25.2 ± 0.0 | 1/1 | 11.4 | 10.4 | 6.0 | 4.8 |
| ADM1 | 11-2016 | 1/1 | 1/1 | 104.0 / 39.0 | 127.9 ± 0.0 | 40.8 ± 0.0 | 1/1 | 15.6 | 11.1 | 4.3 | 4.8 |
| ADM1 | 09-2017 | 1/1 | 1/1 | 64.0 / 96.0 | 65.5 ± 0.0 | 100.1 ± 0.0 | 0/1 | 14.6 | 18.2 | 4.3 | 4.0 |
| ADM1 | 02-2014 | 1/1 | 1/1 | 28.0 / 35.0 | 23.9 ± 0.0 | 24.1 ± 0.0 | 0/1 | 11.1 | 12.3 | 8.0 | 6.7 |
| ADM2 | 12-2018 | 1/1 | 1/1 | 17.0 / 17.0 | 32.1 ± 0.0 | 32.2 ± 0.0 | 0/1 | 8.0 | 8.2 | 4.1 | 3.9 |
| ADM2 | 01-2014 | 1/1 | 1/1 | 35.0 / 26.0 | 51.4 ± 0.0 | 44.8 ± 0.0 | 1/1 | 10.2 | 11.8 | 5.7 | 5.2 |
| ADM2 | 11-2016 | 1/1 | 1/1 | 21.0 / 20.0 | 95.5 ± 0.0 | 26.6 ± 0.0 | 1/1 | 11.2 | 9.5 | 6.1 | 5.8 |
| ADM2 | 09-2017 | 1/1 | 1/1 | 20.0 / 23.0 | 31.1 ± 0.0 | 37.1 ± 0.0 | 0/1 | 9.0 | 9.5 | 8.9 | 8.7 |
| ADM2 | 02-2014 | 1/1 | 1/1 | 20.0 / 31.0 | 26.7 ± 0.0 | 48.8 ± 0.0 | 0/1 | 10.2 | 9.8 | 7.6 | 7.2 |
| ADM3 | 12-2018 | 1/1 | 1/1 | 4.0 / 4.0 | 19.4 ± 0.0 | 21.8 ± 0.0 | 0/1 | 13.6 | 14.0 | 6.9 | 7.0 |
| ADM3 | 01-2014 | 1/1 | 1/1 | 5.0 / 5.0 | 20.4 ± 0.0 | 24.0 ± 0.0 | 0/1 | 11.7 | 12.5 | 8.9 | 8.6 |
| ADM3 | 11-2016 | 1/1 | 1/1 | 4.0 / 5.0 | 20.8 ± 0.0 | 22.6 ± 0.0 | 0/1 | 12.9 | 13.3 | 8.8 | 7.3 |
| ADM3 | 09-2017 | 1/1 | 1/1 | 5.0 / 5.0 | 23.2 ± 0.0 | 24.2 ± 0.0 | 0/1 | 12.1 | 12.7 | 3.8 | 4.8 |
| ADM3 | 02-2014 | 1/1 | 1/1 | 4.0 / 5.0 | 20.6 ± 0.0 | 23.2 ± 0.0 | 0/1 | 13.3 | 14.1 | 7.7 | 9.0 |
| UFRGS | 08-2011 | 1/1 | 1/1 | 0.0 / 19.0 | 25.4 ± 0.0 | 13.7 ± 0.0 | 1/1 | 13.3 | 10.9 | 11.8 | 10.9 |
| UFRGS | 06-2014 | 1/1 | 1/1 | 14.0 / 15.0 | 61.9 ± 0.0 | 12.3 ± 0.0 | 1/1 | 26.3 | 10.2 | 8.6 | 9.1 |
| UFRGS | 12-2002 | 1/1 | 1/1 | 0.0 / 44.0 | 28.0 ± 0.0 | 29.5 ± 0.0 | 0/1 | 13.9 | 12.1 | 35.9 | 34.4 |
| UFRGS | 04-2015 | 1/1 | 1/1 | 18.0 / 14.0 | 15.2 ± 0.0 | 13.3 ± 0.0 | 1/1 | 11.1 | 11.7 | 9.1 | 8.8 |
| UFRGS | 10-2012 | 1/1 | 1/1 | 23.0 / 22.0 | 17.7 ± 0.0 | 19.0 ± 0.0 | 0/1 | 10.4 | 11.9 | 11.3 | 12.0 |

## Resumo por voo

- **ADM1**: λ com MAE médio menor em 3/5 mapas; λ venceu 3/5 pares (mapa × seed); MAE médio ab 55.3 m × λ 43.2 m.
- **ADM2**: λ com MAE médio menor em 2/5 mapas; λ venceu 2/5 pares (mapa × seed); MAE médio ab 47.4 m × λ 37.9 m.
- **ADM3**: λ com MAE médio menor em 0/5 mapas; λ venceu 0/5 pares (mapa × seed); MAE médio ab 20.9 m × λ 23.2 m.
- **UFRGS**: λ com MAE médio menor em 3/5 mapas; λ venceu 3/5 pares (mapa × seed); MAE médio ab 29.6 m × λ 17.6 m.

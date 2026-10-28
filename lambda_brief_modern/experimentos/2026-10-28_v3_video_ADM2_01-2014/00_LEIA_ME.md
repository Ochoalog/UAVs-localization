# Vídeos v3 — ADM2 · 01-2014, seed 1

Gerado por `python3 scripts/v3_video.py --voo ADM2 --mapa 01-2014`.

| Arquivo | O que é |
|---|---|
| `video_ab_voo.mp4` | abBRIEF: o voo no mapa com a nuvem de partículas convergindo + os 6 painéis ao lado |
| `video_ab_drone.mp4` | abBRIEF: drone, mapa, S2 e peso na orientação da câmera (pose do GT) |
| `video_ab_mapa.mp4` | abBRIEF: mapa, S2 e peso norte para cima, com pegadas e partículas |
| `video_ab_s2a_voo.mp4` | abBRIEF + peso S2-A: o voo no mapa com a nuvem de partículas convergindo + os 6 painéis ao lado |
| `video_ab_s2a_drone.mp4` | abBRIEF + peso S2-A: drone, mapa, S2 e peso na orientação da câmera (pose do GT) |
| `video_ab_s2a_mapa.mp4` | abBRIEF + peso S2-A: mapa, S2 e peso norte para cima, com pegadas e partículas |
| `quadros/<método>/` | um quadro a cada N frames |
| `area_sentinel.png` | área inteira: mapa, S2 RGB e NDVI do dia do voo, peso S2-A, GT |

Painéis: 1 drone RGB (quantizado) + pares · 2 drone NDVI + pontos + máscara de amostragem · 3 mapa · 4 S2 RGB · 5 S2 NDVI · 6 peso S2-A + partículas. Pares coloridos pelo peso S2-A no ponto do mapa (pose do GT); ponta branca/magenta = bit concorda/discorda do drone.

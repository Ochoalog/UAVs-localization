# Teste do artigo λ-BRIEF — abBRIEF × λ-BRIEF (α=1), 1 seed

Protocolo **novo**: regime `final_baro`, λ-BRIEF com α = 1, seed 1, erro contra o GT `traj_truth_sync.txt`.

Cada curva é uma execução da engine: em cada voo, cada mapa (ano) roda uma vez com o abBRIEF e uma vez com o λ-BRIEF. O artigo fez o mesmo com 30 repetições por curva (média); aqui é 1.

## Onde olhar

| Arquivo | O que mostra |
|---|---|
| `compilado_artigo_ADM.png` | Como a Fig. 5: ADM1, ADM2 e ADM3, uma cor por mapa, ab pontilhado × λ cheio |
| `compilado_artigo_UFRGS.png` | Como a Fig. 4 |
| `<VOO>/<VOO>_compilado.png` | O gráfico do voo com a miniatura de cada mapa embaixo, na mesma cor da curva |
| `<VOO>/<VOO>_mapa_<MM-AAAA>.png` | Um mapa (ano): erro do ab × λ por frame em cima; o mapa com o GT e as duas trajetórias estimadas embaixo |
| `tabela.md` / `tabela.csv` | MAE, frame de convergência, rastreio e erro final por voo e mapa |
| `execucoes/` | Saída bruta da engine (errors.txt, traj_estimated.txt, cmd.txt, run.log) |

## Voos e mapas usados

| Voo | Pasta | Imagens | Frames | Mapa (legenda do artigo) | Arquivo | Encontrado |
|---|---|---|---|---|---|---|
| ADM1 | `Datasets/ADM/0035` | 0103–0402 | 300 | 12-2018 | `Datasets/ADM/0039/Mapas/ADM_35_17_12_2018.jpg` | sim |
| ADM1 | `Datasets/ADM/0035` | 0103–0402 | 300 | 01-2014 | `Datasets/ADM/0039/Mapas/ADM_35_27_01_2014.jpg` | sim |
| ADM1 | `Datasets/ADM/0035` | 0103–0402 | 300 | 11-2016 | `Datasets/ADM/0039/Mapas/ADM_35_24_11_2016.jpg` | sim |
| ADM1 | `Datasets/ADM/0035` | 0103–0402 | 300 | 09-2017 | `Datasets/ADM/0039/Mapas/ADM_35_08_09_2017.jpg` | sim |
| ADM1 | `Datasets/ADM/0035` | 0103–0402 | 300 | 02-2014 | `Datasets/ADM/0039/Mapas/ADM_35_02_02_2014.jpg` | sim |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | 12-2018 | `Datasets/ADM/0039/Mapas/ADM_35_17_12_2018.jpg` | sim |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | 01-2014 | `Datasets/ADM/0039/Mapas/ADM_35_27_01_2014.jpg` | sim |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | 11-2016 | `Datasets/ADM/0039/Mapas/ADM_35_24_11_2016.jpg` | sim |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | 09-2017 | `Datasets/ADM/0039/Mapas/ADM_35_08_09_2017.jpg` | sim |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | 02-2014 | `Datasets/ADM/0039/Mapas/ADM_35_02_02_2014.jpg` | sim |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | 12-2018 | `Datasets/ADM/0040_zenodo/Maps/ADM_17_12_2018.jpg` | sim |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | 01-2014 | `Datasets/ADM/0040_zenodo/Maps/ADM_27_01_2014.jpg` | sim |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | 11-2016 | `Datasets/ADM/0040_zenodo/Maps/ADM_24_11_2016.jpg` | sim |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | 09-2017 | `Datasets/ADM/0040_zenodo/Maps/ADM_08_09_2017.jpg` | sim |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | 02-2014 | `Datasets/ADM/0040_zenodo/Maps/ADM_02_02_2014.jpg` | sim |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 08-2011 | `Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_11_08_2011.jpg` | sim |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 06-2014 | `Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_14_06_2014.jpg` | sim |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 12-2002 | `Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_17_12_2002.jpg` | sim |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 04-2015 | `Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_02_04_2015.jpg` | sim |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 10-2012 | `Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_12_10_2012.jpg` | sim |

Notas:

- ADM3: mapas e GT do Zenodo do artigo (zenodo.org/record/4070013), em `Datasets/ADM/0040_zenodo`. O erro é contra `GT_DAT/ADM3_ground_truth.txt` (não há GT re-sincronizado para esse voo); a altura vem do log de voo (`GT_DAT/ADM3.kml`), em `Datasets/ADM/0040/altimeter_baro.txt`.
- As cinco datas da Fig. 5 vêm do `Maps.zip` do Zenodo do ADM3: 17/12/2018, 27/01/2014, 24/11/2016, 08/09/2017 e 02/02/2014 (arquivos idênticos aos ADM_35_* do repositório).
- ADM1 usa `Datasets/ADM/0035/traj_comp.txt` (nomes de imagem corrigidos; o traj.txt original apontava para .png que não existem).

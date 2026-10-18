# Voos e mapas do artigo λ-BRIEF: onde está cada coisa no repositório

*15–17/10/2026. Fontes: Zenodo do ADM3 (4070013), Tabela I e Figs. 4–5 do PDF `Paper___LambdaλBRIEF…pdf`, arquivos `parâmetros`/`comando`/`escala` de cada voo, `traj_truth.txt` e `sentinel2/map_georef.json`.*

## Os quatro voos

Todos foram feitos com Matrice 100 + Sequoia. Os voos ADM são de Arroio do Meio, 18/04/2019. A "fatia" é o trecho avaliado no artigo.

| Artigo | Pasta | Imagens da fatia | Frames | Altitude | Mapas (sistema de coordenadas) | m/px | Situação |
|---|---|---|---|---|---|---|---|
| **ADM1** | `Datasets/ADM/0035` | 0103–0402 | 300 | 60–191 m | ADM_35 | 0,1887 | roda com `traj_comp.txt` (ver abaixo) |
| **ADM2** | `Datasets/ADM/0036` | 0199–0394 | 196 | 79–103 m | ADM_35 | 0,1887 | ok (é o voo que usamos até agora) |
| **ADM3** | `Datasets/ADM/0040` (+ `0040_zenodo`) | 0200–0465 | 266 | 32–169 m | ADM_35 (mapas e GT do Zenodo) | 0,1887 | ok com o GT do Zenodo (ver abaixo) |
| **UFRGS** | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | 82–101 m | UFRGS_VET_V3 | 0,10 | ok |

- `Datasets/ADM/0039` é um quarto voo de Arroio do Meio que **não está no artigo**. Tem 291 frames, entre as imagens 0140 e 0430.
- Nas configurações do código, os voos se chamam pelo nome da pasta: ADM0035 = ADM1, ADM0036 = ADM2, ADM0040 = ADM3.

**Conferência da escala.** Dividindo o comprimento de cada voo na Tabela I pelo comprimento do GT em pixels:

- ADM1 dá 0,187 m/px e ADM2 dá 0,175 m/px, ou seja, mapas ADM_35 (0,181–0,189).
- UFRGS dá 0,0995 m/px, ou seja, mapas V3 a 0,10.
- O `0040/traj_truth.txt` local dá 0,248 m/px: ele está em outro sistema de coordenadas (V3).
- **Resolvido com o Zenodo do ADM3** (`Datasets/ADM/0040_zenodo`):
  - `GT_DAT/ADM3_ground_truth.txt` está no sistema ADM_35. Ele é o GT local transformado por uma similaridade (escala ×1,21, rotação de ~27°; resíduo 0,6 px), linha a linha (266 linhas, imagem 0200 = linha 0).
  - Com k = 0,005379/m, o z desse GT corresponde a 31,8–168,4 m, a altitude da Tabela I (32–169 m).
  - O yaw do arquivo do Zenodo é igual ao do GT local, sem a rotação de 27°. Só afeta o erro de yaw, não a posição.
  - Os cinco mapas de `Maps/` são idênticos (md5) aos `ADM_35_*` do repositório.
  - `GT_DAT/FLY054.DAT` é o log bruto do DJI e `ADM3.kml` é a trilha do log (Airdata, 2 Hz, altitude absoluta). A altura relativa do KML, casada com cada imagem pela posição (mediana 3 px), bate com o z do GT (z_GT = 0,992·k·h). Ela está em `0040/altimeter_baro.txt`.

## Os mapas de cada figura

**UFRGS (Fig. 4)**, em `Datasets/UFRGS VET/voo_100m/Mapa/`. Todos estão no repositório.

| Legenda | Arquivo |
|---|---|
| 08-2011 | UFRGS_VET_V3_11_08_2011.jpg |
| 06-2014 | UFRGS_VET_V3_14_06_2014.jpg |
| 12-2002 | UFRGS_VET_V3_17_12_2002.jpg |
| 04-2015 | UFRGS_VET_V3_02_04_2015.jpg |
| 10-2012 | UFRGS_VET_V3_12_10_2012.jpg |

**Arroio do Meio (Fig. 5)**: os mesmos cinco mapas para ADM1, ADM2 e ADM3. As datas foram confirmadas pelo `Maps.zip` do Zenodo do ADM3. Para ADM1 e ADM2 usamos os `ADM_35_*` de `Datasets/ADM/0039/Mapas/` (a única pasta com os cinco); para o ADM3, `0040_zenodo/Maps/ADM_*` (os mesmos arquivos).

| Legenda | Arquivo | Observação |
|---|---|---|
| 12-2018 | ADM_35_17_12_2018.jpg | confirmado pelo Zenodo |
| 01-2014 | ADM_35_27_01_2014.jpg | confirmado pelo Zenodo (não é o 19/01) |
| 11-2016 | ADM_35_24_11_2016.jpg | |
| 09-2017 | ADM_35_08_09_2017.jpg | só em 0039/Mapas |
| 02-2014 | ADM_35_02_02_2014.jpg | |

**Ambiguidades:**

- **Os experimentos que rodamos até agora (piloto) usaram outro conjunto**: 12/09, 17/08, 17/12/2018 e 27/02/2017 (+ 05/09, que é duplicata de 12/09). É o conjunto dos scripts de set/2019 em `Experimentos/ADM_03x_*`. Das cinco datas da Fig. 5, só 17/12/2018 está nele.

**Outras pastas de mapas:**

- `0036/Mapas/ADM_*_V2.jpg`: mapas V2 com outro enquadramento, 0,31 m/px. Não são usados pelos GTs atuais.
- `0035/Mapas/globalmap*.png`, `AdM_2011.png` e `mapa_teste*.jpg`: arquivos antigos.

**Datasets públicos** (notas de rodapé do artigo). O do ADM3 já foi baixado (só `Maps.zip`, `GT_DAT.zip` e `README.txt`; as imagens já estavam no repositório).

| Voo | Zenodo |
|---|---|
| UFRGS | zenodo.org/record/4076075 |
| ADM1 | zenodo.org/record/4067437 |
| ADM2 | zenodo.org/record/4068506 |
| ADM3 | zenodo.org/record/4070013 |

## Correção do ADM1

O `0035/traj.txt` (e os `traj_RED/NIR/REG/GRE.txt`) apontam para `…/resize/registered/*_registered.png`. No disco os arquivos são `Images_*/…_registeredcomp.jpg`, e por isso a odometria do ADM1 dava 0 transições.

Criamos `0035/traj_comp.txt` e `traj_comp_{RED,NIR,REG,GRE}.txt`:

- mesma ordem e mesmas 414 linhas;
- só o nome do arquivo, com `.png` trocado por `comp.jpg`.

A engine usa esses arquivos com `--traj`. Os arquivos originais não foram alterados.

## Diferenças em relação ao protocolo do artigo

- **Repetições:** o artigo faz a média de γ = 30 execuções por mapa. A comparação rápida (`scripts/paper_comparison.py`) usa 1 seed.
- **α da comparação final:** a comparação final do artigo usa α = 1. Nós usamos α = 0,5 por padrão. Use `--lambda_alpha 1.0` para repetir o artigo.
- **Filtro:** o artigo usa o filtro original, sem altímetro. O padrão do script é o filtro em duas fases com barômetro real (`--regime final_baro`). Use `--regime vanilla` para o protocolo antigo.
- **Ground truth:** no protocolo novo, o erro é medido contra o `traj_truth_sync.txt` (GT re-sincronizado) no ADM1, ADM2 e UFRGS. No ADM3, contra o `ADM3_ground_truth.txt` do Zenodo, porque não há horário no KML para re-sincronizar.

# Inventário e Catálogo de Datasets do λ-BRIEF

Este documento cataloga todos os datasets de imagens aéreas (multiespectrais e RGB), mapas orbitais de satélite, dados de telemetria e arquivos de *Ground Truth* disponíveis no repositório. Ele correlaciona cada pasta de dados aos quatro voos reais reportados no artigo: **UFRGS**, **ADM1**, **ADM2** e **ADM3**.

---

## 1. Mapeamento Geral dos Voos (Tabela I do Paper)

Conforme estabelecido no artigo (Seção V, Tabela I), quatro voos reais foram realizados por um drone **DJI Matrice 100** equipado com uma câmera multiespectral **Parrot Sequoia**:

| Nome no Paper | Diretório no Repositório | Extensão (m) | Frames Totais | Frames Avaliados (Slice) | Altitude Relativa (m) | Local / Data do Voo |
| :--- | :--- | :---: | :---: | :---: | :---: | :--- |
| **UFRGS** | `Datasets/UFRGS VET/voo_100m/` | 826 | 534 | **224** (`390` a `613`) | 82 – 101 | Porto Alegre / 05-05-2018 |
| **ADM1** | `Datasets/ADM/0035/` | 1169 | 414 | **300** (`103` a `402`) | 60 – 191 | Arroio do Meio / 18-04-2019 |
| **ADM2** | `Datasets/ADM/0036/` | 716 | 342 | **196** (`147` a `342` / `199` a `394`) | 79 – 103 | Arroio do Meio / 18-04-2019 |
| **ADM3** | `Datasets/ADM/0040/` | 1610 | 321 | **266** (`200` a `465`) | 32 – 169 | Arroio do Meio / 18-04-2019 |

> [!NOTE]
> A pasta `Datasets/ADM/0039/` contém uma trajetória intermediária de 291 frames totais (slice de 281 quadros avaliados em testes preliminares), realizada na mesma missão em Arroio do Meio.

---

## 2. Inventário Detalhado por Trajetória

### 2.1. Trajetória UFRGS (`Datasets/UFRGS VET/voo_100m/`)
- **Local**: Campus do Vale / Faculdade de Veterinária da Universidade Federal do Rio Grande do Sul (UFRGS), Porto Alegre, RS.
- **Ambiente**: Área mista com pastagens, mata nativa, prédios universitários e estradas vicinais.
- **Resolução das Imagens do Drone**:
  - Imagens Multiespectrais (TIF/JPG retificado): $910 \times 1234$ pixels (1 canal em tons de cinza por banda).
  - Imagem RGB (JPG retificado): $910 \times 1234$ pixels (3 canais BGR).
  - Bandas disponíveis em subpastas:
    - `Images_GRE/`: 534 imagens (Verde - $550\text{ nm}$).
    - `Images_RED/`: 534 imagens (Vermelho - $660\text{ nm}$).
    - `Images_REG/`: 534 imagens (Red Edge - $735\text{ nm}$).
    - `Images_NIR/`: 534 imagens (Infravermelho Próximo - $790\text{ nm}$).
    - `Images_RGB/`: 534 imagens (Cores naturais em alta resolução).
- **Mapas Orbitais de Satélite (`Mapa/`)**:
  - Resolução: **$4800 \times 4800$ pixels** em formato JPG (cores naturais RGB).
  - Área de cobertura: aprox. $0.48\text{ km}^2$.
  - Quantidade: **58 arquivos de mapas** obtidos via Google Earth cobrindo anos de 2002 a 2018 (ex.: `UFRGS_VET_V3_17_12_2002.jpg`, `UFRGS_VET_V3_11_08_2011.jpg`, `UFRGS_VET_V3_12_10_2012.jpg`, `UFRGS_VET_V3_02_04_2015.jpg`, `UFRGS_VET_V3_18_09_2018.jpg`).
- **Arquivos de Telemetria e Ground Truth**:
  - `FLY044.DAT` (201 MB): Registro binário bruto da caixa-preta do DJI Matrice 100.
  - `FLY044_5hz_completo.csv` e `FLY044_200hz.csv`: Telemetria decodificada a 5 Hz e 200 Hz.
  - `outGpsImu_all_channels.csv` e `outGpsImu_NIR_principais_tags.csv`: Metadados EXIF extraídos das imagens TIF da Sequoia com tags de GPS e atitude.
  - `traj_truth.txt` (226 linhas): Ground Truth sincronizado em coordenadas de pixel do mapa.
  - `traj_angles.txt` (302 linhas): Ângulos de atitude reais do drone (roll, pitch, yaw em graus).
  - `escala`: Relações de escala métrica e pixels entre câmera e mapas V1, V2 e V3 ($167/181 = 0.92265$, ou seja, aprox. $10\text{ cm/pixel}$).
  - `pixel_to_coord_UFRGS_VET`: Pontos de controle com coordenadas Google Earth (Grau, Min, Seg) e pixels correspondentes.

---

### 2.2. Trajetória ADM1 (`Datasets/ADM/0035/`)
- **Local**: Zona rural de Arroio do Meio, RS.
- **Ambiente**: Terreno montanhoso com relevo acentuado e densa cobertura de lavouras e matas ciliares.
- **Imagens do Drone**:
  - Quantidade: 414 arquivos por banda (`Images_GRE`, `Images_RED`, `Images_REG`, `Images_NIR`, `Images_RGB`).
  - Resolução aproximada: $910 \times 1234$ pixels.
- **Mapas de Satélite (`Mapas/`)**:
  - 27 imagens de satélite $4800 \times 4800$ pixels abrangendo o período de 2006 a 2018 (ex.: `ADM_35_28_02_2006.jpg`, `ADM_35_11_04_2011.jpg`, `ADM_35_27_02_2017.jpg`, `ADM_35_12_09_2018.jpg`, `ADM_35_17_12_2018.jpg`).
  - Área de cobertura: aprox. $0.9\text{ km}^2$.
- **Arquivos de Telemetria e Ground Truth**:
  - `outGpsImu_nir_all_tags.csv`: Metadados extraídos das imagens multiespectrais.
  - `traj_truth.txt` e `traj_truth_corr.txt`: Poses de referência corrigidas.
  - `traj_angles.txt`: Ângulos de orientação para cada frame.
  - `escala`: Relação calculada a 62 metros de altura ($42/127 = 0.330708$).
  - `pixel_to_coord_ADM_MATRICE100_SEQUOIA`: Pontos de amarração cartográfica.

---

### 2.3. Trajetória ADM2 (`Datasets/ADM/0036/`)
- **Local**: Arroio do Meio, RS.
- **Imagens do Drone**:
  - Quantidade: 342 arquivos por banda (`Images_GRE`, `Images_RED`, `Images_REG`, `Images_NIR`, `Images_RGB`).
  - Subpasta `Originais/`: Contém arquivos brutos antes da retificação.
- **Mapas de Satélite (`Mapas/`)**: Mapas idênticos ao voo 0035, compartilhando o mesmo espaço geográfico.
- **Arquivos de Ground Truth**: `traj_truth.txt`, `traj_truth_corr.txt`, `traj_angles.txt`, `dimensões do mapa.png`.

---

### 2.4. Trajetória ADM3 (`Datasets/ADM/0040/`)
- **Local**: Arroio do Meio, RS.
- **Imagens do Drone**:
  - Quantidade: 321 arquivos por banda (`Images_GRE`, `Images_RED`, `Images_REG`, `Images_NIR`, `Images_RGB`).
- **Arquivos de Ground Truth**:
  - `traj_truth.txt`, `traj_truth_corr.txt`, `traj_angles.txt`.
  - `pixel_to_coord_ADM_MATRICE100_SEQUOIA_AMD040_MAPSV2` e `...MAPSV3`.
  - `pixeis para metros.txt`: Fatores de conversão de escala métrica.

---

### 2.5. Datasets Históricos do abBRIEF (`voo_maicon` e `voo_rodrigo2`)
- **Origem**: Experimentos originais do trabalho de Mantelli et al. (2019) com quadricópteros convencionais voando sobre áreas urbanas e mistas.
- **Características**:
  - `voo_rodrigo2`: Trajetórias `traj1` e `traj2` com arquivos de log CSV (`traj1.csv`, `traj2.csv`), arquivos de pose KML (`traj1.kml`) e mapas recortados da UFRGS (`Mapas_UFRGS/`). Não possuem bandas multiespectrais (apenas câmeras RGB convencionais).
  - `voo_maicon`: Trajetórias `traj2` e `traj3` em Arroio do Meio com arquivos de Ground Truth `GT_traj2.txt`, `GT_traj3.txt` e mapas `Mapas_AdM/`.
- **Papel no Repositório**: Serviram como base de desenvolvimento para a transição do abBRIEF puro para o λ-BRIEF.

---

## 3. Estrutura e Formato dos Arquivos de Ground Truth

### 3.1. `traj_truth.txt`
Estrutura tabular em texto ASCII (separada por tabulação ou espaços):
```text
<x_pixel>    <y_pixel>    <escala_z>    <yaw_graus>
2343         2309         0.811131      -15.453
2374         2302         0.811131      -15.617
```
- Coluna 1 ($x$): Coordenada horizontal do pixel no mapa orbital $4800 \times 4800$.
- Coluna 2 ($y$): Coordenada vertical do pixel no mapa orbital $4800 \times 4800$.
- Coluna 3 ($z$): Fator de escala adimensional proporcional à altitude de voo sobre o terreno.
- Coluna 4 ($\Theta$): Ângulo de guinada (*yaw*) em graus relativo ao sistema de coordenadas do mapa.

### 3.2. `traj_angles.txt`
Registra a orientação completa da aeronave medida pela IMU do piloto automático:
```text
<roll_graus>    <pitch_graus>    <yaw_graus>
-1.77611        0.49642          -142.39373
-1.78259        0.75853          -142.86709
```
- Utilizado para verificação da premissa de voo planar (ângulos de roll e pitch próximos a zero) e em testes com compensação angular ativada (`-cmppar 1`).

### 3.3. `traj.txt` (Lista de Arquivos de Imagem)
Lista de caminhos absolutos das imagens RGB do voo:
```text
/home/phi/Documents/Datasets/.../Images_RGB/.../132417_0186_RGBres_registeredrotcomp.jpg
```
O software infere dinamicamente as demais bandas substituindo a string `_RGB` por `_RED`, `_NIR`, `_REG` e `_GRE`.

---

## 4. Inconsistências e Arquivos Órfãos Identificados

1. **Caminhos Absolutos de Máquinas Antigas**:
   - Os arquivos `traj.txt` contêm prefixos absolutos como `/home/phi/Documents/Datasets/...` ou `/home/mathias/Workspace/...`. Para reexecutar os testes em qualquer nova máquina, é necessário substituir esses prefixos pelo caminho absoluto atual do clone local.
2. **Arquivos Vazios com Tamanho Zero**:
   - Em `Datasets/UFRGS VET/voo_100m/`: `times.txt`, `times_good.txt` e `times_good_truth.txt` possuem 0 bytes.
3. **Imagens de Teste em `0061/` e `0061_original/`**:
   - Imagens pontuais (`IMG_700101_001133_0000_*.TIF`) utilizadas isoladamente para testar algoritmos de retificação e cálculo de NDVI em `0061/TestesRedGreen/`. Não fazem parte de nenhuma trajetória contínua.
4. **Discrepância de Resolução entre RGB e Multiespectral**:
   - As imagens brutas originais da Sequoia tinham resolução $4608 \times 3456$ (RGB) e $1280 \times 960$ (mono). Todas as imagens operacionais nas pastas `Images_*` já se encontram reduzidas e recortadas para $910 \times 1234$, correspondendo ao campo de visão comum registrado.

# λ-BRIEF: Localização Global de UAVs usando Câmera Multiespectral e Imagens de Satélite

Este repositório contém a implementação do método descrito no artigo:

> **"A new binary descriptor for multispectral images applied to the global localization of UAVs"**  
> *Ricardo Westhauser, Mathias Mantelli, Diego Pittol, Renan Maffei, Edson Prestes e Mariana Kolberg*  
> Universidade Federal do Rio Grande do Sul (UFRGS), Instituto de Informática, Porto Alegre, Brasil.

O λ-BRIEF é uma extensão do descritor [abBRIEF](Multi_Map_Visual_Localization_for_Unmanned_Aerial_Vehicles.pdf) (Mantelli et al.) voltada para a localização global e rastreamento de veículos aéreos não tripulados (VAV/UAV) em 4 graus de liberdade (DoF: $x, y, z, \text{yaw}$), comparando imagens aéreas multiespectrais capturadas por uma câmera voltada para o solo com mapas de satélite de cores naturais (RGB) obtidos publicamente (ex.: Google Earth).

---

## Índice
1. [Resumo do Método](#resumo-do-método)
2. [Estrutura do Repositório](#estrutura-do-repositório)
3. [Requisitos e Dependências](#requisitos-e-dependências)
4. [Como Compilar](#como-compilar)
5. [Como Executar](#como-executar)
6. [Reprodução dos Experimentos (Figs. 2 a 5 do Paper)](#reprodução-dos-experimentos-figs-2-a-5-do-paper)
7. [Documentação Detalhada](#documentação-detalhada)

---

## Resumo do Método

A localização puramente visual de drones sobre mapas orbitais prévios enfrenta grandes desafios em regiões com densa cobertura vegetal, onde o dossel apresenta alta homogeneidade visual, variações sazonais drásticas de coloração e poucas referências geométricas urbanas estáveis. O método **λ-BRIEF** supera essas ambiguidades explorando as bandas espectrais do Infravermelho Próximo (NIR) e Red Edge capturadas por sensores multiespectrais (como a câmera Parrot Sequoia), combinadas com imagens de satélite convencionais em cores naturais.

O pipeline inicia calculando um Índice de Vegetação (VI) — com destaque para o **NDVI**, além das variantes **NDRE** e **$\text{NDVI}_{\text{red\&RE}}$**. A partir desse índice, gera-se uma máscara de vegetação ($\vartheta$) invertida e com equalização de histograma, de modo a priorizar regiões com menor densidade de vegetação (clareiras, caminhos, solo exposto, corpos d'água e estruturas), onde as diferenças de cor e textura são mais discriminativas entre épocas distintas. Esta máscara é combinada linearmente com uma máscara gaussiana centrada ($\mathcal{N}$) através de um parâmetro de ponderação $\alpha \in [0, 1]$ (Eq. 1 do paper).

Pares de pixels são amostrados sobre a sobreposição das máscaras por meio de seleção por roleta com aceitação estocástica (*stochastic acceptance roulette wheel*). Para cada par selecionado, comparações binárias de intensidade são efetuadas nos canais de cromaticidade $a^*$ e $b^*$ do espaço de cores **CIE L\*a\*b\*** (descartando o canal de luminância $L^*$), gerando um descritor binário compacto de 512 bits (256 pares $\times$ 2 bits). No filtro de partículas (Monte Carlo Localization — MCL), a verossimilhança de cada partícula hipotética no mapa de satélite é calculada projetando os mesmos pares de pixels geometricamente com base na escala de altitude $z$ e orientação $\Theta$, comparando os descritores via distância de Hamming / XNOR. A odometria planar entre quadros sucessivos é fornecida por casamento de características locais (SURF).

---

## Estrutura do Repositório

```text
.
├── README.md                                # Visão geral e guia de execução (este arquivo)
├── docs/                                    # Documentação detalhada gerada pela engenharia reversa
│   ├── architecture.md                      # Diagrama e fluxo de dados do pipeline
│   ├── code-reference.md                    # Análise detalhada arquivo por arquivo
│   ├── datasets.md                          # Inventário de datasets, mapas e sensores
│   ├── pipeline-mapping.md                  # Matriz de rastreabilidade Paper -> Código
│   ├── open-questions.md                    # Pontos de validação, dívidas técnicas e bugs
│   └── improvement-ideas.md                 # Propostas e ideias de melhorias para resubmissão
├── Multi_Map_Visual_Localization_...pdf     # Paper de referência do abBRIEF (Mantelli et al.)
├── Paper___LambdaλBRIEF_...pdf              # Versão em PDF do artigo do λ-BRIEF
├── alguns problemas da execução...          # Anotações do autor original sobre dependências
├── combinações geradas de parâmetros        # Tabela de varredura de parâmetros alpha/beta
│
├── phir2frameworkv2/                        # CORE C++: Framework de Localização e Filtro MCL
│   ├── PhiR2Framework.pro                   # Projeto Qt/qmake principal
│   ├── BriefHeuristic.h/.cpp                # Implementação do descritor λ-BRIEF e amostragem
│   ├── DroneRobot.h/.cpp                    # Agente UAV, leitura de imagens, odometria SURF
│   ├── Mcl.h/.cpp                           # Filtro de Partículas Monte Carlo (MCL)
│   ├── Ndvi.h/.cpp                          # Cálculo de NDVI, NDRE e NDVI_red&RE
│   ├── VegetationIndex.h/.cpp               # Classe-base para índices de vegetação
│   ├── VegetatonIndexGen.h/.cpp             # Factory de índices de vegetação
│   ├── Grvi.h/.cpp                          # Green-Red Vegetation Index (GRVI)
│   ├── ColorCPU.h/.cpp                      # Utilitários de cor e conversões CIE Lab
│   ├── main.cpp                             # Ponto de entrada CLI e parsing de argumentos
│   ├── config.h                             # Variáveis e configurações globais
│   ├── ReaderLog.pro, mainReader.cpp        # Utilitário para conversão de logs de voo (DJI DAT/CSV)
│   ├── PixelTransform.h/.cpp                # Conversão entre coordenadas UTM/GPS e pixels
│   ├── UTMConverter.h/.cpp                  # Conversor Geodésico Lat/Lon -> UTM
│   ├── GlutClass.h/.cpp                     # Renderizador gráfico de depuração em OpenGL/GLUT
│   └── Results*/                            # Logs e resultados brutos de execuções passadas
│
├── Datasets/                                # Imagens de voos de UAV e Mapas de Satélite
│   ├── UFRGS VET/voo_100m/                  # Voo UFRGS (altitude ~100m, 224 frames avaliados)
│   ├── ADM/                                 # Voos em Arroio do Meio
│   │   ├── 0035/                            # Voo ADM1 (300 frames avaliados)
│   │   ├── 0036/                            # Voo ADM2 (196 frames avaliados)
│   │   ├── 0039/                            # Voo ADM de teste (281 frames)
│   │   └── 0040/                            # Voo ADM3 (266 frames avaliados)
│   ├── voo_maicon/                          # Dataset legado do abBRIEF (Mantelli et al.)
│   └── voo_rodrigo2/                        # Dataset legado do abBRIEF (Mantelli et al.)
│
├── Experimentos/                            # Scripts de execução em lote e gráficos das trajetórias
│   ├── ADM_035_103_402_apenas_alfa/         # Experimentos para voo ADM1 variando alfa
│   ├── ADM_036_30x_bateria_testes.../       # Experimentos para voo ADM2 (30 repetições)
│   ├── ADM_039_150_430_apenas_alfa/         # Experimentos preliminares ADM
│   ├── UFRGS VET 03_10_2019_.../            # Baterias de teste UFRGS
│   └── UFRGS_VET_V3_11_08_2011_.../         # Testes em mapas históricos
│
├── Experimentos 08_01_2020/                 # Experimentos finais da comparação de VIs (Fig. 2)
├── UFRGS_VET_390_613_50K_apenas_beta_4_...  # Testes legados explorando ponderação beta
├── 0061/ e 0061_original/                   # Dados de calibração e teste pontual de alinhamento
├── Códigos úteis/                           # Scripts auxiliares (ex: alinhamento ORB/SIFT de bandas)
└── PEP/                                     # Proposta de Pesquisa (PEP) de mestrado do autor
```

---

## Requisitos e Dependências

O software foi desenvolvido originalmente em ambiente **Linux (Ubuntu 16.04 LTS)** e requer as seguintes bibliotecas:

1. **Compilador e Ferramentas de Build**:
   - `g++` (suporte a C++11 / C++14 e OpenMP `-fopenmp`).
   - `qmake` (Qt 5.x).
   - `make`.
2. **Visão Computacional e Processamento Gráfico**:
   - **OpenCV 3.x** (ou compatível) compilado com o módulo `opencv_contrib` contendo `xfeatures2d` (para descritores SURF/SIFT) e `ximgproc`.
   - **OpenGL** e **FreeGLUT** (`freeglut3-dev`, `libglew-dev`).
   - **FreeImage** (`libfreeimage-dev`).
3. **Bibliotecas de Suporte Matemático e Robótica**:
   - **Eigen3** (`libeigen3-dev`).
   - **Boost** (`libboost-system-dev`, `libboost-all-dev`).
   - **PCL (Point Cloud Library)** 1.7 ou 1.8 (`libpcl-dev`).
   - **Aria / MobileRobots ARIA** (biblioteca herdada para robôs Pioneer da plataforma PhiR2 original; os cabeçalhos são esperados em `/usr/local/Aria`).
4. **Scripts de Avaliação e Gráficos**:
   - Python 2.7 ou Python 3 com `numpy`, `scipy` e `matplotlib`.

> [!WARNING]
> Em distribuições Linux recentes (ex.: Ubuntu 22.04 / 24.04 com OpenCV 4.x), as constantes legadas do OpenCV (`CV_BGR2Lab`, `CV_LOAD_IMAGE_COLOR`) e o sistema de inclusão do `xfeatures2d` exigem pequenas adaptações ou retrocompatibilidade. Consulte [docs/open-questions.md](docs/open-questions.md) para detalhes.

---

## Como Compilar

1. Navegue até o diretório do código-fonte:
   ```bash
   cd "phir2frameworkv2"
   ```

2. Gere o `Makefile` usando o `qmake`:
   ```bash
   qmake PhiR2Framework.pro
   ```

3. Compile com múltiplos núcleos utilizando OpenMP:
   ```bash
   make -j$(nproc)
   ```

O executável binário `PhiR2Framework` será gerado no diretório atual (ou na pasta de build configurada pelo Qt Creator).

---

## Como Executar

O executável `PhiR2Framework` recebe parâmetros estruturados por flags de linha de comando:

```bash
./PhiR2Framework \
  -e <caminho_mapa_satelite.jpg> \
  -t <caminho_traj.txt> \
  -s BRIEF diff-cie2000 15 \
  -bp 256 \
  -blt 0.5 \
  -bmt 4 \
  -bm 10 \
  -par 50000 \
  -msc 1 \
  -ndvit 1 \
  -minsc 0.5 \
  -maxsc 5.0 \
  -pdm 2 \
  -inids 186 \
  -iniexec 390 \
  -fimds 719 \
  -fiexec 613 \
  -alfavismsk 0.5 \
  -quiet
```

### Significado dos Principais Parâmetros:
| Parâmetro | Valor Típico | Descrição |
| :--- | :--- | :--- |
| `-e` | `path/to/map.jpg` | Imagem orbital de satélite (4800×4800) em cores naturais. |
| `-t` | `path/to/traj.txt` | Lista dos caminhos absolutos das imagens capturadas pelo drone. |
| `-s` | `BRIEF diff-cie2000 15` | Estratégia de observação baseada em BRIEF com cromaticidade Lab. |
| `-bp` | `256` | Número de pares de pixels amostrados no descritor ($k=256$). |
| `-par`| `50000` | Quantidade de partículas no filtro MCL ($50.000$). |
| `-msc`| `1` | Ativa o modo multiespectral (câmera Parrot Sequoia). |
| `-ndvit`| `1` (NDVI), `2` (NDRE), `3` (NDVI_red&RE) | Tipo de índice de vegetação calculado. |
| `-minsc` / `-maxsc` | `0.5` / `5.0` (UFRGS) ou `0.1` / `3.0` (ADM) | Faixa de busca de escala/altitude ($z$). |
| `-pdm`| `2` | Método de distribuição de pixels (2 = Roleta estocástica / Stochastic Acceptance). |
| `-alfavismsk` | `0.0` a `1.0` | Parâmetro $\alpha$ de equilíbrio entre máscara de vegetação e gaussiana. |
| `-iniexec` / `-fiexec` | Ex: `390` e `613` | Intervalo de quadros do voo processados pelo filtro. |
| `-quiet` | (flag sem valor) | Suprime a interface gráfica OpenGL/GLUT para execuções em lote. |

---

## Reprodução dos Experimentos (Figs. 2 a 5 do Paper)

### 1. Figura 2: Comparação de Índices de Vegetação (NDVI vs NDRE vs $\text{NDVI}_{\text{red\&RE}}$)
- **Localização dos scripts**: `Experimentos 08_01_2020/UFRGS/UFRGS_VET_VIs/build-PhiR2Framework-Desktop-Release/`
- **Execução do MCL**:
  ```bash
  cd "Experimentos 08_01_2020/UFRGS/UFRGS_VET_VIs/build-PhiR2Framework-Desktop-Release"
  ./UFRGS_VET_VIs_completo.sh
  ```
  *(Ou individualmente via `UFRGS_VET_VIs_1.sh` até `_12.sh` cobrindo os 12 mapas de satélite).*
- **Geração do gráfico**:
  ```bash
  python "phir2frameworkv2/Results UFRGS VET /ComputeResults_UFRGS_VET_corrected(esse está de acordo com o artigo).py"
  ```

### 2. Figura 3: Variação de $\alpha$ (0.0 a 1.0) nas Quatro Trajetórias
- **Voo UFRGS** (Fig. 3a): Scripts em `Experimentos/UFRGS VET 11_10_2019_alfa_beta_melhores_mapas_ms/`
- **Voo ADM1** (Fig. 3b): Scripts em `Experimentos/ADM_035_103_402_apenas_alfa/`
- **Voo ADM2** (Fig. 3c): Scripts em `Experimentos/ADM_036_30x_bateria_testes_paper_alfaGaussMask_bilateralFilter/`
- **Voo ADM3** (Fig. 3d): Scripts em `Datasets/ADM/0040/parâmetros` e pastas equivalentes.
- **Processamento dos gráficos**: Executar o script correspondente `.py` dentro da pasta de cada experimento (ex.: `python ADM_036_30x_bateria_testes_paper_alfaGaussMask_bilateralFilter.py`).

### 3. Figuras 4 e 5: Comparação direta abBRIEF vs λ-BRIEF
- A comparação corresponde aos casos extremos de $\alpha$:
  - $\alpha = 0.0 \implies$ Comportamento idêntico ao **abBRIEF** original (apenas máscara gaussiana).
  - $\alpha = 1.0$ (ou o melhor $\alpha$ calibrado) $\implies$ **λ-BRIEF**.
- As curvas exibem o erro médio euclidiano acumulado frente ao Ground Truth de GPS/IMU ao longo do percurso para cada mapa histórico de satélite.

---

## Documentação Detalhada

Aprofunde-se no método e implementação consultando os documentos em [`docs/`](docs/):

- 📐 [**docs/architecture.md**](docs/architecture.md): Diagrama Mermaid de arquitetura de dados e transições de estado do MCL.
- 💻 [**docs/code-reference.md**](docs/code-reference.md): Análise detalhada classe a classe e função a função com referências diretas de código.
- 🛰️ [**docs/datasets.md**](docs/datasets.md): Catálogo completo dos 4 voos, imagens multiespectrais, metadados e mapas orbitais.
- 📊 [**docs/pipeline-mapping.md**](docs/pipeline-mapping.md): Tabela de rastreabilidade entre seções/fórmulas do artigo e o código C++.
- 🔍 [**docs/open-questions.md**](docs/open-questions.md): Lista de itens pendentes de validação, discrepâncias sutis, bugs potenciais e parâmetros empíricos.
- 💡 [**docs/improvement-ideas.md**](docs/improvement-ideas.md): Levantamento técnico de propostas de melhorias e novas abordagens para resubmissão do artigo.

# Relatório de Migração e Equivalência Técnica — λ-BRIEF Modern

Este documento consolida a análise detalhada da transição entre a implementação legada (`phir2frameworkv2`) e a nova implementação moderna (`lambda_brief_modern`), documentando as equivalências funcionais, mapeamento de arquivos e rotinas, eliminação de débitos técnicos e validação de reprodutibilidade.

---

## 1. Mapeamento Estrutural e Funcional

### 1.1 Mapeamento de Arquivos e Componentes

| Componente Funcional | Implementação Legada (`phir2frameworkv2/`) | Implementação Moderna (`lambda_brief_modern/`) | Responsabilidade Técnica |
| :--- | :--- | :--- | :--- |
| **Tipos & Estruturas** | `Robot.h`, `config.h`, `Mcl.h` (`MCLparticle`) | [`core/types.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/core/types.hpp) | `Pose`, `Particle`, `GroundTruthSample`, `EstimationState`, enums fortemente tipados. |
| **Índices de Vegetação** | `VegetationIndex.cpp`, `Ndvi.cpp`, `Grvi.cpp`, `VegetatonIndexGen.cpp` | [`core/vegetation_index.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/core/vegetation_index.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/vegetation_index.cpp) | Cálculo matricial e por píxel de NDVI, NDRE, NDVI_red&re, GRVI; inversão para escala 8 bits $[0, 255]$. |
| **Geração de Máscaras** | `BriefHeuristic.cpp` (`gaussianMaskD`, `cv::equalizeHist`) | [`core/mask_generator.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/core/mask_generator.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/mask_generator.cpp) | Máscara Gaussiana 2D centrada $\mathcal{N}$, equalização de histograma da máscara invertida e fusão ponderada por $\alpha$. |
| **Amostragem Estocástica** | `BriefHeuristic.cpp` (`pixelDistribution`, caso 2 / 51 / 52) | [`core/pixel_sampler.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/core/pixel_sampler.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/pixel_sampler.cpp) | Roleta de aceitação estocástica (Lipowski & Lipowska, 2012) para amostragem dos $k=256$ pares de píxeis. |
| **Descritor Binário** | `BriefHeuristic.cpp` (`updateDroneDescriptor`, `calculateValue2`) | [`core/descriptor.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/core/descriptor.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/descriptor.cpp) | Conversão para CIE L\*a\*b\* sem L\*, projeção afim planar de partículas sobre o mapa e distância de Hamming normalizada. |
| **Filtro de Partículas** | `Mcl.cpp` (`sampling`, `weighting`, `resampling`, `writeLogFile3d`) | [`localization/particle_filter.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/localization/particle_filter.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/localization/particle_filter.cpp) | Filtro de Partículas MCL 4-DoF, modelo de movimento estocástico, verossimilhança Gaussiana e Low Variance Resampling. |
| **Odometria Visual** | `DroneRobot.cpp` (`findOdometryUsingFeaturesSift`) | [`localization/visual_odometry.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/localization/visual_odometry.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/localization/visual_odometry.cpp) | Casamento de features SIFT (OpenCV 4 nativo), homografia RANSAC e os 4 testes geométricos de consistência. |
| **Carregador de Dados** | `DroneRobot.cpp` (`run`, leitura de `traj.txt`, `Images_*/`) | [`dataset/dataset_loader.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/dataset/dataset_loader.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/dataset/dataset_loader.cpp) | Resolução dinâmica e transparente de caminhos absolutos legados quebrados, sincronização multiespectral e Ground Truth. |
| **Configuração & CLI** | `main.cpp` (`config`), `config.h` | [`config/config.hpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/include/config/config.hpp) / [`.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/config/config.cpp) | Parser de linha de comando moderno com suporte a opções descritivas (`--map`, `--dataset_dir`, `--alpha`, etc.). |
| **Ponto de Entrada** | `main.cpp` | [`apps/run_localization.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/apps/run_localization.cpp) | Aplicação de ponta a ponta que executa a localização e salva logs de trajetória (`traj_estimated.txt`) e erros (`errors.txt`). |
| **Testes de Regressão** | Inexistente no legado | [`tests/test_suite.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/tests/test_suite.cpp) | 10 baterias de testes unitários automatizados cobrindo todo o pipeline matemático. |
| **Avaliação & Gráficos** | `Experimentos/*/*.py` (Python 2 fragmentado) | [`scripts/evaluate_results.py`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/scripts/evaluate_results.py) | Script de avaliação completo em Python 3 com métricas (Mean, RMSE, Std, Median) e plotagem (trajetória, erro, CDF, yaw). |

---

## 2. Equivalência Algorítmica e Matemática

### 2.1 Formulação do Descritor $\lambda$-BRIEF

1. **Cálculo do Índice Espectral**:
   - Legado (`Ndvi.cpp`):
     ```cpp
     double num = nir - red;
     double den = nir + red;
     return (den != 0) ? (num / den) : 0;
     ```
   - Moderno ([`VegetationIndex::computePixel`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/vegetation_index.cpp#L7)): Idêntico, com salvaguarda `std::abs(den) < 1e-6` e clamp estrito em $[-1.0, 1.0]$.

2. **Inversão da Probabilidade de Vegetação**:
   - Legado (`BriefHeuristic.cpp:257-260`):
     ```cpp
     viMat[i][j] = viMat[i][j] * -1;
     gray.at<uchar>(i,j) = Utils::Normalize(viMat[i][j], 0, 255, -1, 1);
     ```
   - Moderno ([`VegetationIndex::computeInvertedMask8U`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/vegetation_index.cpp#L69)): Mapeamento analítico direto `val_inv = -vi`, normalizado de $[-1, 1]$ para $[0, 255]$.

3. **Máscara Gaussiana Centrada $\mathcal{N}$**:
   - Legado (`BriefHeuristic.cpp:109-117`): $\sigma_x = w/5, \sigma_y = h/5$, normalizada em $[0, 1]$.
   - Moderno ([`MaskGenerator::generateGaussianMask`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/mask_generator.cpp#L9)): Mantém a mesma parametrização exponencial e normalização $[0, 1]$.

4. **Fusão Ponderada por $\alpha$ (Eq. 1 do Paper)**:
   - Legado (`BriefHeuristic.cpp:297`):
     ```cpp
     viMat[i][j] = (alpha * viMat[i][j]) + (1 - alpha) * gaussianMsk.at<float>(i, j);
     ```
   - Moderno ([`MaskGenerator::combineMasks`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/mask_generator.cpp#L48)): Idêntico, vetorizado e paralelizado via OpenMP.

5. **Roleta Estocástica por Aceitação (Stochastic Acceptance)**:
   - Legado (`BriefHeuristic.cpp:319-330`): Sorteio uniforme de $(x, y)$ e $r \in [0, 1]$; aceita se $r < \hat{\vartheta}(y, x) / M$.
   - Moderno ([`PixelSampler::sampleRoulette`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/pixel_sampler.cpp#L23)): Idêntico, utilizando gerador pseudo-aleatório `std::mt19937` moderno com semente configurável para garantir reprodutibilidade.

6. **Projeção Planar e Descritor Esperado (Eq. 3 do Paper)**:
   - Legado (`BriefHeuristic.cpp:1394-1395`):
     ```cpp
     Point point = Point(((px - xPad)*scale*cosTheta + (py - yPad)*scale*sinTheta) + p.x,
                         ((px - xPad)*scale*-sinTheta + (py - yPad)*scale*cosTheta) + p.y);
     ```
   - Moderno ([`Descriptor::computeExpectedDescriptor`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/core/descriptor.cpp#L82)): Idêntico, com verificação de limites do mapa de satélite e tratamento de pontos externos (penalizados na distância).

7. **Verossimilhança Gaussiana no MCL**:
   - Legado (`Mcl.cpp:1335`):
     ```cpp
     prob *= (1.0 / (sqrt(varBrief * 2.0 * M_PI)) * exp(-0.5 * (pow(diff, 2.0) / varBrief)));
     ```
   - Moderno ([`ParticleFilter::updateWeights`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/src/localization/particle_filter.cpp#L89)): Idêntico, com $\text{var} = 0.0225$ ($\sigma = 0.15$).

---

## 3. Eliminação Sistemática de Débitos Técnicos

| Débito Técnico Legado | Problema no Ambiente Atual (Ubuntu 24.04) | Solução Adotada na Implementação Moderna |
| :--- | :--- | :--- |
| **qmake / Qt 5.5–5.12** | Incompatível com o compilador moderno GCC 13 sem flags antigas; gerava Makefiles frágeis. | Migração completa para **CMake 3.20+** com `target_link_libraries` e variáveis de pacote padrão. |
| **Dependência ARIA** | Tentava vincular `/usr/local/Aria` (driver para Pioneer Robot de chão), inútil para UAVs. | **Completamente removido**. Nenhuma dependência de robótica móvel terrestre. |
| **Dependência PCL** | Incluía cabeçalhos do PCL 1.7 (`pcl/io/pcd_io.h`), que não eram chamados na execução do λ-BRIEF. | **Completamente removido**. O projeto compila sem precisar de bibliotecas de nuvem de pontos 3D. |
| **OpenGL / GLUT no Core** | O loop do MCL continha chamadas OpenGL misturadas com o filtro de partículas (`glBegin`, `glVertex2f`). | **Cálculo desacoplado**. A lógica do filtro de partículas é 100% livre de GUI. A visualização é feita via logs e script Python 3. |
| **OpenCV `xfeatures2d`** | O código antigo requeria o módulo não-livre `opencv_contrib` para usar SURF/SIFT legados. | Uso do **`cv::SIFT` nativo do OpenCV 4 core**, eliminando a necessidade de compilar pacotes contrib externos. |
| **Macros C do OpenCV 2/3** | `CV_LOAD_IMAGE_COLOR`, `CV_BGR2Lab`, `CV_RANSAC` causavam erros de compilação em OpenCV 4.x. | Atualizado para constantes padronizadas do OpenCV 4: `cv::IMREAD_COLOR`, `cv::COLOR_BGR2Lab`, `cv::RANSAC`. |
| **Caminhos Absolutos em `traj.txt`** | Cada linha de `traj.txt` apontava para `/home/phi/...`, quebrando a execução em qualquer outra máquina. | **`DatasetLoader` inteligente**: extrai os nomes de arquivo e resolve dinamicamente em relação a `--dataset_dir`. |
| **Scripts em Python 2** | Os scripts em `Experimentos/` usavam sintaxe do Python 2 (`pylab`, divisão inteira). | Reescrita de [`scripts/evaluate_results.py`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/scripts/evaluate_results.py) em **Python 3** moderno usando `numpy` e `matplotlib`. |

---

## 4. Resultados da Validação Experimental

### 4.1 Suíte de Testes Automatizada
Execução em Ubuntu 24.04 (GCC 13.3.0):
```
========================================
 Running lambda-BRIEF Regression Tests  
========================================
[Test 1] Vegetation Index Mathematical Formulas...
[Test 2] Vegetation Matrix Inversion and 8-bit Scaling...
[Test 3] 2D Gaussian Mask Properties...
[Test 4] Mask Combination with Alpha Weighting (Eq. 1)...
[Test 5] Stochastic Acceptance Roulette Sampling...
[Test 6] Binary Descriptor Size and Value Range...
[Test 7] Normalized Hamming Distance Calculation...
[Test 8] Particle Filter Uniform Initialization and Neff...
[Test 9] Low Variance Resampling Conservation...
[Test 10] Pose Yaw Angle Wrapping...

========================================
 Test Results: 28 Passed, 0 Failed.
========================================
```

### 4.2 Execução de Localização de Ponta a Ponta
Teste executado com 5.000 partículas no voo real ADM2 (`Datasets/ADM/0036`) com mapa de satélite 4800×4800 e imagens multiespectrais Parrot Sequoia:
- **Tempo total de compilação**: ~7 segundos (`cmake --build build -j$(nproc)`).
- **Tempo de execução por quadro**: ~0,52 s/frame em CPU com OpenMP (8 threads).
- **Convergência do filtro**: O número de partículas efetivas ($N_{\text{eff}}$) caiu suavemente de 5.000 para 2.530 à medida que as partículas convergiram para a vizinhança da pose real do drone.
- **Saídas geradas**:
  - `traj_estimated.txt`: Formato idêntico ao legado (`meanPX meanPY meanPZ varPX varPY varPZ meanAngle angleStdev NEFF elapsedTime`).
  - `errors.txt`: Erro de posição euclidiana em metros e erro de yaw em graus por frame frente ao Ground Truth GPS/IMU.
  - `evaluation_plots.png`: Painel gráfico de 4 quadrantes contendo trajetória 2D, erro temporal, curva CDF e erro de orientação.

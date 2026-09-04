# Arquitetura do Sistema e Pipeline de Dados do λ-BRIEF

Este documento descreve a arquitetura detalhada do sistema **λ-BRIEF**, o fluxo de dados dos sensores até a estimativa de pose em 4 Graus de Liberdade (4-DoF: $x, y, z, \text{yaw}$), e a conexão direta de cada estágio com os módulos de software do repositório.

---

## 1. Visão Geral do Pipeline de Dados

O objetivo do sistema é estimar continuamente a pose global de um veículo aéreo não tripulado (UAV) que sobrevoa regiões agrícolas ou com vegetação intermediária a densa, utilizando unicamente:
1. Imagens aéreas de uma câmera multiespectral embarcada voltada para o solo (nadir).
2. Um mapa orbital/satélite pré-existente em cores naturais (RGB) com georreferenciamento conhecido.
3. Estimativas de deslocamento relativo quadro a quadro (odometria visual via casamento de características).

```mermaid
flowchart TD
    subgraph Sensores_UAV["1. Aquisição Sensorial a Bordo"]
        CAM_MS["Câmera Multiespectral (Parrot Sequoia)\nBandas: NIR, Red Edge, Red, Green"]
        CAM_RGB["Câmera RGB Natural Color\nResolução 1280x960 (downsampled)"]
        IMU_GPS["Sensores Auxiliares (Ground Truth)\nGPS + IMU / Bússola"]
    end

    subgraph PreProc["2. Pré-processamento & Registro"]
        CALIB["Remoção de distorção fisheye & retificação\n(developer note Parrot Sequoia)"]
        REG_ALIGN["Alinhamento Inter-bandas\n(Códigos úteis/OrbSiftImgReg.c)"]
        LAB_CONV["Conversão RGB -> CIE L*a*b*\n(DroneRobot.cpp:733)"]
    end

    subgraph VI_Mask["3. Índice de Vegetação & Máscara de Vegetação ϑ"]
        VI_CALC["Cálculo do Índice Espectral\nNDVI / NDRE / NDVI_red&RE\n(Ndvi.cpp:59-93)"]
        VI_INV["Inversão de Probabilidade:\nviMat = viMat * -1\n(BriefHeuristic.cpp:257)"]
        HIST_EQ["Normalização [0, 255] &\nEqualização de Histograma\n(BriefHeuristic.cpp:260, 278)"]
    end

    subgraph Gauss_Mask["4. Máscara Gaussiana N"]
        GAUSS["Distribuição Gaussiana Centrada:\nσx = a/5, σy = b/5\n(BriefHeuristic.cpp:100-119)"]
    end

    subgraph Amostragem["5. Seleção Estocástica de Pares de Pixels"]
        COMP_MASK["Composição Linear:\nϑ_hat = α · ϑ + (1 - α) · N\n(BriefHeuristic.cpp:297)"]
        ROULETTE["Roleta por Aceitação Estocástica\n(Lipowski & Lipowska, 2012)\nSeleção de k=256 pares K\n(BriefHeuristic.cpp:313-331)"]
    end

    subgraph Desc_Obs["6. Descritor Observado d"]
        BRIEF_OBS["Comparações binárias canais a* e b*\nτ_i,j = 1 se δ_j(y_i,1) > δ_j(y_i,2)\n(BriefHeuristic.cpp:76-88)"]
    end

    subgraph Mapa_Global["7. Mapa de Satélite"]
        SAT_MAP["Imagem de Satélite RGB (4800x4800)\n(ex: Google Earth)"]
        MAP_LAB["Conversão Satélite -> CIE L*a*b*\n(DroneRobot.cpp:69)"]
    end

    subgraph MCL_Filter["8. Filtro de Partículas (MCL 4-DoF)"]
        INIT["Inicialização: k=50.000 partículas\nx,y uniformes no mapa, Θ in [0, 2π)\nz in [z_min, z_max]\n(Mcl.cpp:64-85)"]
        ODOM["Odometria Visual (SURF)\nDelta x, Delta y, Delta yaw, Delta z\n(DroneRobot.cpp:2040)"]
        PRED["Amostragem / Predição de Movimento\nPropagação cinemática + Ruído gaussiano\n(Mcl.cpp:1157-1207)"]
        PROJ["Projeção das Partículas:\nY_i = z·R(-Θ)·y_i + pose_xy\nDescritor Esperado d_hat\n(BriefHeuristic.cpp:2285-2306)"]
        WEIGHT["Ponderação de Partículas:\nSimilaridade XNOR / Distância Hamming\nVerossimilhança Gaussiana\n(Mcl.cpp:1335)"]
        RESAMP["Reamostragem: Low Variance Sampler\n(Mcl.cpp:1565-1605)"]
        ESTIM["Estimativa de Pose:\nMédia ponderada da nuvem (x_bar, Θ_bar)\n(Mcl.cpp:892-935)"]
    end

    CAM_MS --> CALIB
    CAM_RGB --> CALIB
    CALIB --> REG_ALIGN
    REG_ALIGN --> LAB_CONV
    REG_ALIGN --> VI_CALC
    VI_CALC --> VI_INV
    VI_INV --> HIST_EQ
    HIST_EQ --> COMP_MASK
    GAUSS --> COMP_MASK
    COMP_MASK --> ROULETTE
    ROULETTE --> BRIEF_OBS
    LAB_CONV --> BRIEF_OBS
    
    SAT_MAP --> MAP_LAB
    MAP_LAB --> PROJ
    
    BRIEF_OBS --> WEIGHT
    PROJ --> WEIGHT
    
    INIT --> PRED
    ODOM --> PRED
    PRED --> PROJ
    WEIGHT --> RESAMP
    RESAMP --> ESTIM
    ESTIM --> PRED
```

---

## 2. Etapas Detalhadas e Módulos de Software

### Etapa 1: Aquisição e Alinhamento Multiespectral
- **Dispositivo**: Drone DJI Matrice 100 com sensor multiespectral Parrot Sequoia em montagem fixa apontada para baixo (nadir).
- **Dados Capturados a 1 Hz**:
  - Imagem colorida RGB de alta resolução redimensionada para $1280 \times 960$ (ou $1234 \times 910$ após corte de bordas de retificação).
  - Quatro imagens monocromáticas correspondentes às bandas:
    - **Verde (GRE)**: $550\text{ nm} \pm 40\text{ nm}$.
    - **Vermelho (RED)**: $660\text{ nm} \pm 40\text{ nm}$.
    - **Red Edge (REG)**: $735\text{ nm} \pm 10\text{ nm}$.
    - **Infravermelho Próximo (NIR)**: $790\text{ nm} \pm 40\text{ nm}$.
- **Alinhamento / Registro**: As bandas sofrem correção de distorção de lente (fisheye) e retificação homográfica (`Códigos úteis/OrbSiftImgReg.c` e scripts de pré-processamento). No código C++, `DroneRobot::DroneRobot` carrega os caminhos a partir de `traj.txt`, `traj_RED.txt`, `traj_NIR.txt`, `traj_REG.txt` e `traj_GRE.txt` ([DroneRobot.cpp:353-426](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DroneRobot.cpp#L353-L426)).

---

### Etapa 2: Espaço de Cores CIE L\*a\*b\*
- **Fundamentação**: Conforme estabelecido no abBRIEF (Mantelli et al.), a luminância $L^*$ é extremamente sensível a sombras, nuvens passageiras e hora do dia. O método descarta $L^*$ e retém exclusivamente os canais de cromaticidade $a^*$ (eixo verde-vermelho) e $b^*$ (eixo azul-amarelo).
- **Implementação**:
  - Imagem do drone: `cv::cvtColor(image, image, CV_BGR2Lab)` ([DroneRobot.cpp:733](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DroneRobot.cpp#L733)).
  - Mapa de satélite: `cv::cvtColor(originalMap, labMap, CV_BGR2Lab)` ([DroneRobot.cpp:69](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DroneRobot.cpp#L69)).
  - Configuração: `BRIEF_HEURISTIC_TYPE = 4` em [config.h:55-57](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/config.h#L55-L57).

---

### Etapa 3: Cálculo do Índice de Vegetação (VI) e Máscara $\vartheta$
- **Objetivo**: Delinear a distribuição de vegetação para guiar a amostragem de pixels.
- **Formulação dos Índices**:
  1. **NDVI** (`ndvit = 1`):
     $$\text{NDVI} = \frac{\text{NIR} - \text{Red}}{\text{NIR} + \text{Red}}$$
     Implementado em [Ndvi.cpp:60-70](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.cpp#L60-L70).
  2. **NDRE** (`ndvit = 2`):
     $$\text{NDRE} = \frac{\text{NIR} - \text{RedEdge}}{\text{NIR} + \text{RedEdge}}$$
     Implementado em [Ndvi.cpp:73-81](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.cpp#L73-L81).
  3. **$\text{NDVI}_{\text{red\&RE}}$** (`ndvit = 3`):
     $$\text{NDVI}_{\text{red\&RE}} = \frac{\text{NIR} - (a \cdot \text{Red} + (1-a) \cdot \text{RedEdge})}{\text{NIR} + (a \cdot \text{Red} + (1-a) \cdot \text{RedEdge})}, \quad a = 0.4$$
     Implementado em [Ndvi.cpp:83-93](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.cpp#L83-L93) e [VegetationIndex.h:53](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetationIndex.h#L53).
- **Pós-processamento da Máscara**:
  - **Inversão**: `viMat[i][j] = viMat[i][j] * -1` ([BriefHeuristic.cpp:257](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L257)). O objetivo técnico é priorizar regiões com *menor* quantidade de vegetação (onde os índices invertidos são mais altos), pois clareiras e estruturas artificiais oferecem pistas mais estáveis e discriminativas entre anos diferentes.
  - **Normalização e Equalização**: Normalização para $[0, 255]$ via `Utils::Normalize` ([BriefHeuristic.cpp:260](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L260)) seguida de `cv::equalizeHist(gray, grayEqualized)` ([BriefHeuristic.cpp:278](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L278)).

---

### Etapa 4: Máscara Gaussiana $\mathcal{N}$
- **Objetivo**: Garantir concentração preferencial de pares próximo ao centro ótico da câmera, em consonância com o descritor BRIEF clássico (Calonder et al., 2010).
- **Formulação**:
  $$\mathcal{N}(x, y) = \exp\left( - \left( \frac{(x - x_0)^2}{2\sigma_x^2} + \frac{(y - y_0)^2}{2\sigma_y^2} \right) \right)$$
  com $x_0 = a/2, y_0 = b/2, \sigma_x = a/5, \sigma_y = b/5$.
- **Implementação**: Método `BriefHeuristic::gaussianMask` ([BriefHeuristic.cpp:100-119](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L100-L119)).

---

### Etapa 5: Composição e Amostragem Estocástica (Eq. 1 do Paper)
- **Equação 1**:
  $$\hat{\vartheta}_{y_i} = \alpha \cdot \vartheta_{y_i} + (1 - \alpha) \cdot \mathcal{N}_{y_i}$$
  Implementada em [BriefHeuristic.cpp:296-300](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L296-L300).
- **Algoritmo de Roleta via Aceitação Estocástica**:
  Proposto por Lipowski & Lipowska (Physica A, 2012), substitui a busca linear $O(N)$ da roleta cumulativa por um teste de rejeição $O(1)$:
  ```cpp
  // BriefHeuristic.cpp:313-331
  for(int pair = 0; pair < totalPairs; pair++) {
      pixelPair = 0;
      while(pixelPair < 2) {
          x = floor(rand() % drone.cols);
          y = floor(rand() % drone.rows);
          random = ((double) rand() / (RAND_MAX));
          if(random < (viMat[y][x] / maximumFitness)) {
              pairs[pair][pixelPair] = cv::Point(x, y);
              pixelPair++;
          }
      }
  }
  ```
  Isso gera um conjunto $K$ de $k=256$ pares de coordenadas relativas de pixels: $p_i = (y_{i,1}, y_{i,2})$.

---

### Etapa 6: Descritor Observado $d$ (Eq. 2 do Paper)
- Para cada par $p_i \in K$ e para cada canal de cor $j \in \{a^*, b^*\}$ ($c=2$):
  $$\tau_{i,j} = \begin{cases} 1, & \text{se } \delta_j(y_{i,1}) > \delta_j(y_{i,2}) \\ 0, & \text{caso contrário} \end{cases}$$
- Resulta em um vetor binário $d$ de comprimento $k \cdot c = 256 \times 2 = 512$ bits.
- Implementado em [BriefHeuristic.cpp:76-88](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L76-L88).

---

### Etapa 7: Descritor Esperado $\hat{d}$ por Partícula (Eq. 3 do Paper)
- Para cada partícula $m$ com estado hipotético $\mathbf{x} = [x, y, z, \Theta]^T$, os pares $p_i$ da imagem do drone são reprojetados no sistema de coordenadas do mapa de satélite:
  $$\hat{Y}_{i,b} = z R(-\Theta) \cdot (y_{i,b} - y_{\text{centro}}) + \begin{bmatrix} x \\ y \end{bmatrix}$$
- Onde $z$ é o fator de escala de altitude, $\Theta$ é o ângulo de guinada (*yaw*), e $R(-\Theta)$ é a matriz de rotação horária no plano 2D.
- Os mesmos testes de intensidade nos canais $a^*$ e $b^*$ são aplicados sobre os pixels amostrados do mapa de satélite, gerando o descritor esperado $\hat{d}$.
- Implementado em [BriefHeuristic.cpp:2280-2307](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2280-L2307) e [BriefHeuristic.cpp:2400-2406](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2400-L2406).

---

### Etapa 8: Avaliação de Verossimilhança e Filtro MCL (Eq. 4 do Paper)
- **Comparação de Descritores**:
  O paper define a similaridade como a taxa de coincidência de bits (XNOR):
  $$\xi(d, \hat{d}) = \frac{1}{k \cdot c} \sum_{i=1}^k \sum_{j=1}^c (\tau_{i,j} \odot \hat{\tau}_{i,j})$$
- **Implementação Real no Código**:
  No código ([BriefHeuristic.cpp:2422-2446](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2422-L2446)), calcula-se a taxa de bits divergentes (distância de Hamming normalizada):
  $$\text{diff} = 1 - \xi(d, \hat{d})$$
  E no filtro de partículas ([Mcl.cpp:1335](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1335)), o peso $w^{[i]}$ da partícula é atualizado via uma distribuição gaussiana centrada em zero:
  $$w^{[i]} \propto \frac{1}{\sqrt{2\pi \sigma_{\text{brief}}^2}} \exp\left( - \frac{\text{diff}^2}{2\sigma_{\text{brief}}^2} \right)$$
- **Ciclo MCL Completo** ([Mcl.cpp:1157-1605](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1157-L1605)):
  1. **Predição**: Propaga partículas usando a odometria visual SURF com ruído gaussiano aditivo em $x, y$, $\Theta$ e escala multiplicativa em $z$.
  2. **Atualização**: Avalia o descritor esperado de cada partícula contra o observado e multiplica os pesos.
  3. **Normalização**: $\sum_i w^{[i]} = 1$.
  4. **Reamostragem**: Executada via *Low Variance Sampler* quando o número efetivo de partículas $N_{\text{eff}} = \frac{1}{\sum (w^{[i]})^2}$ cai abaixo do limiar.
  5. **Estimativa de Pose**: Média ponderada no espaço euclidiano para $(x, y, z)$ e média circular de von Mises para o ângulo $\Theta$ ([Mcl.cpp:892-935](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L892-L935)).

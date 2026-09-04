# Questões Abertas, Validações e Débitos Técnicos

Este documento compila todos os pontos críticos identificados durante a engenharia reversa do repositório **λ-BRIEF** classificados como **PRECISA VALIDAÇÃO**, incluindo dívidas técnicas, dependências não documentadas, discrepâncias sutis entre o texto do artigo e a implementação real, e parâmetros empíricos hardcoded.

---

## 1. Divergências Conceituais: Artigo vs. Código-Fonte

### 1.1. Inversão Semântica da Máscara de Vegetação
- **Arquivo / Linha**: [`phir2frameworkv2/BriefHeuristic.cpp:257`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L257)
- **Código**:
  ```cpp
  viMat[i][j] = viMat[i][j] * -1; // Inverter a probabilidade, pois no default valores grandes indicam vegetação,
                                  // mas quero fazer com que os valores grandes sejam referentes à probabilidade
                                  // de não haver vegetação.
  ```
- **Problema**: O texto do paper afirma na Seção IV-A: *"The vegetation mask is computed for each set of drone images to identify the regions with vegetation and their concentration"*. Em seguida, na Seção IV-C, afirma que a roleta prioriza a seleção de pixels *"with less vegetation"*. No entanto, a fórmula da Eq. 1 ($\hat{\vartheta} = \alpha \vartheta + (1-\alpha)\mathcal{N}$) dá a entender que $\vartheta$ cresce com a vegetação. Na realidade do código, $\vartheta$ é **estritamente invertido** e submetido à equalização de histograma antes da composição linear com a Gaussiana.
- **Ação de Validação**: Para novos trabalhos ou extensão do artigo, a formulação matemática deve explicitar a transformação $\vartheta_{\text{inv}} = \text{Equalize}(-\text{VI})$ para garantir reprodutibilidade matemática exata.

---

### 1.2. Equação da Gaussiana 2D Anisotrópica
- **Arquivo / Linha**: [`phir2frameworkv2/BriefHeuristic.cpp:109-110`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L109-L110)
- **Código**:
  ```cpp
  X = ((x - x0) * (x - x0)) / (2.0f * sigmaX * sigmaY);
  Y = ((y - y0) * (y - y0)) / (2.0f * sigmaX * sigmaY);
  value = amplitude * exp(-(X + Y));
  ```
- **Problema**: O denominador para $X$ utiliza o produto cruzado `sigmaX * sigmaY` em vez de $\sigma_x^2$, e para $Y$ também utiliza `sigmaX * sigmaY` em vez de $\sigma_y^2$. Quando a imagem é retangular ($1234 \times 910$, com $\sigma_x = 246.8$ e $\sigma_y = 182.0$), a variância efetiva em ambos os eixos torna-se a média geométrica $\sigma_x \sigma_y$.
- **Ação de Validação**: Verificar se essa formulação foi proposital para manter simetria circular em pixels ou se trata-se de um bug de implementação analítica da distribuição normal bidimensional.

---

### 1.3. Modelo de Verossimilhança no Filtro MCL (Gaussiana de Erro vs. XNOR Direto)
- **Arquivo / Linha**: [`phir2frameworkv2/BriefHeuristic.cpp:2445`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2445) e [`phir2frameworkv2/Mcl.cpp:1335`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1335)
- **Código**:
  ```cpp
  // BriefHeuristic.cpp:2445
  result = 1.0 - ((float)sumNdviWeightsPatch / ((float)this->sumNdviWeightsDrone * (float)bitsPerPair));

  // Mcl.cpp:1335
  prob *= (1.0 / (sqrt(varBrief * 2.0 * M_PI)) * exp(-0.5 * (pow(diff, 2.0) / varBrief)));
  ```
- **Problema**: O paper descreve formalmente na Seção IV-F que a similaridade entre o descritor observado $d$ e o esperado $\hat{d}$ é dada pela Eq. 4:
  $$\xi(d, \hat{d}) = \frac{1}{kc} \sum_{i=1}^k \sum_{j=1}^c \tau_{i,j} \odot \hat{\tau}_{i,j}$$
  Entretanto, a heurística retorna a taxa de bits *divergentes* ($\text{diff} = 1 - \xi$) e o filtro de partículas aplica uma verossimilhança gaussiana com $\sigma^2 = \text{varBrief} = 0.0009$. Sem esse modelo exponencial de verossimilhança, o MCL não atribuiria probabilidades infinitesimais a partículas com divergências moderadas de bits.
- **Ação de Validação**: O artigo omitiu a equação de conversão entre a distância de Hamming e o peso probabilístico do MCL. Isso deve ser formalmente integrado à Seção IV.

---

### 1.4. O Descarte do Parâmetro $\beta$ (Hamming Weight)
- **Arquivo / Linha**: [`phir2frameworkv2/config.h:40`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/config.h#L40) e [`phir2frameworkv2/BriefHeuristic.cpp:2429-2443`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2429-L2443)
- **Problema**: No código existem dezenas de referências e blocos condicionais para `betaHammingWeight` (`-betaweight`), que multiplicava o peso do bit pelo valor local do NDVI na imagem. No entanto, os arquivos de resultados e o título do próprio manuscrito (`Paper___LambdaλBRIEF__sem_beta...pdf`) confirmam que $\beta$ foi abandonado antes da submissão final, fixando `pairWeight[pairCount] = 1.0`.
- **Ação de Validação**: Todo o código em torno de `pairWeight` que não seja $1.0$ representa código morto na versão final do método publicado.

---

## 2. Constantes "Mágicas" e Parâmetros Empíricos Sem Justificativa Explícita

| Parâmetro / Constante | Localização no Código | Valor | Contexto e Observação |
| :--- | :--- | :---: | :--- |
| `multExcalaZ` | [`config.h:34`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/config.h#L34)<br>[`main.cpp:64`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/main.cpp#L64) | `45 / 0.188679245` $\approx 238.5$ | Fator empírico de escala vertical para pixels. Relaciona a distância focal da lente, resolução do sensor e resolução do mapa. Não documentado na literatura. |
| Divisor de Projeção Z | [`BriefHeuristic.cpp:2266`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L2266) | `0.366197183` | Constante utilizada para converter o fator de escala relativo da partícula na distância euclidiana da câmera ao terreno (`-scale / 0.366197183`). |
| Relação de Escala VET | [`Datasets/UFRGS VET/voo_100m/escala`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/Datasets/UFRGS%20VET/voo_100m/escala) | `167 / 181` $\approx 0.92265$ | Medição empírica feita "no olho" entre dois marcos do campus identificados visualmente no mapa e na imagem da câmera. |
| Relação de Escala ADM | [`Datasets/ADM/0035/escala`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/Datasets/ADM/0035/escala) | `42 / 127` $\approx 0.3307$ | Medição de largura de galpão na imagem do drone vs mapa de satélite para altura nominal de $62\text{ m}$. |
| Variância Gaussiana MCL | [`Mcl.cpp:1335`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1335) | `varBrief = 0.0009` | Desvio padrão implícito $\sigma = 0.03$. Torna a penalidade de bits divergentes severa. Sem análise de sensibilidade no artigo. |
| Proporção Red no $\text{NDVI}_{\text{red\&RE}}$ | [`VegetationIndex.h:53`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetationIndex.h#L53) | `paramA = 0.4` | Ponderação fixa da banda Vermelha ($40\%$) e Red Edge ($60\%$). Citada de artigo agronômico de sensoriamento remoto, mas nunca otimizada para visão robótica. |
| Limiar de Vegetação | [`BriefHeuristic.cpp:253`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L253) | `viMat >= 0.3` | Considera como "alta chance de conter vegetação" qualquer pixel com valor $\ge 0.3$, utilizado apenas para calcular a porcentagem global de vegetação da imagem. |

---

## 3. Débitos Técnicos e Problemas Estruturais

### 3.1. Caminhos de Arquivo Hardcoded
- **Gravidade**: **Alta** (impede compilação e execução imediata *out-of-the-box* em novos sistemas).
- **Descrição**:
  - No arquivo de projeto [`PhiR2Framework.pro`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/PhiR2Framework.pro#L78-L106), caminhos como `/usr/local/Aria/include`, `/usr/include/pcl-1.7`, `/usr/local/include/pcl-1.9` e diretórios relativos com `$$PWD/../../../../../usr/local/lib/` impedem que o qmake localize bibliotecas em instalações modernas do Linux.
  - Nos arquivos `traj.txt` de todos os datasets, todos os caminhos de imagens estão salvos como `/home/phi/Documents/Datasets/...` ou `/home/mathias/Workspace/...`. Qualquer execução atual exige substituir essas strings pelos caminhos locais da máquina onde o repositório estiver clonado.

### 3.2. Dependência de APIs Descontinuadas do OpenCV
- **Gravidade**: **Média-Alta** (quebra de compilação em OpenCV 4.x).
- **Descrição**:
  - Uso dos símbolos em estilo C: `CV_BGR2Lab`, `CV_LOAD_IMAGE_COLOR`, `CV_LOAD_IMAGE_GRAYSCALE`, `CV_TM_CCORR_NORMED`. Em OpenCV 4+, esses identificadores foram movidos para o namespace C++ (ex.: `cv::COLOR_BGR2Lab`, `cv::IMREAD_COLOR`).
  - Módulos `xfeatures2d::SURF` requerem compilação com `OPENCV_ENABLE_NONFREE=ON` no OpenCV recente devido a patentes históricas.

### 3.3. Dependências Fantasmas / Órfãs
- **Gravidade**: **Média** (aumenta o atrito de instalação sem necessidade funcional).
- **Descrição**:
  - O projeto `PhiR2Framework.pro` linka explicitamente a biblioteca `Aria` da MobileRobots e a biblioteca de nuvens de pontos `PCL` (`lpcl_common`, `lpcl_io`, `lpcl_registration`).
  - A arquitetura do λ-BRIEF atua estritamente sobre matrizes 2D do OpenCV (`cv::Mat`) e não instancia nem manipula nuvens de pontos PCL nem robôs terrestres Pioneer durante o rastreamento do drone. Essas dependências são resquícios do framework PhiR2 original e podem ser desacopladas para tornar o pacote autônomo e leve.

### 3.4. Gestão de Memória e Threads
- **Gravidade**: **Baixa-Média**.
- **Descrição**:
  - Em [`main.cpp:87`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/main.cpp#L87), utiliza-se `sleep(2)` para retardar a inicialização do OpenGL na expectativa de que a thread do robô já tenha carregado os dados. Se o carregamento do mapa de 4800×4800 demorar mais que 2 segundos, pode ocorrer *race condition* de ponteiro nulo.
  - Alocações brutas com `malloc()` em [`Ndvi.cpp:104`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.cpp#L104) sem liberação correspondente via `free()` em algumas ramificações de erro.

---

## 4. Recomendações de Ação Imediata

1. **Criar script de re-mapeamento de caminhos**: Um script simples em Python ou Bash que substitua os prefixos `/home/phi/...` nos arquivos `traj*.txt` pelo caminho absoluto dinâmico atual (`$PWD/Datasets/...`).
2. **Atualizar `PhiR2Framework.pro`**:
   - Remover as dependências estritas de ARIA e PCL das flags de compilação da heurística BRIEF.
   - Utilizar `pkg-config` dinâmico para o OpenCV moderno: `CONFIG += link_pkgconfig` e `PKGCONFIG += opencv4` (ou manter compatibilidade com `opencv` 3).
3. **Formalizar as Equações na Documentação e Resubmissão**:
   - Incorporar explicitamente a etapa de inversão da máscara de vegetação na formulação matemática.
   - Detalhar a transição da similaridade XNOR (Eq. 4) para a verossimilhança exponencial do filtro MCL.

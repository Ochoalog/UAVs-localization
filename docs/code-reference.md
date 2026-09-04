# Referência do Código-Fonte do λ-BRIEF

Este documento apresenta uma análise exaustiva, arquivo por arquivo, de todos os componentes de código-fonte relevantes do repositório `UAVs Localization` (concentrados majoritariamente no diretório `phir2frameworkv2/`). Cada seção detalha o propósito do arquivo, suas principais estruturas, parâmetros hardcoded, conexão com as equações do artigo científico e trechos marcados como **PRECISA VALIDAÇÃO**.

---

## Módulo 1: Núcleo do Descritor λ-BRIEF e Heurísticas de Visão

### [`phir2frameworkv2/BriefHeuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.h) e [`phir2frameworkv2/BriefHeuristic.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp)
- **Resumo**: É o arquivo central de implementação do descritor λ-BRIEF e da heurística abBRIEF. Gerencia a amostragem de pares de pixels guiada por índices de vegetação e distribuição gaussiana, computa os testes binários de intensidade no espaço CIE L\*a\*b\* para a imagem observada e reprojeta os pares sobre o mapa de satélite para gerar o descritor esperado de cada partícula no MCL.
- **Principais Classes e Métodos**:
  - `class BriefHeuristic : public Heuristic`
  - `gaussianMask()` / `gaussianMaskD()`: Gera a máscara gaussiana centrada de probabilidade $\mathcal{N}$.
  - `pixelDistribution()`: Computa a matriz do índice de vegetação ($\vartheta$), inverte valores, aplica equalização de histograma, combina com $\mathcal{N}$ via $\alpha$ e seleciona $k=256$ pares de pixels por roleta estocástica (*stochastic acceptance*).
  - `updateDroneDescriptor()`: Computa o descritor binário observado $d$ (512 bits) a partir dos canais $a^*$ e $b^*$ da imagem do drone.
  - `calculateValue2()` / `abBriefParicle()`: Projeta as coordenadas dos pares de pixels sobre o mapa de satélite para uma dada pose $(x, y, z, \text{yaw})$, extrai o descritor esperado $\hat{d}$ e calcula a distância de Hamming / similaridade.
  - `transform()`: Aplica a matriz de rotação e escala aos pontos amostrados.
- **Conexão com o Paper**:
  - **Seção IV-A** (*Computing the vegetation mask*): `pixelDistribution()` linhas 243-290.
  - **Seção IV-B** (*Computing the gaussian mask*): `gaussianMask()` linhas 100-119 ($\sigma_x = a/5, \sigma_y = b/5$).
  - **Seção IV-C** (*Pixel Selection Method*): `pixelDistribution()` linhas 296-331 — Eq. 1 ($\hat{\vartheta} = \alpha \cdot \vartheta + (1-\alpha) \cdot \mathcal{N}$).
  - **Seção IV-D** (*Observed descriptor* $d$): `updateDroneDescriptor()` linhas 75-88 — Eq. 2 ($\tau_{i,j}$).
  - **Seção IV-E** (*Expected descriptor* $\hat{d}$): `abBriefParicle()` linhas 2280-2369 — Eq. 3 (transformação com escala $z$ e rotação $R(-\Theta)$).
  - **Seção IV-F** (*Similarity*): `abBriefParicle()` linhas 2420-2448 — Eq. 4 (cálculo de coincidência/divergência de bits).
- **Parâmetros Hardcoded**:
  - Número de pares de pixels: padrão 256 pares (`totalPairs = 256` configurado via flag `-bp 256`).
  - Desvio padrão gaussiano: $\sigma_x = \text{cols}/5$, $\sigma_y = \text{rows}/5$ ([BriefHeuristic.cpp:165-166](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/BriefHeuristic.cpp#L165-L166)).
  - Ajuste de ângulo yaw inicial: `theta = dronePose.yaw * (-1) - 1.5708` (rotação de $-90^\circ$ e inversão de sentido, linha 2250).
  - Constantes de escala vertical: `dir[2] = (-scale / 0.366197183) * (multExcalaZ)` (linha 2266).
- **PRECISA VALIDAÇÃO**:
  1. *Fórmula da Gaussiana*: Na linha 109, o denominador de $X$ e $Y$ está escrito como `(2.0f * sigmaX * sigmaY)`. Se $\sigma_x \neq \sigma_y$ (imagens não quadradas como $1234 \times 910$), o termo deveria ser $2\sigma_x^2$ e $2\sigma_y^2$. Como a máscara é normalizada com `cv::NORM_MINMAX`, o impacto prático foi mascarado.
  2. *Inversão do VI*: Na linha 257, `viMat[i][j] = viMat[i][j] * -1;` inverte a probabilidade para que valores baixos de NDVI tenham maior peso na roleta. O texto do paper descreve a máscara $\vartheta$, mas a inversão explícita só é mencionada de passagem ao dizer que prioriza "pixels with less vegetation".
  3. *Métrica de Retorno*: Na linha 2445, `result` retorna a taxa de erro / distância de Hamming normalizada ($1 - \text{similaridade}$), e não a similaridade direta $\xi(d, \hat{d})$ da Eq. 4. Isso ocorre porque o `Mcl.cpp` aplica uma gaussiana de erro sobre esse valor.
  4. *Código duplicado*: Métodos `calculateValue2` e `abBriefParicle` possuem grandes blocos de código duplicados para diferentes tipos de descritores (tipo 0 = grayscale, tipo 1 = Lab 3 bits, tipo 2 = Lab 6 bits, tipo 4 = Lab sem L 2 bits).

---

### [`phir2frameworkv2/Heuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Heuristic.h) e [`phir2frameworkv2/Heuristic.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Heuristic.cpp)
- **Resumo**: Classe-base abstrata para todos os modelos de observação (heurísticas) do framework PhiR2. Define a interface polimórfica para cálculo de probabilidade e verossimilhança de partículas, bem como o acesso direto a pixels e cores em matrizes do OpenCV.
- **Principais Classes e Métodos**:
  - `class Heuristic`
  - `calculateValue(int x, int y, cv::Mat *image, cv::Mat* map)`: Método virtual puro.
  - `getValuefromPixel()`: Retorna o valor vetorial de cor (`vec3`) de um pixel da imagem.
  - `getColorDifference()`: Interface para métricas de distância cromática.
- **Conexão com o Paper**: Seção IV (Measurement model do MCL).
- **Parâmetros Hardcoded**: `HEURISTIC_UNDEFINED_DOUBLE = -100.0`, `HEURISTIC_UNDEFINED_INT = -100`.
- **PRECISA VALIDAÇÃO**: Código legado mantido sem alterações do framework base original de 2017.

---

### [`phir2frameworkv2/ColorCPU.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/ColorCPU.h) e [`phir2frameworkv2/ColorCPU.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/ColorCPU.cpp)
- **Resumo**: Biblioteca de conversão de espaços de cor e métricas perceptuais avançadas de diferença de cor em CPU. Implementa transformações entre RGB, XYZ, CIE L\*a\*b\* e CIE LCH, além das métricas formais de $\Delta E$ (CIE1976, CIE1994, CIEDE2000, CMC1984).
- **Principais Classes e Métodos**:
  - `class CPUColorConverter`
  - `DeltaECIE1976()`: Distância euclidiana no espaço Lab.
  - `DeltaECIE2000()` / `DeltaEMixCIE2000()`: Diferença perceptual avançada CIEDE2000 com fatores de compensação de cromaticidade e matiz.
  - `RGBtoCIELab()` / `CIELabtoRGB()`: Conversão analítica completa de modelos de cor.
- **Conexão com o Paper**: Seção IV-D e Seção IV-F (justificativa teórica para o uso de canais $a^*$ e $b^*$ do espaço CIE L\*a\*b\*).
- **Parâmetros Hardcoded**: Constantes do iluminante padrão CIE D65 ($X_n=95.047, Y_n=100.0, Z_n=108.883$).
- **PRECISA VALIDAÇÃO**: As métricas complexas $\Delta E_{2000}$ são utilizadas principalmente em estratégias legadas como `DENSITY` ou na opção `-s BRIEF diff-cie2000 15`. Na implementação real do λ-BRIEF (`type = 4`), a comparação é binária direta sobre os canais individuais `color1.r > color2.r` (onde $r$ mapeia para o primeiro canal Lab na estrutura `vec3`).

---

### [`phir2frameworkv2/colorheuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/colorheuristic.h) e [`phir2frameworkv2/colorheuristic.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/colorheuristic.cpp)
- **Resumo**: Heurística de observação legada baseada unicamente na diferença de cor média entre regiões, sem descritores binários ou máscaras de vegetação.
- **Principais Classes e Métodos**: `class ColorHeuristic : public Heuristic`
- **Conexão com o Paper**: Não utilizada no λ-BRIEF. Pertence a comparações anteriores com abBRIEF ou SSD puro.
- **PRECISA VALIDAÇÃO**: Código legado inativo durante as execuções do λ-BRIEF (`-s BRIEF`).

---

## Módulo 2: Índices Espectrais de Vegetação

### [`phir2frameworkv2/VegetationIndex.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetationIndex.h) e [`phir2frameworkv2/VegetationIndex.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetationIndex.cpp)
- **Resumo**: Classe-base abstrata para o cálculo de índices de vegetação multiespectrais. Fornece suporte para calibração radiométrica (fatores aditivos e multiplicativos de radiância/reflectância e ângulo de zênite solar $\theta_{se}$).
- **Principais Classes e Métodos**:
  - `class VegetationIndex`
  - `setReflectance()`, `getReflectance()`: Conversão de valores digitais brutos (DN) em reflectância de superfície para as bandas espectrais.
  - `paramA`: Proporção de reflectância da banda vermelha para o índice composto $\text{NDVI}_{\text{red\&RE}}$.
- **Conexão com o Paper**: Seção III (*Vegetation Detection with Multispectral Cameras*) e Seção IV-A.
- **Parâmetros Hardcoded**: `paramA = 0.4` definido em [VegetationIndex.h:53](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetationIndex.h#L53).
- **PRECISA VALIDAÇÃO**: A correção de radiância (`radianceFix`) é configurada como 0 por padrão nas chamadas do código (pois as imagens da Parrot Sequoia foram calibradas e retificadas em pré-processamento).

---

### [`phir2frameworkv2/Ndvi.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.h) e [`phir2frameworkv2/Ndvi.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Ndvi.cpp)
- **Resumo**: Implementa o cálculo dos três principais índices de vegetação avaliados no artigo: NDVI tradicional, NDRE (Red Edge NDVI) e o índice combinado $\text{NDVI}_{\text{red\&RE}}$, tanto em precisão simples (`float`) quanto dupla (`double`).
- **Principais Classes e Métodos**:
  - `class Ndvi : public VegetationIndex`
  - `setIndexPixel(shortWave, redVisible)`: Calcula $\frac{\text{NIR} - \text{Red}}{\text{NIR} + \text{Red}}$ (NDVI clássico).
  - `setIndexPixelReNdvi(shortWave, redEdge)`: Calcula $\frac{\text{NIR} - \text{RedEdge}}{\text{NIR} + \text{RedEdge}}$ (NDRE).
  - `setIndexPixelRedReNdvi(shortWave, redEdge, redVisible)`: Calcula $\frac{\text{NIR} - (a\cdot\text{Red} + (1-a)\cdot\text{RedEdge})}{\text{NIR} + (a\cdot\text{Red} + (1-a)\cdot\text{RedEdge})}$.
  - `setIndexD()`: Dispatcher para execução em double baseado no atributo `ndviType`.
- **Conexão com o Paper**: Seção IV-A e Seção V-A (Figura 2 do paper, comparando os 3 índices).
- **Parâmetros Hardcoded**: Constante $a = 0.4$ herdada da classe-base.
- **PRECISA VALIDAÇÃO**: O código contém linhas comentadas que testavam elevar o resultado do NDVI ao quadrado (`pow(..., 2)`). A fórmula ativa é a linear padrão.

---

### [`phir2frameworkv2/Grvi.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Grvi.h) e [`phir2frameworkv2/Grvi.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Grvi.cpp)
- **Resumo**: Implementa o índice Green-Red Vegetation Index ($\text{GRVI} = \frac{\text{Green} - \text{Red}}{\text{Green} + \text{Red}}$), utilizando apenas canais do espectro visível.
- **Principais Classes e Métodos**: `class Grvi : public VegetationIndex`, método `setIndexPixel()`.
- **Conexão com o Paper**: Experimento exploratório preliminar; não é utilizado nos resultados finais do paper λ-BRIEF.
- **PRECISA VALIDAÇÃO**: Código totalmente funcional, mas inativo quando `ndvi_type != 0`.

---

### [`phir2frameworkv2/VegetatonIndexGen.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetatonIndexGen.h) e [`phir2frameworkv2/VegetatonIndexGen.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/VegetatonIndexGen.cpp)
- **Resumo**: Factory simples que instancia polimorficamente o objeto de índice de vegetação (`Ndvi` ou `Grvi`) com base nas variáveis globais `ndvi_type` e `grvi_type`.
- **Principais Classes e Métodos**: `VegetatonIndexGen::InitVegetationIndex()`.
- **Conexão com o Paper**: Seção IV-A.
- **PRECISA VALIDAÇÃO**: Nota-se erro de digitação no nome do arquivo original (`Vegetaton` sem o "i"), mantido para integridade de compilação.

---

## Módulo 3: Filtro de Partículas Monte Carlo (MCL) e Robô UAV

### [`phir2frameworkv2/Mcl.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.h) e [`phir2frameworkv2/Mcl.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp)
- **Resumo**: Implementação completa do algoritmo Monte Carlo Localization (MCL) em 4 graus de liberdade. Gerencia o conjunto de partículas (estruturas com pose $x, y, z, \text{yaw}$ e peso $w$), a dispersão inicial uniforme no mapa, a propagação cinemática através da odometria com modelo de ruído, a ponderação de verossimilhança pelas heurísticas de observação, a reamostragem por baixa variância e o cálculo da estimativa de pose agregada e variâncias.
- **Principais Classes e Métodos**:
  - `struct MCLparticle`: Pose 3D, peso $w$, fator de escala $s$.
  - `MCL::MCL()`: Construtor que inicializa as $50.000$ partículas uniformemente pelo mapa global, com escala entre `minScale` e `maxScale` e orientação entre $[-\pi, \pi)$.
  - `sampling()`: Modelo de movimento probabilístico com ruído gaussiano adicionado ao deslocamento relativo $(\Delta x, \Delta y, \Delta \text{yaw}, \Delta z)$ vindo da odometria.
  - `weighting()`: Atualização de pesos das partículas através da chamada `bh->calculateValue2()`.
  - `resampling()`: Reamostragem por baixa variância (*Low Variance Sampler*, Thrun et al., 2005).
  - `writeLogFile3d()`: Computa a média espacial $(\bar{x}_t, \bar{y}_t, \bar{z}_t)$ e média circular de ângulos $\bar{\Theta}_t$, gravando o log com o número efetivo de partículas ($N_{\text{eff}}$) e tempo de processamento.
- **Conexão com o Paper**:
  - **Seção IV**: Formulismo geral de estimação de pose em 4 DoF.
  - **Seção V**: Inicialização uniforme de partículas ($\Theta \in [0, 2\pi)$, altitude $z \in [20\text{ m}, 250\text{ m}]$ para ADM e $z \in [46\text{ m}, 466\text{ m}]$ para UFRGS).
  - **Seção V**: Fórmulas da estimativa de posição ponderada $\bar{x}_t$ e da estimativa angular circular $\bar{\Theta}_t$.
- **Parâmetros Hardcoded**:
  - `varBrief = 0.0009` (ou $0.03^2$): Variância do modelo de medição gaussiano para o descritor BRIEF ([Mcl.cpp:1335](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1335)).
  - Ruído na odometria confiável: ruído em $x, y$ de $\pm 10\text{ pixels}$, ruído angular de $\pm 5^\circ$ ($\pm 5\pi/180$), ruído em escala de $\pm 0.02578$ ([Mcl.cpp:1179-1185](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1179-L1185)).
  - Ruído na odometria não-confiável: ruído angular ampliado para $\pm 30^\circ$ ([Mcl.cpp:1202](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Mcl.cpp#L1202)).
  - Limiar de reamostragem: `resamplingThreshold = numParticles / 8` (re-amostra quando $N_{\text{eff}} < N/8$).
- **PRECISA VALIDAÇÃO**:
  1. *Cálculo da Variância Angular*: Linha 938 calcula o desvio padrão circular como $\sqrt{-\ln(\sin^2 + \cos^2)}$, fórmula teórica da variância de dispersão de von Mises; trechos antigos comentados usavam médias aritméticas lineares que falham na descontinuidade $-\pi / +\pi$.
  2. *Compensação de Ângulo (Roll/Pitch)*: Chamada `compensateCloudPosition()` (linha 1170) só age se `compensateParticles` for verdadeiro; nos experimentos principais com voo estritamente nadir, o flag permanece desativado.

---

### [`phir2frameworkv2/DroneRobot.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DroneRobot.h) e [`phir2frameworkv2/DroneRobot.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DroneRobot.cpp)
- **Resumo**: Representa a entidade do UAV e orquestra o ciclo de vida da execução. Carrega as sequências de imagens de todas as bandas multiespectrais, gerencia os arquivos de trajetória e ground truth, calcula a odometria visual entre quadros sucessivos por correspondência de pontos SURF (ou SIFT/ORB/ECC), converte as imagens para o espaço Lab e invoca os passos de predição e atualização do filtro MCL.
- **Principais Classes e Métodos**:
  - `class DroneRobot : public Robot`
  - `initialize()`: Carrega o mapa de satélite global, converte para Lab, inicializa as heurísticas e lê os arquivos de trajetória.
  - `run()`: Loop principal por quadro da trajetória ($t = 0 \dots N$). Lê o quadro atual do drone, calcula a odometria relativa frente ao quadro anterior, invoca `MCL::run()` e registra erros.
  - `findOdometry()` / `findOdometryUsingFeaturesSurf()`: Detecta pontos-chave SURF nas imagens sucessivas do drone, filtra correspondências via RANSAC/distância e estima a translação $(\Delta x, \Delta y)$, rotação $\Delta \Theta$ e razão de escala $\Delta z$.
  - `readGroundTruth()` e `readCorrectedGroundTruth()`: Lê as poses de referência de GPS/IMU.
- **Conexão com o Paper**: Seção IV (*Visual Odometry via feature-matching*) e Seção V (*Experimental setup*).
- **Parâmetros Hardcoded**:
  - Limiar de corte de quadros: `imgIniExec`, `imgFimExec` fatiam os vetores de nomes de imagens para selecionar os intervalos de voo exatos da Tabela I.
  - `ODOM_TYPE = 1`: SURF ativado por padrão ([config.h:64](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/config.h#L64)).
  - Dimensões do mapa: pressupõe mapas quadrangulares $4800 \times 4800$.
- **PRECISA VALIDAÇÃO**:
  1. *Substituição de Nomes de Arquivo*: Linhas 369-415 assumem que os nomes dos arquivos de lista de imagens contêm a terminação `.txt` exata para trocar por `_RED.txt`, `_NIR.txt`, etc. Se o caminho fornecido não tiver extensão `.txt`, o código falha silenciosamente sem carregar as bandas multiespectrais.
  2. *Visualização Gráfica*: Em modo não-quieto (`quiet = false`), chama funções `imshow` do OpenCV que podem bloquear ou gerar crash se não houver um display X11 ativo.

---

### [`phir2frameworkv2/Robot.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Robot.h) e [`phir2frameworkv2/Robot.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Robot.cpp)
- **Resumo**: Classe-base virtual para robôs móveis no arcabouço PhiR2, definindo a interface genérica de controle de threads, execução e parada.
- **Principais Classes e Métodos**: `class Robot`
- **Conexão com o Paper**: Abstração de software herdada.

---

## Módulo 4: Entrada do Sistema, Configuração e Interface

### [`phir2frameworkv2/main.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/main.cpp)
- **Resumo**: Ponto de entrada (`main()`) do executável `PhiR2Framework`. Realiza o parsing completo de argumentos de linha de comando (`argc`, `argv`), instancia o objeto `DroneRobot`, inicia a thread de execução do robô e a thread de renderização gráfica GLUT/OpenGL (caso `-quiet` não esteja ativado).
- **Principais Funções**:
  - `main(int argc, char* argv[])`: Inicializa mutexes pthreads, invoca `config()` e dispara `startRobotThread`.
  - `config()`: Parser com mais de 30 parâmetros CLI (`-e`, `-t`, `-s`, `-bp`, `-par`, `-ndvit`, `-pdm`, `-alfavismsk`, etc.).
  - `startRobotThread()` / `startGlutThread()`: Funções de entrada das threads de execução e interface gráfica.
- **Conexão com o Paper**: Seção V (recebe todos os hiperparâmetros dos experimentos).
- **Parâmetros Hardcoded**: Exemplos de execução fixos nos comentários das linhas 21-24 com caminhos absolutos das máquinas originais dos autores (`/home/phi/...` e `/home/raziel/...`).
- **PRECISA VALIDAÇÃO**: A sincronização entre a thread do robô e a thread GLUT utiliza `sleep(1)` e `sleep(2)` rudimentares, sem variáveis de condição formais (`pthread_cond_wait`).

---

### [`phir2frameworkv2/config.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/config.h)
- **Resumo**: Arquivo de cabeçalho global que declara variáveis externas e constantes de configuração compartilhadas entre `main.cpp`, `DroneRobot.cpp`, `Mcl.cpp` e `BriefHeuristic.cpp`.
- **Principais Parâmetros e Valores**:
  - `BRIEF_HEURISTIC_TYPE = 4`: Modo abBRIEF/λ-BRIEF (Lab sem L, 2 bits por canal).
  - `NUM_THREADS = 8`: Quantidade de threads do OpenMP para paralelização das matrizes de VI.
  - `ODOM_TYPE = 1`: SURF para odometria.
  - `multispectralCam = true`: Habilita câmera multiespectral por padrão.
  - `ndvi_type = 1`: NDVI tradicional como índice padrão.
  - `multExcalaZ = (45 / 0.188679245)`: Fator de conversão de escala vertical para pixels.
- **Conexão com o Paper**: Tabela I e Seção IV.
- **PRECISA VALIDAÇÃO**: Contém constantes comentadas e nomes históricos como `betaHammingWeight` (parâmetro $\beta$ que foi excluído da versão final do paper).

---

### [`phir2frameworkv2/GlutClass.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/GlutClass.h) e [`phir2frameworkv2/GlutClass.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/GlutClass.cpp)
- **Resumo**: Módulo de renderização visual interativa usando OpenGL clássico e GLUT. Desenha em tempo real o mapa de satélite global, as $50.000$ partículas (com orientação e escala representadas graficamente), a nuvem estimada e a trajetória real do Ground Truth.
- **Principais Classes e Métodos**: `class GlutClass` (Singleton), `render()`, `drawParticles()`, `drawPath()`.
- **Conexão com o Paper**: Utilizado para depuração visual e geração de figuras como a Fig. 1 do paper.
- **PRECISA VALIDAÇÃO**: Dependência obrigatória de bibliotecas de janelas (`libglut`, `libGL`, `libGLEW`), desativada em execuções de cluster via `-quiet`.

---

## Módulo 5: Geodésia, Conversão de Coordenadas e Leitura de Logs

### [`phir2frameworkv2/UTMConverter.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/UTMConverter.h) e [`phir2frameworkv2/UTMConverter.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/UTMConverter.cpp)
- **Resumo**: Implementação de equações geodésicas baseadas no elipsoide WGS-84 para conversão de coordenadas de latitude e longitude (graus decimais) para projeção UTM (Universal Transverse Mercator: Norte/Leste em metros).
- **Principais Classes e Métodos**: `class UTMConverter`, métodos `toUTM()` e `toLatLon()`.
- **Conexão com o Paper**: Seção V (preparação e alinhamento métrico das coordenadas GPS com as imagens de satélite).
- **Parâmetros Hardcoded**: Parâmetros do elipsoide WGS-84 ($a = 6378137.0\text{ m}, 1/f = 298.257223563$).

---

### [`phir2frameworkv2/PixelTransform.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/PixelTransform.h) e [`phir2frameworkv2/PixelTransform.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/PixelTransform.cpp)
- **Resumo**: Realiza o georreferenciamento de imagens de satélite através de pontos de controle no solo (GCP - Ground Control Points). Converte coordenadas métricas UTM para posições discretas de pixels $(u, v)$ no mapa orbital através de ajuste linear de escala e rotação ou matriz afim.
- **Principais Classes e Métodos**: `class PixelTransform`, `transformPixelToCoord()`, `transformCoordToPixel()`.
- **Conexão com o Paper**: Seção V (construção dos arquivos de Ground Truth `traj_truth.txt` em pixels do mapa).
- **PRECISA VALIDAÇÃO**: Os pares de calibração são carregados a partir dos arquivos `pixel_to_coord_*` encontrados nas pastas de cada dataset.

---

### [`phir2frameworkv2/readerlog.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/readerlog.h) e [`phir2frameworkv2/readerlog.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/readerlog.cpp)
- **Resumo**: Parser de telemetria bruta gravada pelo piloto automático DJI (arquivos `.DAT` proprietários ou logs exportados em CSV). Extrai pacotes de GPS, acelerômetro, giroscópio e atitude (quaterniões / ângulos de Euler roll, pitch, yaw).
- **Principais Classes e Métodos**: `class ReaderLog`, estruturas `IMU`, `GPS`, `ATT`.
- **Conexão com o Paper**: Seção V (extração do Ground Truth do GPS e IMU).

---

### [`phir2frameworkv2/mainReader.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/mainReader.cpp)
- **Resumo**: Utilitário executável (`ReaderLog`) que sincroniza temporalmente as marcações de tempo das imagens capturadas com a telemetria do piloto automático via interpolação linear, gerando os arquivos de texto `traj.txt`, `traj_truth.txt` e `traj_angles.txt`.
- **Principais Funções**: `getImagesIMU()`, `exportGoodTraj()`, `parseArgs()`.
- **Conexão com o Paper**: Preparação dos dados experimentais da Seção V.

---

### [`phir2frameworkv2/angleutil.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/angleutil.h) e [`phir2frameworkv2/angleutil.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/angleutil.cpp)
- **Resumo**: Utilitários matemáticos para operações angulares, normalização no intervalo $[-\pi, \pi)$ e cálculo de menor diferença angular cíclica.
- **Conexão com o Paper**: Seção V (cálculo do erro de atitude $\kappa_t$).

---

## Módulo 6: Grids Espaciais e Heurísticas Legadas

### [`phir2frameworkv2/Grid.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Grid.h), [`phir2frameworkv2/Grid.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Grid.cpp), [`phir2frameworkv2/MapGrid.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/MapGrid.h) e [`phir2frameworkv2/MapGrid.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/MapGrid.cpp)
- **Resumo**: Implementam estruturas de grade de ocupação e mapas pré-computados com caching espacial para aceleração de consultas de similaridade durante a avaliação do MCL.
- **Conexão com o Paper**: Infraestrutura de dados do framework abBRIEF original.

---

### [`phir2frameworkv2/DistanceTransform.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DistanceTransform.h) e [`phir2frameworkv2/DistanceTransform.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/DistanceTransform.cpp)
- **Resumo**: Computa mapas de transformada de distância euclidiana para imagens binárias (bordas/linhas), usado em heurísticas estruturais de linha de voo.
- **Conexão com o Paper**: Não utilizado no λ-BRIEF final.

---

### [`phir2frameworkv2/densityheuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/densityheuristic.h), [`phir2frameworkv2/densityheuristic.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/densityheuristic.cpp), [`phir2frameworkv2/Kernel.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Kernel.h), [`phir2frameworkv2/SomeKernels.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/SomeKernels.h)
- **Resumo**: Implementam estimadores de densidade por kernel (KDE - Kernel Density Estimation: gaussiano, circular, invertido) utilizados em trabalhos anteriores do grupo Phi Robotics para localização por densidade de cor.
- **Conexão com o Paper**: Linhas de base históricas de comparação.

---

### [`phir2frameworkv2/MeanShiftHeuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/MeanShiftHeuristic.h), [`phir2frameworkv2/MeanShiftHeuristic.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/MeanShiftHeuristic.cpp), [`phir2frameworkv2/miheuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/miheuristic.h), [`phir2frameworkv2/mutualinfheuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/mutualinfheuristic.h), [`phir2frameworkv2/SiftHeuristic.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/SiftHeuristic.h)
- **Resumo**: Módulos que implementam estratégias alternativas de mensuração: rastreamento por Mean-Shift, Informação Mútua (Mutual Information) e SIFT-MCL clássico.
- **Conexão com o Paper**: Métodos do estado da arte referenciados na introdução e trabalhos relacionados (Seção II).

---

### [`phir2frameworkv2/CorrelativeSM.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/CorrelativeSM.h) e [`phir2frameworkv2/CorrelativeSM.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/CorrelativeSM.cpp)
- **Resumo**: Implementa Correlative Scan Matching planar entre quadros através de nuvens de pontos sintéticas geradas a partir de bordas das imagens.
- **Conexão com o Paper**: Alternativa de odometria para ambientes sem textura pontual rica.

---

### [`phir2frameworkv2/mainMap.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/mainMap.cpp) e [`phir2frameworkv2/mainView.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/mainView.cpp)
- **Resumo**: Binários auxiliares (`MapfromImage` e `ViewDensity`) destinados a recortar, quantizar cores e pré-gerar mapas de densidade a partir de ortomosaicos de satélite.
- **Conexão com o Paper**: Ferramentas de suporte à preparação de mapas.

---

## Módulo 7: Matemática e Utilitários de Apoio

### [`phir2frameworkv2/vec2.h/.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/vec2.h), [`phir2frameworkv2/vec3.h/.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/vec3.h), [`phir2frameworkv2/vec4.h/.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/vec4.h) e [`phir2frameworkv2/mat3x3.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/mat3x3.h)
- **Resumo**: Classes matemáticas leves de álgebra linear para vetores 2D, 3D, 4D e matrizes $3 \times 3$, incluindo produtos escalares, vetoriais, normas e operações de projeção.
- **Conexão com o Paper**: Operações geométricas de projeção do descritor esperado (Eq. 3).

---

### [`phir2frameworkv2/Utils.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Utils.h) e [`phir2frameworkv2/Utils.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/Utils.cpp)
- **Resumo**: Coleção de rotinas utilitárias estáticas:
  - `Normalize()`: Mapeia linearmente um valor de $[min_{in}, max_{in}]$ para $[min_{out}, max_{out}]$.
  - `RandomFloat()`, `RandomInt()`: Geração uniforme de números aleatórios.
  - `getRotatedROIFromImage()`: Extrai uma sub-região rotacionada de imagem através de interpolação afim.
  - `matchImages()`: Executa casamento de gabaritos (`cv::matchTemplate`).
- **Conexão com o Paper**: Funções essenciais para normalização do índice de vegetação e amostragem de partículas.

---

### [`phir2frameworkv2/PioneerRobot.h`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/PioneerRobot.h) e [`phir2frameworkv2/PioneerRobot.cpp`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/phir2frameworkv2/PioneerRobot.cpp)
- **Resumo**: Interface para robô terrestre Pioneer da MobileRobots utilizando a biblioteca ARIA.
- **Conexão com o Paper**: Nenhuma conexão com o λ-BRIEF. É um artefato herdado do repositório base PhiR2Framework.
- **PRECISA VALIDAÇÃO**: Pode ser completamente ignorado ou removido em refatorações futuras.

---

## Módulo 8: Scripts e Códigos Úteis

### [`Códigos úteis/OrbSiftImgReg.c`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/C%C3%B3digos%20%C3%BAteis/OrbSiftImgReg.c)
- **Resumo**: Script C/C++ independente para registro e alinhamento das quatro bandas espectrais da câmera Parrot Sequoia (GRE, RED, REG, NIR) com a imagem RGB do sensor. Utiliza detecção de pontos ORB ou SIFT, casamento por força bruta com limiar de Hamming e calcula a matriz homográfica via RANSAC para deformar e sobrepor perfeitamente as bandas.
- **Conexão com o Paper**: Seção V (*Experimental setup* — "simultaneously, the natural color images were downsampled to 1280x960 and aligned with the multispectral images").
- **Parâmetros Hardcoded**: `MAX_FEATURES = 500`, `GOOD_MATCH_PERCENT = 0.15f`, dimensões nominais $1280 \times 960$.
- **PRECISA VALIDAÇÃO**: Apesar da extensão `.c`, o arquivo utiliza namespaces C++ (`using namespace cv; using namespace std;`).

---

### [`Códigos úteis/rosaria_phi-master/`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/C%C3%B3digos%20%C3%BAteis/rosaria_phi-master/)
- **Resumo**: Pacote ROS 1 (catkin) para controle e publicação de dados do robô Pioneer (ARIA) e telemetria laser.
- **Conexão com o Paper**: Não tem nenhuma relação com os experimentos do λ-BRIEF. Código auxiliar do laboratório.

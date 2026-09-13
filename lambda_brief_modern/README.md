# $\lambda$-BRIEF Modern — UAV Multispectral Localization Engine

Uma re-engenharia moderna, modular e de alto desempenho do algoritmo **$\lambda$-BRIEF** (*A new binary descriptor for multispectral images applied to the global localization of UAVs*), desenvolvida em **C++17**, **CMake 3.20+**, **OpenCV 4.x**, **Eigen3**, **OpenMP** e **Python 3** para Ubuntu 24.04 LTS (e distribuições Linux contemporâneas).

Esta implementação elimina todos os débitos técnicos do projeto legado (`phir2frameworkv2`), operando diretamente sobre os datasets originais de 44 GB sem requerer qualquer modificação manual em arquivos de imagem ou trajetórias.

---

## 🚀 Principais Melhorias em Relação ao Código Legado

| Aspecto | Implementação Legada (`phir2frameworkv2`) | Implementação Moderna (`lambda_brief_modern`) |
| :--- | :--- | :--- |
| **Linguagem & Padrão** | C++98 / C++03 com ponteiros crus e macros C | **C++17** com tipagem estrita, `std::filesystem`, `std::unique_ptr` |
| **Sistema de Build** | Qt 5 `qmake` (`.pro`) com caminhos absolutos locais | **CMake 3.20+** modular e independente de IDE |
| **Visão Computacional** | OpenCV 2.x/3.x com macros legadas (`CV_LOAD_IMAGE_COLOR`) e `opencv_contrib` (`xfeatures2d`) | **OpenCV 4.6+** nativo (`cv::SIFT`, `cv::COLOR_BGR2Lab`, etc.) |
| **Dependências Órfãs** | PCL (Point Cloud Library), ARIA (robô Pioneer), OpenGL/GLUT | **Zero dependências órfãs**; cálculo matemático desacoplado de GUI |
| **Resolução de Datasets** | Caminhos absolutos hardcoded (`/home/phi/...`) em `traj.txt` que quebravam execução | **Resolução dinâmica e transparente** via basename e subpastas relativas |
| **Paralelização** | OpenMP com pragmas legados e conflitos de concorrência | **OpenMP** moderno com agendamento estático/dinâmico sem race conditions |
| **Avaliação & Gráficos** | Scripts Python 2 fragmentados (`pylab`, caminhos antigos) | **Python 3** moderno (`numpy`, `matplotlib`, `argparse`) com CDF e boxplots |
| **Qualidade & Testes** | Inexistente (apenas execução manual completa) | **Suíte de regressão automatizada** com 10 testes cobrindo todo o pipeline |

---

## 📦 Requisitos do Sistema

- **Sistema Operacional**: Ubuntu 22.04 LTS ou Ubuntu 24.04 LTS (x86_64)
- **Compilador**: GCC >= 11 ou Clang >= 14 (suporte a C++17)
- **CMake**: >= 3.20
- **Bibliotecas**:
  ```bash
  sudo apt update
  sudo apt install -y build-essential cmake libopencv-dev libeigen3-dev libomp-dev python3-numpy python3-matplotlib python3-scipy
  ```

---

## 🛠️ Compilação e Testes

### 1. Compilando o Projeto
```bash
cd lambda_brief_modern
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Os artefatos gerados em `build/` incluem:
- `liblambda_brief_core.a`: Biblioteca estática modular contendo todo o pipeline.
- `lambda_brief_loc`: Executável de linha de comando para localização.
- `tests/test_suite`: Binário da suíte de testes de regressão.

### 2. Executando a Suíte de Testes
```bash
ctest --test-dir build --output-on-failure
# ou diretamente:
./build/tests/test_suite
```
A suíte valida:
1. Fórmulas exatas de NDVI, NDRE, GRVI e divisão por zero.
2. Inversão do VI ($\vartheta_{\text{inv}} = -VI$) e mapeamento para 8 bits $[0, 255]$.
3. Propriedades analíticas da máscara Gaussiana 2D centrada $\mathcal{N}$.
4. Fusão ponderada de máscaras via $\alpha$ (Eq. 1 do artigo).
5. Roleta estocástica por aceitação (Lipowski & Lipowska, 2012).
6. Tamanho e formato estritamente binário do descritor (512 bits para 256 pares).
7. Distância normalizada de Hamming para casos idênticos, opostos e parciais.
8. Inicialização uniforme do filtro de partículas e cálculo analítico de $N_{\text{eff}}$.
9. Conservação do número de partículas na reamostragem Low Variance.
10. Normalização angular cíclica de yaw em $[-\pi, \pi]$.

---

## 💻 Guia de Uso da Aplicação CLI (`lambda_brief_loc`)

```bash
./build/lambda_brief_loc [opções]
```

### Parâmetros Suportados

| Opção | Tipo | Padrão | Descrição |
| :--- | :---: | :---: | :--- |
| `--map` | `string` | *(obrigatório)* | Caminho para a imagem do mapa global de satélite |
| `--dataset_dir` | `string` | *(obrigatório)* | Diretório do voo (ex: `../Datasets/ADM/0036`) |
| `--output_dir` | `string` | `./results` | Pasta de destino dos logs de trajetória e erros |
| `--num_particles` | `int` | `50000` | Quantidade de partículas do filtro MCL |
| `--alpha` | `float` | `0.5` | Peso $\alpha \in [0, 1]$ da máscara de vegetação na Eq. (1) |
| `--vi` | `string` | `NDVI` | Índice espectral: `NDVI`, `NDRE`, `NDVI_RED_RE`, `GRVI` |
| `--pairs` | `int` | `256` | Número de pares de píxeis no descritor (256 pares = 512 bits) |
| `--color_mode` | `string` | `LAB_NO_L` | Modo de cor: `LAB_NO_L` (legado), `LAB`, `GRAY` |
| `--start_frame` | `int` | `-1` | Frame inicial para fatiamento da trajetória (-1 = início) |
| `--end_frame` | `int` | `-1` | Frame final para fatiamento da trajetória (-1 = final) |
| `--pixel_to_meter`| `float` | `0.188679` | Fator de conversão píxel para metros (`0.188679` para ADM, `0.053` para UFRGS) |
| `--ms_filter` | `string` | `none` | Pré-processamento espectral: `none`, `bilateral`, `gaussian` |
| `--bf_d` | `int` | `6` | Diâmetro do filtro bilateral |
| `--bf_sc` | `float` | `5.0` | SigmaColor do filtro bilateral |
| `--bf_ss` | `float` | `5.0` | SigmaSpace do filtro bilateral |
| `--seed` | `int` | `42` | Semente aleatória para reprodutibilidade estrita |
| `--quiet` | flag | `false` | Reduz a verbosidade de saída no console |

---

## 🔬 Exemplos de Reprodução de Trajetórias

### Exemplo 1: Voo ADM2 (`Datasets/ADM/0036`) — 10.000 partículas
```bash
./build/lambda_brief_loc \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --dataset_dir "../Datasets/ADM/0036" \
  --output_dir "./results_adm0036" \
  --num_particles 10000 \
  --alpha 0.5 \
  --vi NDVI \
  --start_frame 0 \
  --end_frame 50
```

### Exemplo 2: Voo UFRGS VET (`Datasets/UFRGS VET/voo_100m`)
```bash
./build/lambda_brief_loc \
  --map "../Datasets/UFRGS VET/voo_100m/Mapas/2018_09_16.png" \
  --dataset_dir "../Datasets/UFRGS VET/voo_100m" \
  --output_dir "./results_ufrgs" \
  --num_particles 20000 \
  --pixel_to_meter 0.053 \
  --alpha 0.5 \
  --vi NDVI \
  --start_frame 390 \
  --end_frame 450
```

---

## 📊 Avaliação e Visualização com Python 3

Após a execução, avalie as métricas de erro de posição e orientação contra o Ground Truth GPS/IMU e gere os gráficos comparativos:

```bash
python3 scripts/evaluate_results.py \
  --results_dir ./results_adm0036 \
  --scale 0.188679245 \
  --plot \
  --output_plot evaluation_plots.png
```

O script produzirá:
- Relatório no terminal com **Erro Médio (m)**, **RMSE (m)**, **Desvio Padrão (m)**, **Mediana (m)** e **Erro de Yaw (graus)**.
- Painel 2x2 salvo em imagem contendo:
  1. Comparação 2D da trajetória estimada vs Ground Truth;
  2. Evolução temporal do erro de posição por frame;
  3. Curva de Distribuição Cumulativa de Erro (CDF);
  4. Evolução do erro de orientação (Yaw) por frame.

---

## 📁 Estrutura de Diretórios da Implementação

```
lambda_brief_modern/
├── CMakeLists.txt                 # Configuração de compilação modular
├── README.md                      # Este documento
├── include/
│   ├── core/
│   │   ├── types.hpp              # Tipos fundamentais (Pose, Particle, Enums)
│   │   ├── vegetation_index.hpp   # Cálculo de NDVI, NDRE, NDVI_red&re, GRVI
│   │   ├── mask_generator.hpp     # Máscara Gaussiana 2D, HistEq e fusão por alpha
│   │   ├── pixel_sampler.hpp      # Roleta estocástica (Lipowski & Lipowska)
│   │   └── descriptor.hpp         # Descritor Lab sem L*, projeção planar e Hamming
│   ├── localization/
│   │   ├── visual_odometry.hpp    # Odometria SIFT/ORB homográfica + 4 filtros geométricos
│   │   └── particle_filter.hpp    # Filtro MCL 4-DoF com Low Variance Resampling
│   ├── dataset/
│   │   └── dataset_loader.hpp     # Carregador com resolução automática de paths
│   └── config/
│       └── config.hpp             # Parser CLI robusto com flags equivalentes
├── src/
│   ├── core/                      # Implementações do núcleo matemático
│   ├── localization/              # Implementações do MCL e Odometria
│   ├── dataset/                   # Implementação do leitor de datasets
│   └── config/                    # Implementação do parser de configurações
├── apps/
│   └── run_localization.cpp       # Ponto de entrada do executável principal
├── tests/
│   ├── CMakeLists.txt             # Configuração CTest
│   └── test_suite.cpp             # Suíte com 10 baterias de testes unitários
└── scripts/
    └── evaluate_results.py        # Avaliação e plotagem em Python 3
```

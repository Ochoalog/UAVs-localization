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
| **Qualidade & Testes** | Inexistente (apenas execução manual completa) | **Suíte de regressão automatizada** com 21 baterias de testes cobrindo todo o pipeline |
| **Repetições** | 30 repetições no mesmo processo, com nuvem inicial repetida e escala herdada (`MCL::restart`) | Um processo por `--seed`, nuvem sorteada do zero; abBRIEF e λ-BRIEF pareados pela seed |
| **Baseline abBRIEF** | Caminho de código separado, pares quase uniformes e fixos | abBRIEF = λ-BRIEF com α = 0 (mesmo amostrador, mesma máscara gaussiana) |

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
- `diag_likelihood`: Diagnóstico da discriminabilidade do modelo de observação (d(GT) × poses aleatórias).
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
11. Quantização de cor em 25 níveis bit-exata com o legado.
12. Atualização bayesiana multiplicativa dos pesos ($w_t \propto w_{t-1}\,p(z|x)$).
13. Reamostragem a cada frame quando `resample_neff_ratio ≥ 1`.
14. Priors de altímetro (escala) e bússola (yaw) multiplicam a verossimilhança e valem por um único update.
15. Injeção de partículas aleatórias re-sorteia ≈ `inject_ratio`·N partículas e respeita o prior de escala.
16. Estimador `mode` escolhe o cluster dominante (não o ponto médio entre dois modos).
17. Gate de injeção: contraste medido, nada injetado abaixo do limiar; Neff antes da reamostragem registrado.
18. Inicialização de diagnóstico em torno do GT (`--init_around_gt`).
19. Controlador de duas fases: transições GLOBAL → TRACKING → GLOBAL.
20. Injeção desligada continua medindo o contraste e não mexe na nuvem.
21. Odometria visual em fluxo (streaming) igual à calculada quadro a quadro.

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
| `--traj` | `string` | `<dataset_dir>/traj.txt` | Outro `traj.txt` (ex.: `Datasets/ADM/0035/traj_comp.txt` no ADM1) |
| `--gt` | `string` | `<dataset_dir>/traj_truth.txt` | Outro arquivo de GT (ex.: `ADM3_ground_truth.txt` do Zenodo no ADM3) |
| `--num_particles` | `int` | `50000` | Quantidade de partículas do filtro MCL |
| `--alpha` | `float` | `0.5` | Peso $\alpha \in [0, 1]$ da máscara de vegetação na Eq. (1) |
| `--vi` | `string` | `NDVI` | Índice espectral: `NDVI`, `NDRE`, `NDVI_RED_RE`, `GRVI` |
| `--pairs` | `int` | `256` | Número de pares de píxeis no descritor (256 pares = 512 bits) |
| `--color_mode` | `string` | `LAB_NO_L` | Modo de cor: `LAB_NO_L` (legado), `LAB`, `GRAY` |
| `--pair_pattern` | `string` | `roulette` | Como os pares são escolhidos: `roulette` (roleta sobre a máscara do α, re-sorteada a cada frame), `roulette_fixed` (sorteada uma vez), `uniform`, `legacy_ab` (o abBRIEF do legado: padrão fixo e quase uniforme; ver `docs/MANUAL_TESTES_ARTIGO.md`, seção 8) |
| `--stability_map` | `string` | — | **v3:** raster 8-bit do tamanho do mapa (255 = peso 1, de `scripts/make_stability_map.py`). Cada par pesa `min(W(A), W(B))` nos pontos projetados no mapa. Sem ele, a distância é a da v2 |
| `--stability_formula` | `string` | `normalized` | **v3:** `normalized` (Σw·e / Σw) ou `coin` (o par instável conta 0,5) |
| `--save_pairs` | flag | off | Grava os pares de cada frame (`pairs_log.csv`, `frames_log.csv`) e a máscara de amostragem / NDVI do drone (`amostragem/`), para `scripts/v3_video.py`. Não muda o resultado |
| `--start_frame` | `int` | `-1` | Frame inicial para fatiamento da trajetória (-1 = início) |
| `--end_frame` | `int` | `-1` | Frame final para fatiamento da trajetória (-1 = final) |
| `--pixel_to_meter`| `float` | `0.188679` | Fator de conversão píxel para metros (`0.188679245` para ADM, `0.10` para UFRGS VET mapas V3) |
| `--quantize` | `int` | `25` | Quantização de cor do mapa e dos frames (legado `-mappp 1`); `0` desliga |
| `--min_scale` / `--max_scale` | `float` | `0.1` / `3.0` | Faixa da escala z das partículas (ADM 0.1–3; UFRGS 0.5–5) |
| `--var_brief` | `float` | `0.0225` | Variância σ² da verossimilhança gaussiana sobre a distância de Hamming |
| `--resample_neff` | `float` | `1.0` | Reamostra quando Neff < r·N; `r ≥ 1` reamostra a cada frame (legado) |
| `--pos_noise` | `float` | `10` | Ruído uniforme ±px em x,y por passo de predição |
| `--inject_ratio` | `float` | `0` | Fração de partículas re-sorteadas no mapa a cada frame (recuperação de colapso). Recomendado `0.05`–`0.10` |
| `--two_phase` | flag | off | Filtro em duas fases: priors, injeção e `--vo_scale_deadband` só até convergir; depois rastreio no modo do artigo. Log em `phase_log.txt` |
| `--converge_std` / `--converge_frames` / `--converge_contrast` | `float`/`int`/`float` | `100` / `5` / `1.3` | Convergência = dispersão da nuvem < px por N frames seguidos, com contraste C ≥ c (`0` ignora C; C exige `--inject_ratio > 0`) |
| `--relocalize_std` / `--relocalize_frames` | `float`/`int` | `0` / `5` | Volta à fase global se a dispersão passar de px por N frames (`0` = nunca) |
| `--track_vo_scale_deadband` | `float` | `0.01` | Zona morta do Δz da VO na fase de rastreio (legado) |
| `--inject_gate` | `float` | `0` | Só injeta em frames informativos: contraste C = (média nula − média da nuvem)/desvio nulo ≥ c (ex.: `0.8`) |
| `--estimator` | `string` | `mean` | `mean` (média da nuvem) ou `mode` (cluster mais denso + mean-shift) |
| `--mode_radius` | `float` | `100` | Célula/kernel do estimador `mode` (px) |
| `--motion_noise` | `float` | `0` | Ruído extra ±f·\|passo da odometria\| em x,y |
| `--inject_spread` | `float` | `0` | Escala a injeção por σ_xy da nuvem / px (nuvem concentrada ⇒ quase sem injeção). Recomendado `300` |
| `--scale_prior_sigma` | `float` | `0` | Prior de escala (emula altímetro): σ relativo em torno de `gain·z_GT`. Recomendado `0.10` |
| `--scale_prior_gain` | `float` | `1.0` | Centro do prior = `gain·z_GT`. ADM/0036 (RGBres, HFOV 63,9°): `0.87` |
| `--altimeter_file` | `string` | — | Altímetro real por frame (`altimeter_baro.txt` de `scripts/extract_altimeter.py` ou `scripts/adm3_altimeter_from_kml.py`): o prior de escala passa a ser centrado em `gain·z_baro`, sem ler o GT. Usado no regime `final_baro` com `--scale_prior_gain 1` |
| `--yaw_prior_sigma` | `float` | `0` | Prior de yaw em graus (emula bússola) em torno do yaw do GT |
| `--init_around_gt` | `float` | — | Diagnóstico: inicia a nuvem como gaussiana de σ = px em torno do GT do 1º frame (em vez de uniforme) |
| `--vo_scale_deadband` | `float` | `0.01` | Δz da VO com \|Δz−1\| < e vira 1 (legado 0,01). `1` ignora a escala da VO (recomendado: ela não se correlaciona com a altitude) |
| `--sift_features` | `int` | `0` | Máximo de features SIFT da odometria (0 = ilimitado, como no legado). Recomendado `4000` com `--vo_matcher ratio` |
| `--vo_matcher` | `string` | `legacy` | `legacy` (1-NN + heurística `dist ≤ 2·d_min`, ~50 matches) ou `ratio` (Lowe 0,75 + checagem mútua, milhares de matches) |
| `--vo_ratio` | `float` | `0.75` | Razão de Lowe para `--vo_matcher ratio` |
| `--vo_robust` | `string` | `ransac` | Estimador da homografia: `ransac` (legado) ou `magsac` (`cv::USAC_MAGSAC`) |
| `--vo_reproj` | `float` | `3.0` | Limiar de inlier da homografia (px). Recomendado `2` com `magsac` |
| `--vo_min_inliers` | `int` | `0` | Marca como não confiável a transição com menos inliers (0 = desligado) |
| `--unreliable_motion` | `string` | `zero` | Movimento usado quando a homografia é reprovada: `zero` (legado), `cv` (velocidade constante dos últimos passos confiáveis) ou `apply` |
| `--odom_file` | `string` | — | Odometria pré-gravada (`traj/odom-*.txt`) em vez da VO online. Avisa se tiver menos passos que a fatia |
| `--apply_unreliable_odom` | flag | `false` | Alias de `--unreliable_motion apply` (mantido para scripts antigos) |
| `--gt_offset` | `int` | auto | Linha do `traj.txt` correspondente à linha 0 do `traj_truth.txt` (ADM/0036: 84; UFRGS: 204). **No ADM/0036 o GT está ~2 frames atrasado: use `86`** (ver `../docs/revisao_odometria.md`) |
| `--ms_filter` | `string` | `none` | Pré-processamento espectral: `none`, `bilateral`, `gaussian` |
| `--bf_d` | `int` | `6` | Diâmetro do filtro bilateral |
| `--bf_sc` | `float` | `5.0` | SigmaColor do filtro bilateral |
| `--bf_ss` | `float` | `5.0` | SigmaSpace do filtro bilateral |
| `--seed` | `int` | `42` | Semente aleatória para reprodutibilidade estrita |
| `--quiet` | flag | `false` | Reduz a verbosidade de saída no console |

---

## 🔬 Exemplos de Reprodução de Trajetórias

> Os índices `--start_frame/--end_frame` são **linhas do `traj.txt`**, não números de imagem.
> O Ground Truth só existe a partir do offset (ADM/0036: linha 84 = imagem 0199; UFRGS: linha 204 = imagem 0390).

### Exemplo 1: Voo ADM2 (`Datasets/ADM/0036`, imagens 0199–0394) — configuração do artigo
```bash
./build/lambda_brief_loc \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --dataset_dir "../Datasets/ADM/0036" \
  --output_dir "./results_adm0036" \
  --num_particles 50000 \
  --alpha 0.5 \
  --vi NDVI \
  --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 \
  --min_scale 0.1 --max_scale 3.0 \
  --start_frame 84 \
  --end_frame 279
```

**Filtro em duas fases (a testar, ver `../docs/analise_fim_do_voo_adm2.md`):** a análise mostrou que priors e injeção aceleram a convergência, mas fazem o filtro perder o rastro no pouso. `--two_phase` os mantém só até convergir:
```bash
  ... --two_phase --scale_prior_sigma 0.10 --scale_prior_gain 0.87 --yaw_prior_sigma 15 \
      --inject_ratio 0.05 --inject_spread 300 --vo_scale_deadband 1
```

**Configuração atual (regime `final_baro`, usada nos testes do artigo de 17/10/2026):** duas fases, injeção adaptativa, barômetro real e odometria fixa da VO nova. Os scripts montam esse comando sozinhos (ver a seção de experimentos abaixo):
```bash
  ... --odom_file insumos/odom/ADM0036_vo_ratio.txt --two_phase --inject_ratio 0.05 --inject_spread 300 \
      --pos_noise 17 --scale_prior_sigma 0.10 --altimeter_file ../Datasets/ADM/0036/altimeter_baro.txt \
      --scale_prior_gain 1
```

**Odometria visual recomendada (12/10/2026, ver `../docs/revisao_odometria.md`):** matcher com ratio test + MAGSAC, 4000 features e ruído de predição coerente com o erro real da VO. No `--two_phase` (abBRIEF, 3 sementes) reduziu o erro médio de 33,8 para 28,9 m e o tempo de 1,6 para 0,95 s/frame:
```bash
  ... --vo_matcher ratio --vo_robust magsac --vo_reproj 2 --sift_features 4000 --pos_noise 17 --vo_scale_deadband 1
```
Para avaliar contra o GT alinhado no tempo (ADM/0036), acrescente `--gt_offset 86`, ou reavalie execuções antigas com `scripts/check_gt_alignment.py --errors ... --lag -2`.

**Configuração robusta recomendada (anterior)** (ver `../docs/diagnostico_convergencia_e_pipeline.md`, seção 2.7). Com altímetro, emulado pelo GT de escala (voo completo, seed 1: média 78 m contra 272 m; mediana 8 m nos frames 110–195):
```bash
  ... --inject_ratio 0.05 --inject_spread 300 --vo_scale_deadband 1 --scale_prior_sigma 0.10 --scale_prior_gain 0.87
```
Sem sensores extras (converge, mas o rastreio é frágil):
```bash
  ... --inject_ratio 0.10
```

### Exemplo 2: Voo UFRGS VET (`Datasets/UFRGS VET/voo_100m`, imagens 0390–0613)
```bash
./build/lambda_brief_loc \
  --map "../Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_18_09_2018.jpg" \
  --dataset_dir "../Datasets/UFRGS VET/voo_100m" \
  --output_dir "./results_ufrgs" \
  --num_particles 50000 \
  --pixel_to_meter 0.10 \
  --min_scale 0.5 --max_scale 5.0 \
  --alpha 0.5 \
  --vi NDVI \
  --start_frame 204 \
  --end_frame 427
```

Diagnóstico da convergência e descrição detalhada do pipeline: `../docs/diagnostico_convergencia_e_pipeline.md`.

---

## 🧪 Experimentos, testes do artigo e onde ficam os resultados

Todo experimento fica em **`experimentos/<AAAA-MM-DD>_<nome>/`** e ganha uma linha em **[`experimentos/INDICE.md`](experimentos/INDICE.md)**, que também traz um resumo de cada teste. Os scripts fazem isso sozinhos quando não se passa `--out`/`--output_base_dir` (ver `scripts/experimentos.py`). Entradas reutilizadas (odometria fixa, relatórios do altímetro) ficam em `insumos/`.

**Teste do artigo (Figs. 4 e 5):** abBRIEF × λ-BRIEF em 4 voos × 5 mapas. Passo a passo em **[`docs/MANUAL_TESTES_ARTIGO.md`](docs/MANUAL_TESTES_ARTIGO.md)**.

```bash
python3 scripts/paper_comparison.py --protocolo novo --lambda_alpha 1.0              # 1 seed
python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0        # média de 5 seeds
python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 \
        --reaproveitar experimentos/2026-10-17_artigo_novo_a1_5seeds                  # reusa o abBRIEF
```

**v3 (branch `v3`): peso de estabilidade do Sentinel-2.** Os pares que caem onde o mapa costuma mudar (desvio do NDVI do S2 ao longo dos anos) pesam menos na distância. Passo a passo na seção 10 do manual.

```bash
python3 scripts/make_stability_map.py --modo A                 # insumos/estabilidade/ADM_35_A.png
python3 scripts/v3_comparison.py --seeds 1                     # ab e λ com e sem o peso, ADM1–3 × 5 mapas
```

**Regimes do filtro** (`scripts/benchmark_runner.py`):

| Regime | O que é |
|---|---|
| `vanilla` | protocolo do artigo: sem priors, sem injeção, odometria gravada do legado |
| `final_baro` | duas fases + injeção adaptativa + `--pos_noise 17` + barômetro real (`--altimeter_file`) + odometria fixa da VO nova (`insumos/odom/`). Recomendado |
| `final_noalt` | o mesmo, sem nenhum prior de escala (só câmera + VO) |
| `final_gtalt` | o mesmo, com altímetro emulado pelo z do GT (só referência) |

**Scripts** (`scripts/`):

| Script | Para quê |
|---|---|
| `paper_comparison.py` | teste do artigo com 1 seed (figuras como as Figs. 4 e 5, tabela por voo e mapa) |
| `paper_comparison_seeds.py` | o mesmo com N seeds e média; reaproveita execuções idênticas de outra pasta |
| `ab_legado_comparison.py` | abBRIEF do legado (`--pair_pattern legacy_ab`) × abBRIEF × λ: quanto do ganho do artigo vinha do baseline |
| `make_stability_map.py` | v3: raster de peso do Sentinel-2 (A = estabilidade ao longo dos anos, B = mudança mapa → voo) |
| `v3_comparison.py` | v3: ab e λ com e sem `--stability_map`, com Wilcoxon e o critério mínimo |
| `v3_video.py` | v3: vídeos do drone (RGB, NDVI, máscara), do mapa e do Sentinel-2 com os pares e as partículas |
| `benchmark_runner.py` | benchmark pareado por dataset, mapa, descritor/α, regime e seed; `--make_odom` grava a odometria fixa |
| `consolidate_benchmark.py` | tabelas, testes estatísticos e gráficos de um benchmark (`--benchmark_dir`) |
| `extract_altimeter.py` | barômetro real e `traj_truth_sync.txt` a partir dos logs do DJI (ADM1, ADM2, UFRGS) |
| `adm3_altimeter_from_kml.py` | altura do ADM3 a partir da trilha `ADM3.kml` do Zenodo |
| `make_videos.py` / `render_video.py` | vídeos da nuvem de partículas |
| `plot_evaluation_compare.py`, `plot_paper_grid.py` | figuras comparativas do piloto e do benchmark |
| `check_gt_alignment.py` | mede o atraso entre GT e imagens a partir de um log `LB_DEBUG_ODOM` |
| `analise_fim.py`, `eval_two_phase.sh`, `batch_run_all_maps.py` | análises do ADM2 que motivaram as duas fases |
| `experimentos.py` | cria e registra pastas (`listar`, `nova`, `registrar`, `onde <caminho antigo>`) |

**Documentação:**

- [`HISTORICO_RESULTADOS.md`](HISTORICO_RESULTADOS.md): diário de cada rodada (objetivo, comando, resultados, conclusão).
- [`docs/achados_codigo_legado.md`](docs/achados_codigo_legado.md): por que o λ ganhava com folga no artigo (baseline e repetições do legado).
- [`docs/voos_e_mapas_do_artigo.md`](docs/voos_e_mapas_do_artigo.md): voos, imagens e mapas usados no artigo.
- [`REORGANIZACAO_PASTAS.md`](REORGANIZACAO_PASTAS.md): de onde veio cada pasta de `experimentos/`.

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

## 🎥 Gerando Vídeos da Eliminação de Partículas (Abordagem B)

A versão moderna inclui um pipeline para gravação de vídeos em alta resolução (**Full HD / 720p / 4K**) demonstrando visualmente o enxame de partículas sendo filtrado e convergindo sobre o mapa de satélite:

### Passo 1: Executar a localização com exportação de partículas
Adicione a flag `--save_particles` (e opcionalmente `--export_particles <N>` para limitar o número de partículas exportadas por frame, ex: 1000):

```bash
./build/lambda_brief_loc \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --dataset_dir "../Datasets/ADM/0036" \
  --output_dir "./results_adm0036" \
  --num_particles 10000 \
  --alpha 0.5 \
  --vi NDVI \
  --save_particles \
  --export_particles 1000
```

### Passo 2: Renderizar o vídeo MP4 com o script Python
```bash
python3 scripts/render_video.py \
  --results_dir ./results_adm0036 \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --output ./results_adm0036/localization_video.mp4 \
  --fps 6.0
```

### O que o vídeo renderizado exibe:
- **Mapa de Satélite Real** no fundo com foco dinâmico (*zoom automático*) na área do voo;
- **Nuvem de Partículas Colorida**: pontinhos com transparência ponderados por probabilidade (partículas fracas em laranja/vermelho translúcido, partículas fortes em amarelo/verde brillante com vetores de direção Yaw);
- **Ground Truth (GPS/IMU real)** em verde com rastro do trajeto;
- **Estimativa do Filtro MCL** em amarelo/ciano com mira de precisão;
- **Painel de Telemetria (HUD)** no canto superior com número do frame, coordenadas atuais, erro instantâneo de translação (metros) e erro de orientação (graus).

---

## 📁 Estrutura de Diretórios da Implementação

```
lambda_brief_modern/
├── CMakeLists.txt                 # Configuração de compilação modular
├── README.md                      # Este documento
├── HISTORICO_RESULTADOS.md        # Diário de bordo de todas as rodadas de teste
├── REORGANIZACAO_PASTAS.md        # Pastas antigas de resultados → experimentos/
├── run_piloto.sh                  # Piloto barômetro × sem altímetro (15/10/2026)
├── include/
│   ├── core/
│   │   ├── types.hpp              # Tipos fundamentais (Pose, Particle, Enums)
│   │   ├── vegetation_index.hpp   # Cálculo de NDVI, NDRE, NDVI_red&re, GRVI
│   │   ├── mask_generator.hpp     # Máscara Gaussiana 2D, HistEq e fusão por alpha
│   │   ├── pixel_sampler.hpp      # Roleta estocástica (Lipowski & Lipowska)
│   │   └── descriptor.hpp         # Descritor Lab sem L*, projeção planar e Hamming
│   ├── localization/
│   │   ├── visual_odometry.hpp    # Odometria SIFT homográfica (legacy ou ratio + MAGSAC)
│   │   ├── particle_filter.hpp    # Filtro MCL 4-DoF: pesos bayesianos, priors, injeção
│   │   └── phase_controller.hpp   # Controlador GLOBAL → TRACKING do --two_phase
│   ├── dataset/
│   │   └── dataset_loader.hpp     # Carregador com resolução automática de paths
│   └── config/
│       └── config.hpp             # Parser CLI (avisa sobre opções desconhecidas)
├── src/                           # Implementações (core, localization, dataset, config)
├── apps/
│   ├── run_localization.cpp       # Executável principal lambda_brief_loc
│   └── diag_likelihood.cpp        # Diagnóstico do modelo de observação
├── tests/
│   ├── CMakeLists.txt             # Configuração CTest
│   └── test_suite.cpp             # Suíte com 21 baterias de testes unitários
├── scripts/                       # Benchmark, teste do artigo, altímetro, avaliação e vídeos
├── docs/
│   ├── MANUAL_TESTES_ARTIGO.md    # Como rodar os testes do artigo
│   ├── achados_codigo_legado.md   # Diferenças legado × engine moderna que mudam a comparação
│   └── voos_e_mapas_do_artigo.md  # Voos e mapas do artigo
├── experimentos/                  # Um experimento por pasta: AAAA-MM-DD_<nome>/ (não versionadas as execuções brutas)
│   └── INDICE.md                  # Índice e resumo de cada teste
└── insumos/
    ├── odom/                      # Odometria fixa da VO nova (<DS>_vo_ratio.txt + .meta.json)
    └── altimetro/                 # Relatórios do altímetro real
```

# CLAUDE.md — λ-BRIEF / UAVs Localization

Guia operacional para agentes e desenvolvedores. Leia antes de alterar `lambda_brief_modern/`.
Diagnóstico detalhado da convergência e descrição completa do pipeline:
`docs/diagnostico_convergencia_e_pipeline.md`.

## 1. O que é o projeto

Localização global (problema do *kidnapped robot*) de um VANT sem GPS. Um filtro de partículas
MCL 4-DoF (x, y, escala z, yaw) compara cada frame RGB do drone (Parrot Sequoia, 1 FPS) com uma
imagem de satélite (Google Earth, 4800×4800 px) usando o descritor binário **λ-BRIEF**. O λ-BRIEF
é o abBRIEF (CIE L\*a\*b\*, 2 bits por par), com os 256 pares de pixels sorteados por uma máscara
que mistura o índice de vegetação invertido (NDVI das bandas NIR/RED) e uma gaussiana centrada.

| Diretório | Conteúdo |
|---|---|
| `lambda_brief_modern/` | Engine moderna: C++17, CMake ≥ 3.20, OpenCV ≥ 4.6 (SIFT no módulo principal), Eigen3, OpenMP |
| `lambda_brief_modern/include,src/core/` | `vegetation_index` (NDVI/NDRE/NDVI_RED_RE/GRVI), `mask_generator` (gaussiana, equalização, fusão α), `pixel_sampler` (roleta por aceitação estocástica), `descriptor` (quantização, Lab, projeção, Hamming) |
| `lambda_brief_modern/.../localization/` | `ParticleFilter` (MCL, Low Variance Sampler), `VisualOdometry` (SIFT + FLANN + homografia RANSAC + 4 filtros geométricos) |
| `lambda_brief_modern/.../dataset/` | `DatasetLoader`: lê `traj*.txt`, resolve caminhos legados pelo *basename*, pareia o GT |
| `lambda_brief_modern/apps/` | `run_localization.cpp` → `lambda_brief_loc`; `diag_likelihood.cpp` → diagnóstico do modelo de observação |
| `lambda_brief_modern/scripts/` | `paper_comparison.py` / `paper_comparison_seeds.py` (teste do artigo, 1 ou N seeds), `benchmark_runner.py` + `consolidate_benchmark.py` (benchmark pareado), `extract_altimeter.py` / `adm3_altimeter_from_kml.py` (altímetro real), `experimentos.py` (pastas e índice), `evaluate_results.py`, `render_video.py` / `make_videos.py`. Lista completa no `lambda_brief_modern/README.md` |
| `lambda_brief_modern/experimentos/` | Um experimento por pasta (`AAAA-MM-DD_<nome>/`), índice e resumo em `INDICE.md`. Entradas reutilizadas em `insumos/` (odometria fixa, altímetro) |
| `phir2frameworkv2/` | Código legado (C++98/Qt/OpenCV 3), **referência de comportamento**. Arquivos-chave: `Mcl.cpp`, `BriefHeuristic.cpp`, `DroneRobot.cpp`, `main.cpp` |
| `Datasets/ADM/0040_zenodo/` | Mapas (`Maps/`) e GT (`GT_DAT/ADM3_ground_truth.txt`, `ADM3.kml`) do ADM3, do Zenodo 4070013. O `traj_truth.txt` local do 0040 está em outro sistema de coordenadas: não use |
| `Datasets/ADM/{0035,0036,0039,0040}`, `Datasets/UFRGS VET/voo_100m` | Imagens `Images_{RGB,NIR,RED,REG,GRE}/`, `traj*.txt`, `traj_truth.txt`, mapas (`Mapas/` no ADM, **`Mapa/`** na UFRGS), odometria gravada em `traj/odom-*.txt` |
| `Experimentos/`, `UFRGS_VET_390_613_50K_apenas_beta_4_mapas/` | Logs do legado (30 execuções por configuração). Servem de baseline |

## 2. Build e testes

```bash
cd lambda_brief_modern
cmake -B build -DCMAKE_BUILD_TYPE=Release        # Release por padrão (-O3 -march=native)
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure        # ou ./build/tests/test_suite

# Debug (asserts, sem -O3; útil com gdb/ASan)
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O0 -g -fsanitize=address"
cmake --build build-debug -j"$(nproc)"
```

Dependências (Ubuntu 22.04/24.04): `build-essential cmake libopencv-dev libeigen3-dev libomp-dev python3-numpy python3-matplotlib python3-opencv`.
`-march=native` gera binário não portável. Remova-o do `CMakeLists.txt` ao compilar para outra máquina.

## 3. Execução (`lambda_brief_loc`)

Os parâmetros abaixo reproduzem os experimentos do artigo (`Experimentos/ADM_036_30x_*` e `UFRGS_VET_390_613_*`).

### ADM2 (dataset 0036, imagens 0199–0394 = linhas 84–279 do `traj.txt`)
```bash
./build/lambda_brief_loc \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --dataset_dir "../Datasets/ADM/0036" --output_dir ./results_adm0036 \
  --start_frame 84 --end_frame 279 \
  --num_particles 50000 --pairs 256 --alpha 0.5 --vi NDVI --color_mode LAB_NO_L \
  --min_scale 0.1 --max_scale 3.0 --quantize 25 \
  --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 \
  --pixel_to_meter 0.188679245 --seed 42
# Para usar a MESMA odometria do benchmark legado:
#   --odom_file "../Datasets/ADM/0036/traj/odom-190723-145134_sift_bruteforce.txt"
# Configuração robusta (ver docs/diagnostico_convergencia_e_pipeline.md, seção 2.7):
#   regime final_baro (atual):   ver lambda_brief_modern/README.md (duas fases + injeção + --altimeter_file)
#   duas fases:                  --two_phase + os priors/injeção abaixo (ver docs/analise_fim_do_voo_adm2.md)
#   com altímetro (recomendado): --inject_ratio 0.05 --inject_spread 300 --vo_scale_deadband 1 \
#                                --scale_prior_sigma 0.10 --scale_prior_gain 0.87
#   sem sensores:                --inject_ratio 0.10   (converge, mas o rastreio é frágil)
```

### UFRGS VET (voo_100m, imagens 0390–0613 = linhas 204–427 do `traj.txt`)
```bash
./build/lambda_brief_loc \
  --map "../Datasets/UFRGS VET/voo_100m/Mapa/UFRGS_VET_V3_18_09_2018.jpg" \
  --dataset_dir "../Datasets/UFRGS VET/voo_100m" --output_dir ./results_ufrgs \
  --start_frame 204 --end_frame 427 \
  --num_particles 50000 --alpha 0.5 --vi NDVI \
  --min_scale 0.5 --max_scale 5.0 --quantize 25 \
  --pixel_to_meter 0.10 --seed 42
#   --odom_file "../Datasets/UFRGS VET/voo_100m/traj/odom-190219-144314.txt"
```

Opções principais (a lista completa sai com `--help`):

| Opção | Padrão | Observação |
|---|---|---|
| `--num_particles` | 50000 | O artigo usa 50k. Com 20k a convergência global fica bem menos confiável |
| `--resample_neff` | 1.0 | `>= 1` reamostra **todo frame** (legado). `< 1` só reamostra quando Neff < r·N |
| `--var_brief` | 0.0225 | σ² da verossimilhança gaussiana sobre a distância de Hamming normalizada |
| `--min_scale/--max_scale` | 0.1 / 3.0 | Faixa de z na inicialização e no clamp. ADM: 0.1–3, UFRGS: 0.5–5 |
| `--quantize` | 25 | Quantização de cor do mapa e dos frames (legado `-mappp 1`). `0` desliga |
| `--odom_file` | — | Usa a odometria gravada em vez da VO online (uma linha por transição: `dx dy dz dyaw reliable`) |
| `--sift_features` | 0 | 0 = ilimitado, como no legado. Recomendado 4000 com `--vo_matcher ratio` |
| `--vo_matcher legacy\|ratio` | legacy | `ratio` = Lowe 0,75 + checagem mútua (milhares de inliers contra ~50 do legado) |
| `--vo_robust ransac\|magsac` / `--vo_reproj px` | ransac / 3 | Recomendado `magsac` com 2 px |
| `--vo_min_inliers n` | 0 | Transições com menos inliers viram não confiáveis |
| `--unreliable_motion zero\|cv\|apply` | zero | Homografia reprovada: movimento nulo (legado), velocidade constante ou o movimento reprovado. `--apply_unreliable_odom` = `apply` |
| `--gt_offset` | auto | Índice do `traj.txt` que corresponde à linha 0 do `traj_truth.txt`. **ADM/0036: o GT está ~2 frames atrasado, use 86** (ver armadilha 13) |
| `--save_particles --export_particles N` | off | Grava `particles_log.csv` para o vídeo |
| `--inject_ratio r` | 0 | Re-sorteia r·N partículas por frame (relocalização). Recomendado 0.05–0.10 |
| `--two_phase` | off | Priors, injeção e `--vo_scale_deadband` só na fase GLOBAL; depois de convergir (`--converge_std 100`, `--converge_frames 5`, `--converge_contrast 1.3`) passa a TRACKING no modo do artigo (sem priors, sem injeção, deadband `--track_vo_scale_deadband 0.01`). `--relocalize_std px` (0 = nunca) volta à GLOBAL. Log por frame em `phase_log.txt`. Usado no regime `final_baro` (piloto de 15/10 e teste do artigo de 17/10) |
| `--inject_spread px` | 0 | Escala a injeção por σ_xy/px: nuvem concentrada ⇒ quase sem injeção. Recomendado 300 (com altímetro) |
| `--scale_prior_sigma s` / `--scale_prior_gain g` | 0 / 1.0 | Prior de altímetro emulado: z ~ 𝒩(g·z_GT, s·g·z_GT). ADM/0036: g = 0.87, s = 0.10 |
| `--altimeter_file f` | — | Altímetro real (`altimeter_baro.txt`): o prior de escala passa a ser centrado em `g·z_baro`, sem ler o GT. Use `--scale_prior_gain 1` |
| `--traj f` / `--gt f` | — | Outro `traj.txt` (ADM1: `traj_comp.txt`) / outro GT (ADM3: GT do Zenodo) |
| `--yaw_prior_sigma deg` | 0 | Prior de bússola emulado (diagnóstico) |
| `--inject_gate c` | 0 | Só injeta quando o contraste do frame C ≥ c (frame informativo). Diagnóstico; não resolveu o fim do ADM2 |
| `--estimator mean\|mode` | mean | `mode` = cluster dominante. No ADM2 piorou (salta entre clusters); mantenha `mean` |
| `--motion_noise f` | 0 | Ruído extra ±f·\|passo\| em x,y. Efeito pequeno no ADM2 |
| `--vo_scale_deadband e` | 0.01 | Zona morta do Δz da VO (legado 0.01). `1` ignora a escala da VO (recomendado) |

Saídas em `--output_dir`: `traj_estimated.txt` (formato do legado: `meanPX meanPY meanPZ varPX varPY varPZ meanAngle angleStdev NEFF elapsedTime`), `errors.txt` (`frame meanPX meanPY meanPZ trueX trueY trueZ error_pos_m error_yaw_deg`) e, opcionalmente, `particles_log.csv`.

## 4. Avaliação e vídeo

```bash
python3 scripts/evaluate_results.py --results_dir ./results_adm0036 --scale 0.188679245 \
        --plot --output_plot ./results_adm0036/evaluation.png
python3 scripts/render_video.py --results_dir ./results_adm0036 \
        --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
        --output ./results_adm0036/video.mp4 --fps 6
```
- O `evaluate_results.py` usa o erro em metros **já gravado** em `errors.txt`, que foi calculado com o `--pixel_to_meter` da execução. O `--scale` do script só aparece no relatório impresso. Se a escala estiver errada, execute de novo o `lambda_brief_loc`.
- O `render_video.py` **não** aceita `--dataset_dir`. Opções: `--results_dir --map --output --fps --video_width --video_height --zoom_margin`.
- Para comparar com o legado, use a média de várias sementes (`--seed 1..30`), como no artigo (γ = 30).

## 5. Convenções de coordenadas (não quebrar)

- **x, y**: pixels do mapa de satélite (origem no canto superior esquerdo, y para baixo). Mapas com 4800×4800 px.
- **z**: fator de escala *mapa/câmera* (px do mapa por px do frame), **não** é altitude em metros. GT do ADM2 ≈ 0.64 no início, caindo para 0.24 no fim (a altitude desce de 103 m para 39 m; no ADM/0036, z_GT = 0.00616 × altitude). UFRGS ≈ 0.81. **O z_GT do ADM/0036 está ~15% acima da escala física**: o pinhole com o HFOV da Sequoia RGB (63,9°) e frames de 1241 px dá z = 0.00536 × h = 0.87·z_GT, e é aí que a distância de Hamming é mínima e que o legado se estabiliza. Use `--scale_prior_gain 0.87` ao emular o altímetro. O `traj_truth.txt` da UFRGS tem z = escala, mas o `UFRGS_VET_100M_GT_390_613.txt` do legado tem altitude em metros na coluna 3.
- **yaw**: radianos em [-π, π] internamente. O GT vem em **graus** (coluna 4). A projeção usa θ = −yaw − π/2: um par (u, v) do frame vai para `m = R(θ)·((u,v) − centro)·z + (x,y)`. O movimento aplica `Δx,Δy` (px do frame) × z, girados por yaw. A convenção de yaw do GT **confere** com a das partículas (verificado: o mínimo de Hamming fica a ±10° do yaw do GT).
- **Metros por pixel**: ADM = **0.188679245**. UFRGS VET (mapas V3) = **0.10** (script de avaliação do artigo: `ComputeResults_UFRGS_VET_corrected(... de acordo com o artigo).py`, "100 px = 10 m"). O valor 0.053 citado antes está errado.
- **Pareamento do Ground Truth**: a linha k do `traj_truth.txt` corresponde à linha `k + offset` do `traj.txt`, com offset = `iniexec − inids` do legado. ADM/0036: 199−115 = **84** (196 linhas de GT). ADM/0035: 103−85 = **18**. UFRGS voo_100m: 390−186 = **204** (224 linhas). Os valores de 0035 e 0040 não foram revalidados. Confirme com `--gt_offset`.
- Fora da janela `[offset, offset + N_gt)` não há GT. A decolagem (ADM/0036, linhas 0–83) não tem GT e tem forte variação de altitude. **Localização global deve começar em `--start_frame = offset`**.
- A odometria gravada (`traj/odom-*.txt`) tem N−1 linhas para N frames da fatia. A linha k é a transição k → k+1.

## 6. Armadilhas conhecidas

1. **Filtro sem memória** (corrigido): pesos precisam ser **multiplicados** (`w ← w·p(z|x)`) e a reamostragem deve rodar todo frame (legado). Se alguém voltar a sobrescrever o peso com Neff-trigger 0.5, o filtro volta a não convergir: Neff fica travado em cerca de 0.55–0.6·N porque as partículas fora do mapa têm peso ≈ 0.
2. **Modos espúrios de escala pequena + nenhuma hipótese nova**: com z ≈ 0.1–0.3 a pegada dos pares no mapa é pequena, os bits ficam correlacionados e a cauda inferior de d é mais baixa. Partículas aleatórias com z pequeno às vezes batem a pose verdadeira. Com 50k partículas uniformes, só ~0.3 cai na bacia do GT. Sem `--inject_ratio`, uma nuvem que convergiu no lugar errado nunca se recupera (foi o caso da execução seed 1 do voo completo: 272 m). Um prior de altímetro **sozinho** deixa o 1º frame tão concentrado que o Neff cai para ~236/50k. Use prior **com** injeção.
3. O λ-BRIEF "LAB_NO_L" compara os canais **0 e 1** do `cv::COLOR_BGR2Lab`, ou seja, **L\* e a\***, e não a\* e b\* como diz o artigo. Mantido por fidelidade ao legado.
4. Os logs do legado (`Mcl::writeLogFile3d`) têm `varPX/varPY/varPZ` **errados** (usam `=` em vez de `+=`, então só entra o termo da última partícula). Não compare variâncias com o legado, só posições.
5. O legado carrega os pesos antigos através da reamostragem, então a média logada é quase a da partícula mais pesada. A engine moderna zera os pesos (1/N) após reamostrar. As médias só coincidem depois da convergência.
6. O parser da CLI agora avisa sobre opções desconhecidas. Opções do legado como `-alfagaussmsk` **não** existem aqui (use `--alpha`). O `main.cpp` do `phir2frameworkv2` versionado também **não** reconhece `-alfagaussmsk` (só `-alfavismsk`, com padrão 0 = só máscara gaussiana). Os scripts do artigo passam `-alfagaussmsk`, então o binário usado no artigo provavelmente era de outra revisão.
7. **As "30 repetições" do legado não são independentes.** Quando o legado roda várias execuções no mesmo processo (`-o dir start finish`), ele chama `MCL::restart()`. Essa função sorteia a escala em `particles[i].s`, um campo que não é usado, **em vez de `p.z`**. Com isso, cada execução começa com o z final da anterior (no ADM2 do artigo, z₀ ≈ 0.43 em vez de ≈ 1.0; no ADM1 a cadeia chega a explodir: 0,97 → 1,05 → … → 27 → 128 → 10¹⁵). Além disso, `restart()` usa `std::default_random_engine` com semente padrão, então as nuvens iniciais de x, y e yaw são idênticas. Na comparação com o legado, use a **primeira** execução de cada lote como a verdadeira localização global. Para reproduzir as demais, passe um prior de escala (`--min_scale/--max_scale` estreitos).
8. As imagens do dataset devem estar registradas (RGB redimensionado para o tamanho das bandas MS, 1241×913 no ADM). `MaskGenerator::combineMasks` faz assert de tamanhos iguais.
9. **A escala da VO (Δz da homografia) não mede a altitude** no ADM/0036: a correlação com a razão de altitude do GT é ≈ 0, e o produto no voo dá ×2,2 contra ×0,37 real. Use `--vo_scale_deadband 1`. Confirmado em 12/10/2026 com milhares de inliers, keyframes de 5 frames e nas imagens originais (`Originais/`).
10. **Trechos fracos do ADM/0036** (frames 74–78 e 94–105 da fatia, com curva de 180° e salto de VO de ~26 m nos frames 73–74): d(GT) fica quase igual ao de poses aleatórias. Nas execuções corrigidas a nuvem perde o rastro ali e relocaliza depois (~frame 119–149). O legado sobe para ~54 m e se recupera no frame 112.
11. O console mostra `Neff(pre)` = Neff **antes** da reamostragem (o antigo `Neff: 50000` era sempre N, sem informação) e, com injeção, `Inj` e o contraste `C` do frame. `LB_DEBUG_PF=1` imprime o mesmo por frame.
12. **O fim do voo ADM/0036 (frames ~175–195) continua falhando** mesmo com prior de bússola, gate e estimador `mode`: a VO subestima o deslocamento em ~27% (pouso, curva) e a observação é fraca. As poses que melhor casam com o mapa (d ≈ 0,31) ficam ~15 m do GPS, enquanto d(GT) ≈ 0,50. Existem `traj_truth_corr.txt` (GT corrigido pela inclinação da câmera, até 23 m de diferença) e `traj_angles.txt` (roll, pitch, yaw) no dataset; o arquivo `referência nas imagens` do autor já avisa que as inclinações bruscas deixam "o GT um pouco diferente das partículas". Próximo passo natural: compensar roll/pitch na projeção (legado `-corrpxpos`).

## 7. Diagnóstico rápido

```bash
# Discriminabilidade do modelo de observação por frame: d(GT) vs d(poses aleatórias)
./build/diag_likelihood --map <mapa> --dataset_dir <ds> --start_frame 84 --end_frame 103
# Traço do filtro (partículas perto do GT, Neff antes da reamostragem, melhor partícula)
LB_DEBUG_PF=1 ./build/lambda_brief_loc ... 2>&1 | grep '^PF'
LB_DEBUG_ODOM=1 ./build/lambda_brief_loc ... 2>&1 | grep '^ODOM'   # comparar com traj/odom-*.txt
```
Colunas extras do `diag_likelihood`: `best_zfac` (fator de z_GT que minimiza d; média geométrica ≈ 0.87 no ADM/0036) e `d_best_z`. `LB_DIAG_NRAND=200` acelera. Valores esperados no ADM/0036: d(GT) ≈ 0.37–0.40 no início do voo (0.45 em média no voo inteiro). Poses aleatórias dentro do mapa ficam em ≈ 0.51–0.55, e o GT fica abaixo do percentil 0.1%. A VO online bate com `bkp odometria/SIFT/FLANN` com diferença abaixo de 1 px em 18 de 19 transições.

## 8. Filtro em duas fases (motivação)

A análise `docs/analise_fim_do_voo_adm2.md` (29 execuções) mostrou que o prior de altímetro, a injeção de partículas e `--vo_scale_deadband 1` aceleram a **convergência global**, mas fazem o filtro **perder o rastro no pouso** (10/10 execuções). A configuração do artigo, depois de convergir, rastreia a ~15 m até o fim. `--two_phase` combina os dois: `include/localization/phase_controller.hpp` decide a fase a partir da dispersão da nuvem e do contraste C (que exige `--inject_ratio > 0`; na fase TRACKING os candidatos continuam sendo avaliados para medir C, mas não entram na nuvem).
13. **O `traj_truth.txt` do ADM/0036 está ~2 frames atrasado em relação às imagens** (12/10/2026, `../docs/revisao_odometria.md`): a imagem t corresponde à linha t−2 do GT. A correlação do Δyaw da VO com o da bússola sobe de 0,44 para 0,74 e a da velocidade com o GPS de 0,55 para 0,66 com esse deslocamento. A deriva de 10 frames da VO cai de 8,9 para 5,4 m. Avaliar com `--gt_offset 86` reduz a mediana de rastreio de ~15 para ~11 m. Os priors emulados (`--scale_prior_*`, `--yaw_prior_sigma`) também leem o GT por esse offset. `scripts/check_gt_alignment.py` mede o atraso a partir de um log `LB_DEBUG_ODOM` e reavalia `errors.txt` antigos. Confirme pelos timestamps antes de mudar o padrão.
14. **O z do GT no fim do ADM/0036 (frames ~160–195) não bate com as imagens**: o GT diz que z cai pela metade (×0,515), mas a VO mede ×1,0–1,23 com quatro métodos, e a translação da VO só fecha com o GPS se z ficar ~constante. Isso explica a armadilha 12 com prior de altímetro (o prior emulado empurra z para baixo no pouso). A "VO subestima o deslocamento em ~27%" da armadilha 12 vinha do GT atrasado e do z_GT errado, e não da VO.
15. **Odometria visual (12/10/2026)**: `VisualOdometry::track()` guarda as features do frame anterior (antes o SIFT do frame anterior era recalculado a cada frame). Com `--vo_matcher legacy` a trajetória é **idêntica** à do código anterior. O ruído de predição padrão U(±10 px) tem σ = 5,8 px, mas o erro real da VO por passo tem σ ≈ 10 px: `--pos_noise 17` (mesmo σ) só ajuda junto com o matcher novo (`experimentos/2026-10-12_odometria_vo_variantes/`).
16. **O abBRIEF do legado não é o λ-BRIEF com α = 0.** No `phir2frameworkv2`, o abBRIEF (`-msc 0`) passa por `BriefHeuristic::updateDroneDescriptor`: `cv::RNG rng;` com semente padrão recriado a cada chamada (os mesmos 256 pares em todo frame) e `rng.gaussian(W²/25)`, que usa a variância como desvio-padrão (pares quase uniformes). O λ (`-msc 1 -pdm 51`) usa a gaussiana central + NDVI, re-sorteada a cada frame. Na engine moderna, abBRIEF = `--alpha 0` no mesmo caminho do λ. Detalhes e linhas em `lambda_brief_modern/docs/achados_codigo_legado.md`.
17. **ADM1 e ADM3 no teste do artigo**: o ADM1 precisa de `--traj Datasets/ADM/0035/traj_comp.txt` (o `traj.txt` original aponta para .png inexistentes). O ADM3 usa os mapas e o GT do Zenodo (`--gt .../ADM3_ground_truth.txt`) e o altímetro de `scripts/adm3_altimeter_from_kml.py`. Os scripts do artigo já fazem isso.

## 9. Experimentos e testes do artigo

- Toda rodada vai para `lambda_brief_modern/experimentos/<AAAA-MM-DD>_<nome>/` e ganha uma linha em `experimentos/INDICE.md`. Os scripts fazem isso sozinhos sem `--out`/`--output_base_dir`. Para uma pasta feita à mão: `python3 scripts/experimentos.py nova <nome> "<descrição>"`.
- Depois de cada rodada, atualize o `lambda_brief_modern/HISTORICO_RESULTADOS.md` (regra do `AGENTS.md`) e o resumo em `experimentos/INDICE.md`.
- O teste do artigo (Figs. 4 e 5) está em `lambda_brief_modern/docs/MANUAL_TESTES_ARTIGO.md`:
  ```bash
  python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1..5 --dry_run
  ```
- Regime recomendado: `final_baro` (duas fases + injeção + barômetro real + odometria fixa em `insumos/odom/`). O protocolo do artigo é o `vanilla`.
- A engine é determinística para a mesma seed. Para a mesma seed, abBRIEF e λ-BRIEF começam da mesma nuvem (comparação pareada). Execuções com comando idêntico podem ser copiadas entre pastas (`--reaproveitar`).
- Resultado de 17/10/2026 (5 seeds, final_baro): todos convergem; o λ ganha no ADM1 e no UFRGS, empata ou ganha por pouco no ADM2 e perde no ADM3. O ganho do λ está na convergência da busca global, não no erro de rastreio.
- **v3 (branch `v3`):** `--stability_map <png>` pondera cada par por `min(W(A), W(B))` nos pontos projetados no mapa (raster do Sentinel-2 de `scripts/make_stability_map.py`, em `insumos/estabilidade/`). O peso entra só na verossimilhança (`Descriptor::evaluateWeightedParticleDifference`), nunca no sorteio dos pares. Sem a opção, a engine é idêntica à v2; com w = 1 em tudo, a distância também (bateria 23). O raster tem de ter o tamanho do mapa; os 5 mapas ADM_35 (e os do Zenodo do ADM3) usam o mesmo raster A. Teste: `scripts/v3_comparison.py` (manual, seção 10).

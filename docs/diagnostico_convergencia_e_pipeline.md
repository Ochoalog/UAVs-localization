# λ-BRIEF Modern: diagnóstico da não-convergência e funcionamento do pipeline

*Análise de 24/09/2026 sobre `lambda_brief_modern/` (C++17) com `phir2frameworkv2/` como referência.*

---

## 0. Resumo executivo

| | |
|---|---|
| **Sintoma** | Erro médio de cerca de 300 m. `varPX` e `varPY` ficam em torno de 1,2·10⁶ px² (a distribuição uniforme num mapa de 4800 px dá 1,92·10⁶). A estimativa fica perto do centro do mapa |
| **Causa raiz (P0)** | `ParticleFilter::updateWeights` **sobrescrevia** o peso com a verossimilhança do frame (`w = p(z\|x)`) e `resampleIfNeeded` só reamostrava com `Neff < 0,5·N`. O Neff de um único frame fica em **0,51–0,63·N**, então o filtro reamostrou **apenas no frame 0** e nunca acumulou evidência. O legado reamostra **todo frame** |
| **Causa secundária (P1)** | Máximos espúrios de verossimilhança em **escala pequena** (z ≈ 0,1–0,3). A pegada dos pares no mapa é pequena, os bits ficam correlacionados e a cauda inferior da distância de Hamming desce abaixo de d(GT). Diferenças de fidelidade que agravam isso: sem quantização de cor em 25 níveis e odometria "não confiável" aplicada assim mesmo |
| **Não são a causa** | A odometria visual online (< 1 px da gravada em 18 de 19 transições), o modelo de observação (d(GT) ≈ 0,38 contra ≈ 0,61 aleatório, percentil < 0,03%), a convenção de yaw e os offsets de GT (84 e 204) |
| **Resultado após as correções** | Com 40 frames reais do ADM2 e 50k partículas, **4 de 6** sementes convergem com VO online e **5 de 6** com filtro bilateral nas bandas MS (config. do legado). O código original converge em **0 de 4**. Depois de convergir, o erro nos frames 29, 34 e 39 é **16, 23 e 22 m**. O legado dá 18 e 23 m nos mesmos pontos: a trajetória do artigo é reproduzida |
| **Expectativa realista** | O legado **não** é submétrico. ADM2: ~14 m de erro médio após a convergência (5–55 m ao longo do voo). UFRGS: 9–13 m nos últimos 100 frames. A meta de erro "métrico" do artigo equivale a **~10–25 m** nesses datasets |
| **Voo completo (atualização)** | A execução `--seed 1` de 196 frames deu 272 m porque nenhuma partícula nasceu na bacia do GT (~0,3 esperada com 50k) e o filtro não tem como criar hipóteses novas. A nuvem convergiu em falsos-positivos de escala pequena (z 0,12–0,2 contra 0,24–0,64) e ficou presa neles. Agravantes: escala da VO sem correlação com a altitude (×2,2 contra ×0,37 real) e z do `traj_truth.txt` 15% acima da escala física. Correções opcionais: injeção de partículas, prior de altímetro (ganho 0,87) e `--vo_scale_deadband 1`. Com as três, a seed 1 converge no frame 24 e o erro médio cai de 272 m para 78 m (mediana de 8 m nos frames 110–195). Detalhes na seção 2.7 |
| **Fim do voo (atualização 2)** | Gate de injeção, estimador `mode`, prior de bússola e ruído proporcional **não** resolveram os frames 175–195 (seção 2.8). A bússola acelera a convergência (frame 12–15). A causa provável é a inclinação da câmera no pouso somada a uma VO que subestima o deslocamento em ~27%. O dataset tem `traj_angles.txt` e `traj_truth_corr.txt`, ainda não usados |
| **Descoberta extra** | As 30 repetições do legado não são independentes. `MCL::restart()` não reinicializa z (escreve em `particles[i].s`) e usa semente fixa. As execuções 2..30 de cada lote começam com a escala já convergida |

---

## 1. Como a análise foi feita

1. Leitura completa da engine moderna (`apps/`, `src/`, `include/`, `tests/`, `scripts/`) e dos trechos relevantes do legado: `Mcl.cpp` (run, sampling, weighting, resampling, writeLogFile3d, restart), `BriefHeuristic.cpp` (pixelDistribution pdm 51, CalculateVIMask, calculateValue2), `DroneRobot.cpp` (construtor, run, quantization, findOdometryUsingFeaturesSift, reinitialize) e `main.cpp` (parser).
2. Compilação da engine moderna em Linux (GCC 13, OpenCV 4.6), com os **40 primeiros frames reais do ADM2** (imagens 0199–0238, 5 bandas) e o mapa `ADM_35_12_09_2018.jpg`. O código original reproduziu **exatamente** o seu `results_voo_completo_02`: 173,672 m no frame 0.
3. Instrumentação do filtro (`LB_DEBUG_PF`, `LB_DEBUG_ODOM`) e ferramenta de diagnóstico do modelo de observação (`diag_likelihood`).
4. Uso dos logs do próprio legado como baseline, com a métrica do artigo:
   - `Experimentos/ADM_036_30x_bateria_testes_paper_alfaGaussMask_bilateralFilter/ADM_35_12_09_2018/.../alfagaussmsk_0.5.../*.txt` (30 execuções);
   - `UFRGS_VET_390_613_50K_apenas_beta_4_mapas/*/traj/abBRIEF__betaweight_0/0.txt` (4 mapas).
5. Varredura de sementes com as correções aplicadas uma a uma.

---

## 2. Diagnóstico

### 2.1 P0 — filtro sem memória (weights overwrite + gatilho de Neff)

**Código original** (`particle_filter.cpp`):

```cpp
double prob = norm_factor * std::exp(-(diff * diff) * inv_two_var);
particles_[i].weight = prob;                 // <- sobrescreve: esquece w_{t-1}
...
if (neff >= N * resample_neff_ratio) return false;   // ratio = 0.5
```

**Formulação correta** (filtro de Bayes recursivo / SIR):

$$ w_t^{[i]} \propto w_{t-1}^{[i]}\; p\big(z_t \mid x_t^{[i]}\big), \qquad \sum_i w_t^{[i]} = 1 $$

Com a sobrescrita, a posterior em t depende **só do frame t**. A reamostragem é o único mecanismo de memória, e ela quase nunca dispara:

- partículas cujo centro ou cujos pares caem fora do mapa recebem d → 1, ou seja, $p \approx e^{-1/0{,}045} \approx 2\cdot10^{-10}$ (peso nulo na prática);
- partículas dentro do mapa têm d ≈ 0,51–0,55 com dispersão pequena, então os pesos entre elas são quase iguais;
- logo $N_{\text{eff}} = 1/\sum w_i^2 \approx$ (fração de partículas "dentro") $\approx 0{,}55$–$0{,}63\,N$, **acima** do limiar de 0,5·N.

**Evidência (código original, ADM2, 20k partículas, seed 42):**

| frame | erro (m) | varPX | varPY | Neff |
|---|---|---|---|---|
| 0 | 173,7 | 1,16·10⁶ | 1,14·10⁶ | 20000 (reamostrou) |
| 1 | 171,9 | 1,18·10⁶ | 1,13·10⁶ | 11930 |
| 5 | 190,6 | 1,17·10⁶ | 1,09·10⁶ | 12334 |
| 10 | 208,8 | 1,16·10⁶ | 1,10·10⁶ | 12014 |
| 19 | 216,0 | 1,17·10⁶ | 1,11·10⁶ | 11034 |

O Neff nunca cai abaixo de 10000 = 0,5·N, então a nuvem permanece uniforme. A estimativa é uma média ponderada de uma nuvem uniforme, isto é, o centro do mapa. **Prova numérica:** a distância do GT do frame 0, (3316, 2539), ao centro (2400, 2400) é 926 px × 0,1887 = **174,8 m**, igual ao erro observado de 173,7 m. Os ~300 m médios vêm dessa mesma estimativa central, que deriva quando a trajetória se afasta do centro e com reamostragens esporádicas num modo qualquer.

**Correção aplicada:**

- atualização multiplicativa em domínio log (evita underflow com 512 bits e muitos frames);
- `resample_neff_ratio = 1.0` por padrão: reamostra todo frame, como `MCL::run` do legado;
- centro fora do mapa recebe peso 0 (legado `Mcl::weighting`);
- se todos os pesos zerarem, volta para pesos uniformes (legado "Correct zero error").

Teste de regressão: `testBayesianWeightAccumulation` verifica $w_0 = 0{,}1\cdot 1/(0{,}1 + 0{,}9\,\ell_1)$.

### 2.2 P1 — máximos espúrios de escala pequena

Só o P0 não basta. Com a reamostragem correta a nuvem concentra, mas frequentemente em **z ≈ 0,1–0,35**, longe do GT (z ≈ 0,64). Traço do filtro (`LB_DEBUG_PF`, 50k partículas):

```
PF 0  near_xy40=13   d_gt=0.396 d_best=0.352 best=(2787,2250, z=0.23, -56°)
PF 7  near_xy40=323  d_gt=0.365 d_best=0.344 best=( 758,1604, z=0.13, -58°)
PF 18 near_xy40=0    d_gt=0.383 d_best=0.365 best=( 792,1572, z=0.17, -61°)
```

A vizinhança do GT chegou a acumular 323 partículas, mas nenhuma tinha yaw e escala certos ao mesmo tempo. Elas foram drenadas por partículas de z pequeno com d menor que o do GT.

**Por quê?** Distribuição de d em poses aleatórias (dentro do mapa), por faixa de escala, no frame 0:

| z | média | desvio | quantil 0,1% | mínimo em 60k |
|---|---|---|---|---|
| 0,1 | 0,509 | 0,059 | **0,385** | **0,354** |
| 0,3 | 0,526 | 0,082 | 0,402 | 0,371 |
| 0,6 | 0,552 | 0,106 | 0,406 | 0,375 |
| 1,0 | 0,585 | 0,124 | 0,408 | 0,367 |
| 2,0 | 0,660 | 0,139 | 0,424 | 0,383 |

Com z pequeno, os 256 pares projetados ocupam uma região de ~124 px no mapa. Os bits ficam fortemente correlacionados (menos graus de liberdade efetivos), o que desloca a cauda inferior para ≤ d(GT) ≈ 0,38. Com 50 000 "bilhetes" por frame, sempre aparece um sorteado. O **perfil da bacia** no GT mostra por que a região certa é estreita:

| desvio | 0 | 5 px | 20 px | 40 px | yaw +10° | yaw +40° | z 0,5 | z 0,7 |
|---|---|---|---|---|---|---|---|---|
| d | 0,385 | 0,414 | 0,420 | 0,475 | 0,340 | 0,430 | 0,439 | 0,436 |

A bacia em z tem ±0,05. A fração do espaço de estados (x, y, yaw, z) "dentro da bacia" é da ordem de $4\cdot10^{-7}$, ou 0,02 partícula esperada com 50k. A convergência depende de um "arrasto" gradual, que é **estocástico**.

**Mitigações validadas:**

| Configuração (50k partículas, 40 frames) | Convergiu (< 30 m no frame 39) | Frame de convergência |
|---|---|---|
| Código original | **0 / 4** | — |
| P0 + quantização 25 + VO online | 4 / 6 | 19–39 |
| + filtro bilateral nas bandas MS (legado `-msimp 2 -bfd 6 -bfsc 5 -bfss 5`) | **5 / 6** | 19–29 |
| + odometria gravada (`traj/odom-*sift_bruteforce.txt`) | 3 / 5 | 14–29 |
| + prior de escala z ∈ [0,4; 1,0] (≈ altitude 57–141 m) | 5 / 6 | **14–24** (1 no frame 39) |

Em todas as execuções que convergiram até o frame 24, o erro nos frames 29, 34 e 39 é **15–17, 23–25 e 22–24 m**. As 30 execuções do legado dão 18,3 m no frame 30 e 23,0 m no frame 40.

### 2.3 Diferenças de fidelidade com o legado

| # | Item | Legado | Moderno (antes) | Status |
|---|---|---|---|---|
| 1 | Atualização de peso | `w = p` + **reamostra todo frame** (SIR) | `w = p` + reamostra se Neff < 0,5N | **Corrigido** (multiplicativo, log, todo frame) |
| 2 | Centro fora do mapa | peso 0 | d = 1 (peso ~10⁻¹⁰) | **Corrigido** |
| 3 | Quantização de cor (`-mappp 1`) | 25 níveis no mapa **e** em cada frame, antes do Lab e da VO | ausente | **Corrigido** (`--quantize 25`, bit-exato: 255 → 254) |
| 4 | Odometria não confiável | `u = Pose3d()` (0, 0, z = 1, 0) + ruído de yaw de 30° | aplicava o Δ da homografia reprovada | **Corrigido** (`--apply_unreliable_odom` restaura o antigo) |
| 5 | Features SIFT | ilimitadas | 2000 | **Corrigido** (`--sift_features 0`) |
| 6 | Faixa de z | `-minsc/-maxsc` por dataset (ADM 0,1–3; UFRGS 0,5–5) | fixo 0,1–3 | **Exposto** na CLI |
| 7 | Pareamento de GT | offset = `iniexec − inids` | offset correto, **mas** a regra "tamanho igual" pareava GT[0] com o 1º frame da fatia | **Corrigido** (sempre usa offset) |
| 8 | Odometria gravada | lida de `traj/odom-*.txt` quando existe | não suportada | **Adicionado** `--odom_file` |
| 9 | Bits de par fora do mapa | empilha 3 valores para 2 bits (desalinha o resto do descritor) | conta 2 divergências | Mantido (o do legado é bug) |
| 10 | Variância logada | `varPX = ...` (sem `+=`), inválida | correta | Não comparar variâncias |
| 11 | Estimativa logada | média com pesos "carregados" pela reamostragem (≈ melhor partícula) | média com w = 1/N | Iguais após convergir |
| 12 | Canais do "Lab sem L" | canais 0 e 1 = **L\*, a\*** | idem | Mantido (fidelidade). O artigo diz a\*, b\* |
| 13 | Parser da CLI | ignora opções desconhecidas | ignorava em silêncio | Agora avisa |

### 2.4 Pareamento temporal imagens × Ground Truth

| Dataset | `traj.txt` | inids / iniexec / fiexec | GT (linhas úteis) | Offset | Fatia |
|---|---|---|---|---|---|
| ADM2 (0036) | 342 linhas (0115–0456) | 115 / 199 / 394 | 196 | **84** | `--start_frame 84 --end_frame 279` |
| UFRGS voo_100m | 534 linhas (0186–0719) | 186 / 390 / 613 | 224 | **204** | `--start_frame 204 --end_frame 427` |

- Os arquivos de avaliação do artigo (`GT2.txt`, `UFRGS_VET_100M_GT_390_613.txt`) pareiam a linha t do log com a linha t do GT. A linha 0 do log corresponde à imagem `iniexec`. **Os offsets 84 e 204 do `DatasetLoader` estão corretos.**
- O `traj_truth.txt` tem linhas em branco. `operator>>` as ignora, então não há deslocamento.
- No legado, o construtor de `DroneRobot` consome a linha 0 do GT antes do primeiro `run()`, então o `realPose` interno fica uma linha adiantado. Isso só afeta a visualização e não entra na avaliação.
- **Risco eliminado:** o "Case 1" do loader (`all_samples.size() == frames_.size()`) pareava GT[i] com o i-ésimo frame da fatia. Em `--start_frame 0 --end_frame 195` no ADM/0036, a imagem de decolagem 0115 recebia o GPS da imagem 0199, um deslocamento de 84 frames e centenas de metros.
- A decolagem (linhas 0–83 do ADM/0036) não tem GT e tem variação forte de altitude. Localização global deve começar no offset.

### 2.5 Odometria online × odometria gravada (benchmark original)

VO online da engine moderna × `bkp odometria/SIFT/FLANN/odom-190723-134129.txt`, transições 1–19 do ADM2 (Δx, Δy em px do frame):

| t | online (Δx, Δy, Δyaw) | gravada SIFT/FLANN | gravada SURF v3 |
|---|---|---|---|
| 1 | 33,23 2,69 −0,0154 | 33,13 2,89 −0,0178 | 32,78 2,86 −0,0174 |
| 5 | 38,45 −5,48 0,0028 | 37,93 −5,18 0,0032 | 39,32 −6,49 0,0097 |
| 10 | 96,41 2,18 −0,0012 | 96,07 1,92 −0,0013 | 96,65 1,69 −0,0025 |
| **13** | **20,12 33,15** −0,0126 | **7,64 10,35** −0,0041 | 12,31 14,19 −0,0070 |
| 16 | −5,61 3,70 −0,0459 | −5,70 3,72 −0,0460 | −5,42 3,72 −0,0479 |
| 19 | 2,72 0,47 −0,0456 | 2,50 0,85 −0,0455 | 2,56 0,84 −0,0458 |

Concordância abaixo de 1 px e 0,003 rad em 18 de 19 transições. A única discrepância (t = 13, cerca de 25 px de frame, ≈ 16 px de mapa) está na ordem do ruído de predição (±10 px). **A VO não explica a divergência**, e usar a odometria gravada não muda o resultado de forma significativa (tabela 2.2). Os arquivos de odometria gravada têm N − 1 linhas; a linha k é a transição k → k+1 da fatia.

### 2.6 Qual é o "erro do paper"

Métrica do artigo: erro médio sobre γ = 30 execuções, com escala do mapa ADM 0,188679245 m/px e UFRGS V3 **0,10 m/px**. A partir dos logs do legado:

| Voo | Convergência | Erro após convergir |
|---|---|---|
| ADM2, mapa 12/09/2018, α = 0,5, 50k | frames 10–21 (30/30 execuções) | média 14,3 m (frames 100–195). Picos de 44–54 m nos frames 60 e 100. 6,4 m no frame 150 |
| UFRGS V3 (4 mapas), 50k | frames 0–75 | 8,7–13,1 m (média dos últimos 100 frames) |

**Ressalva importante:** em cada lote de 30, só a **1ª** execução é localização global de fato. `DroneRobot::reinitialize → MCL::restart` re-sorteia x, y e yaw com `std::default_random_engine` de semente padrão (nuvens idênticas) e grava a escala em `particles[i].s`, que não é usado. As execuções 2..30 herdam z do fim da execução anterior: z₀ = 0,42–0,44 contra 1,00 na primeira. A primeira execução do lote ADM2 convergiu no frame 10. As demais são equivalentes à configuração "prior de escala" da tabela 2.2.

**Conclusão:** "submétrico" não é uma meta alcançável com este método e estes mapas (resolução de 0,19 m/px, mapas de outras datas). A meta realista é **reproduzir ~10–25 m** após a convergência, o que a engine corrigida já faz.

### 2.7 Voo completo (196 frames): por que a execução `--seed 1` deu 272 m

Execução analisada: `--start_frame 84 --end_frame 279 --num_particles 50000 --alpha 0.5 --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 --seed 1`. Resultado: erro médio 272 m, RMSE 288 m, final 364 m, **nenhum frame abaixo de 30 m**.

| frame | 0 | 20 | 40 | 60 | 100 | 140 | 195 |
|---|---|---|---|---|---|---|---|
| erro (m) | 173 | 306 | 402 | 340 | 151 | 247 | 364 |
| z estimado / z do GT | 1,17 / 0,64 | 0,29 / 0,64 | 0,22 / 0,62 | 0,13 / 0,57 | 0,13 / 0,49 | 0,12 / 0,49 | 0,13 / 0,24 |

A nuvem **convergiu** (varPX e varPY de 400 a 1500 px²), mas num modo falso de escala pequena, e nunca saiu dele. Há quatro causas, em ordem de impacto.

**1. Nenhuma partícula nasce na bacia, e nada cria novas hipóteses.** A bacia da verossimilhança em torno do GT tem cerca de ±40 px, ±40° e ±0,15 em z (seção 2.2). Isso é ~6·10⁻⁶ do espaço (x, y, yaw, z), ou seja, **~0,3 partícula esperada com 50k** na inicialização uniforme. Traço `LB_DEBUG_PF` da sua configuração (seed 1):

```
PF 0  neff_pre=18819  near_xy40=19  near_full=0  d_gt=0.406 d_best=0.396 best z=0.83
PF 1  neff_pre=32020  near_xy40=24  near_full=0  d_gt=0.438 d_best=0.369 best z=0.22
PF 3  neff_pre=33668  near_xy40=11  near_full=0  d_gt=0.387 d_best=0.346 best z=0.10
```

Existem partículas perto do GT em x, y, mas nenhuma com yaw e z certos ao mesmo tempo (`near_full = 0`). A reamostragem de cada frame concentra a nuvem nos falsos-positivos de z pequeno. Depois disso, só o ruído de predição (±10 px, ±5°) explora o espaço, e **não existe mecanismo que recoloque partículas perto da pose verdadeira**. Convergir depende de algum ancestral ter caído perto da bacia, ou seja, da semente. Com seed 1 isso não acontece.

Um prior de altímetro e/ou bússola sozinho piora o 1º frame: a verossimilhança fica tão concentrada que o Neff cai para **236 de 50 000** (seed 42, priors z + yaw). A nuvem inteira passa a descender de ~236 ancestrais, nenhum na bacia. Por isso os priors só funcionam junto com a injeção de partículas (tabela abaixo). O legado tem o mesmo comportamento. Das 30 execuções dele, só 2 são inicializações independentes (seção 2.6), e as duas deram sorte.

**2. Verossimilhança quase plana em z, com viés para z pequeno.** Varredura de z na pose do GT, voo completo (`diag_likelihood`, coluna `best_zfac`):

- o mínimo de d fica em z = **0,87·z_GT** (média geométrica em 195 frames), mas varia de 0,5 a 1,5 de um frame para outro;
- em poses aleatórias, z pequeno produz d menor (seção 2.2).

Sem informação de altitude, a nuvem deriva para z = 0,12–0,3. Com z errado, o deslocamento previsto (Δx·z) também fica errado, e a nuvem não consegue seguir a trajetória verdadeira mesmo quando passa perto dela.

**O z do `traj_truth.txt` está 15% acima da escala física.** Modelo pinhole com o HFOV da câmera RGB da Sequoia (63,9°), frames RGBres de 1241 px e mapa de 0,1887 m/px:

$$ z = \frac{2\,h\,\tan(\mathrm{HFOV}/2)}{W \cdot \mathrm{GSD}_{\text{mapa}}} = 0{,}00536\,h \quad\text{contra}\quad z_{GT} = 0{,}00616\,h $$

A razão é 0,87. Ela coincide com o mínimo de d e com o z em que o legado se estabiliza (0,84–0,89·z_GT em todas as 30 execuções). Um prior de altímetro centrado em z_GT (ganho 1,0) empurra a nuvem para fora do mínimo, e só 1 de 3 sementes converge. Com ganho 0,87 ele ajuda.

**3. A escala da odometria visual é ruído com viés.** Com `LB_DEBUG_ODOM` e `--vo_scale_deadband 0`, nos 194 pares do voo:

| | valor |
|---|---|
| correlação entre log Δz(VO) e log(z_t/z_{t−1}) do GT | **−0,006** |
| produto de Δz da VO no voo, com a zona morta de ±1% do legado | **×2,22** |
| razão verdadeira z_final / z_inicial (descida de 103 m para 39 m) | **×0,37** |
| transições dentro da zona morta de ±1% | 140 de 194 |

A zona morta do legado (0,99 < Δz < 1,01 → 1) não evita o problema, porque os saltos de 3–4% que passam por ela (frames 73–80, 174) têm o sinal errado. A estimativa de escala pela homografia dos 9 pontos não mede a altitude neste voo. O correto é ignorar a escala da VO (`--vo_scale_deadband 1`) e deixar z ser corrigido pela verossimilhança ou por um altímetro.

**4. Trechos com observação fraca.** Nos frames 74–78 e 94–105, 20–45% das poses aleatórias têm d menor que o do GT (`frac_below_gt`; no resto do voo o valor fica perto de 0–5%). É um trecho com curva de 180° (frames 88–110). Os frames 73–74 também têm um salto de odometria de ~26 m acima do GPS (249 e 178 px de frame; o arquivo gravado do legado tem os mesmos valores), e nos frames 77–89 a VO mede ~60% do deslocamento do GPS. O legado sobe para 54 m nesse trecho e volta a < 15 m no frame 112. Nas execuções corrigidas a nuvem perde o rastro nos frames 85–100 e relocaliza por volta do frame 119–149.

#### Tentativas e resultados (50k partículas, filtro bilateral, 40 frames salvo indicação)

| Configuração | Converge (< 30 m) | Observação |
|---|---|---|
| Base (seeds 1, 42, 11, 16) | 0/4 | seed 1 = a sua execução |
| Prior de escala, ganho 1,0, σ 15% | 1/3 (frame 34) | centro errado (z_GT) |
| Prior de escala, ganho 0,87, σ 10% | 1/3 (frame 37) | z correto, mas o colapso do frame 0 continua |
| Calibração de d por escala (distribuição nula por faixa de z) | 0/2 | inverte o viés: a nuvem vai para z ≈ 2,4. **Removida** |
| Prior de escala + prior de yaw (bússola, σ 15°) | 2/3 (frames 23 e 38) | seed 42 não converge: sem partículas na bacia após o frame 0 |
| Prior de escala + yaw + injeção de 5% | seed 42: frame 12 | |
| Prior de escala + injeção de 5% | **2/2** (frames 9 e 28) | |
| Só injeção de 10% (sem sensores) | 2/3 (frames 20 e 26) | |
| **Voo completo**, seed 1, injeção 10%, sem sensores | converge no frame 49 | perde o rastro em 89–139, relocaliza em 149; média 139 m, final 27 m |
| **Voo completo**, seed 1, prior de escala + injeção 5% | converge no frame 28 | perde em 79–139, relocaliza em 149; média 132 m, final 66 m |
| 120 frames, seed 42, prior + injeção adaptativa (`--inject_spread 300`) + `--vo_scale_deadband 1` | frame 9 | segura até o frame 89 (22 m), perde em 94–105 |
| **Voo completo**, seed 1, **prior de altímetro + injeção adaptativa + `--vo_scale_deadband 1`** | **converge no frame 24** | perde em 89–109, relocaliza no 119. Frames 110–195: média 26,5 m, mediana **8,4 m**. Perde de novo nos últimos 5 frames (pouso, z cai 4%/frame). Média total **77,9 m** |
| **Voo completo**, seed 1, sem sensores, injeção adaptativa 10% + `--vo_scale_deadband 1` | frame 45 | z volta a colapsar (0,19–0,33) depois do frame 89. Média 202 m. **Pior** que a injeção fixa de 10% |

**Resumo no voo completo (seed 1, 196 frames):**

| Execução | Média | Mediana | Final | Frames < 30 m | 1º frame < 30 m |
|---|---|---|---|---|---|
| Sua execução (config. do artigo) | 272,3 m | 254,0 m | 364,4 m | 0 / 196 | — |
| `--inject_ratio 0.10` | 139,1 m | 91,3 m | 14,0 m | 56 / 195 | 49 |
| prior de altímetro + injeção de 5% | 132,5 m | 70,5 m | 218,1 m | 65 / 195 | 28 |
| **prior de altímetro + injeção adaptativa + deadband 1** | **77,9 m** | **26,6 m** | 154,5 m | **103 / 195** | **24** |
| sem sensores, injeção adaptativa + deadband 1 | 202,5 m | 167,4 m | 301,8 m | 13 / 195 | 45 |

Conclusão: **a informação de altitude é a alavanca principal.** Com ela e com a injeção, o filtro converge cedo e rastreia com erro de 5–25 m, na faixa do legado (seção 2.6). Continua perdendo o rastro nos trechos fracos (90–110) e no pouso. Sem altitude, a injeção ajuda a convergir, mas a escala continua derivando e o rastreio fica frágil. Lembre que as execuções 2..30 do artigo tinham, na prática, um prior de escala (seção 2.6).

**Correções implementadas (todas opcionais; os padrões continuam iguais ao legado):**

| Opção | O que faz | Por quê |
|---|---|---|
| `--inject_ratio r` | Antes de cada atualização, re-sorteia r·N partículas: x, y e yaw uniformes, z uniforme ou do prior de altímetro | Cria hipóteses novas a cada frame. Assim a bacia do GT acaba sendo amostrada e o filtro consegue relocalizar depois de perder o rastro (Augmented MCL simplificado) |
| `--inject_spread px` | Multiplica r por clamp(σ_xy da nuvem / px, 0,02, 1) | Com a nuvem concentrada (rastreando), quase não injeta. Assim, em frames fracos, uma partícula aleatória com sorte não "rouba" a massa |
| `--scale_prior_sigma s --scale_prior_gain g` | Multiplica w por 𝒩(z; g·z_GT, (s·g·z_GT)²) em cada frame (emula altímetro/barômetro) | Remove o modo falso de escala pequena e acompanha a descida de 103 m para 39 m. No ADM/0036, g = 0,87 |
| `--yaw_prior_sigma deg` | Multiplica w por 𝒩(yaw; yaw_GT, σ²) (emula bússola) | Diagnóstico. Reduz o espaço de busca em ~12×, mas sozinho não resolve o colapso do frame 0 |
| `--vo_scale_deadband e` | Zona morta do Δz da VO. `1` ignora a escala da VO | A escala da VO não se correlaciona com a altitude (tabela acima) |

Testes 14 e 15 da suíte cobrem os priors e a injeção.

### 2.8 Fim do voo (frames 175–195): tentativas e o que se aprendeu

Execução do usuário com a configuração recomendada da seção 2.7 (seed 1): média 81 m, **final 349 m**. De 120 a 174 o erro médio é 7,6 m. A perda começa por volta do frame 175 e termina com a estimativa num grupo falso.

**Tentativas (seed 1, voo completo, todas com prior de altímetro + injeção de 5% + `--vo_scale_deadband 1`):**

| Configuração | Média | Mediana | Final | 1º < 30 m |
|---|---|---|---|---|
| `--inject_spread 300` (seção 2.7, execução do usuário) | 81,1 m | 26,2 m | 348,8 m | 24 |
| `--inject_gate 0.8 --estimator mode` | 148,0 m | — | 548,4 m | 27 |
| + `--yaw_prior_sigma 15` | 93,7 m | 29,1 m | 298,7 m | **12** |
| + `--motion_noise 0.25` | 92,2 m | 29,6 m | 299,1 m | 13 |
| mesma, com `--estimator mean` | **79,0 m** | 30,1 m | 263,6 m | 15 |

**O que se aprendeu:**

1. **Gate por contraste.** O contraste C = (média nula − média da nuvem)/desvio nulo **não** distingue um modo falso de rastreio real (os dois dão C ≈ 1–1,2 no ADM2). Por isso a versão "injetar só quando perdido" deixou a nuvem presa num modo falso desde o frame 4 e foi descartada. A versão final injeta só quando C ≥ c (frame informativo). Ela evita injetar nos frames fracos, mas **o fim do voo não falha por causa da injeção**, e sim pela deriva da nuvem que está rastreando.
2. **Prior de bússola.** Acelera a convergência (frame 12–15 contra 24) e encurta a perda na curva de 180° (perde em 109–119 em vez de 85–119), mas o yaw já não é o problema no fim: com ele o erro de yaw fica < 20° e a posição continua derivando.
3. **Estimador `mode`.** Salta de cluster quando a nuvem se divide (29 m → 295 m de um frame para o outro no frame 187). Com a média, a mesma execução termina menos mal. **Mantenha `mean`.**
4. **Ruído proporcional ao deslocamento.** Efeito desprezível.
5. **Causa real do fim do voo.** No `LB_DEBUG_PF`, a melhor partícula tem d ≈ 0,31 enquanto a pose do GT tem d ≈ 0,48–0,52, e essas melhores poses ficam ~15 m do GPS. A VO subestima o deslocamento em ~27% (frames 180–189: 222 px contra 304 do GPS) com o drone descendo de 60 m para 39 m, fazendo curva e freando para pousar. O arquivo `referência nas imagens` do dataset já diz que, mesmo no trecho 199–394, "ocorrem algumas mudanças bruscas [de inclinação], as quais interferem na estimativa, deixando o GT um pouco diferente das partículas".
6. **Dados que ainda não usamos.** O dataset tem `traj_angles.txt` (roll, pitch e yaw por frame; pitch chega a −13°) e `traj_truth_corr.txt` (GT corrigido pela inclinação; até 23 m do GT original, 7,8 m em média). Contra o GT corrigido, a mediana de erro das execuções cai de ~26–30 m para ~17–19 m.

**Próximos passos sugeridos:** compensar roll/pitch na projeção dos pares (legado `-corrpxpos`, com `traj_angles.txt`); reportar também o erro contra `traj_truth_corr.txt`; e rodar 10 seeds, porque as diferenças entre as configurações acima estão dentro da variação de uma única execução.

---

## 3. Funcionamento do pipeline

Laço por frame (`apps/run_localization.cpp`):

```
frame RGB ──quantiza(25)──┬─► VO (SIFT/FLANN/RANSAC) ─► u_t, confiável? ─► predict()
                          │
NIR, RED, REG, GRE ─(bilateral)─► VI ─► −VI ─► 8 bits ─► equalizeHist ─┐
                                                   gaussiana N centrada ─┴► α-fusão ─► roleta ─► 256 pares
                          │                                                              │
                          └──► Lab ─► descritor 512 bits do drone ◄───────────────────────┘
                                         │
mapa ─quantiza(25)─► Lab ─► para cada partícula: projeta pares ─► Hamming d ─► p(z|x) ─► w·p ─► normaliza
                                                                                               │
                                                  estimativa (média ponderada) ◄─ reamostra (LVS) ◄┘
```

### 3.1 Odometria visual (`VisualOdometry::estimateRelativeMotion`, legado `findOdometryUsingFeaturesSift`)

1. **Pré-processamento:** frames quantizados (como no legado) e convertidos para cinza.
2. **Features:** SIFT (`cv::SIFT::create(0)`, sem limite) detectado e descrito no frame atual I_t (kp₁) e no anterior I_{t−1} (kp₂). ORB + BFMatcher Hamming com cross-check é a alternativa.
3. **Correspondência:** FLANN (KD-tree para vetores float de 128 dimensões), consulta I_t → I_{t−1}. Filtro de boas correspondências: $\text{dist} \le \max(2\,d_{\min}, 0{,}02)$, com $d_{\min}$ inicializado em 100 (heurística do tutorial do OpenCV mantida por fidelidade). Mínimo de 6 correspondências.
4. **Homografia:** $H = \texttt{findHomography}(p_t, p_{t-1}, \text{RANSAC})$, que leva coordenadas de I_t para I_{t−1}.
5. **9 pontos canônicos** em I_t (W × H): cantos $c_0=(0,0), c_1=(W,0), c_2=(W,H), c_3=(0,H)$; centro $c_4=(W/2,H/2)$; eixos $c_5=(W,H/2)$, $c_6=(W/2,0)$; pontos de escala $c_7=(3W/4, 3H/4)$, $c_8=(W/4, H/4)$. São projetados: $c'_k = H\,c_k$.
6. **Movimento relativo** (no referencial da câmera, em px do frame):
   - $\Delta x = -(c'_{4,y} - c_{4,y})$ (eixo "para frente" = para cima na imagem)
   - $\Delta y = c'_{4,x} - c_{4,x}$
   - $\Delta z = \lVert c'_7 - c'_8\rVert / \lVert c_7 - c_8\rVert$, arredondado para 1 se estiver em [0,99; 1,01]
   - $\Delta\psi = \operatorname{atan2}(c'_{5,y} - c'_{4,y},\; c'_{5,x} - c'_{4,x})$
7. **Filtros geométricos** (qualquer falha marca "não confiável"):
   1. deslocamento do centro: $\lVert c'_4 - c_4\rVert / (\tfrac12\lVert c_1 - c_3\rVert) \le 0{,}8$;
   2. diagonais: $\max(D_1,D_2)/\min(D_1,D_2) \le 1{,}4$ e $|\max(D)/D_{\text{orig}} - 1| \le 0{,}4$;
   3. ortogonalidade: $|\angle(c'_5 - c'_4,\; c'_6 - c'_4) - 90°| \le 25°$;
   4. lados opostos: $\max(a/c, c/a) \le 1{,}8$ e $\max(b/d, d/b) \le 1{,}8$.
8. **Modelo de movimento** (`ParticleFilter::predict`, legado `MCL::sampling`), para cada partícula (x, y, z, ψ):
   $$ \begin{aligned} x &\leftarrow x + z(\cos\psi\,\Delta x - \sin\psi\,\Delta y) + \mathcal U(-10,10)\\ y &\leftarrow y + z(\sin\psi\,\Delta x + \cos\psi\,\Delta y) + \mathcal U(-10,10)\\ \psi &\leftarrow \psi + \Delta\psi + \mathcal U(\pm 5°)\ \ (\pm 30° \text{ se não confiável})\\ z &\leftarrow z\,\Delta z + \mathcal U(\pm 0{,}02578) \end{aligned} $$
   Com odometria não confiável, $(\Delta x, \Delta y, \Delta z, \Delta\psi) = (0, 0, 1, 0)$.

### 3.2 Processamento espectral e máscara de amostragem

1. **Bandas:** Sequoia NIR (790 nm), RED (660 nm), REG/red-edge (735 nm) e GRE (550 nm). Sem distorção, registradas e com o mesmo tamanho do RGB redimensionado (1241 × 913 no ADM). Filtro opcional por banda: bilateral (d = 6, σ_c = 5, σ_s = 5) ou gaussiano.
2. **Índice por pixel** (`VegetationIndex::computePixel`), com saturação em [−1, 1] e 0 quando o denominador é nulo:
   - NDVI $= \frac{NIR - RED}{NIR + RED}$ (usado no artigo)
   - NDRE $= \frac{NIR - REG}{NIR + REG}$
   - NDVI\_red&RE $= \frac{NIR - (RED+REG)/2}{NIR + (RED+REG)/2}$
   - GRVI $= \frac{GRE - RED}{GRE + RED}$
3. **Inversão:** $\vartheta_{\text{inv}} = -\text{VI}$. Solo, estradas e construções ficam com valor alto, porque têm textura estável entre estações. Vegetação fica baixa, porque muda com a estação e confunde o mapa.
4. **8 bits:** $g = \operatorname{round}\big((\vartheta_{\text{inv}} + 1)/2 \cdot 255\big)$.
5. **Equalização** (`cv::equalizeHist`): espalha o histograma, porque em áreas agrícolas o NDVI se concentra numa faixa estreita. Normaliza para $\hat\vartheta = g_{eq}/255 \in [0,1]$.
6. **Máscara gaussiana centrada:** $\mathcal N(u,v) = \exp\!\Big(-\frac{(u-u_0)^2 + (v-v_0)^2}{2\sigma_x\sigma_y}\Big)$, com $\sigma_x = W/5$, $\sigma_y = H/5$ e normalização min-max para [0, 1]. O denominador $2\sigma_x\sigma_y$ é o do legado. Ela favorece o centro, que tem menos distorção e mais sobreposição com o mapa.
7. **Fusão linear (Eq. 1):** $F(u,v) = \alpha\,\hat\vartheta(u,v) + (1-\alpha)\,\mathcal N(u,v)$. Com α = 0 o descritor é o abBRIEF. O artigo satura em α ≥ 0,5.

### 3.3 Decisão do MCL (eliminação de partículas)

1. **Amostragem de 256 pares** (roleta por aceitação estocástica, Lipowski & Lipowska 2012): sorteia $(u,v) \sim \mathcal U$ na imagem e aceita com probabilidade $F(u,v)/F_{\max}$. Repete para os 2 pontos de cada par. É equivalente em distribuição à roleta cumulativa `pdm 51` do legado e custa em média $F_{\max}/\bar F$ tentativas por ponto. Os pares são **re-sorteados a cada frame**.
2. **Descritor do drone (512 bits):** frame quantizado → CIE Lab (OpenCV 8 bits). Para cada par $(p_1, p_2)$: $b_{2k} = [L(p_1) > L(p_2)]$ e $b_{2k+1} = [a(p_1) > a(p_2)]$. Na prática são os canais **L\* e a\*** (índices 0 e 1), mantidos por fidelidade.
3. **Projeção dos pares no mapa** para a partícula (x, y, z, ψ), com $\theta = -\psi - \pi/2$ e $(\delta u, \delta v) = z\,(u - W/2,\; v - H/2)$:
   $$ m_x = \operatorname{round}(x + \delta u\cos\theta + \delta v\sin\theta), \qquad m_y = \operatorname{round}(y - \delta u\sin\theta + \delta v\cos\theta) $$
   Os mesmos testes de bit são avaliados no mapa quantizado e convertido para Lab. Um par com qualquer ponto fora do mapa conta os 2 bits como divergentes.
4. **Distância de Hamming normalizada:** $d = \frac{1}{2K}\sum_{j=1}^{2K} b_j^{\text{drone}} \oplus b_j^{\text{mapa}} \in [0,1]$. A Eq. 4 do artigo é a similaridade $\xi = 1 - d$ (XNOR, $\eta = 1/(kc)$, c = 2).
5. **Verossimilhança gaussiana:** $p(z_t \mid x) = \frac{1}{\sqrt{2\pi\sigma^2}}\exp\!\big(-\frac{d^2}{2\sigma^2}\big)$, com $\sigma = 0{,}15$. Partícula com centro fora do mapa tem $p = 0$. Exemplo: d(GT) = 0,385 contra d ≈ 0,55 típico dentro do mapa dá razão de verossimilhança $e^{(0{,}55^2 - 0{,}385^2)/0{,}045} \approx 31$ por frame.
6. **Atualização e normalização:** $\log w_i \leftarrow \log w_i + \log p_i$, depois $w_i = e^{\log w_i - \max_j \log w_j} / \sum_j(\cdot)$.
7. **Neff:** $N_{\text{eff}} = 1/\sum_i w_i^2$. É logado e usado como gatilho opcional (`--resample_neff r < 1`).
8. **Low Variance Sampler** (Thrun et al., Tab. 4.4): $r \sim \mathcal U(0, 1/N)$; para m = 0..N−1, $U = r + m/N$; avança i enquanto $U > c_i$ ($c_i$ = soma cumulativa); copia a partícula i. Os pesos voltam a $1/N$. Custo O(N), variância mínima de amostragem.
9. **Estimativa:** $\bar x = \sum w_i x_i$ (idem y e z), yaw pela média circular $\bar\psi = \operatorname{atan2}(\sum w_i\sin\psi_i, \sum w_i\cos\psi_i)$, e variâncias ponderadas. Erro: $\varepsilon_t = \lVert(\bar x,\bar y) - (x^{GT}, y^{GT})\rVert \cdot s_{m/px}$.

---

## 4. Plano de ação priorizado

| Prioridade | Ação | Arquivos | Status |
|---|---|---|---|
| **P0** | Atualização bayesiana multiplicativa em log e reamostragem a cada frame (padrão); centro fora do mapa com peso 0 | `particle_filter.hpp/.cpp` | ✅ feito e testado (Test 12, 13) |
| **P0** | Remover o pareamento de GT por "tamanho igual" | `dataset_loader.cpp` | ✅ feito |
| **P1** | Quantização de cor em 25 níveis no mapa e nos frames, bit-exata com o legado | `descriptor.hpp/.cpp`, `run_localization.cpp` | ✅ feito (Test 11) |
| **P1** | Odometria não confiável → movimento nulo; SIFT ilimitado; `--odom_file` | `run_localization.cpp`, `config.*` | ✅ feito |
| **P1** | Expor `--min_scale/--max_scale/--var_brief/--resample_neff/--pos_noise`; avisar sobre opções desconhecidas | `config.hpp/.cpp` | ✅ feito |
| **P1** | Corrigir documentação: UFRGS 0,10 m/px, pasta `Mapa/`, fatias 84–279 e 204–427, 50k partículas, `render_video.py` sem `--dataset_dir` | `CLAUDE.md`, `lambda_brief_modern/README.md` | ✅ feito |
| **P2** | **Injeção de partículas aleatórias** (`--inject_ratio`, `--inject_spread`) para criar hipóteses novas e relocalizar | `particle_filter.hpp/.cpp`, `config.*` | ✅ feito (Test 15) |
| **P2** | **Prior de escala por altímetro** (`--scale_prior_sigma`, `--scale_prior_gain 0.87` no ADM/0036), emulado pelo z do GT | `particle_filter.*`, `run_localization.cpp` | ✅ feito (Test 14). Falta ler a altitude real (GPS/baro) em vez do GT |
| **P2** | Ignorar a escala da VO (`--vo_scale_deadband 1`) | `visual_odometry.*` | ✅ feito |
| **P2** | Normalizar d pela distribuição nula por faixa de z | `particle_filter.cpp` | ❌ testado e removido (inverte o viés para z ≈ 2,4) |
| **P2** | Robustez nos trechos fracos (frames 74–78, 94–105 do ADM2): rejeitar saltos de VO incompatíveis com a velocidade recente, temperar a verossimilhança (var_brief maior) quando `frac_below_gt` sobe, ou usar a estimativa pela moda (como o legado, média com w²) | `run_localization.cpp`, `particle_filter.cpp` | proposto |
| **P3** | Avaliação estatística como no artigo: 30 sementes **independentes** por mapa, média e mediana por frame, curva de convergência (% de execuções com erro < 20 m por frame). Script `scripts/batch_eval.py` | `scripts/` | proposto |
| **P3** | Validar ADM1/ADM3 (0035, 0039, 0040): conferir offsets 18/55 e as escalas dos mapas V2 (arquivos `*_V2.jpg` têm outro enquadramento) | `dataset_loader.cpp` | pendente |
| **P3** | Performance: pré-computar Lab do mapa como planos separados (cache-friendly) e usar SIMD no teste de bits; ~0,7–1 s/frame com 50k em 2 núcleos hoje | `descriptor.cpp` | opcional |

### Comandos de validação

```bash
cd lambda_brief_modern && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure                     # 52 checks, 17 baterias

# ADM2 completo, configuração do artigo; repita com --seed 1..30
./build/lambda_brief_loc --map ../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg \
  --dataset_dir ../Datasets/ADM/0036 --output_dir ./res_adm2_s1 --start_frame 84 --end_frame 279 \
  --num_particles 50000 --alpha 0.5 --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 --seed 1

# Discriminabilidade do modelo de observação (d_gt ≈ 0,38; frac_below_gt ≈ 0)
./build/diag_likelihood --map <mapa> --dataset_dir ../Datasets/ADM/0036 --start_frame 84 --end_frame 103
```

Com a configuração acima (a do artigo), a convergência depende da semente. A seed 1 **não** converge no voo completo (seção 2.7). Configurações robustas:

```bash
# Recomendado: com altímetro (emulado a partir do z do GT; ganho 0,87 no ADM/0036)
... --inject_ratio 0.05 --inject_spread 300 --vo_scale_deadband 1 --scale_prior_sigma 0.10 --scale_prior_gain 0.87
# Sem sensores extras (só imagem + VO): converge, mas o rastreio é frágil
... --inject_ratio 0.10
```

---

## 5. Limitações desta análise

- A validação usou o ADM2 (40 frames para as varreduras de sementes e os 196 frames para as execuções completas da seção 2.7) com um único mapa. A UFRGS não foi validada.
- Cada configuração do voo completo rodou com 1 semente (cerca de 20 min por execução em 2 núcleos). Os números da seção 2.7 são indicativos. Para comparar com o artigo, rode 10–30 sementes.
- As taxas de convergência vêm de 4 a 6 sementes por configuração, o que dá uma indicação e não uma estatística. O legado converge 30/30, mas 29 dessas execuções tinham prior de escala herdado (seção 2.6). A única execução realmente "fria" do lote convergiu no frame 10.
- Os offsets de ADM 0035 (18) e 0040 (55) vêm do código de auto-detecção e não foram revalidados.

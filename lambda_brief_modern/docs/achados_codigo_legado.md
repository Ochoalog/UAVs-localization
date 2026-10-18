# Achados no código legado: por que o λ-BRIEF ganhava com folga no artigo (18/10/2026)

**Pergunta.** No artigo, o λ-BRIEF melhora muito a localização em relação ao abBRIEF. Na engine moderna, os dois quase empatam, ou o λ ganha por pouco. Por quê?

**Resposta curta.** O ganho do λ é real, mas ele aparece na **taxa de convergência da busca global**, não na precisão depois de convergir. Além disso, o artigo comparava o λ com um abBRIEF mais fraco (item 1), e o protocolo de repetições e a métrica amplificavam a diferença (itens 3 e 4).

Há uma apresentação com desenhos de cada problema (feita para o Renan, em 18/10/2026). As linhas citadas são do `phir2frameworkv2/` versionado.

---

## 1. Os dois métodos não usavam o mesmo baseline

### 1.1 Dois caminhos de código

Os scripts do artigo (por exemplo `Experimentos/ADM_036_30x_bateria_testes_paper*/ADM_036_30x_bateria_testes_paper.sh`) rodam:

- **abBRIEF** com `-msc 0 -pdm 1` (linha 38);
- **λ-BRIEF** com `-msc 1 -pdm 51` (linhas 40–58).

O `main.cpp:430` lê o `-msc`. Em `Mcl.cpp:709–712`, ele escolhe entre `prepareWeighting(z, bands…)` (linha 710, o λ) e `prepareWeighting(z)` (linha 712, o ab).

| | abBRIEF (`-msc 0`) | λ-BRIEF (`-msc 1 -pdm 51`) |
|---|---|---|
| Entrada | `Mcl.cpp:1854` → `BriefHeuristic::updateDroneDescriptor(cv::Mat&)` (`BriefHeuristic.cpp:1132`) | `Mcl.cpp:1814` → `BriefHeuristic.cpp:70` → `pixelDistribution` (`:143`), `switch` em `:219`, `case(51)` em `:530` |
| Distribuição dos pares | "gaussiana" com σ errado → quase **uniforme** na imagem toda | gaussiana central de verdade (σ efetivo ≈ 213 px), fundida com o NDVI |
| Sorteio | **fixo**: os mesmos 256 pares em todo frame, repetição, mapa e voo | **re-sorteado a cada frame** (`srand(time*tick)`, `:539`) |

### 1.2 Padrão fixo no abBRIEF

Em `BriefHeuristic.cpp:1145`, a linha `cv::RNG rng;` cria o gerador sem argumento. Então ele sempre começa com a semente padrão `0xFFFFFFFF`. Como `rng` é variável local, ela é recriada a cada chamada. A função roda todo frame, mas devolve sempre os mesmos 256 pares.

Um padrão fixo é intencional no BRIEF original. O problema é a comparação: o λ re-sorteava os pares a cada frame e o ab não. Se o padrão fixo cair mal (vegetação, borda, sombra), ele cai mal em todos os frames e em todas as 30 repetições.

### 1.3 Variância usada como desvio-padrão

Em `BriefHeuristic.cpp:1149`, a chamada é `rng.gaussian(1.0/25.0*pow(drone.cols,2))`.

- `cv::RNG::gaussian(σ)` recebe o **desvio-padrão**, mas o código passa a **variância** (W/5)².
- Com W = 1241 px, isso dá σ ≈ 61.603 px, em vez de 248 px.
- O laço de rejeição `while(!pointIn(...))` descarta os pontos fora da imagem. O que sobra é praticamente **uniforme** na imagem.

### 1.4 O caminho do λ (case 51)

- Máscara gaussiana: `BriefHeuristic.cpp:99–116`, com denominador 2σxσy (σ efetivo ≈ 213 px).
- NDVI e equalização: `:565–620`.
- Fusão das máscaras: `:621`.
- Roleta: `:650–690`, com um viés pequeno no 2º pixel do par.

**Consequência.** No artigo, "λ × ab" mistura três efeitos: (a) concentrar os pares no centro, (b) re-sortear por frame e (c) usar o NDVI. Na engine moderna, o abBRIEF é o λ com α = 0. Os dois usam o mesmo caminho (`run_localization.cpp:177` `PixelSampler(seed)` → `:273` máscara gaussiana com σ = W/5, H/5 em `mask_generator.cpp:13–14` → `:276` `combineMasks` com `config.alpha` → `:284` `sampleRoulette` a cada frame). Só sobra o efeito (c).

### 1.5 Par fora do mapa

Quando um par cai fora do mapa, o legado grava 3 valores `UNDEFINED` num espaço de 2 bits (`BriefHeuristic.cpp:1407–1409` e `1625–1627`). Isso desalinha o resto do descritor. O efeito é pequeno, mas pesa mais em pares uniformes perto da borda, ou seja, no ab do legado.

### 1.6 A flag de α dos scripts não existe no `main.cpp`

- Os scripts passam `-alfagaussmsk`.
- O `main.cpp` só reconhece `-alfavismsk` (`:696`, `strncmp` de 11 caracteres) e `-multgaussmsk` (`:683`).
- Uma flag desconhecida cai no `else p++` (`:761`) e é ignorada em silêncio. Então `alphaVegetationMask` fica no padrão 0 (`:53`), ou seja, só a máscara gaussiana.
- O binário de 08/01/2020 (`Experimentos 08_01_2020/ADM/build-PhiR2Framework-Desktop-Release/PhiR2Framework`) também não contém a string `-alfagaussmsk`.
- A linha comentada `BriefHeuristic.cpp:821` (`//H(p) = aG(p) + (1-a)V(p)`) indica que a versão antiga ponderava a **gaussiana**.

**Pergunta em aberto:** qual binário gerou as figuras do artigo?

---

## 2. As 30 repetições não eram independentes

- Um único processo roda as 30 repetições: `paper.sh:113` (`-o path $count $(($count+$numTests-1))`, `numTests=30`).
- Entre uma repetição e outra, `DroneRobot::run` (`:776–783`) faz `current++; reinitialize();` e chama `MCL::restart` (`Mcl.cpp:507`).
- O construtor do MCL semeia com o relógio (`Mcl.cpp:57`), cria o gerador (`:62`) e sorteia a escala em `p.z` (`:84`). Só a 1ª repetição passa por ele.

O `MCL::restart` tem dois problemas:

1. **Nuvem repetida.** `Mcl.cpp:144` declara `std::default_random_engine generator;` sem semente. A nuvem inicial (x, y, yaw) é idêntica da repetição 2 em diante.
2. **Escala herdada.** `Mcl.cpp:165` sorteia a escala em `particles[i].s` (campo declarado em `Mcl.h:24`), mas o filtro usa `p.z` (`Mcl.cpp:1176–1185`). Cada repetição começa com o z final da anterior.

**Evidência nos logs de 2019** (`Experimentos/ADM_035_103_402_apenas_alfa/ADM_35_17_12_2018`, ADM1):

- O z inicial por repetição vai 0,97 → 1,05 → 0,11 → 1,04 → 0,10 → 1,04 → 1,6 → 2,2 → 5,8 → 27 → 128 → 597 → … → 10¹⁵ a 10¹⁷.
- O z₀ da repetição k+1 é igual ao z final da repetição k.
- No lote do ADM2 do artigo, z₀ ≈ 0,42–0,44 nas repetições 2..30, contra ≈ 1,0 na 1ª.

**Consequência.** O n efetivo do artigo é bem menor que 30, e a média do lote depende muito da 1ª execução.

**Na engine moderna**, cada repetição é um processo com `--seed N`. O `ParticleFilter` semeia o gerador (`particle_filter.cpp:14–20`) e `initializeUniform` sorteia x, y, z e yaw de todas as partículas (`:23–41`). Para a mesma seed, ab e λ começam da mesma nuvem (comparação pareada); seeds diferentes são independentes.

---

## 3. Outras diferenças

| Item | Legado / artigo | Engine moderna |
|---|---|---|
| Filtro | sem altímetro, sem injeção, sem duas fases; um modo falso não se recupera | barômetro real, injeção adaptativa e duas fases (`final_baro`) |
| Métrica | erro **3D**, com a altitude; GT original | erro 2D; GT re-sincronizado (ADM3: GT do Zenodo) |

Sem altímetro, um erro de escala vira dezenas de metros de erro de altitude. Isso explica erros "convergidos" de 25–150 m no artigo, contra ~10 m na engine moderna.

---

## 4. O que os dados da engine moderna mostram

**Regime vanilla (protocolo do artigo), ADM2, α = 0,5, 30 seeds** (`experimentos/2026-10-01_benchmark_artigo_vanilla`):

| Mapa | Converge ab | Converge λ | MAE ab | MAE λ |
|---|---|---|---|---|
| 12/09/2018 | 53% | 80% | 177 m | 106 m |
| 17/08/2018 | 77% | 83% | 102 m | 93 m |
| 17/12/2018 | 63% | 73% | 151 m | 132 m |
| 27/02/2017 | 73% | 87% | 136 m | 77 m |

A mediana é igual nos dois (10–16 m). Quando os dois convergem, eles erram igual; a vantagem do λ é convergir mais vezes.

**Regime final_baro (5 seeds):** os dois convergem em 5/5 em todos os voos e mapas.

- O λ ganha no ADM1 e no UFRGS.
- No ADM2, ganha por pouco com α = 0,5 (Wilcoxon p = 0,11) e empata com α = 1.
- No ADM3, o abBRIEF é melhor nos dois α.

Detalhes em `experimentos/INDICE.md` (seção "Resumo dos testes").

---

## 5. Implicações para o artigo

- Descrever a contribuição do λ como **robustez da localização global** (converge mais vezes e mais cedo sob mudança sazonal), não como menor erro de rastreio.
- Reportar métricas separadas:
  - taxa de convergência (k/N);
  - tempo até convergir;
  - erro depois de convergir.
- Rodar o protocolo do artigo na engine moderna (`paper_comparison_seeds.py --protocolo artigo`, 5 a 30 seeds) para medir a taxa de convergência.
- Opcional: uma opção "abBRIEF legado" (256 pares uniformes e fixos) para medir quanto do ganho do artigo vinha do baseline.

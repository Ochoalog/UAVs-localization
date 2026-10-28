# Roteiro da reunião: o código legado, passo a passo

Ordem para mostrar os achados abrindo o código ao vivo. Cada passo tem:

- o arquivo, com o caminho completo;
- as linhas a abrir;
- um comando para colar no terminal;
- o que apontar.

Todas as linhas foram conferidas nos arquivos atuais (27/10/2026).

**Abrir na linha certa:**

- VS Code: `code -g "<arquivo>:<linha>"`
- Terminal: `sed -n '<início>,<fim>p' "<arquivo>" | cat -n`

O caminho tem espaço, então use sempre aspas.

Atalho para os comandos abaixo:

```bash
L="/storage/UAVs Localization/phir2frameworkv2"
M="/storage/UAVs Localization/lambda_brief_modern"
E="/storage/UAVs Localization/Experimentos"
```

---

## Parte 1: abBRIEF e λ-BRIEF não usavam o mesmo baseline

### Passo 1: os dois métodos são chamados com flags diferentes

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/ADM_036_30x_bateria_testes_paper.sh`, linhas 38–58.

```bash
for l in 38 40 42 44 46 52 54 56 58; do
  sed -n ${l}p "$L/ADM_036_30x_bateria_testes_paper.sh" | grep -o "\-msc [0-9]\|-pdm [0-9]*\|-multgaussmsk [0-9.]*\|-msimp [0-9]" | tr '\n' ' '; echo " (linha $l)"
done
```

O que aparece:

| Linha | Configuração |
|---|---|
| 38 | abBRIEF: `-msc 0 -pdm 1` |
| 40–58 | λ: `-msc 1 -pdm 51` (variando `-multgaussmsk` e `-msimp`) |

**Dizer:** o `-msc` escolhe a função inteira de amostragem, não só a máscara.

### Passo 2: o `-msc` desvia para dois caminhos de código

**Arquivos:**

- `/storage/UAVs Localization/phir2frameworkv2/main.cpp`, linha 430 (lê o `-msc`)
- `/storage/UAVs Localization/phir2frameworkv2/Mcl.cpp`, linhas 709–712

```bash
sed -n '709,712p' "$L/Mcl.cpp"
```

```cpp
if(multispectralCam)
    prepareWeighting(z, redMap, nirMap, regMap, greMap, currentMapOriginal, droneAngles[currentImage]);   // λ
else
    prepareWeighting(z);                                                                                   // abBRIEF
```

Cada caminho chega a uma função diferente no `Mcl.cpp`:

- **λ**, linha 1814: `bh->updateDroneDescriptor(frame, redMap, nirMap, …)` → `BriefHeuristic.cpp:70` → `pixelDistribution`
- **abBRIEF**, linha 1854: `bh->updateDroneDescriptor(frame)` → `BriefHeuristic.cpp:1132`

**Dizer:** no abBRIEF, o `-pdm 1` nem é lido, porque o `pixelDistribution` não é chamado.

### Passo 3: o abBRIEF usa sempre os mesmos 256 pares

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/BriefHeuristic.cpp`, linhas 1132–1154.

```bash
sed -n '1132,1154p' "$L/BriefHeuristic.cpp" | cat -n
```

Apontar a **linha 1145**: `cv::RNG rng;`

- Sem argumento, o `cv::RNG` nasce sempre com a mesma semente (`0xFFFFFFFF`).
- Como é uma variável local, ela é recriada a cada chamada.
- Resultado: a função roda todo frame, mas devolve sempre os mesmos pares, em toda repetição, mapa e voo.

**Ressalva justa:** no BRIEF original, o padrão fixo é intencional. O problema é que o λ re-sorteava a cada frame e o abBRIEF não.

### Passo 4: a variância foi usada como desvio-padrão

**Mesmo arquivo, linha 1149:**

```cpp
pairs[x][0] = cv::Point((int)rng.gaussian(1.0/25.0*pow(drone.cols,2)) + drone.cols/2, ...
```

- `cv::RNG::gaussian(σ)` recebe o **desvio-padrão**, mas o código passa a variância, (W/5)².
- Com W = 1241 px, isso dá σ ≈ 61.600 px, em vez de 248 px.
- O laço `while (!pointIn(...))` descarta o que cai fora da imagem. O que sobra é **praticamente uniforme**.

### Passo 5: o caminho do λ (case 51)

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/BriefHeuristic.cpp`

| Linha | O que tem |
|---|---|
| 99–116 | `gaussianMask`, gaussiana centrada (denominador `2·σx·σy`) |
| 143 | `pixelDistribution` |
| 219 | `switch(pixelDistMethod)` |
| 530 | `case(51)`: roleta + gaussiana |
| 539 | `srand(time(NULL) * cv::getTickCount())`: **pares novos a cada frame** |
| 565–620 | NDVI → `equalizeHist` → normaliza para [0, 1] |
| 621 | fusão `viMat = α·VI + (1−α)·gaussiana` |
| 650–690 | roleta |

```bash
sed -n '530,540p;618,622p' "$L/BriefHeuristic.cpp"
```

**Dizer:** o λ tinha três coisas que o abBRIEF não tinha:

1. pares concentrados no centro (gaussiana);
2. pares re-sorteados a cada frame;
3. o NDVI.

O ganho do artigo soma os três efeitos. Só o terceiro é da vegetação.

### Passo 6: o α dos scripts não chega ao código

**Arquivos:**

- `/storage/UAVs Localization/phir2frameworkv2/main.cpp`, linhas 53, 683, 696 e 761
- `/storage/UAVs Localization/Experimentos/ADM_035_103_402_apenas_alfa/ADM_035_103_402_apenas_alfa.sh`, linha 36 e linhas 50–60 (passam `-alfagaussmsk`)

```bash
grep -n "alfavismsk\|multgaussmsk\|alphaVegetationMask = 0" "$L/main.cpp"
sed -n '759,762p' "$L/main.cpp"
grep -o "\-alfagaussmsk [^ ]*" "$E/ADM_035_103_402_apenas_alfa/ADM_035_103_402_apenas_alfa.sh" | sort -u
```

O `main.cpp` só reconhece duas flags de α:

- `-multgaussmsk`, linha 683;
- `-alfavismsk`, linha 696.

Uma flag desconhecida cai no `else p++` (linha 761) e é ignorada. Então `alphaVegetationMask` fica no padrão 0, definido na linha 53.

Com α = 0, a linha 621 do `BriefHeuristic.cpp` usa **só a gaussiana**. O binário de 08/01/2020 também não tem a string `-alfagaussmsk`:

```bash
grep -c alfagaussmsk "/storage/UAVs Localization/Experimentos 08_01_2020/ADM/build-PhiR2Framework-Desktop-Release/PhiR2Framework"
```

A linha 821 do `BriefHeuristic.cpp` tem o comentário `//H(p) = aG(p) + (1-a)V(p)`. Isso sugere que uma versão antiga ponderava a **gaussiana** com o parâmetro.

**Mesmo efeito no script do artigo.** O `ADM_036_30x_bateria_testes_paper.sh` passa `-multgaussmsk`, mas no `BriefHeuristic.cpp` o `multiGaussianMask` só é usado no `case(52)` (linha 779), não no `case(51)`:

```bash
grep -n "multiGaussianMask" "$L/BriefHeuristic.cpp"
```

Então, com o código atual, os λ do script também rodariam com α = 0, ou seja, só gaussiana, sem NDVI.

**Pergunta para a reunião:** qual binário rodou os experimentos de 2019?

### Passo 7: par fora do mapa desalinha o descritor (detalhe menor)

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/BriefHeuristic.cpp`, linhas 1407–1409 (e 1625–1627).

```bash
sed -n '1400,1412p' "$L/BriefHeuristic.cpp"
```

São três `push_back(HEURISTIC_UNDEFINED_INT)` num espaço de 2 bits por par. Isso desloca todos os bits seguintes. Pesa mais em pares perto da borda, ou seja, no abBRIEF uniforme.

### Passo 8: como fazemos agora, um caminho só

**Arquivos:**

- `/storage/UAVs Localization/lambda_brief_modern/apps/run_localization.cpp`, linhas 177, 273, 276 e 284
- `/storage/UAVs Localization/lambda_brief_modern/src/core/mask_generator.cpp`, linhas 13–14 (σ = W/5 e H/5)

```bash
sed -n '177p;273,290p' "$M/apps/run_localization.cpp"
sed -n '10,16p' "$M/src/core/mask_generator.cpp"
```

| Linha de `run_localization.cpp` | O que faz |
|---|---|
| 177 | `PixelSampler sampler(config.random_seed);` |
| 273 | gaussiana centrada |
| 276 | `combineMasks(…, config.alpha)` |
| 284 | `sampleRoulette`, a cada frame, para os dois métodos |

**Dizer:** α = 0 é exatamente o abBRIEF. Entre os dois métodos, só a máscara muda.

---

## Parte 2: as 30 repetições eram uma cadeia

### Passo 9: um processo roda as 30 repetições

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/ADM_036_30x_bateria_testes_paper.sh`, linhas 3 e 113.

```bash
sed -n '3p;113p' "$L/ADM_036_30x_bateria_testes_paper.sh"
```

O script define `numTests=30` e chama `-o "$resultPath" $count $(($count+$numTests-1))`, ou seja, 0 a 29 no mesmo processo.

**Arquivo:** `/storage/UAVs Localization/phir2frameworkv2/DroneRobot.cpp`, linhas 778–783 e 507.

```bash
sed -n '778,783p;507p' "$L/DroneRobot.cpp"
```

No fim da trajetória: `current++; reinitialize();`. A linha 507 chama `mcLocalization->restart(...)`.

### Passo 10: o construtor (só a 1ª repetição) × o `MCL::restart` (repetições 2 a 30)

**Arquivos:**

- `/storage/UAVs Localization/phir2frameworkv2/Mcl.cpp`
- `/storage/UAVs Localization/phir2frameworkv2/Mcl.h`

```bash
sed -n '57p;62p;84p' "$L/Mcl.cpp"       # construtor: semente do relógio, z em p.z
sed -n '134p;144p;165p' "$L/Mcl.cpp"    # restart: gerador sem semente, z no campo errado
sed -n '24p' "$L/Mcl.h"                 # double s; //SCALE
sed -n '1176p;1185p' "$L/Mcl.cpp"       # o filtro usa p.z
```

| Onde | Linha | Código |
|---|---|---|
| Construtor | 57 | `seed = system_clock::now()...` |
| Construtor | 62 | `default_random_engine generator(seed);` |
| Construtor | 84 | `particles[i].p.z = randomS(generator);` |
| `restart` | 144 | `std::default_random_engine generator;` **sem semente**: a nuvem (x, y, yaw) é **idêntica** da 2ª repetição em diante |
| `restart` | 165 | `particles[i].s = randomS(generator);` grava a escala em `s` (`Mcl.h:24`), mas o filtro usa `p.z` (`Mcl.cpp:1176` e `1185`); por isso **z é herdado** da repetição anterior |

### Passo 11: a prova nos logs de 2019

**Pasta:** `/storage/UAVs Localization/Experimentos/ADM_035_103_402_apenas_alfa/ADM_35_17_12_2018/traj/alfagaussmsk_0/`

Um arquivo por repetição: `0.txt` a `29.txt`. A 3ª coluna é `meanPZ`.

```bash
D="$E/ADM_035_103_402_apenas_alfa/ADM_35_17_12_2018/traj/alfagaussmsk_0"
for k in $(seq 0 29); do
  printf "%2d  z inicial = %-12s z final = %s\n" $k \
    "$(awk '$3+0==$3 && NF>5{print $3; exit}' "$D/$k.txt")" "$(tail -1 "$D/$k.txt" | awk '{print $3}')"
done
```

O que aparece (já conferido):

| Repetição | z inicial | z final |
|---|---|---|
| 0 | 0,966 | 1,046 |
| 1 | **1,049** | 0,098 |
| 2 | **0,107** | 1,039 |

Mais adiante a cadeia cresce: 1,6 → 2,2 → 5,8 → 27 → 128 → 597 → … → 10¹⁵ a 10¹⁷.

**Dizer:** o z inicial da repetição k+1 é o z final da k. Só a 1ª repetição é uma localização global "fria".

No lote do ADM2 usado no artigo, a escala não explodiu, mas o z inicial é ≈ 1,0 na 1ª repetição e 0,42–0,44 nas repetições 2 a 30.

### Passo 12: como fazemos agora, uma seed por processo

**Arquivo:** `/storage/UAVs Localization/lambda_brief_modern/src/localization/particle_filter.cpp`

| Linhas | O que faz |
|---|---|
| 14–20 | `rng_.seed(seed)` |
| 23–41 | `initializeUniform` sorteia x, y, **z** e yaw de todas as partículas |

```bash
sed -n '12,42p' "$M/src/localization/particle_filter.cpp"
```

- Cada repetição é um processo com `--seed N`; o comando fica em `seed_N/cmd.txt`.
- Mesma seed ⇒ ab e λ partem da mesma nuvem: a comparação é pareada (Wilcoxon).
- A engine é determinística: a seed 1 do teste de 5 seeds reproduziu exatamente o teste de 1 seed.

---

## Parte 3: o que muda nos resultados

### Passo 13: o protocolo do artigo na engine nova, com 30 seeds independentes

**Pasta:** `/storage/UAVs Localization/lambda_brief_modern/experimentos/2026-10-01_benchmark_artigo_vanilla/`

Regime vanilla, ADM2, α = 0,5:

| Mapa | Converge ab | Converge λ | MAE ab | MAE λ |
|---|---|---|---|---|
| 12/09/2018 | 53% | 80% | 177 m | 106 m |
| 17/08/2018 | 77% | 83% | 102 m | 93 m |
| 17/12/2018 | 63% | 73% | 151 m | 132 m |
| 27/02/2017 | 73% | 87% | 136 m | 77 m |

**Dizer:** a mediana é igual nos dois (10–16 m). O λ **converge mais vezes**; quando os dois convergem, eles erram igual.

### Passo 14: o teste do artigo (4 voos × 5 mapas) com o filtro robusto, 5 seeds

**Pastas:**

- `/storage/UAVs Localization/lambda_brief_modern/experimentos/2026-10-17_artigo_novo_a1_5seeds/`
- `/storage/UAVs Localization/lambda_brief_modern/experimentos/2026-10-17_artigo_novo_a0.5_5seeds/`

Abrir `tabela.md`, `compilado_artigo_ADM.png` e `compilado_artigo_UFRGS.png` de cada pasta.

MAE médio por voo, ab → λ:

| Voo | α = 1 | α = 0,5 |
|---|---|---|
| ADM1 | 40,8 → 27,7 | 40,8 → 32,9 |
| ADM2 | 49,8 → 44,8 | 49,8 → 38,4 |
| ADM3 | 23,6 → 37,7 | 23,6 → 28,5 |
| UFRGS | 29,3 → 15,7 | 29,3 → 14,9 |

**Dizer:**

- Com barômetro, injeção e duas fases, os dois convergem em todas as execuções.
- O λ ganha no ADM1 e no UFRGS (busca mais rápida nos mapas difíceis), empata ou ganha por pouco no ADM2 e perde no ADM3.
- O α = 0,5 reduz a perda do ADM3, mas não a elimina.

### Passo 15: próximos passos e perguntas

1. Rodar `--protocolo artigo` (sem altímetro, α = 1) com 5 a 30 seeds e reportar a taxa de convergência. Manual: `/storage/UAVs Localization/lambda_brief_modern/docs/MANUAL_TESTES_ARTIGO.md`.
2. Opção "abBRIEF legado" na engine nova (256 pares uniformes e fixos), para medir quanto do ganho do artigo vinha do baseline.
3. Reportar métricas separadas: taxa de convergência, tempo até convergir e erro depois de convergir.

**Perguntas:**

- Qual binário rodou os experimentos de 2019?
- O padrão fixo e quase uniforme do abBRIEF era intencional?

---

## Se pedirem mais detalhe

- **Resumo escrito, com as mesmas linhas:** `/storage/UAVs Localization/lambda_brief_modern/docs/achados_codigo_legado.md`
- **Todos os experimentos, com resumo:** `/storage/UAVs Localization/lambda_brief_modern/experimentos/INDICE.md`
- **Diário de cada rodada:** `/storage/UAVs Localization/lambda_brief_modern/HISTORICO_RESULTADOS.md`

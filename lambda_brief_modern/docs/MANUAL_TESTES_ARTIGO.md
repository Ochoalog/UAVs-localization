# Manual: como rodar os testes do artigo (abBRIEF × λ-BRIEF, Figs. 4 e 5)

Este manual repete, passo a passo, os quatro testes rodados em 17/10/2026:

| Teste | Pasta | Comando (resumo) |
|---|---|---|
| α = 1, 1 seed | `experimentos/2026-10-17_artigo_novo_a1` | `paper_comparison.py --protocolo novo --lambda_alpha 1.0` |
| α = 1, 5 seeds | `experimentos/2026-10-17_artigo_novo_a1_5seeds` | `paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --sem_reaproveitar` |
| α = 0,5, 1 seed | `experimentos/2026-10-17_artigo_novo_a0.5` | `paper_comparison_seeds.py ... --lambda_alpha 0.5 --seeds 1 --reaproveitar <α=1, 1 seed>` |
| α = 0,5, 5 seeds | `experimentos/2026-10-17_artigo_novo_a0.5_5seeds` | `paper_comparison_seeds.py ... --lambda_alpha 0.5 --seeds 1..5 --reaproveitar <α=1, 5 seeds>` |

Os resultados e a leitura de cada um estão em `experimentos/INDICE.md` (seção "Resumo dos testes") e no `HISTORICO_RESULTADOS.md`.

Todos os comandos abaixo rodam **de dentro de `lambda_brief_modern/`**.

---

## 1. O que o teste faz

O artigo compara o abBRIEF com o λ-BRIEF em 4 voos e em 5 mapas de satélite por voo, com uma curva de erro por mapa e por método. Os scripts fazem o mesmo com a engine moderna:

```
para cada seed (1 no paper_comparison.py; 1..5 no paper_comparison_seeds.py)
  para cada voo (ADM1, ADM2, ADM3, UFRGS)
    para cada mapa do artigo (5 por voo)
      roda o abBRIEF (α = 0) e o λ-BRIEF (α escolhido) com a mesma seed
```

São 40 execuções por seed (4 voos × 5 mapas × 2 métodos). Para a mesma seed, os dois métodos partem da **mesma nuvem inicial**, então a comparação é pareada.

### Voos e mapas

| Voo do artigo | Pasta | Imagens | Frames | Mapas |
|---|---|---|---|---|
| ADM1 | `Datasets/ADM/0035` (usa `traj_comp.txt`) | 0103–0402 | 300 | ADM_35 |
| ADM2 | `Datasets/ADM/0036` | 0199–0394 | 196 | ADM_35 |
| ADM3 | `Datasets/ADM/0040` | 0200–0465 | 266 | Zenodo (`Datasets/ADM/0040_zenodo/Maps`) |
| UFRGS | `Datasets/UFRGS VET/voo_100m` | 0390–0613 | 224 | UFRGS_VET_V3 |

- **Mapas ADM** (legenda das Figs. 4 e 5): 12-2018 = 17/12/2018, 01-2014 = 27/01/2014, 11-2016 = 24/11/2016, 09-2017 = 08/09/2017, 02-2014 = 02/02/2014.
- **Mapas UFRGS**: 08-2011, 06-2014, 12-2002, 04-2015, 10-2012 (`UFRGS_VET_V3_<data>.jpg`).

As datas ADM foram confirmadas pelo `Maps.zip` do Zenodo do ADM3. Esses arquivos são idênticos (md5) aos `ADM_35_*` do repositório.

### Protocolos

| Protocolo | Regime do filtro | α padrão | GT da avaliação |
|---|---|---|---|
| `--protocolo artigo` | `vanilla`: filtro original, sem altímetro, odometria gravada do legado | 1,0 | `traj_truth.txt` original |
| `--protocolo novo` | `final_baro`: duas fases, injeção adaptativa, barômetro real, odometria fixa da VO nova | 0,5 | `traj_truth_sync.txt` (ADM3: GT do Zenodo) |

`--regime`, `--lambda_alpha` e `--gt` sobrescrevem o protocolo. **Os quatro testes de 17/10 usaram `--protocolo novo`** e passaram o α explicitamente.

O regime `final_baro` monta este comando para cada execução (ver `scripts/benchmark_runner.py`):

```
lambda_brief_loc --map <mapa> --dataset_dir <voo> --start_frame <s> --end_frame <e> --gt_offset <s>
  --num_particles 50000 --quantize 25 --min_scale 0.1 --max_scale 3.0 --pixel_to_meter <m/px>
  --color_mode LAB_NO_L --seed <N> --quiet
  --alpha 0                                                     (abBRIEF)
  --alpha <α> --vi NDVI --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5   (λ-BRIEF)
  --odom_file insumos/odom/<DS>_vo_ratio.txt
  --two_phase --inject_ratio 0.05 --inject_spread 300 --pos_noise 17
  --scale_prior_sigma 0.10 --altimeter_file <voo>/altimeter_baro.txt --scale_prior_gain 1
  [--traj Datasets/ADM/0035/traj_comp.txt]                       (só ADM1)
  [--gt Datasets/ADM/0040_zenodo/GT_DAT/ADM3_ground_truth.txt]   (só ADM3)
```

O comando exato de cada execução fica em `execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_<N>/cmd.txt`.

---

## 2. Pré-requisitos (uma vez por máquina)

### 2.1 Compilar a engine

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tests/test_suite        # 21 baterias; todas devem passar
```

### 2.2 Datasets

A pasta `Datasets/` não está no git. Ela fica na raiz do repositório, ao lado de `lambda_brief_modern/`.

| Item | Onde | Para quê |
|---|---|---|
| Voos ADM1, ADM2, UFRGS | `Datasets/ADM/0035`, `0036`, `Datasets/UFRGS VET/voo_100m` | imagens, `traj.txt`, `traj_truth.txt` |
| Mapas ADM_35 | `Datasets/ADM/0039/Mapas/` (os scripts procuram lá) | mapas ADM1/ADM2 |
| `traj_comp.txt` do ADM1 | `Datasets/ADM/0035/traj_comp.txt` | o `traj.txt` original aponta para .png que não existem |
| Logs do DJI (CSV) e EXIF da Sequoia | `<voo>/Banco/...` (ADM) e `voo_100m/FLY044_5hz_completo.csv` (UFRGS); lista em `extract_altimeter.py` | `extract_altimeter.py` (passo 2.3) |
| ADM3 do Zenodo | `Datasets/ADM/0040_zenodo/` | mapas, GT e trilha de voo do ADM3 |

**ADM3 (Zenodo).** O repositório não tem os mapas nem um GT utilizável do voo `0040`: o `traj_truth.txt` local está em outro sistema de coordenadas (rotação de ~27° e escala ×1,21). Os mapas e o GT certos estão no Zenodo do artigo:

1. Baixe `Maps.zip` e `GT_DAT.zip` de <https://zenodo.org/record/4070013>.
2. Descompacte assim:

```
Datasets/ADM/0040_zenodo/
├── Maps/ADM_17_12_2018.jpg, ADM_27_01_2014.jpg, ADM_24_11_2016.jpg, ADM_08_09_2017.jpg, ADM_02_02_2014.jpg
└── GT_DAT/ADM3_ground_truth.txt, ADM3.kml, ADM3_traj.jpg
```

### 2.3 Altímetro real (barômetro) e GT sincronizado

O regime `final_baro` precisa de `<voo>/altimeter_baro.txt`. A avaliação do protocolo novo usa `<voo>/traj_truth_sync.txt`.

```bash
# ADM1, ADM2 e UFRGS: a partir dos logs do DJI. Gera altimeter_baro.txt e traj_truth_sync.txt
# em cada pasta de voo; os relatórios vão para insumos/altimetro/
python3 scripts/extract_altimeter.py

# ADM3: não há log do DJI; a altura vem da trilha GT_DAT/ADM3.kml do Zenodo.
# Gera Datasets/ADM/0040/altimeter_baro.txt (o ADM3 é avaliado direto contra o GT do Zenodo)
python3 scripts/adm3_altimeter_from_kml.py --check   # só o resumo
python3 scripts/adm3_altimeter_from_kml.py           # grava o arquivo
```

O `--check` do ADM3 deve mostrar: 1181 pontos no KML, 266 imagens, distância mediana de ~3 px entre o GT e o ponto casado, altura de 28,8 a 169,8 m, e correlação 1,000 com o z do GT do Zenodo.

### 2.4 Odometria fixa

Os regimes `final_*` usam uma odometria pré-gravada pela VO nova (ratio test + MAGSAC, 4000 SIFT, dz = 1). Assim, as diferenças entre métodos não vêm da VO. Os arquivos ficam em `insumos/odom/<DS>_vo_ratio.txt`: `ADM0035`, `ADM0036`, `ADM0040`, `UFRGS`.

**Os scripts do artigo geram esses arquivos sozinhos** se eles faltarem, antes de rodar. Para gerar à mão:

```bash
python3 scripts/benchmark_runner.py --make_odom --dataset ADM0036 UFRGS
```

O ADM1 e o ADM3 dependem do `traj_comp.txt` e dos mapas do Zenodo. Para esses dois, deixe o `paper_comparison.py` gerar a odometria.

Cada `.txt` tem um `.meta.json` com o comando, o md5 do binário e o commit usado.

---

## 3. Teste com 1 seed: `paper_comparison.py`

```bash
# 1) ver o que vai rodar (não roda nada)
python3 scripts/paper_comparison.py --protocolo novo --lambda_alpha 1.0 --dry_run

# 2) rodar em segundo plano (40 execuções)
nohup python3 scripts/paper_comparison.py --protocolo novo --lambda_alpha 1.0 > artigo.log 2>&1 &
```

A saída vai para `experimentos/<hoje>_artigo_novo_a1/` e ganha uma linha no `INDICE.md`.

Opções úteis:

| Opção | Efeito |
|---|---|
| `--flights ADM1 UFRGS` | só alguns voos |
| `--seed N` | outra seed (padrão 1) |
| `--jobs 4 --omp_threads 2` | execuções em paralelo × threads por execução (padrão) |
| `--continuar` | termina ou estende a pasta mais recente com o mesmo nome (outro dia, por exemplo) |
| `--plot_only` | só refaz figuras e tabela da pasta mais recente |
| `--nome X` / `--out <pasta>` | outro nome ou outra pasta |
| `--force` | roda de novo mesmo se `errors.txt` já existir |

Execuções que já têm `errors.txt` são puladas. Então, se o processo cair, basta rodar o mesmo comando com `--continuar`.

---

## 4. Teste com várias seeds: `paper_comparison_seeds.py`

É o mesmo teste repetido para N seeds (padrão `1..5`), com a média no fim. O `paper_comparison.py` continua existindo para o teste de 1 seed.

### 4.1 α = 1, 5 seeds (rodando tudo do zero)

```bash
python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --sem_reaproveitar --dry_run
nohup python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 1.0 --sem_reaproveitar \
      > seeds.log 2>&1 &
```

- 200 execuções, ~2,8 h com `--jobs 4`.
- Saída: `experimentos/<hoje>_artigo_novo_a1_5seeds/`.

A engine é determinística para a mesma seed: a seed 1 desse teste reproduziu exatamente o teste de 1 seed.

### 4.2 α = 0,5, 1 seed, reaproveitando o abBRIEF

O abBRIEF não usa o α. Então as execuções dele num teste com α = 1 servem para qualquer α. O `--reaproveitar <pasta>` copia as execuções cujo comando é **idêntico** (mesmo regime, mapa, partículas, odometria e seed) e roda só o resto (aqui, as 20 do λ):

```bash
nohup python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1 \
      --nome artigo_novo_a0.5 --reaproveitar experimentos/2026-10-17_artigo_novo_a1 --sem_figuras_seed \
      > a05.log 2>&1 &
```

### 4.3 α = 0,5, 5 seeds, reaproveitando o abBRIEF das 5 seeds

```bash
nohup python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1..5 \
      --nome artigo_novo_a0.5_5seeds --reaproveitar experimentos/2026-10-17_artigo_novo_a1_5seeds \
      > a05_5seeds.log 2>&1 &
```

Rodam 100 execuções (o λ); as 100 do abBRIEF são copiadas.

Cada execução copiada ganha um `REAPROVEITADO_DE.txt` com a origem. Sem `--reaproveitar`, o script procura sozinho a pasta de 1 seed mais recente com o mesmo protocolo e α (`*_artigo_<protocolo>_a<α>`). Use `--sem_reaproveitar` para rodar tudo.

### 4.4 Outras opções

| Opção | Efeito |
|---|---|
| `--seeds 1..30` | o protocolo completo do artigo (γ = 30). Também aceita `1,2,7` |
| `--dry_run` | mostra quantas execuções rodariam, quantas seriam copiadas e o tempo estimado |
| `--sem_figuras_seed` | não gera `<VOO>/seeds/` (uma figura por seed); mais rápido |
| `--continuar` | termina a pasta mais recente com o mesmo nome |
| `--plot_only` | só refaz figuras e tabela (implica `--continuar`) |
| `--flights`, `--jobs`, `--omp_threads`, `--particles`, `--nome`, `--out`, `--force` | iguais ao `paper_comparison.py` |

---

## 5. Acompanhar a execução

```bash
tail -f seeds.log                              # uma linha por execução: [i/N] seed voo mapa método: ok (tempo)
tail -3 seeds.log                              # progresso atual
grep -c ": ok" seeds.log                       # quantas terminaram
grep FALHOU seeds.log                          # falhas (veja o run.log da execução)
pgrep -af paper_comparison                     # o script ainda está vivo?
pgrep -c lambda_brief_loc                      # quantas execuções estão rodando agora (= --jobs)
```

- No fim, o log imprime `Tudo em <pasta> (comece por 00_LEIA_ME.md)`.
- Se o processo morrer, rode o mesmo comando com `--continuar`.
- Para ver uma execução em andamento: `tail <pasta>/execucoes/<VOO>/<mapa>/<método>_regime_final_baro/seed_<N>/run.log`.

---

## 6. O que sai e como ler

```
experimentos/<data>_artigo_novo_a<α>[_5seeds]/
├── 00_LEIA_ME.md               o que é cada arquivo, mapas e datas usados, avisos
├── COMANDO.txt                 os comandos que escreveram na pasta
├── compilado_artigo_ADM.png    como a Fig. 5 (ADM1, ADM2, ADM3); com N seeds, a curva média
├── compilado_artigo_UFRGS.png  como a Fig. 4
├── tabela.md / tabela.csv      números por voo e mapa (o .csv do multi-seed tem o MAE de cada seed)
├── <VOO>/<VOO>_compilado.png   todos os mapas do voo
├── <VOO>/<VOO>_mapa_<MM-AAAA>.png   ab × λ naquele mapa (N seeds: média, faixa min–max e seeds em
│                                    traço fino; embaixo, o mapa com o GT e a trajetória da seed de MAE mediano)
├── <VOO>/seeds/...             a figura de cada seed sozinha (só no multi-seed)
└── execucoes/<VOO>/<mapa>/<método>_regime_<regime>/seed_<N>/
        errors.txt  traj_estimated.txt  phase_log.txt  run.log  cmd.txt  [REAPROVEITADO_DE.txt]
```

### Colunas da `tabela.md` (multi-seed)

| Coluna | Significado |
|---|---|
| Convergiu ab / λ | seeds em que o erro ficou < 25 m por 10 frames seguidos (k/N) |
| Frame conv. | mediana do frame de convergência, entre as seeds que convergiram |
| MAE ab / λ | média ± desvio-padrão, entre seeds, do erro médio do voo |
| λ < ab | em quantas seeds o λ teve MAE menor (mesma seed nos dois métodos) |
| Rastreio | média, entre seeds, da mediana do erro depois de convergir |
| Final | média do erro no último frame |

No fim da tabela, o **resumo por voo** traz:

- em quantos mapas o λ teve MAE médio menor;
- quantos pares (mapa × seed) o λ venceu;
- o MAE médio de cada método;
- o p-valor do teste de Wilcoxon pareado sobre os 25 pares (5 mapas × 5 seeds).

### Erro e GT

- O erro é 2D (x, y), em metros. O artigo usava 3D, com a altitude.
- ADM1, ADM2 e UFRGS são avaliados contra o `traj_truth_sync.txt` (GPS do log no instante de cada imagem).
- O ADM3 é avaliado contra o `ADM3_ground_truth.txt` do Zenodo. As figuras dizem "GT do Zenodo".

---

## 7. Variações

```bash
# Protocolo do artigo (sem altímetro, odometria do legado, GT original), 30 seeds
python3 scripts/paper_comparison_seeds.py --protocolo artigo --seeds 1..30 --dry_run

# Protocolo novo sem altímetro (só câmera + VO)
python3 scripts/paper_comparison_seeds.py --protocolo novo --regime final_noalt --lambda_alpha 0.5

# Outro α, reaproveitando o abBRIEF de um teste com as mesmas seeds
python3 scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.75 \
      --reaproveitar experimentos/2026-10-17_artigo_novo_a1_5seeds
```

---

## 8. abBRIEF do legado: quanto do ganho do artigo vinha do baseline

No legado, o abBRIEF não era o λ-BRIEF com α = 0. Ele usava um padrão de 256 pares **fixo e quase uniforme** (`phir2frameworkv2/BriefHeuristic.cpp:1132–1154`), enquanto o λ concentrava os pares no centro, re-sorteava a cada frame e usava o NDVI (ver `achados_codigo_legado.md`).

A engine reproduz esse abBRIEF com `--pair_pattern legacy_ab`: mesmo `cv::RNG` na semente padrão, mesma variância usada como desvio-padrão, mesmo laço de rejeição. O padrão é calculado uma vez e usado em todos os frames. Ele ignora `--alpha` e `--seed`, como no legado.

| `--pair_pattern` | Pares |
|---|---|
| `roulette` (padrão) | roleta sobre a máscara combinada pelo α, sorteados de novo a cada frame |
| `roulette_fixed` | a mesma roleta, sorteada só no 1º frame e mantida |
| `uniform` | uniformes na imagem, sorteados de novo a cada frame (ignora `--alpha`) |
| `legacy_ab` | o abBRIEF do legado: um padrão fixo e quase uniforme (ignora `--alpha` e `--seed`) |

**Antes de rodar:** recompile a engine (`cmake --build build -j$(nproc)`); a suíte de testes tem a bateria 22 para o `legacy_ab`.

### 8.1 Com 1 seed

O script `scripts/ab_legado_comparison.py` roda o **abBRIEF legado** ao lado do **abBRIEF** da engine e do **λ**, em cada voo × mapa do artigo. Ele separa a diferença "ab legado → λ" (a comparação do artigo) em:

- **baseline** = MAE(ab legado) − MAE(ab);
- **NDVI** = MAE(ab) − MAE(λ).

As execuções do abBRIEF e do λ que já existem num teste do artigo com o mesmo protocolo, α e seeds são copiadas. Só o ab legado roda.

```bash
# Protocolo novo, α = 1, seed 1: copia as 40 de 2026-10-17_artigo_novo_a1 e roda só as 20 do ab legado
python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 \
        --reaproveitar experimentos/2026-10-17_artigo_novo_a1 --dry_run
nohup python3 scripts/ab_legado_comparison.py --protocolo novo --lambda_alpha 1.0 --seeds 1 \
        --reaproveitar experimentos/2026-10-17_artigo_novo_a1 > ab_legado.log 2>&1 &

# Protocolo do artigo (sem altímetro, odometria do legado, α = 1), seed 1: roda as 60 (nada para copiar ainda)
nohup python3 scripts/ab_legado_comparison.py --protocolo artigo --seeds 1 > ab_legado_artigo.log 2>&1 &
```

Para separar também o efeito de **re-sortear** e o de **centralizar** os pares, acrescente `--variantes legacy_ab uniform roulette_fixed`:

- re-sortear = MAE(ab legado) − MAE(ab uniforme);
- centralizar = MAE(ab uniforme) − MAE(ab).

São 20 execuções a mais por variante.

**Saída:** `experimentos/<hoje>_ab_legado_<protocolo>_a<α>/`

| Arquivo | Conteúdo |
|---|---|
| `00_LEIA_ME.md` | o que foi rodado e o resumo por voo |
| `tabela.md` | MAE, convergência, **Baseline**, **NDVI** e **Baseline / total** por mapa e por voo |
| `tabela.csv` | MAE de cada execução |
| `compilado.png` | erro por frame; uma linha por voo, uma coluna por mapa, uma curva por método |

O acompanhamento é igual ao da seção 5 (`tail -f ab_legado.log`). Com 5 seeds, use `--seeds 1..5 --reaproveitar experimentos/2026-10-17_artigo_novo_a1_5seeds`.

**Como ler.** Uma "Baseline / total" alta (por exemplo, acima de 50%) quer dizer que boa parte da vantagem do λ medida no artigo vinha de comparar com o abBRIEF fixo e uniforme, não da vegetação. Com 1 seed, leia como indício: uma única execução que não converge muda o MAE do mapa inteiro.

---

## 9. Problemas comuns

| Sintoma | Causa e solução |
|---|---|
| `[aviso] ADM3: mapa ... não encontrado` | falta o `Maps.zip` do Zenodo em `Datasets/ADM/0040_zenodo/Maps/` (passo 2.2) |
| `[aviso] <voo>: falta .../altimeter_baro.txt` | rode `extract_altimeter.py` ou, no ADM3, `adm3_altimeter_from_kml.py` (passo 2.3) |
| `[aviso] ADM1: falta .../traj_comp.txt` | o ADM1 precisa do `traj.txt` com os nomes de imagem corrigidos |
| `Odometria fixa indisponível` | a geração automática falhou; veja `insumos/odom/<DS>_vo_ratio.log` |
| uma execução `FALHOU (exit N)` | veja o `run.log` dela; depois rode de novo com `--continuar` |
| nada foi copiado com `--reaproveitar` | o comando mudou (binário, partículas, odometria ou caminho); o script só copia comandos idênticos |
| pasta antiga não encontrada | `python3 scripts/experimentos.py onde <caminho antigo>` |
| `Error: stability map is AxB but the satellite map is CxD` | o raster da v3 foi gerado para outro mapa; gere de novo com `--mapa <o mapa usado>` (seção 10.1) |
| `Unknown option --stability_map` ou o `[v3]` não aparece no `run.log` | a engine não foi recompilada depois de atualizar o código (seção 10) |
| `S2B sem raster para ...` | não há cena S2 a até `--tol` dias da data do mapa; a variante B só existe para mapas a partir de 08/2018 no repositório |

---

## 10. v3: peso de estabilidade do Sentinel-2 (branch `v3`)

**Ideia.** Cada par de pixels do descritor recebe um peso pela estabilidade da região do mapa onde ele cai, medida no Sentinel-2. A distância deixa de ser a fração simples de bits diferentes e vira uma média ponderada:

- `d = Σ wᵢ·eᵢ / Σ wᵢ` (fórmula **normalized**, padrão), ou
- `d = Σ [wᵢ·eᵢ + (1 − wᵢ)·0,5] / P` (fórmula **coin**: o par instável vale uma moeda; ablação).

Nas fórmulas:

- `eᵢ` é a fração de bits diferentes do par;
- `wᵢ = min(W(Aᵢ), W(Bᵢ))`, com `W` lido do raster nos dois pontos projetados no mapa;
- um par fora do mapa conta `e = 1, w = 1`, como hoje.

O peso entra na **verossimilhança**, não no sorteio dos pares: os pares são sorteados antes de saber a pose, e a estabilidade está nas coordenadas do mapa. Sem `--stability_map`, a engine é idêntica à v2. Com todos os pesos iguais a 1, as duas fórmulas dão a mesma distância de hoje (bateria 23 da suíte de testes).

| Código | O que mudou |
|---|---|
| `include/core/descriptor.hpp`, `src/core/descriptor.cpp` | `StabilityFormula`, `PairWeightMap`, `Descriptor::evaluateWeightedParticleDifference` |
| `include/localization/particle_filter.hpp`, `src/localization/particle_filter.cpp` | `ParticleFilter::setPairWeightMap`; `updateWeights` usa a distância ponderada (partículas e sondas do contraste) |
| `include/config/config.hpp`, `src/config/config.cpp` | `--stability_map <png>` e `--stability_formula normalized\|coin` |
| `apps/run_localization.cpp` | carrega o raster, confere o tamanho, imprime `[v3] Stability weights: ...`; o diagnóstico `LB_DEBUG_PF` também usa o peso |
| `tests/test_suite.cpp` | bateria 23: w = 1 reproduz a v2; normalized e coin conferidos contra a conta à mão; `updateWeights` usa o peso |
| `scripts/make_stability_map.py` | gera o raster de peso (variantes A e B) |
| `scripts/v3_comparison.py` | roda ab e λ com e sem o peso nos voos ADM × 5 mapas |
| `scripts/v3_video.py` | vídeos do drone, do mapa, do Sentinel-2 e dos pares (`--save_pairs` da engine) |

**Antes de rodar:**

```bash
cmake --build build -j$(nproc) && ./build/tests/test_suite     # 96 passed, 0 failed
```

### 10.1 O raster de peso: `make_stability_map.py`

Ele lê as cenas de `Datasets/Sentinel2/ADM` (as marcadas `kept` no `index.csv`) e junta os dois tiles da mesma data. Também mascara nuvem e sombra pelo SCL. Cada pixel do mapa vai para a sua célula S2 de 10 m pela afim de `sentinel2/map_georef.json` (chave `ADM_35`, erro de 0,13 m). A saída é um PNG 8-bit do tamanho do mapa, com 255 = peso 1.

| Variante | Mede | Peso |
|---|---|---|
| **A**, estabilidade | desvio-padrão do NDVI de cada célula ao longo do tempo (padrão: todas as datas, 2018–2026) | 1 até `--lo 0.08`; `1 − (σ − lo)/(hi − lo)` até `--hi 0.22`; nunca abaixo de `--wmin 0.05` |
| **B**, mudança | \|ΔNDVI\| entre o S2 perto da data do mapa (lida do nome, `dd_mm_aaaa`) e o S2 perto do voo (`--data_voo`, padrão 2019-04-18) | igual, com `--dlo 0.10` e `--dhi 0.35` |

A variante A não depende da data do mapa: o mesmo raster serve para os 5 mapas ADM_35, que têm a mesma geometria, inclusive os do Zenodo do ADM3. A variante B só existe para mapas com cena S2 a até `--tol 20` dias. No teste do artigo, isso vale só para o 12-2018.

```bash
python3 scripts/make_stability_map.py --modo A                       # insumos/estabilidade/ADM_35_A.png
python3 scripts/make_stability_map.py --modo A --janela 2018-08-01:2019-05-31 --nome ADM_35_A_2018-2019
python3 scripts/make_stability_map.py --modo B --mapa ADM_35_17_12_2018.jpg   # ADM_35_B_2018-12-17_2019-04-18.png
```

Cada raster sai com:

- um `.json` ao lado: parâmetros, datas usadas, peso médio e fração do mapa com peso < 0,5;
- um `_preview.png`: o mapa e o peso sobreposto; escuro = estável.

Validação: com só o tile JDN (`--tiles JDN`, as 42 datas do protótipo de 27/10), o raster é igual ao do protótipo a menos do arredondamento para 8 bits (diferença máxima de 0,002).

### 10.2 Teste: `v3_comparison.py`

O script segue o mesmo esquema do `ab_legado_comparison.py`:

- ab e λ (α = 0,5, protocolo novo) são copiados de `experimentos/2026-10-17_artigo_novo_a0.5` (ou da pasta `_5seeds`) quando o comando é idêntico;
- rodam só as versões com peso;
- os rasters que faltam são gerados na hora.

```bash
# teste rápido (~4 min): só ab+S2A, ADM2 e ADM3 × mapas 12-2018 e 01-2014 = 4 execuções novas
# (o ab copiado ao lado); serve para ver se a v3 roda e se o sinal vai na direção certa
python3 scripts/v3_comparison.py --seeds 1 --flights ADM2 ADM3 --mapas 12-2018 01-2014 --metodos_s2 ab
# peso mais suave (par instável vale no mínimo 0,5): gera insumos/estabilidade/ADM_35_A_wmin0.5.png na hora;
# --lo e --hi mudam a rampa do mesmo jeito. A pasta ganha o sufixo _wmin0.5
python3 scripts/v3_comparison.py --seeds 1 --flights ADM2 ADM3 --mapas 12-2018 01-2014 --metodos_s2 ab --wmin 0.5

# seed 1: 30 execuções novas (ab+S2A e λ+S2A em ADM1–3 × 5 mapas), ~25 min com --jobs 4
python3 scripts/v3_comparison.py --seeds 1 --dry_run
nohup python3 scripts/v3_comparison.py --seeds 1 > v3.log 2>&1 &

# se promissor: 5 seeds (copia ab e λ de 2026-10-17_artigo_novo_a0.5_5seeds)
nohup python3 scripts/v3_comparison.py --seeds 1..5 > v3_5seeds.log 2>&1 &

# ablações
python3 scripts/v3_comparison.py --seeds 1 --formulas normalized coin --metodos_s2 ab   # fórmula
python3 scripts/v3_comparison.py --seeds 1 --pesos A B                                 # variante B (só 12-2018)
```

Para testar outra janela ou outros limiares, gere o raster com outro `--nome` e passe-o à mão a uma execução (`--stability_map insumos/estabilidade/<nome>.png`), ou substitua o `ADM_35_A.png` e rode com `--nome` próprio no `v3_comparison.py`.

**Saída:** `experimentos/<hoje>_v3_novo_a0.5[_<N>seeds]/`

| Arquivo | Conteúdo |
|---|---|
| `00_LEIA_ME.md` | métodos, opções da engine e resumo por voo |
| `tabela.md` | MAE e convergência por mapa e por voo; ganho do S2 pareado por mapa e seed, com Wilcoxon; o critério mínimo |
| `tabela.csv` | cada execução |
| `compilado.png` | erro por frame; uma linha por voo, uma coluna por mapa |

### 10.3 Vídeos: o que o drone vê, o que o Sentinel-2 vê e onde caem os pares

`scripts/v3_video.py` roda a engine em **um** voo × mapa com `--save_pairs` e `--save_particles` e monta três vídeos 1920×1080 por método (escolha com `--vistas voo drone mapa`).

`--save_pairs` grava, sem mudar o resultado (o `errors.txt` sai idêntico):

- `pairs_log.csv`: os 256 pares de cada frame, projetados na estimativa e na pose do GT, com o erro do bit e o peso;
- `frames_log.csv`: as imagens usadas, a estimativa, o GT, d(GT), d(estimativa) e a fase;
- `amostragem/`: a máscara da roleta e o NDVI do drone, em meia resolução.

A pose do GT usa a escala do altímetro (`gain·z_baro`, a mesma referência do prior de escala). Sem altímetro, usa `0,87·z_GT`.

| Vídeo | Painéis |
|---|---|
| `video_<método>_voo.mp4` | **o voo**: à esquerda, o mapa grande com a trilha do GT (verde) e da estimativa (laranja), as pegadas e a nuvem de partículas (amarelo) convergindo, com o zoom acompanhando a nuvem; com `--fundo peso`, o mapa aparece misturado com o peso S2-A. À direita, os 6 painéis da vista do drone, menores |
| `video_<método>_drone.mp4` | tudo **na orientação da câmera**: 1 drone RGB (quantizado, como o descritor vê) + pares · 2 mapa visto da pose do GT · 3 S2 RGB do dia do voo visto da pose do GT · 4 drone NDVI + pontos sorteados + máscara da roleta · 5 S2 NDVI · 6 peso S2-A |
| `video_<método>_mapa.mp4` | **norte para cima**, recorte em volta do GT: 1–2 como acima · 3 mapa · 4 S2 RGB · 5 S2 NDVI · 6 peso S2-A com as partículas e a visão geral do voo |

Embaixo dos painéis fica o erro por frame (log, azul), o peso médio dos pares e a fase do filtro.

Cores:

- **Par:** a cor é o peso S2-A onde ele cai no mapa, na pose do GT: verde é estável, vermelho é instável. No ab puro, o peso só é exibido.
- **Ponta do par:** branca quando o bit concorda com o drone na pose do GT, magenta quando discorda.
- **Pegada e trilha:** verde é o GT, laranja é a estimativa. Partículas em amarelo.

```bash
# ab e ab+S2A no ADM2 · 01-2014 (2 execuções em paralelo, ~4 min, + ~4 min de vídeo)
python3 scripts/v3_video.py --voo ADM2 --mapa 01-2014
# só o trecho misto (a engine roda o voo inteiro, porque o filtro depende do passado)
python3 scripts/v3_video.py --voo ADM2 --mapa 01-2014 --frames 80:120 --continuar
# ADM3 com o λ junto, peso suave
python3 scripts/v3_video.py --voo ADM3 --mapa 01-2014 --metodos ab ab_s2a lambda --wmin 0.5
# só renderizar uma execução que já tem pairs_log.csv
python3 scripts/v3_video.py --run_dir <execução> --map <mapa.jpg> --stability_map insumos/estabilidade/ADM_35_A.png
```

Saída: `experimentos/<hoje>_v3_video_<VOO>_<mapa>[_wmin…]/`, com:

- os vídeos;
- `quadros/<método>/{drone,mapa}/` (um PNG a cada `--png_cada 10` frames, para slides);
- `area_sentinel.png` (a área do voo: mapa, S2 RGB e NDVI do dia do voo, peso e GT).

**Como ler.** As hipóteses da proposta:

- **H1:** ab+S2A perto do λ no ADM2.
- **H2:** ab+S2A ≈ ab no ADM3.
- **H3:** o rastreio urbano não piora.

O **critério mínimo** para seguir é ab+S2A convergir pelo menos tanto quanto o melhor entre ab e λ, em cada voo. Com 1 seed são 5 pares por voo, e o menor p possível no Wilcoxon é 0,06: leia como indício.


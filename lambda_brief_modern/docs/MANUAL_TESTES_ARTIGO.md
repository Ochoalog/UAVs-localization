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

---

## 10. v3 (peso de estabilidade do Sentinel-2): só no branch `v3`

A v3 ponderava cada par pela estabilidade do mapa medida no Sentinel-2 (`--stability_map`). O resultado foi negativo com 5 seeds (resumo em `docs/RESUMO_PARA_NOVO_AGENTE.md`, seção 2.6), então ela não entra na `main`. Para rodar ou consultar: `git switch v3` e a seção 10 deste manual naquele branch.

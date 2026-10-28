# Plano de implementação — H0: de onde vem o piso de 8–15 m do rastreio

28/10/2026. Contexto e motivação: projeto "UAVs" no claude.ai, `claude/teses_e_hipoteses_icuas2027.md` (seção H0).
Este plano é para a branch `main` (abBRIEF e λ-BRIEF; a v3 ficou no branch `v3`).

## 0. Pergunta e decisão

Depois de convergir, ab e λ rastreiam a ~8–15 m. O H0 mede **quanto desse erro é do filtro, quanto é do descritor e
quanto é do próprio dado** (GT, inclinação da câmera, georreferência do mapa). Para cada frame t:

```
erro_total = |P_filtro − P_brief|   → FILTRO     (estimativa do filtro × mínimo da distância de Hamming)
           + |P_brief  − P_centro|  → DESCRITOR  (mínimo do BRIEF × onde o centro da imagem cai segundo um matcher forte)
           + |P_centro − G|         → DADO       (matcher forte × GT)
```

| Cenário | DADO | DESCRITOR | Leitura | Próximo passo |
|---|---|---|---|---|
| A | grande (≥ 6 m, sistemático) | pequeno (≤ 3 m) | o piso é do dado; o BRIEF já está no teto | tese de custo (H1); reportar erro também contra o GT do centro da imagem |
| B | pequeno (≤ 4 m) | grande (≥ 5 m) ou vale largo (> 20 m) | o piso é do descritor | deep-BRIEF mira também o rastreio |
| C | pequeno | pequeno, mas FILTRO ≥ 5 m | o piso é do filtro | ajustar ruído/estimador; não é contribuição de descritor |

Os limiares são de partida; o que se reporta é a mediana e o p90 de cada termo por voo × mapa.

**Ponte para o deep-BRIEF:** a ferramenta da superfície (seção 3) vira a bancada offline do deep-BRIEF. Trocar a imagem
(RGB → features) e comparar a superfície (deslocamento do mínimo, largura do vale, contraste) antes de rodar o filtro inteiro.

## 1. Definições (conferir antes de codar)

| Símbolo | O que é | De onde vem |
|---|---|---|
| P_filtro | média ponderada da nuvem | `errors.txt` / `frames_log.csv` (`est_x, est_y`) |
| P_brief | argmin de d(x, y) numa grade em volta do GT, com yaw do GT e z de referência | `diag_surface` (nova, seção 3) |
| P_centro | ponto do mapa onde cai o **centro da imagem** segundo a homografia drone → mapa do matcher forte | `h0_matcher.py` (seção 4) |
| P_câmera | posição horizontal da câmera (nadir), pela decomposição da homografia com K | `h0_matcher.py` |
| G_gps | GPS sincronizado | `traj_truth_sync.txt` (ADM1/2, UFRGS); ADM3: GT do Zenodo |
| G_centro | GPS + deslocamento da inclinação (h·tan θ) = onde o centro da imagem cai no chão | `h0_gt_centro.py` (seção 2) |

- **A partícula representa o centro da imagem projetado** (na projeção, `m = R(θ)·((u,v) − centro)·z + (x,y)`). Logo o GT
  "justo" para o filtro e para o descritor é G_centro, não G_gps. Hoje o erro é medido contra G_gps.
- `z` é px do mapa por px do frame. z de referência = `gain·z_baro` (o mesmo do `--save_pairs`) ou 0,87·z_GT.
- Inconsistência a resolver na etapa 0: o `altimeter_baro.txt` do 0036 usa f_px = 1029 e GSD = 0,1807 m/px; a engine usa
  `--pixel_to_meter 0.188679245` (4% de diferença). Não muda posição, muda a escala z e a altura h da decomposição.

## 2. Etapa 0 — GT e ângulos (½–1 dia)

Script novo: `scripts/h0_gt_centro.py --voo ADM2`

1. Ler `traj_truth.txt`, `traj_truth_corr.txt`, `traj_angles.txt` (roll, pitch, yaw em graus; 196 linhas, mesma indexação do
   `traj_truth.txt`), `traj_truth_sync.txt` e `altimeter_baro.txt`.
2. **Entender o `traj_truth_corr.txt`:** ajustar Δ = corr − truth ao modelo h·[tan(pitch), tan(roll)] girado pelo yaw
   (testar as 4 combinações de sinal e a troca roll↔pitch; h do barômetro). Reportar o resíduo. Se o modelo fechar, sabemos
   como ele foi feito; se não fechar, não usar o arquivo.
3. **Atraso:** `traj_truth.txt` e `traj_angles.txt` estão ~2 frames atrasados (armadilha 13). Aplicar o mesmo deslocamento aos
   ângulos e conferir pela correlação do yaw dos ângulos com o Δyaw da VO (`check_gt_alignment.py` já faz isso para o GT).
4. Gerar `G_centro` = G_gps (sync) + deslocamento da inclinação com os ângulos sincronizados →
   `insumos/gt_centro/<VOO>_gt_centro.txt` (mesmo formato do `traj_truth_sync.txt`: x y z yaw_deg, px do mapa).
5. Reavaliar sem rodar nada (como em `check_gt_alignment.py`): MAE e mediana de rastreio das execuções de
   `experimentos/2026-10-17_artigo_novo_a0.5_5seeds` contra G_gps e contra G_centro. **Só isso já pode responder parte do H0.**

Voos: ADM2 primeiro (tem tudo). Depois ADM1 e UFRGS se tiverem `traj_angles.txt`; ADM3 pelo KML/Zenodo, se houver atitude.
Sem ângulos, o voo entra só com G_gps.

## 3. `diag_surface` — superfície de verossimilhança do BRIEF (C++, 1 dia)

App novo `apps/diag_surface.cpp` (alvo no `CMakeLists.txt`), mesmas opções do `lambda_brief_loc` mais:

| Opção | Padrão | O que faz |
|---|---|---|
| `--pairs_log <csv>` | — | usa exatamente os pares de uma execução (`pairs_log.csv` do `--save_pairs`); sem ela, sorteia como a engine |
| `--pair_draws K` | 1 | sem `--pairs_log`: K sorteios independentes de pares por frame (incerteza do termo DESCRITOR) |
| `--gt_ref <txt>` | GT do loader | centro da grade (ex.: `G_centro`) |
| `--radius_m` / `--step_px` | 40 / 2 | grade ±40 m (±212 px no ADM) com passo de 2 px → ~45 mil avaliações por frame |
| `--refine` | on | no argmin, varre yaw ±10° (2°) e z ×0,8–1,2 (0,02) e refaz a grade fina |
| `--png_every N` | 0 | salva o mapa de calor da superfície a cada N frames |

Saída `surface_summary.csv`, uma linha por frame (× sorteio):
`frame, draw, d_gt, d_min, dx_m, dy_m, off_m, yaw_off_deg, z_fac, vale_m2 (área com d < d_min + 0,01), eixo_maior_m,
eixo_menor_m, contraste ((média aleatória − d_min)/desvio aleatório), frac_pares_validos`.

Implementação: reaproveitar `Descriptor::evaluateParticleDifference` (o mesmo código do filtro), OpenMP na grade.
Custo esperado: 45 mil × 256 pares ≈ 0,1–0,3 s por frame e sorteio.

Teste novo na suíte (bateria 24): drone = recorte do mapa sintético → argmin exatamente na pose verdadeira, d_min = 0,
superfície simétrica; com o recorte deslocado em (dx, dy), o argmin desloca igual.

## 4. `h0_matcher.py` — matcher forte, P_centro e P_câmera (Python, 1½ dia)

`scripts/h0_matcher.py --voo ADM2 --mapa 12-2018 --metodos sift lightglue roma [--frames 0:196] [--device cuda|cpu]`

Por frame:

1. **Pré-alinhamento com o oráculo:** com (x, y, yaw, z_ref) de G_centro, montar a similaridade W0 drone → mapa com a mesma
   fórmula da engine e recortar o mapa em volta (pegada + margem de 150 m). Reamostrar a imagem do drone por W0 (norte para
   cima, escala do mapa). Isso tira rotação e escala do problema do matcher.
2. **Casamento:** SIFT + ratio + MAGSAC (referência barata), SuperPoint + LightGlue, RoMa (denso). Imagem do drone:
   `Images_RGB` (a mesma da engine, sem quantizar).
3. **Homografia residual** H_res com `cv2.findHomography(..., cv2.USAC_MAGSAC, 3.0)`. Aceitar o frame se inliers ≥ 50,
   razão de inliers ≥ 0,2 e H_res perto de uma similaridade (escala 0,7–1,4, rotação < 20°). H = H_res ∘ W0.
4. **P_centro** = H · (centro da imagem). Comparar com P_brief e G_centro.
5. **P_câmera:** passar H para metros no chão (GSD do mapa), decompor H ~ K[r1 r2 t] (plano Z = 0) e tirar C = −Rᵀt:
   posição (x, y), altura h, roll e pitch. Comparar com G_gps, com o barômetro e com `traj_angles.txt`.
   K: f_px = 1029 e ponto principal no centro (o RGB foi recortado e redimensionado no registro; ver `parâmetros` do dataset).
   A altura h estimada × barômetro valida K e a escala.
6. Tempo por frame de cada método (vira o "padrão-ouro" de custo do H1).

Saídas: `matcher_<metodo>.csv` (`frame, n_matches, n_inliers, rmse_px, cen_x, cen_y, cam_x, cam_y, cam_h, roll, pitch,
ms`) e `sobreposicao/` com o drone reprojetado sobre o mapa em 10 frames por voo (checagem visual obrigatória).

Testes do script (`--autoteste`): recorte do mapa deformado por uma homografia conhecida → centro recuperado com erro < 0,5 px;
câmera sintética com pose conhecida → C recuperado com erro < 1%.

Ambiente: `.venv-h0` com `torch`, `kornia`, `lightglue` (GitHub cvg/LightGlue) e `romatch`. Conferir GPU com `nvidia-smi`.
Sem GPU: LightGlue ~1 s/frame, RoMa ~10–30 s/frame (ADM2 × 2 mapas com RoMa ≈ 1–2 h; rodar à noite). Alternativa: enviar só os
~200 RGB do voo e os mapas para a nuvem e rodar lá.

## 5. Bônus — registro entre mapas (½ dia)

`scripts/h0_registro_mapas.py --base 12-2018`: ladrilhos de 1024 px (passo 512) de cada mapa ADM contra o mapa base,
LightGlue/RoMa, vetor de deslocamento por ladrilho. Saída: tabela (mediana, p90 do deslocamento por mapa) e figura com o campo
de vetores. Se os mosaicos estiverem deslocados alguns metros entre si, isso explica parte da dependência do mapa e entra no
termo DADO.

## 6. `h0_decomposicao.py` — junta tudo (1 dia)

`scripts/h0_decomposicao.py --exp experimentos/<data>_h0_ADM2`

- Junta, por frame: `frames_log.csv` da engine, `surface_summary.csv`, `matcher_*.csv`, G_gps e G_centro.
- Calcula os três termos (com G_gps e com G_centro), separa fase GLOBAL × TRACKING (`phase` do `frames_log.csv`) e marca os
  trechos fracos conhecidos (ADM2 ~55–60, ~90–110, pouso).
- Separa viés (vetor médio, sistemático) de espalhamento (aleatório) em cada termo.
- `tabela.md`: mediana e p90 de cada termo por voo × mapa × método; taxa de frames em que o matcher falha por trecho;
  ms/frame de cada matcher × BRIEF.
- `decomposicao.png`: as três curvas por frame (uma linha por mapa) e barras empilhadas por voo.
- Linha automática em `experimentos/INDICE.md` e entrada no `HISTORICO_RESULTADOS.md`.

## 7. Execuções

| Passo | O quê | Tamanho |
|---|---|---|
| 1 | engine com `--save_pairs`, regime `final_baro`, ADM2 × {12-2018, 01-2014} × {ab, λ α = 0,5}, seed 1 | 4 execuções, ~4 min em paralelo |
| 2 | `diag_surface` com `--pairs_log` dessas 4 + `--pair_draws 5` sem pairs_log | ~2 × 196 frames × 5 sorteios, minutos |
| 3 | `h0_matcher.py` ADM2 × 2 mapas × {sift, lightglue, roma} | ~1–2 h sem GPU |
| 4 | `h0_decomposicao.py` | segundos |
| 5 | se informativo: ADM1, ADM3, UFRGS × 5 mapas (termo FILTRO com 5 seeds; DESCRITOR e DADO não dependem da seed do filtro) | ~1 dia de máquina |

Pasta: `experimentos/<data>_h0_<VOO>/` (`scripts/experimentos.py nova h0_ADM2 "..."`).

## 8. Cronograma (~5 dias)

| Dia | Entrega |
|---|---|
| 1 | etapa 0 (`h0_gt_centro.py`, reavaliação contra G_centro) + execuções do passo 1 |
| 2 | `diag_surface` + bateria 24 da suíte |
| 3 | `h0_matcher.py` com SIFT e LightGlue, decomposição da homografia, autoteste e sobreposições |
| 4 | RoMa + `h0_decomposicao.py` + figuras |
| 5 | registro entre mapas + escrita (HISTORICO, doc do projeto) + decisão A/B/C |

## 9. Riscos

- **Matcher falha entre datas** na lavoura: usar o 12-2018 como base; a taxa de falha por trecho é um resultado (mostra onde
  nenhum método casa).
- **K incerto** (imagem recortada/redimensionada no registro): P_centro não precisa de K; só P_câmera precisa, e a altura
  estimada × barômetro denuncia K errado.
- **Sinais de roll/pitch** desconhecidos: resolvidos empiricamente na etapa 0 (e conferidos pela decomposição da homografia).
- **Circularidade:** P_centro explica o piso; não vira "GT novo" para comparar métodos de imagem sem discutir isso no artigo.

## 10. Depois do H0: deep-BRIEF

Preparação que sai daqui:

- `--rgb_dir <pasta>` na engine e no `diag_surface`: troca a imagem RGB de cada frame por outra com o mesmo nome de arquivo
  (as imagens "pseudo-RGB" de features). Junto com `--map <mapa de features>` e `--quantize 0`, roda o deep-BRIEF sem mudar o
  descritor.
- `diag_surface` compara RGB × features frame a frame (deslocamento do mínimo, largura do vale, contraste) antes de gastar
  execuções do filtro.

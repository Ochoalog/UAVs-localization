# λ-BRIEF Modern: revisão da odometria visual

*Análise de 12/10/2026 sobre `lambda_brief_modern/` (VO em `visual_odometry.cpp`, uso em `run_localization.cpp` e `ParticleFilter::predict`). Complementa `diagnostico_convergencia_e_pipeline.md`. As mudanças da seção 3 foram **aplicadas** no repositório, e a validação no filtro está na seção 3.1 (`lambda_brief_modern/results_odometria_vo/`).*

---

## 0. Resumo

| | |
|---|---|
| **Pergunta** | Dá para melhorar a odometria da engine moderna? |
| **Resposta curta** | A VO em si já é boa: a escala de translação bate com o pinhole (ganho 0,88 contra os 0,87 previstos) e o erro de yaw é < 1°/frame. Trocar o matcher melhora pouco a precisão (−4% de deriva), mas muito a robustez e o tempo. **O que mais pesa não está no código da VO, e sim no GT**: o `traj_truth.txt` está ~2 frames atrasado em relação às imagens, e o z do GT no fim do voo não bate com as imagens |
| **Achado 1 (GT atrasado ~2 frames)** | Três testes independentes apontam o mesmo deslocamento: yaw (corr. 0,44 → 0,74), velocidade (0,55 → 0,66) e deriva de 10 frames (8,9 → 5,4 m). Reavaliando os resultados existentes com o GT deslocado, a mediana de rastreio cai de **15–16 m para 11–12 m** nas 25 sementes que convergiram (`results_2fases_s1`: média 28,2 → 23,2 m). Correção: `--gt_offset 86` no ADM/0036 |
| **Achado 2 (z do GT no fim do voo)** | Nos frames 160–195 o GT diz que z cai pela metade (×0,515). A VO mede ×1,0–1,23 (homografia 1 a 1, keyframes de 5, similaridade, imagens originais). A translação também só fecha com o GPS se z ficar ~constante (ganho 1,24 com z_GT, 1,02 com z constante). Isso explica por que o prior de altímetro emulado do GT atrapalha no pouso |
| **Achado 3 (matcher frágil)** | A heurística `dist ≤ max(2·d_min, 0,02)` mantém ~50 de ~15 mil matches. Em **27 de 195** transições ficam < 20 inliers (mínimo 0). Com o ratio test de Lowe + checagem mútua, ficam ≥ 837 (1500 com 4000 features) e todas as 195 transições ficam confiáveis |
| **Achado 4 (custo)** | O SIFT do frame anterior é recalculado a cada frame (~0,6 s). Com cache, a trajetória fica **bit-idêntica** e o tempo cai de 1,86 para 1,31 s/frame. Com ratio + 4000 features, cai para 0,72 s/frame (2000 partículas, 2 núcleos) |
| **Não vale a pena** | Escala da VO (Δz): continua sem correlação com a altitude, mesmo com milhares de inliers e nas imagens originais. Manter `--vo_scale_deadband 1`. Correção de inclinação (roll/pitch) na VO: depois de alinhar o tempo, piora (5,4 → 6,4 m). Fallback de velocidade constante: só 4 transições não confiáveis, efeito marginal |
| **A testar no filtro** | O ruído de predição U(±10 px) tem σ = 5,8 px. O erro real da VO por passo tem σ ≈ 10 px (o GPS é suave, com 2ª diferença de σ ≈ 2,3 px). Com `--pos_noise 17` (mesmo σ ≈ 10 px) e a VO nova, o erro médio no filtro caiu 15% (seção 3.1) |

---

## 1. Como foi medido

- 196 frames RGBres do ADM2 (0199–0394), `traj_truth.txt`, `traj_truth_corr.txt` e `traj_angles.txt`. Mais 36 imagens originais (0359–0394) para checar a escala.
- Benchmark offline em Python/OpenCV 5 com as mesmas fórmulas da engine (9 pontos canônicos, 4 checagens geométricas, Δx = −(c′₄ᵧ − c₄ᵧ), Δy = c′₄ₓ − c₄ₓ, Δψ por c₅ − c₄).
- Referência por transição: deslocamento do GPS girado pelo yaw do GT e dividido por z = 0,87·z_GT.
- Métrica principal: **deriva de 10 frames**. Parte da pose do GT, integra só a VO (com z do GT) e mede o erro no fim. Ela filtra o ruído de passo único do GPS.
- Depois, a engine C++ foi compilada (OpenCV 4.6) com as mudanças, testada (82 checks, 21 baterias) e rodada nos frames reais.

---

## 2. Achados

### 2.1 O GT está ~2 frames atrasado em relação às imagens

Correlação entre a VO da transição k e o GT da transição k + L (VO da própria engine, `LB_DEBUG_ODOM`, 195 transições):

| L | −4 | −3 | **−2** | −1 | 0 | +1 | +2 |
|---|---|---|---|---|---|---|---|
| corr. Δyaw (VO × bússola do GT) | 0,31 | 0,74 | **0,74** | 0,64 | 0,44 | 0,37 | 0,39 |
| corr. velocidade (VO × GPS) | 0,59 | 0,63 | **0,66** | 0,64 | 0,55 | 0,45 | 0,37 |

Deriva de 10 frames (VO legada) com deslocamento fracionário: mínimo em L = −2,25, onde a mediana cai de 8,9 m para 5,4 m. O yaw e a posição vêm de sensores diferentes (bússola/IMU e GPS) e concordam. Isso indica pareamento de timestamps, e não ruído de um sensor.

**Interpretação:** a imagem t corresponde à linha t − 2 do `traj_truth.txt`. Com `--start_frame 84`, a linha 0 do GT pertence à imagem 0201, e não à 0199. No loader: `--gt_offset 86`.

**Impacto nos resultados já existentes** (reavaliação só do erro, sem rodar de novo):

| Execução | Média L=0 | Média L=−2 | Mediana de rastreio L=0 | L=−2 |
|---|---|---|---|---|
| `results_2fases_s1` | 28,2 m | 23,2 m | 14,1 m | 11,5 m |
| benchmark λ-BRIEF 12/09/2018, 25 sementes convergidas | — | ~−5 m em cada | 13,8–16,6 m | **10,7–12,8 m** |

A mesma correção vale para os priors emulados (`--scale_prior_*`, `--yaw_prior_sigma`), que hoje leem o GT 2 frames adiantado.

**Ressalvas.**
- Isso afeta a avaliação e os priors, e não a VO.
- Antes de mudar o padrão do loader, vale confirmar pelos timestamps do log de voo ou do EXIF.
- No artigo, a métrica precisa ser a mesma para o legado e para a engine moderna.
- `traj_truth_corr.txt` (corrigido pela inclinação) com L = 0 dá 7,6 m. Somar a correção de inclinação ao alinhamento piora (5,4 → 6,4 m). O desalinhamento temporal é o efeito dominante, e não a inclinação.

### 2.2 O z do GT no fim do voo é inconsistente com as imagens

| Trecho | z_GT (fim/início) | VO 1 a 1 (H) | VO keyframe 5 (H) | VO similaridade | Originais (H) |
|---|---|---|---|---|---|
| 160 → 195 | **×0,515** | ×1,20 | ×1,23 | ×1,03 | ×1,23 |
| 80 → 160 | ×0,865 | ×1,05 | ×1,09 | — | — |

Teste independente da escala da VO: com o GT alinhado, o ganho que leva a translação da VO até o GPS fica em 0,81–0,95 em todo o voo, e sobe para **1,24 nos frames 175–195 quando se usa z_GT**. Com z constante desde o frame 175, o ganho fica em 1,02. Os dois sinais (escala da homografia e magnitude da translação) dizem que a câmera **não** desceu como o GT indica. O EXIF das originais mostra 131,5 → 114,7 m MSL nesse trecho (−17 m, contra −36 m implícitos no z_GT).

**Consequência:** o prior de altímetro emulado do z_GT empurra z para baixo ~4%/frame no pouso. Isso explica as perdas no fim do voo da seção 2.8 do diagnóstico, e também por que o `--two_phase` (sem prior no rastreio) termina bem. Vale conferir a altitude no log do barômetro antes de usar z_GT como sensor.

### 2.3 Matcher legado frágil

| Variante | Inliers (mediana) | Transições < 20 inliers | Confiáveis | Deriva 10 fr. (GT alinhado) mediana / p90 | Δyaw mediana / p90 |
|---|---|---|---|---|---|
| Legado (quantizado 25, 1-NN + d_min) | 51 | 27 | 191/195 | 6,92 / 13,9 m | 0,77° / 4,3° |
| Legado sem quantização | 51 | — | 190/195 | 7,04 / 15,2 m | 0,75° / 4,4° |
| Ratio + mútuo + MAGSAC 2 px, SIFT ilimitado | 4649 | 0 | 195/195 | **6,64 / 13,4 m** | 0,74° / 4,5° |
| Ratio + mútuo + MAGSAC 2 px, **4000 features**, quantizado | 1506 | 0 | 195/195 | 6,66 / 13,4 m | 0,73° / 4,5° |
| Similaridade (4 DOF) em vez de homografia | 2211 | — | 195/195* | 6,85 / 15,0 m | 0,68° / 4,5° |

\* O critério de razão de inliers precisa ser relaxado: a perspectiva da inclinação derruba a razão para 10–20%.

- A quantização em 25 níveis **não** prejudica a VO de forma mensurável. Pode continuar, por fidelidade.
- A similaridade não traz ganho sobre a homografia com o ponto central.
- O ganho do ratio test está na **margem**: o legado vive com 15 inliers no p10 e 0 no pior caso.

### 2.4 Escala da VO continua inútil

Produto de Δz em 195 transições: ×2,37 (legado), ×2,33 (ratio + MAGSAC) e ×1,52 (similaridade), contra ×0,37 do GT. A correlação passo a passo fica entre −0,08 e +0,04, e em janelas de 10 frames entre −0,06 e −0,19. Mesmo resultado nas imagens originais, então não é efeito do registro RGB → multiespectral. **Mantenha `--vo_scale_deadband 1`** (e `--track_vo_scale_deadband 1` no `--two_phase`).

### 2.5 Ruído de predição subdimensionado

Erro da VO por passo contra o GPS alinhado, em px do mapa: σ = (10,7; 9,9), mediana |e| = 7 px, para um passo mediano de 19 px. O modelo usa U(−10, 10), com σ = 5,8 px. Gaussiano com σ ≈ 10 px ou `--pos_noise 17` (uniforme com o mesmo σ) é o valor coerente com os dados. O `--motion_noise` proporcional não teve efeito no diagnóstico anterior, mas lá o GT estava desalinhado. Vale retestar.

---

## 3. Mudanças aplicadas no repositório (padrões = legado)

| Arquivo | Mudança |
|---|---|
| `include/localization/visual_odometry.hpp`, `src/localization/visual_odometry.cpp` | `VisualOdometryOptions` (matcher `LEGACY`/`RATIO`, `RANSAC`/`MAGSAC`, limiar, `min_inliers`, deadband); `track()` com cache das features do frame anterior; `lastStats()` (matches, inliers). `estimateRelativeMotion()` continua igual |
| `apps/run_localization.cpp` | Usa `vo.track()` (SIFT uma vez por frame); `--unreliable_motion zero\|cv\|apply`; `LB_DEBUG_ODOM` imprime matches e inliers; avisa quando `--odom_file` é mais curto que a fatia (antes repetia o último passo em silêncio) |
| `include/config/config.hpp`, `src/config/config.cpp` | `--vo_matcher`, `--vo_ratio`, `--vo_robust`, `--vo_reproj`, `--vo_min_inliers`, `--unreliable_motion` (`--apply_unreliable_odom` virou alias) |
| `tests/test_suite.cpp` | Test 21: translação sintética conhecida nos dois matchers e `track()` igual ao estimador sem estado |
| `scripts/check_gt_alignment.py` | Mede o atraso GT × imagens a partir de um log `LB_DEBUG_ODOM` e reavalia `errors.txt` com o GT deslocado |

Verificação: compila (GCC 13, OpenCV 4.6) e passa 82/82 checks. No modo legado, a trajetória de 20 frames é **idêntica** à do código original.

### 3.1 Validação no filtro (`results_odometria_vo/`)

Voo completo do ADM2, abBRIEF (`--alpha 0`), 50k partículas, `--two_phase --inject_ratio 0.05 --inject_spread 300 --scale_prior_sigma 0.10 --scale_prior_gain 0.87 --vo_scale_deadband 1`, sementes 1–3. Valores em GT atual / GT alinhado (lag −2):

| Variante | Erro médio | Mediana | Final | 1º < 25 m | s/frame |
|---|---|---|---|---|---|
| V0: VO legada | 33,8 / 29,1 m | 18,4 / 12,6 m | 15,4 / 10,9 m | 26, 4, 19 | 1,57 |
| V1: VO nova | 37,0 / 32,6 m | 17,8 / 13,1 m | 14,8 / 10,4 m | 35, 4, 23 | 0,94 |
| **V2: VO nova + `--pos_noise 17`** | **28,9 / 24,4 m** | **16,4 / 11,8 m** | **14,6 / 10,3 m** | 24, 5, 13 | **0,94** |
| V3: VO legada + `--pos_noise 17` | 33,7 / 29,4 m | 17,8 / 12,0 m | 15,1 / 10,6 m | 25, 5, 14 | 1,64 |

- Só a combinação V2 melhora o erro médio (−15%) e o final (nas 3 sementes). Cada mudança sozinha fica dentro do ruído entre sementes.
- A interpretação é coerente com a seção 2.5: com a VO mais precisa, o ruído de predição passa a ser o fator limitante da exploração na fase GLOBAL.
- No rastreio (frames 100–195) as quatro variantes ficam em ~7,5–7,9 m (GT alinhado).
- O tempo por frame cai ~40%. O V0 já inclui o cache de features: o código anterior gastava ~0,55 s/frame a mais.
- **n = 3 é indicativo.** Repetir com 10–30 sementes e com λ-BRIEF (α = 0,5, bandas MS) antes de citar no artigo.

**Configuração recomendada para a VO:**

```bash
--vo_matcher ratio --vo_robust magsac --vo_reproj 2 --sift_features 4000 --pos_noise 17 --vo_scale_deadband 1
# avaliação no ADM/0036 com o GT alinhado
--gt_offset 86
```

Para o benchmark com odometria fixa (AGENTS.md §3.2), regenere o `--odom_file` com essa VO, a partir do log `LB_DEBUG_ODOM`.

---

## 4. Próximos passos sugeridos

1. Confirmar o atraso de 2 frames pelos timestamps (log de voo ou EXIF `DateTimeOriginal` + GPS) e decidir se `--gt_offset 86` vira o padrão. Rodar `check_gt_alignment.py` na UFRGS (offset 204) e no ADM1/ADM3.
2. Reavaliar o benchmark abBRIEF × λ-BRIEF com o GT alinhado (só reavaliação, sem rodar de novo).
3. Conferir a altitude real no fim do ADM2 (barômetro) antes de usar z_GT como altímetro emulado.
4. Repetir a seção 3.1 com 10–30 sementes, λ-BRIEF (α = 0,5) e `--gt_offset 86`.

## 5. Limitações

- Só o ADM2. A UFRGS não foi testada.
- O GPS também tem erro próprio (a 2ª diferença tem σ ≈ 2,3 px), e a deriva de 10 frames inclui esse ruído.
- O registro frame × mapa para medir z de forma absoluta falhou (3–5 inliers: mapa de outra estação). A inconsistência de z no fim do voo vem de duas medidas da própria VO, e não de uma referência externa.

## 6. Registro das alterações (12/10/2026)

| Arquivo | Tipo |
|---|---|
| `lambda_brief_modern/include/localization/visual_odometry.hpp` | reescrito: `VisualOdometryOptions`, `VOMatcher`, `VORobust`, `VOStats`, `track()`, `reset()`, `lastStats()` |
| `lambda_brief_modern/src/localization/visual_odometry.cpp` | reescrito: cache de features, matcher `RATIO` (Lowe + mútuo), MAGSAC, contagem de inliers, `min_inliers`. Matcher `LEGACY` e checagens geométricas inalterados |
| `lambda_brief_modern/apps/run_localization.cpp` | VO via `track()`, `--unreliable_motion`, inliers no `LB_DEBUG_ODOM`, aviso de `--odom_file` curto. Removida a cópia `prev_rgb` |
| `lambda_brief_modern/include/config/config.hpp`, `src/config/config.cpp` | novas opções `--vo_matcher --vo_ratio --vo_robust --vo_reproj --vo_min_inliers --unreliable_motion`. `unreliable_zero_motion` (bool) virou `unreliable_motion` (string) |
| `lambda_brief_modern/tests/test_suite.cpp` | Test 21 (VO sintética, os dois matchers, `track()` = estimador sem estado). 82/82 checks |
| `lambda_brief_modern/scripts/check_gt_alignment.py` | novo |
| `lambda_brief_modern/README.md`, `CLAUDE.md` | opções novas, configuração recomendada, armadilhas 13–15 |
| `lambda_brief_modern/HISTORICO_RESULTADOS.md` | entrada `results_odometria_vo` |
| `lambda_brief_modern/results_odometria_vo/` | 12 execuções (4 variantes × 3 sementes) |

Os padrões continuam os do legado: sem flags novas, o binário reproduz bit a bit a trajetória anterior.

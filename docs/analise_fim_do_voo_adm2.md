# Análise: fim do voo ADM2 — por que a config. do artigo (seed 42) termina bem e a config. nova (seed 1) perde o rastro

Todas as execuções abaixo usam o dataset ADM/0036, fatia `--start_frame 84 --end_frame 279` (196 frames, offset de GT 84), mapa `ADM_35_12_09_2018.jpg`, `--num_particles 50000 --pairs 256 --alpha 0.5 --vi NDVI --color_mode LAB_NO_L --min_scale 0.1 --max_scale 3.0 --quantize 25 --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 --pixel_to_meter 0.188679245`, salvo indicação em contrário (E4 usa uma fatia menor). Build e suíte de testes (59 casos, incluindo os 7 novos do `--init_around_gt`) passaram antes de qualquer execução; `E1` reproduziu `results_vis_adm0036/errors.txt` **byte a byte**, então o binário atual não mudou o comportamento padrão.

A única mudança de código foi a opção de diagnóstico `--init_around_gt <sigma_px>` (default 0 = desligada, comportamento igual ao legado): inicializa a nuvem como uma Gaussiana em torno do GT do 1º frame da fatia (σ_xy = sigma_px, σ_yaw = 5°, z ~ 𝒩(0,87·z_GT, 5% de 0,87·z_GT)), usada só no experimento E4 para isolar o rastreio da convergência global.

## 1. Tabela com todas as execuções

### E1+E2 — H1 (variação só de semente, config. do artigo)

| execução | mean (m) | median (m) | rmse (m) | final (m) | média últ. 10 | 1º frame <30m | frames <30m |
|---|---|---|---|---|---|---|---|
| seed42 (artigo) | 150.46 | 171.33 | 190.64 | 15.29 | 23.64 | 125 | 64 |
| seed1 (artigo) | 272.34 | 254.03 | 288.36 | 364.36 | 375.37 | — | 0 |
| seed7 (artigo) | 49.59 | 22.40 | 87.43 | 15.10 | 23.60 | 30 | 125 |
| seed123 (artigo) | 264.75 | 231.12 | 286.95 | 233.20 | 239.48 | — | 0 |
| seed2024 (artigo) | 30.46 | 17.81 | 49.60 | 15.24 | 23.72 | 18 | 139 |

### E3 — H3/H4 (um fator por vez sobre seed 42)

| execução | mean (m) | median (m) | rmse (m) | final (m) | média últ. 10 | 1º frame <30m | frames <30m |
|---|---|---|---|---|---|---|---|
| seed42 baseline (=E1) | 150.46 | 171.33 | 190.64 | 15.29 | 23.64 | 125 | 64 |
| + scale_prior (σ0.10, g0.87) | 297.29 | 308.44 | 309.80 | 239.65 | 263.95 | — | 0 |
| + vo_scale_deadband 1 | 235.50 | 236.53 | 247.68 | 124.99 | 132.36 | 52 | 1 |
| + inject_ratio 0.05/spread 300 | 253.67 | 230.55 | 280.33 | 158.44 | 212.24 | — | 0 |
| a+b+c ("config nova", seed42) | 150.20 | 130.85 | 199.40 | 196.86 | 173.76 | 10 | 51 |
| d + yaw_prior 15° | 59.64 | 24.05 | 112.66 | 278.80 | 206.32 | 10 | 108 |

### E4 — isolando o rastreio da convergência (`--start_frame 234 --end_frame 279`, `--init_around_gt 20`, 46 frames, seed 42)

| execução | mean (m) | median (m) | rmse (m) | final (m) | média últ. 10 |
|---|---|---|---|---|---|
| artigo (sem prior/injeção) | **15.31** | 12.39 | 17.95 | **15.68** | 24.04 |
| config nova completa | 35.18 | 13.00 | 61.84 | 251.35 | 108.71 |
| só scale_prior, gain 0.87, σ0.05 | 23.27 | 13.11 | 30.21 | 54.51 | 53.79 |
| só scale_prior, gain 1.0, σ0.05 | 17.37 | 11.85 | 21.09 | 26.94 | 33.03 |
| só scale_prior, gain 1.3, σ0.05 | 16.31 | 14.60 | 18.53 | 19.68 | 25.80 |
| só scale_prior, gain 1.6, σ0.05 | 72.93 | 53.79 | 92.67 | 185.05 | 162.81 |
| gain 0.87 + sem injeção (=igual à linha gain0.87 acima) | 23.27 | 13.11 | 30.21 | 54.51 | 53.79 |
| gain 0.87 + inject_gate 0.8 | 27.69 | 14.19 | 40.30 | 143.71 | 70.90 |
| gain 0.87 + inject_spread 300 | 32.90 | 13.79 | 51.82 | 180.04 | 96.23 |

### E5 — estatística (voo completo, 5 sementes cada, config. escolhidas de E3)

| execução | mean (m) | median (m) | rmse (m) | final (m) | 1º frame <30m |
|---|---|---|---|---|---|
| CFG1 (config nova) seed1 | 106.85 | 37.37 | 168.23 | 222.06 | 25 |
| CFG1 seed7 | 85.05 | 32.30 | 136.23 | 163.03 | 16 |
| CFG1 seed42 | 150.20 | 130.85 | 199.40 | 196.86 | 10 |
| CFG1 seed123 | 77.80 | 27.07 | 127.00 | 354.81 | 9 |
| CFG1 seed2024 | 73.73 | 29.17 | 119.66 | 108.70 | 21 |
| CFG2 (config nova + yaw_prior 15°) seed1 | 69.42 | 26.78 | 124.80 | 264.38 | 14 |
| CFG2 seed7 | 41.75 | 22.32 | 67.01 | 228.80 | 5 |
| CFG2 seed42 | 59.64 | 24.05 | 112.66 | 278.80 | 10 |
| CFG2 seed123 | 55.45 | 22.72 | 107.59 | 299.78 | 11 |
| CFG2 seed2024 | 48.04 | 22.93 | 76.41 | 275.32 | 11 |

**Contra o GT corrigido por inclinação (`traj_truth_corr.txt`)**, amostra de execuções representativas (erro médio / mediana, m):

| execução | mean | mean_corr | median | median_corr | final | final_corr |
|---|---|---|---|---|---|---|
| seed42 (artigo) | 150.46 | 150.66 | 171.33 | 170.02 | 15.29 | 11.95 |
| seed7 (artigo) | 49.59 | 43.61 | 22.40 | 13.98 | 15.10 | 11.85 |
| seed2024 (artigo) | 30.46 | 24.58 | 17.81 | 10.97 | 15.24 | 12.06 |
| config nova (seed42) | 150.20 | 145.56 | 130.85 | 132.16 | 196.86 | 196.92 |
| config nova+yaw (seed42) | 59.64 | 53.86 | 24.05 | 13.25 | 278.80 | 282.53 |
| E4a artigo, tail isolada | 15.31 | 11.60 | 12.39 | 8.25 | 15.68 | 12.54 |
| E4b config nova, tail isolada | 35.18 | 31.87 | 13.00 | 9.66 | 251.35 | 248.01 |

O GT corrigido reduz a mediana em ~5–10 m nas execuções que convergiram bem (compensar a inclinação da câmera ajuda), mas **não** muda qual configuração vence nem explica a perda no fim de voo — o padrão de qual execução termina bem ou mal é o mesmo com os dois GTs.

## 2. Veredito das hipóteses

**H1 — "A diferença vem só da semente": CONFIRMADA para a convergência global, mas ela não é a causa da perda no fim do voo.**
Com a config. do artigo, a semente decide sozinha se o filtro converge: seed 42, 7 e 2024 terminam em 15 m; seed 1 e 123 nunca chegam perto do GT (233–364 m, 0 frames abaixo de 30 m). É a assinatura clássica da seção 2.7 do diagnóstico — sem prior nem injeção, converger depende de algum ancestral aleatório ter caído na bacia estreita do GT (~0,3 partícula esperada em 50k). Mas **nenhuma das 5 sementes da config. do artigo perde o rastro no fim depois de ter convergido** (seed42/7/2024 terminam com 15 m, tão bem quanto no meio do voo). Já a config. nova, testada com as mesmas 5 sementes no E5, **perde o rastro no fim em 10 de 10 execuções** (final entre 109 e 355 m em CFG1, 229–300 m em CFG2), incluindo sementes onde a convergência foi rápida e estável. Ou seja: a semente explica a convergência global da config. do artigo, mas a perda no fim do voo é sistemática na config. nova e independente da semente — outra causa (H3/H4) está em jogo.

**H2 — "Escala maior no fim ajuda o pouso": REFUTADA.**
No experimento isolado do rastreio (E4c, tail-only, partindo do GT), variar só o ganho do prior de altímetro (`--scale_prior_gain`) com σ0.05 dá: gain 0.87 → final 54,5 m; gain 1.0 → 26,9 m; gain 1.3 → 19,7 m; gain 1.6 → 185,0 m. Não há tendência monotônica de "quanto maior, melhor": o meio da faixa (1,0–1,3) é o menos ruim, mas mesmo o melhor caso com prior (gain 1.3: 16,3/19,7 m) não supera a execução **sem prior nenhum** (15,3/15,7 m). Inflar a escala não compensa a subestimação de 27% da VO; ela só desloca o mínimo de verossimilhança, e o ganho recomendado para a convergência global (0,87) é justamente um dos piores nessa fase.

**H3 — "Prior de altímetro e/ou vo_scale_deadband causam a perda": CONFIRMADA.**
No voo completo (E3, seed 42), adicionar só o prior de escala piora a execução de 150→297 m de média e de 15→240 m final; só o `--vo_scale_deadband 1` piora de 150→235 m média e 15→125 m final. No rastreio isolado (E4), qualquer variante com prior de escala (b, c, d1) fica pior que a execução sem prior (E4a). O prior por si resolve o modo espúrio de escala pequena na inicialização (seção 2.7 do diagnóstico) mas, quando a nuvem já está corretamente localizada, ele **desloca** a estimativa de z para longe do valor que de fato explica o frame durante a descida/curva final, e o filtro segue a estimativa errada.

**H4 — "A injeção atrapalha o rastreio no fim": CONFIRMADA.**
No voo completo (E3c), só a injeção piora seed42 de 150→254 m média e 15→158 m final. No rastreio isolado com o prior de escala já fixo (E4d), acrescentar `--inject_gate 0.8` ou `--inject_spread 300` piora ainda mais o resultado do prior sozinho (final 54,5 m sem injeção → 143,7 m com gate → 180,0 m com spread). Nos frames finais (curva, descida, pouso) o contraste C não distingue "nuvem certa" de "poste falso" (achado já registrado na seção 2.8 do diagnóstico); qualquer injeção nesse trecho é uma chance real de a massa migrar para uma partícula aleatória.

**H5 — "Uma nuvem que convergiu tarde chega mais espalhada/recente e por isso aguenta melhor o fim": REFUTADA.**
No E5, a config. CFG2 (com prior de bússola) converge muito mais cedo e de forma muito mais consistente entre sementes do que CFG1 (1º frame <30 m: 5–14 contra 9–25; desvio-padrão do erro médio entre sementes: 10,6 m contra 31,5 m — ver seção 4). Mesmo assim, CFG2 perde o rastro no fim em **5 de 5** sementes, com erro final ainda pior em média que CFG1 (269±26 m contra 209±92 m). Se "convergir cedo e com nuvem mais compacta" ajudasse o fim do voo, CFG2 deveria terminar melhor — o oposto acontece. O fator decisivo não é o histórico de convergência da nuvem, é se o prior/injeção **continuam ativos** durante o trecho de observação fraca (confirmado diretamente por H3/H4 no rastreio isolado, onde a nuvem sempre parte de cima do GT).

## 3. Gráficos

Erro de posição × frame e z_est/z_GT × frame, um par de PNGs por grupo de experimento (`lambda_brief_modern/analise_fim/plots/`):

- **H1 (E1+E2, variação de semente, config. do artigo):**
  `../lambda_brief_modern/analise_fim/plots/G1_H1_seeds_erro.png`
  `../lambda_brief_modern/analise_fim/plots/G1_H1_seeds_zratio.png`
- **E3 (fatores isolados sobre seed 42):**
  `../lambda_brief_modern/analise_fim/plots/G2_E3_fatores_erro.png`
  `../lambda_brief_modern/analise_fim/plots/G2_E3_fatores_zratio.png`
- **E4 (rastreio isolado, partindo do GT, frames 150–195):**
  `../lambda_brief_modern/analise_fim/plots/G3_E4_tail_isolada_erro.png`
  `../lambda_brief_modern/analise_fim/plots/G3_E4_tail_isolada_zratio.png`
- **E5 (estatística, 5 sementes × 2 configurações, voo completo):**
  `../lambda_brief_modern/analise_fim/plots/G4_E5_estatistica_erro.png`
  `../lambda_brief_modern/analise_fim/plots/G4_E5_estatistica_zratio.png`

Nos gráficos de E4 (rastreio isolado) as curvas de erro das variantes com prior/injeção destacam-se subindo a partir de ~frame 30–35 (correspondente ao frame 180–185 do voo completo, exatamente o trecho onde a VO subestima o deslocamento em ~27%), enquanto a curva "artigo" permanece plana perto de 15 m até o fim. Nos gráficos de z_est/z_GT, as variantes com prior ficam presas perto do gain configurado (0,87/1,0/1,3/1,6×z_GT) mesmo quando isso diverge do z que melhor explica o frame, enquanto a execução sem prior acompanha a queda real de z.

## 4. Configuração recomendada

Nenhuma das duas configurações testadas em E5 resolve o fim do voo — as duas perdem o rastro em 100% das sementes testadas. A escolha é sobre qual delas dá o melhor comportamento **até aí**:

| | CFG1 (config nova) | CFG2 (config nova + yaw_prior 15°) |
|---|---|---|
| erro médio (5 sementes) | 98,7 ± 31,5 m | **54,9 ± 10,7 m** |
| erro mediano (5 sementes) | 51,4 ± 44,6 m | **23,8 ± 1,8 m** |
| erro final (5 sementes) | 209,1 ± 91,9 m | 269,4 ± 26,1 m |
| 1º frame < 30 m (faixa) | 9–25 | **5–14** |

**Recomendação: CFG2** (config. nova + `--yaw_prior_sigma 15`) para uso geral. Ela converge mais cedo e de forma muito mais previsível entre sementes (desvio-padrão do erro médio 3× menor que CFG1), o que é mais valioso do que o erro final ligeiramente pior — sobretudo porque **o próprio erro final de CFG1 tem desvio-padrão de 92 m**, ou seja, não é confiável tomar uma única execução de CFG1 como representativa. Se o requisito for erro final baixo especificamente no pouso, nenhuma das duas serve hoje; a config. do artigo **sem** prior/injeção, mas sujeita à sorte da semente (H1), continua sendo a única que historicamente termina bem (seeds 42/7/2024 do E2, 15 m).

Comando completo recomendado:
```bash
./build/lambda_brief_loc \
  --map "../Datasets/ADM/0036/Mapas/ADM_35_12_09_2018.jpg" \
  --dataset_dir "../Datasets/ADM/0036" --start_frame 84 --end_frame 279 \
  --num_particles 50000 --pairs 256 --alpha 0.5 --vi NDVI --color_mode LAB_NO_L \
  --min_scale 0.1 --max_scale 3.0 --quantize 25 \
  --ms_filter bilateral --bf_d 6 --bf_sc 5 --bf_ss 5 --pixel_to_meter 0.188679245 \
  --scale_prior_sigma 0.10 --scale_prior_gain 0.87 --vo_scale_deadband 1 \
  --inject_ratio 0.05 --inject_spread 300 --yaw_prior_sigma 15 --seed <42|1|7|123|2024>
```

## 5. Próximos passos

1. **Desligar prior/injeção depois que a nuvem convergiu.** É a implicação direta de H3+H4+H5: o prior e a injeção são necessários só para escapar do modo espúrio de escala pequena da inicialização (seção 2.7 do diagnóstico); uma vez com Neff alto e variância baixa por várias janelas seguidas, eles deixam de ajudar e passam a atrapalhar. Hoje não existe essa opção — seria um novo modo (`--disable_priors_after_convergence` ou critério automático por Neff/variância) e precisa de teste próprio antes de virar padrão.
2. **Compensar roll/pitch na projeção (`traj_angles.txt`, como o `-corrpxpos` do legado).** H2 foi refutada — inflar a escala não é o caminho —, mas o erro contra `traj_truth_corr.txt` (GT já corrigido pela inclinação) é sistematicamente menor (mediana cai ~5–10 m nas execuções aqui medidas), e a causa raiz documentada na seção 2.8 do diagnóstico é justamente a inclinação da câmera during a descida/curva final. Compensar a projeção dos pares diretamente com roll/pitch (em vez de mexer em z) ataca a causa, não o sintoma.
3. **Investigar a subestimação de ~27% da VO nos frames 180–189** especificamente (correlacionar com os ângulos de `traj_angles.txt` nesse trecho) para saber se dá para corrigir a odometria em vez de compensar via GT.
4. **Rodar mais sementes de CFG2** (H5 já mostrou baixa variância em 5, vale confirmar com 10–15) e considerar testar `--yaw_prior_sigma` com outros valores, já que ele foi o único fator que reduziu a variância entre sementes sem piorar o erro médio.
5. Os resultados antigos (`results_vis_adm0036`, `results_adm0036_alt`) e os novos (`analise_fim/`) foram mantidos; nenhum arquivo foi apagado ou sobrescrito.

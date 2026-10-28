# λ-BRIEF: resumo do que já foi tentado (para um agente novo) — 28/10/2026

Este documento existe para quem chega sem memória das conversas anteriores e vai propor **novas ideias** de contribuição. Tudo aqui está no repositório ou nos docs do projeto "UAVs" no claude.ai (`claude/*.md`). Números = metros de erro horizontal salvo indicação.

## 1. Contexto em 6 linhas

- **Trabalho:** localização global de UAV por MCL (filtro de partículas, 50k) com descritor binário BRIEF comparando a imagem do drone (Parrot Sequoia: RGB + NIR/RED/REG/GRE) com um mapa de satélite RGB (Google, ~0,19 m/px) de **outra data**.
- **λ-BRIEF** (artigo do grupo, 2019/2020): os pares do BRIEF são sorteados por uma máscara F = α·(−NDVI equalizado) + (1−α)·gaussiana central, para evitar vegetação (que muda entre mapa e voo). **abBRIEF** = α = 0 (só gaussiana).
- **Meta:** artigo no **ICUAS 2027**. O autor (Vitor, mestrando UFRGS, orientadores Renan Maffei e Mariana Kolberg) quer uma contribuição nova; gosta de ângulos com **deep learning** e com **desastres / enchente RS 2024 + Sentinel-2**. Não há voos novos nem câmera multiespectral disponível.
- **Repositório** `UAVs Localization/` (máquina do autor, Linux, `/storage/UAVs Localization`):
  - `lambda_brief_modern/` = engine nova C++17/OpenCV (é onde se trabalha);
  - `phir2frameworkv2/` = código legado do artigo (referência);
  - `Datasets/ADM/{0035,0036,0040,0040_zenodo}`, `Datasets/UFRGS VET`, `Datasets/Sentinel2/ADM` (292 cenas S2 L2A, 110 datas úteis, 2018–2019 e 2023–2026, grade 10 m 324×324, EPSG:32722);
  - `sentinel2/map_georef.json` = afim pixel do mapa ADM_35 → UTM (erro 0,13 m).
- **Leia primeiro:** `CLAUDE.md` (raiz, armadilhas), `lambda_brief_modern/README.md`, `docs/MANUAL_TESTES_ARTIGO.md` (seções 1–10), `experimentos/INDICE.md`, `HISTORICO_RESULTADOS.md`, `docs/achados_codigo_legado.md`.
- **Voos do artigo:** ADM1 = 0035 (300 frames), ADM2 = 0036 (196), ADM3 = 0040 (266, GT do Zenodo), UFRGS (224). 5 mapas por voo (ADM: 12-2018, 01-2014, 11-2016, 09-2017, 02-2014; mesmos arquivos ADM_35 em todos os voos ADM).

## 2. Linha do tempo do que foi feito

### 2.1 Fazer a engine nova convergir (set/2026) — `claude/diagnostico_convergencia_e_pipeline.md`
- Bug raiz: pesos sobrescritos a cada frame + reamostragem rara → corrigido (bayesiano em log, reamostra todo frame).
- Fidelidade ao legado: quantização de cor 25 níveis, odometria não confiável = movimento nulo, SIFT ilimitado.
- Problemas estruturais do método: máximos espúrios em escala pequena (z 0,1–0,3), bacia do GT minúscula (~10⁻⁶ do espaço), escala da odometria visual sem correlação com a altitude.
- Soluções que viraram o **regime `final_baro`** (recomendado): duas fases (busca global → rastreio), injeção adaptativa de partículas, barômetro real (`altimeter_baro.txt`, ADM3 a partir do KML), odometria fixa pré-gravada (VO ratio + MAGSAC), `--vo_scale_deadband 1`, `--pos_noise 17`.
- Com isso **todas** as execuções convergem (ab e λ). Rastreio pós-convergência ~8–15 m. Trechos fracos conhecidos: ADM2 frames ~55–60 e ~90–110 (curva de 180° sobre lavoura), pouso.

### 2.2 Piloto barômetro × sem altímetro (15/10) — `claude/piloto_baro_vs_noalt.md`
- Sem altímetro ainda converge 95–100%, mas mais devagar; MAE do ADM2 dobra. λ × ab sem diferença com barômetro.
- Único indício forte a favor do λ: sem altímetro, mapa 12-09 (lavoura diferente do voo): convergência no frame 25 × 117.

### 2.3 Teste do artigo no regime novo, 5 seeds (17/10) — `claude/resultado_artigo_novo_a05_5seeds.md`
| Voo | ab → λ (α = 1) | ab → λ (α = 0,5) |
|---|---|---|
| ADM1 | 40,8 → 27,7 (p = 0,002) | 40,8 → 32,9 (p = 0,04) |
| ADM2 | 49,8 → 44,8 (n.s.) | 49,8 → 38,4 (p = 0,11) |
| ADM3 | 23,6 → **37,7** (λ perde, p < 0,001) | 23,6 → **28,5** (λ perde) |
| UFRGS | 29,3 → 15,7 (p = 0,007) | 29,3 → 14,9 (p = 0,001) |
- O ganho do λ está na **convergência da busca global**, não no rastreio. α = 0,5 é o melhor equilíbrio. O λ perde no ADM3.

### 2.4 Por que o artigo mostrava ganho muito maior (18/10) — `claude/por_que_lambda_ganhava_no_artigo.md`, `docs/achados_codigo_legado.md`
- O abBRIEF do legado era outro código: 256 pares **fixos e quase uniformes** (bug: variância usada como desvio-padrão em `rng.gaussian`), enquanto o λ era central e re-sorteado por frame.
- As 30 repetições do artigo não eram independentes (nuvem inicial idêntica, z herdado da repetição anterior).
- Métrica do artigo era 3D com altitude e GT atrasado.
- **Teste `--pair_pattern legacy_ab` (27/10, α = 1, 1 seed, `experimentos/2026-10-27_ab_legado_novo_a1`):** fração da diferença "ab legado → λ" que vem só do baseline: **ADM2 82%**, ADM1 2%; no ADM3 o ab da engine (20,9) ganha do λ (47,5). Ou seja, boa parte do ganho publicado no ADM2 era o baseline fraco.

### 2.5 Sentinel-2 como diagnóstico (13/10) — `claude/sentinel2_analise_adm.md`
- No dia do voo (18/04/2019) a lavoura do ADM2 estava verde e densa (NDVI 0,80); nos mapas de set/2018 era solo arado. O λ evita essa região → coerente com "evitar o que mudou".
- Enchente mai/2024: 30–57% das pegadas dos voos com queda de NDVI > 0,2 (lama visível a 10 m; destruição de prédios quase não aparece a 10 m).
- Similaridade S2 mapa×voo **não** ordena os mapas por desempenho (n = 3, inconclusivo). Correlação S2 × diferença entre mapas Google por célula: 0,13–0,26 (ruído de registro/sombra domina).

### 2.6 v3: peso de estabilidade do Sentinel-2 na verossimilhança (27–28/10) — **resultado negativo** — `claude/v3_sentinel_peso_estabilidade.md`
- **Ideia:** cada par pesa w = min(W(A), W(B)) lido de um raster do tamanho do mapa; distância d = Σw·e/Σw ("normalized") ou Σ[w·e + (1−w)·0,5]/P ("coin"). Peso só na verossimilhança (os pares são sorteados igual ao ab).
- **Peso A:** desvio-padrão do NDVI do S2 por célula de 10 m em 2018–2026; w = clip(1 − (σ−0,08)/(0,22−0,08), wmin, 1). Peso B (|ΔNDVI| mapa→voo) implementado mas só existe para mapas ≥ 08/2018.
- **Resultados com 5 seeds** (ADM2 e ADM3 × mapas 12-2018 e 01-2014, ab+S2A × ab × λ α = 0,5):

| Voo · mapa | ab | λ | S2A wmin 0,05 | S2A wmin 0,5 |
|---|---|---|---|---|
| ADM2 · 12-2018 | 30,4 | 30,3 | 45,9 | 40,2 |
| ADM2 · 01-2014 | 57,6 | 45,0 | 50,7 | 58,4 |
| ADM3 · 12-2018 | 21,5 | 23,4 | 22,0 | 21,8 |
| ADM3 · 01-2014 | 23,5 | 26,7 | **65,9** | **54,9** |

- Fórmula coin (1 seed): muito pior (busca do ADM2 só converge no frame ~120; ADM3 se perde de vez) → os pares na lavoura **carregam informação útil**; descontá-los apaga sinal.
- ADM3 · 01-2014: o ab+S2A perde o rastreio nos frames ~135–180 em 5/5 seeds (forte) e 4/5 (suave). Com o mapa de 2018 o mesmo trecho passa → **depende do mapa**. Hipótese não confirmada: mudança entre 2014 e 2018 aparece como "estável" no peso A (que só vê 2018+).
- Único sinal positivo: no trecho misto do ADM2 (frames ~95–110, metade estável / metade lavoura) o peso forte reduz o pico de erro (01-2014: ~55–90 m × ~150–200 ab × ~400 λ), mas não muda o MAE.
- **Conclusão:** a variante A **não** é contribuição. Rastreio pós-convergência inalterado em tudo.

## 3. Ferramentas que já existem (use, não reescreva)

| O quê | Onde / como |
|---|---|
| Engine | `lambda_brief_modern/build/lambda_brief_loc`; testes `./build/tests/test_suite` (97 passam) |
| Teste do artigo (1 ou N seeds, copia execuções idênticas) | `scripts/paper_comparison.py`, `scripts/paper_comparison_seeds.py --protocolo novo --lambda_alpha 0.5 --seeds 1..5` |
| Baseline legado | `--pair_pattern legacy_ab|uniform|roulette_fixed`; `scripts/ab_legado_comparison.py` |
| v3 | `--stability_map <png> --stability_formula normalized|coin`; `scripts/make_stability_map.py --modo A|B [--wmin --lo --hi --janela]`; `scripts/v3_comparison.py --seeds 1..5 --flights ADM2 ADM3 --mapas 12-2018 01-2014 --metodos_s2 ab [--wmin 0.5]` |
| Vídeos de diagnóstico | `--save_pairs` (grava pares, máscara e NDVI por frame; não muda resultado) + `scripts/v3_video.py --voo ADM3 --mapa 01-2014 [--frames 120:190] [--vistas voo drone mapa] [--fundo peso]`: drone RGB/NDVI, mapa, S2 RGB/NDVI do dia do voo e peso, na orientação da câmera e no mapa, com a nuvem de partículas |
| Pastas de experimento | `experimentos/<data>_<nome>/` + linha automática em `experimentos/INDICE.md` (`scripts/experimentos.py`) |
| Tempo | ~150–215 s por execução do voo inteiro, 4 em paralelo |

Métrica padrão: MAE do voo, convergência = erro < 25 m por 10 frames seguidos, rastreio = mediana pós-convergência, Wilcoxon pareado por (mapa, seed). GT: `traj_truth_sync.txt` (ADM1/2, UFRGS), Zenodo (ADM3).

## 4. O que se aprendeu (restrições para novas ideias)

1. Com barômetro + injeção + duas fases, **a busca global já converge 100%**: métodos que só "melhoram o rastreio" têm pouco espaço (rastreio ~8–15 m, limitado pela resolução/registro do mapa e pela inclinação da câmera).
2. A diferença entre descritores aparece em **velocidade de convergência** e nos **trechos fracos** (lavoura que mudou, curvas, pouso), e em cenários **sem altímetro**.
3. Vegetação não é só ruído: pares na lavoura ajudam a busca global mesmo quando a lavoura mudou (coin e peso forte pioram).
4. O ganho publicado do λ no ADM2 vinha em grande parte do baseline fraco do legado; o λ **perde** no ADM3 (5 seeds).
5. Sentinel-2 a 10 m é bom para detectar **mudanças grandes** (lavoura, lama, rio) e ruim para mudanças urbanas finas; um peso fixo derivado dele (variabilidade 2018–2026) não generaliza para mapas antigos (2014).
6. n pequeno engana: várias conclusões de 1 seed se inverteram com 5 seeds. Use ≥ 5 seeds e reporte por voo × mapa.

## 5. Ideias ainda não testadas (do brainstorm anterior; nenhuma implementada)

- **Rede neural:** descritor aprendido / matching denso drone↔mapa; ou CNN como **vigia de relocalização** nos trechos fracos (sinal de "perdido" para disparar injeção); o grupo já tem o artigo MMVL (Multi Map Visual Localization, deep learning) como base.
- **Enchente RS 2024:** simular mapa desatualizado alterando a textura do mapa onde o S2 viu lama (0–50% da pegada) e medir taxa de convergência ab × λ × variantes; peso B com S2 de mai/2024.
- **Seleção/combinação de mapas** (vários mapas de datas diferentes ao mesmo tempo, multi-mapa) usando S2 ou a própria verossimilhança.
- **Peso B / mudança mapa→voo** (só mapa 12-2018 com S2; mapas 2014–2017 precisariam de Landsat 30 m).
- **Peso só como diagnóstico** ou só em frames mistos; ou usar o S2 para escolher α por frame.
- **Compensar roll/pitch** na projeção dos pares (`traj_angles.txt`, GT corrigido `traj_truth_corr.txt`) — trecho do pouso.
- **Cenário sem altímetro** como foco (onde o λ mostrou vantagem).

## 6. Armadilhas práticas (ambiente)

- A máquina do autor é acessada por ferramentas de arquivo (stage/commit); **não há shell remoto** estável — o autor roda os comandos. Não mande scripts para mover/apagar arquivos; git é com ele (branch atual `v3`).
- Já aconteceu de um arquivo gravado na máquina **voltar para a versão anterior** logo depois do commit; grave a partir de um caminho novo e confira o tamanho depois.
- Arquivos em `experimentos/.../execucoes/<voo>/<mapa>/<método>/seed_N/` estão fundos demais para copiar daqui; leia `tabela.csv`/`tabela.md`/`compilado.png` da raiz do experimento.
- O z da pose do GT não está na escala do filtro: use `0,87·z_GT` (ADM RGBres) ou o altímetro.

# Roteiro: piloto abBRIEF × λ-BRIEF (15/10/2026)

**Mensagem em uma frase:** com o filtro em duas fases e o **barômetro real** do drone, o abBRIEF e o λ-BRIEF (o novo) acham o drone em menos de 20 frames e o seguem até o pouso. A diferença entre os dois só aparece quando tiramos o altímetro.

Termos usados aqui:

- **abBRIEF** é o baseline (α = 0). **λ-BRIEF** é o método novo (α = 0,5, máscara espectral pelo NDVI).
- **Convergiu** quer dizer erro abaixo de 25 m por 10 frames seguidos.
- **MAE** é o erro médio do voo inteiro, incluindo a busca.

Cada pasta segue a mesma ordem:

1. teste do abBRIEF
2. teste do λ-BRIEF
3. vídeo do abBRIEF
4. vídeo do λ-BRIEF
5. evaluation comparando os dois

Os "testes" mostram a seed de MAE mediano (a execução típica, não a melhor), com as 5 seeds desenhadas no gráfico de erro.

---

## Ato 1: ADM2 com barômetro real (`01_ADM2_barometro_real/`)

Voo ADM2 (0036), mapa de 12/09/2018, 196 frames.

**Contexto para dizer antes:** o filtro precisa achar a posição, a escala (altura) e a direção do drone num mapa inteiro. A altura agora vem do barômetro do DJI, sincronizado com a câmera pelo relógio, e não mais do ground truth.

**`1_teste_abBRIEF/evaluation_abBRIEF.png`**: o baseline.

- Seed 3: convergiu no frame 16, MAE de 29,3 m.
- Nas 5 seeds: 5/5 convergiram (frame mediano 16), erro de rastreio de 12,0 m.
- Mostre no painel de trajetória: antes da bolinha laranja a estimativa vagueia (fase de busca); depois, cola no verde.

**`2_teste_lambdaBRIEF/evaluation_lambdaBRIEF.png`**: o novo.

- Seed 3: convergiu no frame 17, MAE de 28,3 m.
- Nas 5 seeds: 5/5 convergiram (frame mediano 17), erro de rastreio de 13,3 m.

**`3_video_abBRIEF.mp4`**: *ainda sendo gerado; entra quando o comando terminar.*

**`4_video_lambdaBRIEF.mp4`**: o vídeo do novo.

- Esquerda: câmera do drone. Direita: mapa com o GT (verde), a estimativa (amarelo) e as partículas (laranja).
- Pause no frame ~17, quando a nuvem cai sobre o ponto verde.
- Mostre os frames ~55–75 e ~95–105: o erro sobe (lavoura e curva) e depois o filtro se recupera.
- Erro final: 8,0 m.

**`5_evaluation_abBRIEF_x_lambdaBRIEF.png`**: os dois juntos.

- **Frase:** com barômetro, os dois convergem quase no mesmo frame e seguem juntos até o fim. Os trechos difíceis do voo são os mesmos para os dois.

## Ato 2: UFRGS com barômetro real (`02_UFRGS_barometro_real/`)

Voo UFRGS, mapa de 18/09/2018, 224 frames. É o voo "fácil", para mostrar que o resultado se repete.

- **abBRIEF:** seed 4 converge no frame 8, MAE de 10,4 m. Nas 5 seeds: 5/5, frame mediano 9, rastreio de 10,8 m.
- **λ-BRIEF:** seed 5 converge no frame 10, MAE de 11,5 m. Nas 5 seeds: 5/5, frame mediano 10, rastreio de 11,2 m.
- **Vídeo do λ:** converge no frame 10, erro final de 7,8 m.
- **Frase:** em terreno urbano, sem mudança de vegetação, os dois descritores são equivalentes.

## Ato 3: os números com 5 seeds (`03_resultados_5_seeds/`)

**`1_comparacao_abBRIEF_x_lambdaBRIEF.png`**: convergência, rastreio e MAE de todos os mapas, com IC de 95%.

- Com barômetro, 100% convergem nos dois voos.
- **ADM2 (4 mapas):** frame mediano de 16 nos dois; MAE de ~28 m nos dois.
- **UFRGS (2 mapas):** frame ~11; MAE de ~12 m.
- O λ rastreia de 0,6 a 1,2 m pior que o abBRIEF depois de convergir. É pequeno, mas significativo (Wilcoxon pareado, p < 0,01). Convergência e MAE não mostram diferença.

**`2_convergencia_por_frame.png`**: % das execuções localizadas a cada frame.

- Linha cheia é com barômetro; com ela tudo chega a 100% perto do frame 25.
- No ADM2, todos caem perto dos frames 55–60 e 95–105 e se recuperam. Esse é o gargalo que sobrou: **recuperar o drone**, não achá-lo.

**`3`–`6`:** o erro por mapa e a distribuição do erro de rastreio, por voo.

**`7_evaluations_outros_mapas_barometro/`:** a mesma figura de evaluation para os outros mapas, para mostrar se perguntarem.

**`tabelas/`:** tudo em números.

- `summary.md` tem as tabelas prontas.
- `runs.csv` tem uma linha por execução.
- `tests.csv` tem os testes estatísticos.
- `tables.tex` é a versão para o artigo.

> Ressalva para dizer: 5 seeds mostram tendência. Uma taxa de 5/5 tem IC de 57–100%. A rodada com 30 seeds confirma.

## Ato 4 (extra): sem altímetro (`04_extra_sem_altimetro/`)

É onde o λ-BRIEF se diferencia. O mapa é o de 12/09/2018: a lavoura do meio do voo estava arada no mapa e verde no dia do voo.

**`1_teste_abBRIEF`:** o abBRIEF se perde.

- Seed 1 só converge no frame 170, com MAE de 126,7 m.
- Nas 5 seeds, o frame mediano é 117.

**`2_teste_lambdaBRIEF`:** o λ-BRIEF acha o drone.

- Seed 2 converge no frame 23, com MAE de 34,7 m.
- Nas 5 seeds, o frame mediano é 25 e o MAE médio é 40,8 m (contra 117 m do abBRIEF).

**`3_video_abBRIEF.mp4`:** *ainda sendo gerado.* Mostra a estimativa vagueando pelo mapa por mais de 100 frames antes de achar o drone.

**`4_video_lambdaBRIEF.mp4`:** converge no frame 23, erro final de 8,7 m.

**`5_evaluation_abBRIEF_x_lambdaBRIEF.png`:** **a figura mais forte a favor do λ.**

**`6`–`8`:** os outros mapas. Nos outros mapas a diferença é menor ou some.

- **Frase:** sem altura, a busca fica difícil. Nesse caso a máscara espectral evita a lavoura que mudou e encontra o drone ~90 frames antes.
- **Ressalva:** é um mapa e 5 seeds; é indício, não prova.

## Fechamento

1. Com o barômetro real, o problema de *achar* o drone está resolvido nesses dois voos, para os dois descritores.
2. O que sobra é *recuperar* depois de perder (ADM2, frames 55–60 e 95–105). Isso motiva a CNN global, estilo MMVL, como vigia de relocalização.
3. O λ-BRIEF ajuda quando a busca é difícil (sem altura, vegetação que mudou). Próximo passo: 30 seeds para confirmar.

---

Origem dos arquivos (nada foi movido, tudo foi copiado):

- `experimentos/2026-10-15_piloto_baro_vs_noalt/` (antes `results_final/B_piloto/`): as execuções e `summary_traj_truth_sync/`.
- `experimentos/2026-10-15_piloto_baro_vs_noalt/videos/` (antes `results_final/videos/`): os vídeos.

Os vídeos desta pasta foram recomprimidos (~15 MB) e ganharam um título com descritor, regime, voo e seed. Os originais, em `videos/` da pasta do piloto, estão em qualidade máxima.

As figuras de evaluation foram geradas com `scripts/plot_evaluation_compare.py` (opção `--descs` para um descritor só).

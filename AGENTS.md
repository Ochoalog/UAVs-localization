# AGENTS.md — Diretrizes e Regras Operacionais para Agentes de IA

Este documento define as regras de governança, padrões de desenvolvimento e protocolos de experimentação científica que **todos os agentes de IA** e desenvolvedores devem seguir estritamente ao atuar neste repositório.

---

## 1. Contexto do Projeto

Este projeto desenvolve sistemas de localização global visual baseados em visão computacional (sem GPS) para Veículos Aéreos Não Tripulados (VANTs) sobrevoando áreas agrícolas e de alta densidade vegetal, comparando frames multiespectrais/RGB de drone contra imagens de satélite multitemporais (Google Earth) via Filtro de Partículas de Monte Carlo (MCL) em 4 graus de liberdade ($x, y, z, \text{yaw}$).

Os dois descritores centrais de pesquisa são:
- **abBRIEF** (Mantelli et al., 2019): Descritor binário baseado em amostragem espacial Gaussiana 2D nos canais de cor CIE L\*a\*b\* ($a^*$ e $b^*$, desconsiderando $L^*$). Corresponde matematicamente ao caso **$\alpha = 0.0$**.
- **$\lambda$-BRIEF** (Westhauser et al.): Extensão que combina a distribuição Gaussiana espacial com a máscara de índice de vegetação invertida (NDVI/NDRE), ponderada por $\alpha \in (0, 1]$.

O código ativo de desenvolvimento e experimentação é a engine moderna C++17 em [`lambda_brief_modern/`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern). O diretório `phir2frameworkv2/` é mantido apenas como referência histórica legada.

---

## 2. REGRA MANDATÓRIA: Gestão e Registro Histórico de Resultados

> [!IMPORTANT]
> **REGRA OPERACIONAL #1:**
> Nenhum agente deve executar uma rodada de testes, simulação piloto ou bateria em batch sem **atualizar imediatamente o arquivo de histórico** [`lambda_brief_modern/HISTORICO_RESULTADOS.md`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/lambda_brief_modern/HISTORICO_RESULTADOS.md).

### 2.1. O que deve ser registrado a cada rodada de teste:
Toda nova adição de resultados deve incluir uma entrada com os seguintes campos:
1. **Identificador e Pasta de Saída:** Caminho relativo exato onde os arquivos foram salvos.
2. **Data / Timestamp:** Momento da execução.
3. **Objetivo Científico / Hipótese:** O que se pretendia testar ou validar nesta rodada.
4. **Dataset, Trajetória e Mapa(s):** Ex.: ADM2 (`0036`), frames 84–279, mapa `ADM_35_12_09_2018.jpg`.
5. **Comando CLI / Parâmetros-Chave:** Ex.: `--num_particles 50000 --alpha 0.0 --seed 1 --scale_prior_sigma 0.10 ...`.
6. **Resultados e Métricas Principais:** Erro médio, erro mediano, erro final (m), convergência ($S_{\%}$).
7. **Conclusão e Próximo Passo:** O que os dados demonstraram e qual o impacto na pesquisa.

### 2.2. Convenção de Nomenclatura para Pastas de Resultados (desde 17/10/2026)
Para manter a raiz de `lambda_brief_modern/` limpa e legível, é proibido criar pastas genéricas soltas. Todo experimento vai para:
```text
lambda_brief_modern/experimentos/<AAAA-MM-DD>_<nome>/
```
e ganha uma linha em `lambda_brief_modern/experimentos/INDICE.md`. Os scripts (`benchmark_runner.py`, `paper_comparison*.py`, `run_piloto.sh`, `batch_run_all_maps.py`) fazem isso sozinhos quando não se passa `--out`/`--output_base_dir`; para uma pasta feita à mão, use `python3 scripts/experimentos.py nova <nome> "<descrição>"`.
- **Dentro de um benchmark:** `<dataset>/<mapa>/<descritor>_regime_<regime>/seed_<N>/`.
- **Dentro de um teste do artigo:** `execucoes/<VOO>/<mapa>/<descritor>_regime_<regime>/seed_<N>/`.
- **Entradas reutilizadas** (odometria fixa, relatórios do altímetro): `lambda_brief_modern/insumos/`.
- Além do `HISTORICO_RESULTADOS.md`, acrescente um resumo curto do teste na seção "Resumo dos testes" do `INDICE.md`.
- As pastas antigas (`results_*`, `resultados/`, `results_final/`) foram movidas; ver `lambda_brief_modern/REORGANIZACAO_PASTAS.md`.

---

## 3. Diretrizes de Rigor Metodológico e Pesquisa

1. **Reprodutibilidade Estrita com Sementes (`--seed`):** Nunca execute experimentos sem declarar uma semente explícita (`--seed N`). Para análises estatísticas pareadas, utilize conjuntos idênticos de sementes (ex.: sementes 1 a 30) para todos os descritores comparados.
2. **Isolamento de Variáveis (Odometria Fixa):** Ao comparar descritores (como abBRIEF vs $\lambda$-BRIEF), utilize a mesma odometria pré-gravada (`--odom_file`) para garantir que as diferenças de erro decorram puramente do modelo de observação visual e não de ruídos estocásticos da VO.
3. **Não Quebrar Convenções Físicas e Coordenadas:**
   - Origem do mapa: canto superior esquerdo (y para baixo).
   - Metros por pixel: ADM = **0.188679245 m/px**, UFRGS VET (V3) = **0.10 m/px**.
   - Janelas de avaliação com Ground Truth:
     - ADM2 (`0036`): `--start_frame 84 --end_frame 279`
     - UFRGS VET (`voo_100m`): `--start_frame 204 --end_frame 427`
     - ADM1 (`0035`): `--start_frame 18 --end_frame 318`
4. **Preservação de Código e Documentação:** Mantenha comentários, documentação e contratos de API intactos. Consulte [`CLAUDE.md`](file:///home/vitor-ochoa/Storage/UAVs%20Localization/CLAUDE.md) para detalhes técnicos aprofundados da engine C++17.

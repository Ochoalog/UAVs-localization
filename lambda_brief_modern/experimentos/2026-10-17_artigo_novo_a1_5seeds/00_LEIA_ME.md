# Teste do artigo λ-BRIEF — abBRIEF × λ-BRIEF (α=1), média de 5 seeds

Protocolo **novo**: regime `final_baro`, λ-BRIEF com α = 1, seeds 1..5, erro contra o GT `traj_truth_sync.txt` (ADM3: GT do Zenodo).

Mesmo teste do `paper_comparison.py` (cada voo × cada mapa do artigo × abBRIEF e λ-BRIEF), repetido para 5 seeds. As curvas dos compilados são a **média** das seeds, como no artigo (que usa 30).

## Onde olhar

| Arquivo | O que mostra |
|---|---|
| `tabela.md` | Por voo e mapa: convergiu k/N, MAE médio ± dp, em quantas seeds o λ ganhou; resumo por voo no fim |
| `compilado_artigo_ADM.png` / `compilado_artigo_UFRGS.png` | Como as Figs. 5 e 4, com a curva média de cada mapa |
| `<VOO>/<VOO>_mapa_<MM-AAAA>.png` | Um mapa (ano): média de cada método (linha grossa), cada seed (traço fino) e a faixa melhor–pior; embaixo, o mapa com as trajetórias da seed de MAE mediano |
| `<VOO>/<VOO>_compilado.png` | Curvas médias do voo com a miniatura de cada mapa |
| `<VOO>/seeds/` | A figura de cada seed sozinha |
| `tabela.csv` | Tudo em números, incluindo o MAE de cada seed |
| `execucoes/` | Saída bruta da engine |

## Resumo por voo

- **ADM1**: λ com MAE médio menor em 5/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 40.8 m × λ 27.7 m; Wilcoxon pareado p = 0.002.
- **ADM2**: λ com MAE médio menor em 3/5 mapas; λ venceu 11/25 pares (mapa × seed); MAE médio ab 49.8 m × λ 44.8 m; Wilcoxon pareado p = 0.791.
- **ADM3**: λ com MAE médio menor em 0/5 mapas; λ venceu 2/25 pares (mapa × seed); MAE médio ab 23.6 m × λ 37.7 m; Wilcoxon pareado p = 0.000.
- **UFRGS**: λ com MAE médio menor em 4/5 mapas; λ venceu 18/25 pares (mapa × seed); MAE médio ab 29.3 m × λ 15.7 m; Wilcoxon pareado p = 0.007.

Voos, mapas e datas: os mesmos do teste de 1 seed (ver o `00_LEIA_ME.md` daquela pasta e `docs/voos_e_mapas_do_artigo.md`).

# Teste do artigo λ-BRIEF — abBRIEF × λ-BRIEF, média de 1 seeds

Protocolo **novo**: regime `final_baro`, λ-BRIEF com α = 0.5, seeds 1, erro contra o GT `traj_truth_sync.txt` (ADM3: GT do Zenodo).

Mesmo teste do `paper_comparison.py` (cada voo × cada mapa do artigo × abBRIEF e λ-BRIEF), repetido para 1 seeds. As curvas dos compilados são a **média** das seeds, como no artigo (que usa 30).

20 execuções foram copiadas de `experimentos/2026-10-17_artigo_novo_a1` (mesmo comando, mesma seed) em vez de rodadas de novo; cada uma tem um `REAPROVEITADO_DE.txt`.

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

- **ADM1**: λ com MAE médio menor em 3/5 mapas; λ venceu 3/5 pares (mapa × seed); MAE médio ab 55.3 m × λ 43.2 m.
- **ADM2**: λ com MAE médio menor em 2/5 mapas; λ venceu 2/5 pares (mapa × seed); MAE médio ab 47.4 m × λ 37.9 m.
- **ADM3**: λ com MAE médio menor em 0/5 mapas; λ venceu 0/5 pares (mapa × seed); MAE médio ab 20.9 m × λ 23.2 m.
- **UFRGS**: λ com MAE médio menor em 3/5 mapas; λ venceu 3/5 pares (mapa × seed); MAE médio ab 29.6 m × λ 17.6 m.

Voos, mapas e datas: os mesmos do teste de 1 seed (ver o `00_LEIA_ME.md` daquela pasta e `docs/voos_e_mapas_do_artigo.md`).

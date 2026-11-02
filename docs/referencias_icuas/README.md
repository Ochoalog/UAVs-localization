# Referências para o paper do ICUAS (λ-BRIEF)

Levantamento de 02/11/2026 (~40 buscas na web). As chaves BibTeX estão em `referencias.bib`.
Status: **✔** = DOI/metadados confirmados em pelo menos uma fonte bibliográfica;
**⚠** = falta conferir DOI, páginas ou autores no IEEE Xplore ou na editora antes de submeter.

## Como cada grupo entra no texto

| Seção do paper | O que citar | Chaves |
|---|---|---|
| Introdução: VANT sem GNSS, localização visual absoluta | revisão + trabalhos clássicos | `couturier2021review` ✔, `conte2009vision` ✔, `masselli2016terrain` ✔, `shan2017googlemap` ⚠ |
| Trabalho base (abBRIEF, MCL sobre satélite, mesmo grupo/dataset) | o artigo que o λ-BRIEF estende | `mantelli2019abbrief` ✔ |
| Relacionados: filtro de partículas + casamento com mapa | os mais próximos do nosso pipeline | `kinnari2021gnss` ✔, `kinnari2022season` ✔, `kinnari2023lsvl` ✔ (usa o **abBRIEF como baseline**), `jurevicius2019robust` ⚠, `jurevicius2017application` ⚠, `couturier2019uav` ✔, `yuan2025swapf` ✔ (arXiv, 4-DoF como o nosso), `li2024expediting` ✔ |
| Relacionados: métodos aprendidos e cross-view (contraste com descritor artesanal e leve) | justificam por que um descritor binário ainda interessa embarcado | `goforth2019gps` ⚠, `patel2020visual` ⚠, `gurgu2022wildnav` ✔, `zheng2020university1652` ✔, `keetha2023anyloc` ⚠, `oquab2024dinov2` ✔, `xiao2024sthn` ⚠, `xu2024uavvisloc` ✔ (arXiv), `aerialvl2024` ⚠ |
| Método: MCL, Low Variance Sampler, *kidnapped robot*, injeção de partículas | fundamentos do filtro | `dellaert1999mcl` ✔, `fox1999mcl` ✔, `thrun2001robust` ⚠ (Mixture-MCL, base da injeção), `thrun2005probabilistic` ✔ (Low Variance Sampler, cap. 4), `lenser2000sensor` ⚠ (sensor resetting = nossa `--inject_ratio`), `engelson1992error` ✔ |
| Método: descritor | BRIEF e derivados | `calonder2010brief` ✔, `rublee2011orb` ✔ |
| Método: odometria visual | SIFT + FLANN + homografia robusta | `lowe2004sift` ✔, `muja2009flann` ✔, `fischler1981ransac` ✔, `barath2020magsacpp` ✔ |
| Método: máscara de vegetação | NDVI e câmera Sequoia | `rouse1974ndvi` ⚠ (páginas), `fawcett2020multiscale` ✔, `alamrani2021sequoia` ⚠ (título/autor a confirmar) |

## Observações importantes

1. **Nenhum paper do próprio ICUAS sobre localização visual com satélite apareceu nas buscas**
   (três tentativas, uma restrita ao `ieeexplore.ieee.org`). Recomendo procurar manualmente nos anais do
   ICUAS 2022–2025 no IEEE Xplore (*Publication Title = "International Conference on Unmanned Aircraft
   Systems"*), com os termos "localization", "GNSS-denied", "satellite", "particle filter". Citar 2–3
   trabalhos do próprio evento costuma ajudar na revisão.
2. **LSVL usa o abBRIEF como baseline e mostra que perde para descritores aprendidos.** Isso precisa ser
   discutido no texto: o argumento do λ-BRIEF é custo baixo, nenhum treino e o uso da informação
   multiespectral, e não superar métodos aprendidos.
3. **SWA-PF (arXiv, set/2025)** é o trabalho mais próximo em formulação (filtro de partículas 4-DoF, x, y,
   altitude, yaw). Ainda é só arXiv: confira se saiu em algum evento antes de citar.
4. O `thrun2001robust` (Mixture-MCL) e o `lenser2000sensor` (sensor resetting) são as referências certas
   para a injeção de partículas aleatórias (`--inject_ratio`) e a relocalização. O `--two_phase` não tem
   referência direta: é contribuição nossa.
5. O NDVI costuma ser atribuído a Rouse et al. (1974), mas o texto original fala em "transformed vegetation
   index". A citação é a padrão da área; as páginas variam entre fontes (309–317 / 301–317).
6. IEEE Xplore também listou estes trabalhos, sem metadados completos (autores, DOI); vale abrir se
   precisar de mais relacionados recentes:
   - *An Oblique-Robust Absolute Visual Localization Method for GPS-Denied UAV With Satellite Imagery*
     (doc. 10356107)
   - *Aerial-Satellite Image Matching Framework for UAV Absolute Visual Localization using Contrastive
     Learning* (doc. 9649999)
   - *GeoRVLF: A Robust Drone-Satellite Visual Geo-Localization Framework* (doc. 11009138)
   - *Multi Map Visual Localization for Unmanned Aerial Vehicles* (doc. 10803035, MCL com vários mapas de satélite)
   - *VL-MFL: UAV Visual Localization Based on Multisource Image Feature Learning* (doc. 10488142)

## Fontes principais consultadas

- abBRIEF: <https://www.sciencedirect.com/science/article/abs/pii/S092188901830438X>
- LSVL: <https://arxiv.org/abs/2212.03581>, <https://dl.acm.org/doi/10.1016/j.robot.2023.104497>
- Kinnari ICAR/RA-L: <https://arxiv.org/abs/2103.14381>, <https://arxiv.org/abs/2110.01967>
- SWA-PF: <https://arxiv.org/abs/2509.13795>
- Jurevičius: <https://arxiv.org/abs/1910.12121>
- Goforth & Lucey: <https://publications.ri.cmu.edu/gps-denied-uav-localization-using-pre-existing-satellite-imagery>
- Patel et al.: <https://www.dynsyslab.org/wp-content/papercite-data/pdf/patel-icra20.pdf>
- Conte & Doherty: <https://doi.org/10.1155/2009/387308>
- Wildnav: <https://arxiv.org/abs/2210.09727>
- University-1652: <https://arxiv.org/abs/2002.12186>
- AnyLoc: <https://arxiv.org/abs/2308.00688>
- STHN: <https://arxiv.org/abs/2405.20470>
- UAV-VisLoc: <https://arxiv.org/abs/2405.11936>
- MCL: <https://www.ri.cmu.edu/publications/monte-carlo-localization-for-mobile-robots>,
  <https://www.ri.cmu.edu/publications/robust-monte-carlo-localization-for-mobile-robots>
- Sensor resetting: <https://www.ri.cmu.edu/publications/sensor-resetting-localization-for-poorly-modelled-mobile-robots>
- BRIEF: <https://doi.org/10.1007/978-3-642-15561-1_56>; ORB: <https://mlanthology.org/iccv/2011/rublee2011iccv-orb>
- SIFT: <https://doi.org/10.1023/B:VISI.0000029664.99615.94>; FLANN: <https://www.scitepress.org/Papers/2009/17878>
- MAGSAC++: <https://openaccess.thecvf.com/content_CVPR_2020/html/Barath_MAGSAC_a_Fast_Reliable_and_Accurate_Robust_Estimator_CVPR_2020_paper.html>
- NDVI (Rouse 1974): <https://ntrs.nasa.gov/citations/19740022614>
- Sequoia: <https://doaj.org/article/f12e0fe505b74fd19b667bc1ecce1024>, <https://api.crossref.org/works/10.3390%2FRS12030514>

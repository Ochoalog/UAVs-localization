from common import *
S=load_scenes(); FP=np.load('footprints.npz')
o=map_outline('ADM_35'); MM=np.zeros((SZ,SZ),np.uint8); cv2.fillPoly(MM,[np.round(o*8).astype(np.int32)],1,shift=3); MM=MM.astype(bool)
R={'mapa':MM,**{k:FP[k] for k in FP.files}}
def dn(a,b): return idx(S[b])['ndvi']-idx(S[a])['ndvi'], ~(bad(S[a])|bad(S[b]))
print('fração de pixels com queda de NDVI > 0.2')
print(f"{'par':34s}"+''.join(f'{k:>9s}' for k in R))
for name,(a,b) in {'base 2023 08-15→08-30':('2023-08-15','2023-08-30'),'set/2023 08-30→09-29':('2023-08-30','2023-09-29'),
                   'base 2024 04-06→04-21':('2024-04-06','2024-04-21'),'mai/2024 04-21→05-06':('2024-04-21','2024-05-06'),
                   'mai/2024 04-21→06-30':('2024-04-21','2024-06-30'),
                   'abril: voo 2019→2024-04-21':('2019-04-18','2024-04-21'),'abril: voo 2019→2025-04-21':('2019-04-18','2025-04-21'),
                   'abril: voo 2019→2026-04-06':('2019-04-18','2026-04-06')}.items():
    d,ok=dn(a,b); print(f'{name:34s}'+''.join(f'{((d<-0.2)&ok)[m].mean()*100:8.1f}%' for m in R.values()))

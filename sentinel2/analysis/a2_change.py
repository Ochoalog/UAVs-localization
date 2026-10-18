from common import *
S=load_scenes(); FP=np.load('footprints.npz')
B=['B02','B03','B04','B08','B11']
def cva(a,b):
    A=np.stack([S[a][k] for k in B]); Bq=np.stack([S[b][k] for k in B])
    v=np.sqrt(((A-Bq)**2).sum(0)); ok=~(bad(S[a])|bad(S[b])); return v,ok
# map extent mask
o=map_outline('ADM_35'); MM=np.zeros((SZ,SZ),np.uint8); cv2.fillPoly(MM,[np.round(o*8).astype(np.int32)],1,shift=3); MM=MM.astype(bool)
regions={'AOI inteira':np.ones((SZ,SZ),bool),'mapa ADM_35':MM,**{k:FP[k] for k in FP.files}}
pairs={'base 2023 (08-15→08-30)':('2023-08-15','2023-08-30'),'enchente set/2023 (08-30→09-29)':('2023-08-30','2023-09-29'),
       'base 2024 (04-06→04-21)':('2024-04-06','2024-04-21'),'enchente mai/2024 (04-21→05-06)':('2024-04-21','2024-05-06'),
       'mai/2024 +3 sem (04-21→05-21)':('2024-04-21','2024-05-21')}
# threshold from baseline pairs: 99th pct over AOI
bl=np.concatenate([cva(*pairs[k])[0][cva(*pairs[k])[1]] for k in pairs if k.startswith('base')])
thr=np.percentile(bl,99); print('CVA threshold (99th pct of baseline pairs) =',round(thr,4))
print(f"{'par':38s}"+''.join(f'{r:>14s}' for r in regions))
out={}
for name,(a,b) in pairs.items():
    v,ok=cva(a,b); ch=(v>thr)&ok
    row=[ch[m&ok].mean() for m in regions.values()]; out[name]=row
    print(f'{name:38s}'+''.join(f'{x*100:13.1f}%' for x in row))
# NDVI/brightness means in footprints
print('\nmédias dentro da pegada ADM2 / ADM1 / ADM3 (NDVI | brilho):')
for d in ['2019-04-18','2023-08-30','2023-09-29','2024-04-21','2024-05-06','2024-05-21','2024-06-30','2025-04-21','2026-04-06']:
    I=idx(S[d]); s=' | '.join(f"{k}: {I['ndvi'][FP[k]].mean():.2f} {I['bright'][FP[k]].mean():.3f}" for k in FP.files)
    print(d,s)
np.save('cva_thr.npy',thr)

from common import *
from scipy.stats import spearmanr
S=load_scenes(); MD='Datasets/ADM/0036/Mapas/'
yy,xx=np.mgrid[0:4800,0:4800]; c,r=map2s2('ADM_35',xx.ravel(),yy.ravel()); cell=(np.floor(r).astype(int)*SZ+np.floor(c).astype(int))
def cellmean(v):
    s=np.bincount(cell,weights=v.ravel(),minlength=SZ*SZ); n=np.bincount(cell,minlength=SZ*SZ); return (s/np.maximum(n,1)).reshape(SZ,SZ), n.reshape(SZ,SZ)
def lab(f): return cv2.cvtColor(cv2.imread(MD+f),cv2.COLOR_BGR2LAB).astype(np.float32)
A=lab('ADM_35_12_09_2018.jpg'); Bm=lab('ADM_35_17_12_2018.jpg')
pix=np.sqrt(((A-Bm)**2).sum(-1))                     # per-pixel |ΔLab| at 0.18 m
hr_tex,n=cellmean(pix)
mA=[cellmean(A[...,k])[0] for k in range(3)]; mB=[cellmean(Bm[...,k])[0] for k in range(3)]
hr_mean=np.sqrt(sum((a-b)**2 for a,b in zip(mA,mB)))   # Δ of cell-mean colour
full=n>=2500   # cells fully inside map (~55x55 px of 0.18 m)
Bd=['B02','B03','B04','B08','B11']
s2cva=np.sqrt(sum((S['2018-09-05'][k]-S['2018-12-04'][k])**2 for k in Bd)); s2dn=np.abs(idx(S['2018-12-04'])['ndvi']-idx(S['2018-09-05'])['ndvi'])
s2rgb=np.sqrt(sum((S['2018-09-05'][k]-S['2018-12-04'][k])**2 for k in ['B02','B03','B04']))
print('células 10 m dentro do mapa:',full.sum())
for nm,h in [('mapa: Δ cor média da célula',hr_mean),('mapa: média de |ΔLab| por pixel (inclui textura)',hr_tex)]:
    for sn,s in [('S2 CVA 5 bandas',s2cva),('S2 |ΔNDVI|',s2dn),('S2 Δ só RGB',s2rgb)]:
        rho=spearmanr(h[full],s[full]).correlation
        top_h=h[full]>np.percentile(h[full],80); top_s=s[full]>np.percentile(s[full],80)
        iou=(top_h&top_s).sum()/(top_h|top_s).sum()
        print(f'{nm:48s} vs {sn:16s}: Spearman {rho:.2f} | IoU dos 20% mais alterados {iou:.2f}')
np.savez('res_check.npz',hr_mean=hr_mean,hr_tex=hr_tex,s2cva=s2cva,s2dn=s2dn,full=full)

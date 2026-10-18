from common import *
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
S=load_scenes(); FP=np.load('footprints.npz'); thr=float(np.load('cva_thr.npy'))
B=['B02','B03','B04','B08','B11']
def rgb(d): b=S[d]; return np.clip(np.stack([b['B04'],b['B03'],b['B02']],-1)/0.22,0,1)**(1/1.5)
def cva(a,b): return np.sqrt(sum((S[a][k]-S[b][k])**2 for k in B))
o=map_outline('ADM_35'); cols={'ADM1':'red','ADM2':'yellow','ADM3':'magenta'}
panels=[('RGB 2024-04-21 (pré)',rgb('2024-04-21'),None),('RGB 2024-05-06 (pós)',rgb('2024-05-06'),None),
        ('ΔNDVI 04-21→05-06',idx(S['2024-05-06'])['ndvi']-idx(S['2024-04-21'])['ndvi'],('RdBu',-0.6,0.6)),
        ('mudança > limiar\nenchente mai/2024',(cva('2024-04-21','2024-05-06')>thr).astype(float),('Reds',0,1)),
        ('mudança > limiar\nbase 04-06→04-21 (sem evento)',(cva('2024-04-06','2024-04-21')>thr).astype(float),('Reds',0,1)),
        ('mudança > limiar\nenchente set/2023',(cva('2023-08-30','2023-09-29')>thr).astype(float),('Reds',0,1))]
fig,ax=plt.subplots(1,6,figsize=(22,4.6))
for a,(t,im,cm) in zip(ax,panels):
    if cm: h=a.imshow(im,cmap=cm[0],vmin=cm[1],vmax=cm[2])
    else: a.imshow(im)
    a.plot(*np.r_[o,o[:1]].T,'c-',lw=0.8)
    for k in FP.files: a.contour(FP[k],[0.5],colors=cols[k],linewidths=0.9)
    a.set_xlim(90,260); a.set_ylim(230,60); a.set_title(t,fontsize=10); a.axis('off')
fig.tight_layout(); fig.savefig('s2_change_maps.png',dpi=110)

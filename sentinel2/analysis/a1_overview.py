from common import *
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
S=load_scenes(); print(sorted(S))
F=flights(); fm={k:footprint_mask(t,m)[0] for k,(t,m) in F.items()}
np.savez('footprints.npz',**{k.split()[0]:v for k,v in fm.items()})
dates=['2019-04-18','2023-08-30','2023-09-29','2023-10-14','2024-04-21','2024-05-06','2024-05-21','2024-06-30','2024-12-17','2025-10-15','2026-09-08']
fig,ax=plt.subplots(2,6,figsize=(19,7)); ax=ax.ravel()
cols={'ADM1 (0035)':'red','ADM2 (0036)':'yellow','ADM3 (0040)':'magenta'}
for i,d in enumerate(dates):
    b=S[d]; rgb=np.clip(np.stack([b['B04'],b['B03'],b['B02']],-1)/0.22,0,1)**(1/1.5)
    ax[i].imshow(rgb); ax[i].set_title(d,fontsize=10)
    o=map_outline('ADM_35'); ax[i].plot(*np.r_[o,o[:1]].T,'c-',lw=0.8)
    for k,m in fm.items(): ax[i].contour(m,[0.5],colors=cols[k],linewidths=0.8)
    ax[i].set_xlim(40,300); ax[i].set_ylim(280,20); ax[i].axis('off')
ax[-1].axis('off')
for k,c in cols.items(): ax[-1].plot([],[],color=c,label=k)
ax[-1].plot([],[],'c-',label='mapa ADM_35'); ax[-1].legend(loc='center')
fig.suptitle('Sentinel-2 L2A (RGB, 10 m) — Arroio do Meio: voo 2019, enchentes set/2023 e mai/2024, recuperação',fontsize=12)
fig.tight_layout(); fig.savefig('s2_overview.png',dpi=110)

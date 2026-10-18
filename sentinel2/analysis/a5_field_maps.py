from common import *
S=load_scenes(); FP=np.load('footprints.npz')
poly=np.array([(470,150),(560,160),(610,300),(530,420),(450,380),(455,250)],float)*6
c,r=map2s2('ADM_35',poly[:,0],poly[:,1]); FM=np.zeros((SZ,SZ),np.uint8); cv2.fillPoly(FM,[np.round(np.c_[c,r]*8).astype(np.int32)],1,shift=3); FM=FM.astype(bool)
er=cv2.erode(FM.astype(np.uint8),np.ones((3,3),np.uint8)).astype(bool)  # interior only
print('pixels lavoura (interior):',er.sum(), ' fração da pegada ADM2 ocupada pela lavoura:', round((FM&FP['ADM2']).sum()/FP['ADM2'].sum(),3))
print('\nNDVI e brilho da lavoura (interior) vs. resto da pegada ADM2')
rest=FP['ADM2']&~FM
for d in ['2018-08-16','2018-08-26','2018-09-05','2018-09-10','2018-10-05','2018-12-04','2019-03-24','2019-04-18','2019-05-03']:
    I=idx(S[d]); print(d, f"lavoura NDVI {I['ndvi'][er].mean():.2f} brilho {I['bright'][er].mean():.3f} | resto ADM2 NDVI {I['ndvi'][rest].mean():.2f}")
print('\nT1: mudança (data do mapa -> data do voo) dentro da pegada ADM2 vs. sucesso no benchmark')
B=['B02','B03','B04','B08','B11']; f='2019-04-18'
succ={'2018-08-16':('17/08/2018',76.7,83.3),'2018-09-05':('05/09/2018 (=12/09)',53.3,80.0),'2018-12-04':('17/12/2018',63.3,73.3)}
for d,(nm,ab,lb) in succ.items():
    v=np.sqrt(sum((S[d][k]-S[f][k])**2 for k in B)); dnd=np.abs(idx(S[d])['ndvi']-idx(S[f])['ndvi'])
    m=FP['ADM2']; print(f"mapa {nm:22s} S2 {d}: CVA médio {v[m].mean():.3f} | |ΔNDVI| médio {dnd[m].mean():.3f} | lavoura |ΔNDVI| {dnd[er].mean():.3f} | sucesso abBRIEF {ab}% λ-BRIEF {lb}%")
np.save('field_mask.npy',FM)

# Rodar a partir da raiz do repositório: cd "UAVs Localization" && PYTHONPATH=sentinel2/analysis .venv-s2/bin/python sentinel2/analysis/a1_overview.py  (depois a2..a6). Requer: pip install opencv-python-headless scipy
import numpy as np, json, glob, os, cv2
S2DIR='Datasets/Sentinel2/ADM/scenes'
DS='Datasets/ADM/'
G=json.load(open('sentinel2/map_georef.json'))['maps']
X0,Y0,R=406840.0,6748780.0,10.0; SZ=324
def px2utm(name,x,y):
    ce,cn=G[name]['px_to_utm']; x=np.asarray(x,float); y=np.asarray(y,float)
    return ce[0]*x+ce[1]*y+ce[2], cn[0]*x+cn[1]*y+cn[2]
def utm2s2(E,N): return (E-X0)/R, (Y0-N)/R
def map2s2(name,x,y): return utm2s2(*px2utm(name,x,y))
def load_scenes():
    S={}
    for f in sorted(glob.glob(S2DIR+'/*.npz')):
        d=np.load(f); r=d['refl'].astype(np.float32)/1e4
        b=dict(zip([str(x) for x in d['bands']],r)); b['scl']=d['scl']; S[os.path.basename(f)[:10]]=b
    return S
def idx(b):
    e=1e-6
    return dict(ndvi=(b['B08']-b['B04'])/(b['B08']+b['B04']+e),
                mndwi=(b['B03']-b['B11'])/(b['B03']+b['B11']+e),
                ndwi=(b['B03']-b['B08'])/(b['B03']+b['B08']+e),
                bright=(b['B02']+b['B03']+b['B04'])/3)
def bad(b): return np.isin(b['scl'],[0,1,3,8,9,10])
W,H=1241,913
def footprint_mask(traj,mapname,step=1):
    m=np.zeros((SZ,SZ),np.uint8); per=[]
    for (x,y,z,yaw) in traj[::step]:
        th=np.radians(-yaw)-np.pi/2
        pts=[]
        for u,v in [(-W/2,-H/2),(W/2,-H/2),(W/2,H/2),(-W/2,H/2)]:
            du,dv=z*u,z*v; pts.append((x+du*np.cos(th)+dv*np.sin(th), y-du*np.sin(th)+dv*np.cos(th)))
        pts=np.array(pts); c,r=map2s2(mapname,pts[:,0],pts[:,1])
        poly=np.round(np.c_[c,r]*8).astype(np.int32)  # subpixel shift 3
        fm=np.zeros((SZ,SZ),np.uint8); cv2.fillPoly(fm,[poly],1,shift=3); per.append(fm.astype(bool)); m|=fm
    return m.astype(bool), per
def flights():
    T35=np.loadtxt(DS+'0035/traj_truth.txt'); T36=np.loadtxt(DS+'0036/traj_truth.txt'); T40=np.loadtxt(DS+'0040/traj_truth.txt')
    return {'ADM1 (0035)':(T35,'ADM_35'),'ADM2 (0036)':(T36,'ADM_35'),'ADM3 (0040)':(T40,'ADM_V2')}
def map_outline(name):
    c,r=map2s2(name,[0,4800,4800,0],[0,0,4800,4800]); return np.c_[c,r]

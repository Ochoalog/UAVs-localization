# -*- coding: utf-8 -*-
import numpy as np
import scipy
from scipy import stats
import sys
from pylab import *
import itertools
import os

DS = "UFRGS"#ADM/VET

# Parameters
################## MAPS ####################
Maps=[
"UFRGS_VET_V3_02_01_2013",
"UFRGS_VET_V3_02_03_2013",
"UFRGS_VET_V3_04_04_2014",
"UFRGS_VET_V3_05_08_2017",
"UFRGS_VET_V3_06_05_2017",
"UFRGS_VET_V3_11_07_2014",
"UFRGS_VET_V3_12_01_2017",
"UFRGS_VET_V3_12_08_2016",
"UFRGS_VET_V3_12_09_2013",
"UFRGS_VET_V3_13_01_2013",
"UFRGS_VET_V3_13_05_2016",
"UFRGS_VET_V3_14_06_2014",
"UFRGS_VET_V3_14_07_2014",
"UFRGS_VET_V3_15_06_2015",
"UFRGS_VET_V3_16_01_2013",
"UFRGS_VET_V3_16_02_2013",
"UFRGS_VET_V3_17_12_2012",
"UFRGS_VET_V3_18_03_2015",
"UFRGS_VET_V3_19_01_2017",
"UFRGS_VET_V3_19_05_2015",
"UFRGS_VET_V3_20_07_2014",
"UFRGS_VET_V3_21_03_2018",
"UFRGS_VET_V3_22_01_2012",
"UFRGS_VET_V3_22_11_2013",
"UFRGS_VET_V3_23_08_2017",
"UFRGS_VET_V3_24_02_2010",
"UFRGS_VET_V3_24_05_2016",
"UFRGS_VET_V3_26_02_2013",
"UFRGS_VET_V3_26_08_2015",
"UFRGS_VET_V3_26_08_2016",
"UFRGS_VET_V3_28_04_2017",
"UFRGS_VET_V3_29_08_2018",
"UFRGS_VET_V3_30_10_2010",
"UFRGS_VET_V3_30_11_2013",
"UFRGS_VET_V3_31_07_2010"
]

MapsName=[
"UFRGS_VET_V3_02_01_2013",
"UFRGS_VET_V3_02_03_2013",
"UFRGS_VET_V3_04_04_2014",
"UFRGS_VET_V3_05_08_2017",
"UFRGS_VET_V3_06_05_2017",
"UFRGS_VET_V3_11_07_2014",
"UFRGS_VET_V3_12_01_2017",
"UFRGS_VET_V3_12_08_2016",
"UFRGS_VET_V3_12_09_2013",
"UFRGS_VET_V3_13_01_2013",
"UFRGS_VET_V3_13_05_2016",
"UFRGS_VET_V3_14_06_2014",
"UFRGS_VET_V3_14_07_2014",
"UFRGS_VET_V3_15_06_2015",
"UFRGS_VET_V3_16_01_2013",
"UFRGS_VET_V3_16_02_2013",
"UFRGS_VET_V3_17_12_2012",
"UFRGS_VET_V3_18_03_2015",
"UFRGS_VET_V3_19_01_2017",
"UFRGS_VET_V3_19_05_2015",
"UFRGS_VET_V3_20_07_2014",
"UFRGS_VET_V3_21_03_2018",
"UFRGS_VET_V3_22_01_2012",
"UFRGS_VET_V3_22_11_2013",
"UFRGS_VET_V3_23_08_2017",
"UFRGS_VET_V3_24_02_2010",
"UFRGS_VET_V3_24_05_2016",
"UFRGS_VET_V3_26_02_2013",
"UFRGS_VET_V3_26_08_2015",
"UFRGS_VET_V3_26_08_2016",
"UFRGS_VET_V3_28_04_2017",
"UFRGS_VET_V3_29_08_2018",
"UFRGS_VET_V3_30_10_2010",
"UFRGS_VET_V3_30_11_2013",
"UFRGS_VET_V3_31_07_2010"
]

printCorrigido = 0;

################## NUMTESTES ####################
numTests = 10

################## LENTRAJ ####################
LenTraj=224

################## CONFIGS ####################
Configs=[
"alfa_0__beta_0",
"alfa_0__beta_1" ,
"alfa_0__beta_4",
"alfa_0__beta_16",

"alfa_0,2__beta_0",
"alfa_0,2__beta_1" ,
"alfa_0,2__beta_4",
"alfa_0,2__beta_16",

"alfa_0,6__beta_0",
"alfa_0,6__beta_1" ,  
"alfa_0,6__beta_4",
"alfa_0,6__beta_16",

"alfa_1__beta_0",
"alfa_1__beta_1" , 
"alfa_1__beta_4",
"alfa_1__beta_16"
]

ConfigsName=[
"abBRIEF__alfa_0__beta_0",
"alfa_0__beta_1" ,
"alfa_0__beta_4",
"alfa_0__beta_16",

"alfa_0,2__beta_0",
"alfa_0,2__beta_1" ,
"alfa_0,2__beta_4",
"alfa_0,2__beta_16",

"alfa_0,6__beta_0",
"alfa_0,6__beta_1" ,  
"alfa_0,6__beta_4",
"alfa_0,6__beta_16",

"alfa_1__beta_0",
"alfa_1__beta_1" , 
"alfa_1__beta_4",
"alfa_1__beta_16"

"alfa_1__beta_0",
"alfa_1__beta_1",  
"alfa_1__beta_4",
"alfa_1__beta_16"
]

#=== ESCALA MAPAS V1 refeita (DS 035)================
#Imagem base: 140834_0120_RGBres_registeredcomp.jpg
#drone voando à 61,5 m de altura
#Mapa referência: ADM_35_12_09_2018.jpg
#drone: 808,161 - 939,164 = 131
#mapa: 2815,2483 - 2859, 2483 = 44
#escala = 44/131 = 0,335877863
#====================================================

#=== ESCALA  MAPAS V3 usando a imagem 390, à 101,09999847412 metros de altitude
#mapa (UFRGS_VET_V3_ESCALA.jpg)
#2116 - 2155 = 39
#camera drone (132741_0390_RGBres_registeredrotcomp.jpg)
#518-482 = 36
#escala = 39/36 = 1,083333333
#=========================
#1.083333333 = à 101.09999847412 m 
#0.5 = 46,66 m           5.0 = 466,62 m
#====================================================

if DS == "ADM":
	gtFileName = "GT2.txt"
	escalaPxM = 0.188679245
	divMultExcalaZ = 54.5
	zEscalaMetros = 0.335877863
	altitudeFotoRefEscala = 61.5
elif DS == "UFRGS":
	gtFileName = "UFRGS_VET_100M_GT_390_613.txt"
	escalaPxM = 0.10
	divMultExcalaZ = 135
	zEscalaMetros = 1.083333333   
	altitudeFotoRefEscala = 101.09999847412
################## TRAJ ####################
Traj="traj"


#Angles
roll = []
pitch = []
yaw = []
yawInvertido = []

################## Create array to put the values ########################
#Justone GT
gtX = []
gtY = []
gtZ = []
gtT = []

#range NO PYTHON 2, range NO PYTHON 3
#numTests values per config per map
testX = [[[[] for x in range(numTests)] for c in range(len(Configs))] for n in range(len(Maps))]
testY = [[[[] for x in range(numTests)] for c in range(len(Configs))] for n in range(len(Maps))]
testZ = [[[[] for x in range(numTests)] for c in range(len(Configs))] for n in range(len(Maps))]
testT = [[[[] for x in range(numTests)] for c in range(len(Configs))] for n in range(len(Maps))]

#One mean per config per map
meanX = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
meanY = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
meanZ = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
meanT = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
meanG = [[[] for c in range(len(Configs))] for n in range(len(Maps))]

#One error per config per map
errorXYZ = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
errorXYZcorr = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
errorXYZperImageAndConfig = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
errorXYZperImageAndConfigCorr = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
foraMapa = 0;

################## READ GROUND TRUTH ####################
gtFile =  open(gtFileName)
count = 0;
for line in gtFile:
	w = line.split()
	gtX.append(float(w[0])*escalaPxM)#pixel para metros
	gtY.append(float(w[1])*escalaPxM)#pixel para metros
	gtZ.append(float(w[2])) #altura do solo (já está em metros)
	gtT.append(float(w[3])) 

################## READ ANGLES ####################
angFile =  open("traj_angles.txt")
for line in angFile:
	w = line.split()
	roll.append(math.radians(float(w[0])))
	pitch.append(math.radians(float(w[1])))
	yaw.append(math.radians(float(w[2])))
	yawInvertido.append(math.radians(math.fmod((float(w[2]) + 180 + 180 + 360), 360)-180))

multExcalaZ = (divMultExcalaZ/escalaPxM); #Multiplicador utilizado p converter de escala p pixeis
dirX = 0;
dirY = 0;
dirZ = 0;

################## READ TEST FILES ####################
for c in range(len(Configs)):
	erroConfig = 0

	for m in range(len(Maps)):
		erroMap = 0
		for x in range(numTests):
			curFile =  open(Maps[m]+"/"+Traj+"/"+Configs[c]+ "/" + str(x) + ".txt")
			#print(Maps[m]+"/"+Traj+"/"+Configs[c]+ "/" + str(x) + ".txt")
			for line in curFile:
				w = line.split()
				if "meanPX" in line: 
    					continue
				else:
					testX[m][c][x].append(float(w[0]))
					testY[m][c][x].append(float(w[1]))
					testZ[m][c][x].append(float(w[2]))
					testT[m][c][x].append(float(w[6]))
			curFile.close()
			
		for t in range(LenTraj):			
			meanXAux = 0
			meanYAux = 0
			meanZAux = 0
			meanTAux = 0
			erroG = 0
			
			for x in range(numTests):
				if testX[m][c][x][t] > 4800 or testY[m][c][x][t] > 4800 or testX[m][c][x][t] < 0 or testY[m][c][x][t] < 0:
					foraMapa = 1;
					
				meanXAux = math.pow(testX[m][c][x][t]*escalaPxM-gtX[t],2)#pixel para metros
				meanYAux = math.pow(testY[m][c][x][t]*escalaPxM-gtY[t],2)#pixel para metros
				meanZAux = math.pow(testZ[m][c][x][t]*altitudeFotoRefEscala/zEscalaMetros-gtZ[t],2)			
				#erroG += math.sqrt(meanYAux + meanXAux)
				erroG += math.sqrt(meanZAux + meanYAux + meanXAux)		
			
			errorXYZ[m][c].append(erroG)
			erroConfig += erroG;
			erroMap += erroG;

		if foraMapa == 1:
			print("Alguma estimativa ficou  fora do mapa")
		foraMapa = 0;
	print(ConfigsName[c] + " " + str(erroConfig) + "\n\n")
		#print(MapsName[m] + " " + str(erroMap))

for image in range(LenTraj):     
    for c in range(len(Configs)): 
        errorXYZperConfigAUX = 0;
        for m in range(len(Maps)):        
           #print("Erro acumulado:" + MapsName[m] + "  " + ConfigsName[c] + " Image: " + str(image) + " " + str(errorXYZ[m][c][image]) + "\n\n")
           errorXYZperConfigAUX += errorXYZ[m][c][image]
        #print("errorXYZperConfigAUX: " + str(errorXYZperConfigAUX))
        errorXYZperImageAndConfig[m][c].append(errorXYZperConfigAUX/(numTests*len(Maps)))
        
##CORRIGIDO##
################## READ TEST FILES ####################
if printCorrigido == 1:
	for c in range(len(Configs)):
		erroConfig = 0

		for m in range(len(Maps)):
			erroMap = 0
			for x in range(numTests):
				curFile =  open(Maps[m]+"/"+Traj+"/"+Configs[c]+ "/" + str(x) + ".txt")

				for line in curFile:
					w = line.split()
					if "meanPX" in line: 
	    					continue
					else:
						testX[m][c][x].append(float(w[0]))
						testY[m][c][x].append(float(w[1]))
						testZ[m][c][x].append(float(w[2]))
						testT[m][c][x].append(float(w[6]))

				curFile.close()
			
			for t in range(LenTraj):
			
				meanXAux = 0
				meanYAux = 0
				meanZAux = 0
				meanTAux = 0
				erroG = 0
			
				for x in range(numTests):
					dirZ = (-testZ[m][c][x][t] /zEscalaMetros) * (multExcalaZ);#Z do teste, negativo, situado próximo ao chão
					xCorr = testX[m][c][x][t];
					yCorr = testY[m][c][x][t];
					zCorr = testZ[m][c][x][t];

					if testX[m][c][x][t] > 4800 or testY[m][c][x][t] > 4800 or testX[m][c][x][t] < 0 or testY[m][c][x][t] < 0:
						foraMapa = 1;

					xCorr+= dirX*(math.cos(pitch[t])*math.cos(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])-math.cos(roll[t])*math.sin(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])+math.sin(roll[t])*math.sin(yawInvertido[t]))    			
					yCorr += dirX*(math.cos(pitch[t])*math.sin(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])+math.cos(roll[t])*math.cos(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])-math.sin(roll[t])*math.cos(yawInvertido[t]))

					meanXAux = math.pow(xCorr*escalaPxM-gtX[t],2)#pixel para metros
					meanYAux = math.pow(yCorr*escalaPxM-gtY[t],2)#pixel para metros
					meanZAux = math.pow(testZ[m][c][x][t]*altitudeFotoRefEscala/zEscalaMetros-gtZ[t],2)#o Z não é corrigido
                    
                    #erroG += math.sqrt(meanYAux + meanXAux)
					erroG += math.sqrt(meanZAux + meanYAux + meanXAux)
			
				errorXYZcorr[m][c].append(erroG)			
				erroConfig += erroG;
				erroMap += erroG;

			if foraMapa == 1:
				print("Alguma estimativa ficou  fora do mapa")
			foraMapa = 0;
		print(ConfigsName[c] + " " + str(erroConfig) + "__CORR\n\n")
			#print(MapsName[m] + " " + str(erroMap))
if printCorrigido == 1:
    for image in range(LenTraj):     
        for c in range(len(Configs)): 
            errorXYZperConfigAUXCorr = 0;
            for m in range(len(Maps)):        
               #print("Erro acumulado:" + MapsName[m] + "  " + ConfigsName[c] + " Image: " + str(image) + " " + str(errorXYZcorr[m][c][image]) + "\n\n")
               errorXYZperConfigAUXCorr += errorXYZcorr[m][c][image]
            #print("errorXYZperConfigAUX: " + str(errorXYZperConfigAUXCorr))
            errorXYZperImageAndConfigCorr[m][c].append(errorXYZperConfigAUXCorr/(numTests*len(Maps)))
        
        
#===================================GRAFICO==================================================
font = {'family' : 'sans-serif',
'weight' : 'bold',
'size'   : 10}

matplotlib.rc('font', **font)

figure(figsize=(16,9))
legendNames = []
lstyle = ['-', '-','-','-', ':',':',':',':','--', '--','--','--','-.','-.','-.','-.','o','o','o','o','^','^','^','^','-','-','-']
cstyle = ['r', 'g', 'b','c','r', 'g', 'b','c','r', 'g', 'b','c','r', 'g', 'b','c','r', 'g', 'b','c','r', 'g', 'b','c','r', 'g', 'b','c',]

lstyleCorr = [':', ':',':',':', ':',':',':',':',':', ':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':',':']
cstyleCorr = ['r', 'g', 'b','c','m','y','k',('#6f0dba'),('#ba6c0d'),'r', 'g', 'b','c','m','y','k',('#00ff00'),('#ff0066'),'r','g','b','c','m','y','k',('#00ff00'),('#ff0066')]
for c in range(len(Configs)):
    legendNames.append(ConfigsName[c])
    #plot(errorXYZperImageAndConfigCorr[m][c],color=cstyle[(c%len(cstyle))],linestyle=lstyle[(c%len(lstyle))],linewidth=2)
    plot(errorXYZperImageAndConfig[m][c],color=cstyle[(c%len(cstyle))],linestyle=lstyle[(c%len(lstyle))],linewidth=2)
		

#GT CORRIGIDO
#if printCorrigido == 1:
#    for c in range(len(Configs)):
#        plot(errorXYZperImageAndConfigCorr[m][c],color=cstyle[(c%len(cstyleCorr))],linestyle=lstyleCorr[(c%len(lstyleCorr))],linewidth=2)

legend(legendNames, ncol=2)
ax = axes()
ax.set_ylabel("Erro (m)")
ax.set_xlabel("Instante")
ax.set_xlabel("Instante")
ax.set_xlim([0,LenTraj])
ax.set_ylim([0,300])
ax.set_aspect(0.5)
title('(c)')

if len(ConfigsName) == 1:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[0])
elif len(ConfigsName) == 3:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[1] + "_VS_" + ConfigsName[2])#config 0 é o abBRIEF, estou colocand como referência
elif len(MapsName) == 1:
	savefig("Graficos/" + MapsName[m])
else:
	savefig("Graficos/allMaps")
show()

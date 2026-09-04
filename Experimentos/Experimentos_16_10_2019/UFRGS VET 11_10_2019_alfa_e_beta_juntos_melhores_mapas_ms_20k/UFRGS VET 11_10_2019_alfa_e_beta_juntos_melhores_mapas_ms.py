# -*- coding: utf-8 -*-
import numpy as np
import scipy
from scipy import stats
import sys
from pylab import *
import itertools
import os

# Parameters
################## MAPS ####################
Maps=[
#"UFRGS_VET_V3_02_04_2015"
#"UFRGS_VET_V3_17_12_2002"
#"UFRGS_VET_V3_23_08_2016"
"UFRGS_VET_V3_18_09_2018"
]

MapsName=[
#"UFRGS_VET_V3_02_04_2015"
#"UFRGS_VET_V3_17_12_2002"
#"UFRGS_VET_V3_23_08_2016"
"UFRGS_VET_V3_18_09_2018"
]

printCorrigido = 0;

################## TRAJ ####################
Traj="traj"

################## LENTRAJ ####################
LenTraj=224

################## CONFIGS ####################
Configs=[
"abBRIEF__betaweight_0", 
"betaweight_1", 
"betaweight_2",
"betaweight_4",
"betaweight_8",
"betaweight_16",
"betaweight_32" 
]

ConfigsName=[
"abBRIEF__betaweight_0", 
"betaweight_1" ,
"betaweight_2",
"betaweight_4",
"betaweight_8",
"betaweight_16",
"betaweight_32" 
]

################## NUMTESTES ####################
numTests = 30

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
maxError = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
minError = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
qtdDivergiu = [[] for c in range(len(Configs))]

#One error per config per map
errorXYZ = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
errorXY = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
errorXYZcorr = [[[] for c in range(len(Configs))] for n in range(len(Maps))]
foraMapa = 0;

################## READ GROUND TRUTH ####################
#gtFile =  open("GT_"+Traj+".txt")
gtFile =  open("UFRGS_VET_100M_GT_390_613.txt")
count = 0;
for line in gtFile:
	w = line.split()
	gtX.append(float(w[0])*0.10)#pixel para metros
	gtY.append(float(w[1])*0.10)#pixel para metros
	gtZ.append(float(w[2])) #altura do solo (já está em metros)
	gtT.append(float(w[3])) 


#multExcalaZ = (45/0.188679245); #Multiplicador utilizado p converter de escala p pixeis
dirX = 0;
dirY = 0;
dirZ = 0;

#for i in range(len(roll)):
#	print("roll: " + str(roll[i]) + " pitch: " + str(pitch[i]) + " yaw: " + str(yaw[i]) + " yawInvertido: " + str(yawInvertido[i]) +  "\n\n")

################## READ TEST FILES ####################
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

		qtdDivergiuAux = 0	
		for t in range(LenTraj):
			
			meanXAux = 0
			meanYAux = 0
			meanZAux = 0
			meanTAux = 0
			maxErrorAux = 0
			minErrorAux = 999
			erroGxyz = 0
			erroGxyzAux = 0
			erroGxy = 0			
			
			for x in range(numTests):
				if testX[m][c][x][t] > 4800 or testY[m][c][x][t] > 4800 or testX[m][c][x][t] < 0 or testY[m][c][x][t] < 0:
					foraMapa = 1;
					
				meanXAux = math.pow(testX[m][c][x][t]*0.10-gtX[t],2)#pixel para metros
				meanYAux = math.pow(testY[m][c][x][t]*0.10-gtY[t],2)#pixel para metros
				meanZAux = math.pow(testZ[m][c][x][t]*101.09999847412/1.083333333-gtZ[t],2)#a 101,09999847412 metros a escala é de 1,083333333	
				erroGxyzAux = math.sqrt(meanZAux + meanYAux + meanXAux)	
				erroGxyz += erroGxyzAux	

				if t+1 == LenTraj:#Último teste					
					if erroGxyzAux > 100:
						qtdDivergiuAux += 1

				if maxErrorAux < erroGxyzAux:
					maxErrorAux = erroGxyzAux;

				if minErrorAux > erroGxyzAux:
					minErrorAux = erroGxyzAux;

			errorXY[m][c].append(erroGxy/numTests)
			errorXYZ[m][c].append(erroGxyz/numTests)
			maxError[m][c].append(maxErrorAux)
			minError[m][c].append(minErrorAux)			
	
			erroConfig += erroGxyz/numTests;
			erroMap += erroGxyz/numTests;

		if foraMapa == 1:
			print("Alguma estimativa ficou  fora do mapa")
		foraMapa = 0;
		qtdDivergiu[c].append(qtdDivergiuAux)
	print(ConfigsName[c] + " " + str(erroConfig) + "\n\n")
		#print(MapsName[m] + " " + str(erroMap))
			

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
					#dirZ = (-testZ[m][c][x][t] /0.366197183) * (multExcalaZ);#Z do teste, negativo, situado próximo ao chão
					dirZ = (-testZ[m][c][x][t]*101.09999847412/1.083333333)/(0.10) #escala para metros:(-testZ[m][c][x][t]*101.09999847412/1.083333333)	metros para px (-testZ[m][c][x][t]*101.09999847412/1.083333333)/(0.10)

					xCorr = testX[m][c][x][t];
					yCorr = testY[m][c][x][t];

					if testX[m][c][x][t] > 4800 or testY[m][c][x][t] > 4800 or testX[m][c][x][t] < 0 or testY[m][c][x][t] < 0:
						foraMapa = 1;

					xCorr+= dirX*(math.cos(pitch[t])*math.cos(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])-math.cos(roll[t])*math.sin(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])+math.sin(roll[t])*math.sin(yawInvertido[t]));    			
					yCorr += dirX*(math.cos(pitch[t])*math.sin(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])+math.cos(roll[t])*math.cos(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])-math.sin(roll[t])*math.cos(yawInvertido[t]));



					meanXAux = math.pow(xCorr*0.188679245-gtX[t],2)#pixel para metros
					meanYAux = math.pow(yCorr*0.188679245-gtY[t],2)#pixel para metros
					meanZAux = math.pow(testZ[m][c][x][t]*62/0.330769231-gtZ[t],2)#o Z não é corrigido

					erroG += math.sqrt(meanZAux + meanYAux + meanXAux)
					#erroG += math.sqrt(meanYAux + meanXAux)

				errorXYZcorr[m][c].append(erroG/numTests)
			
				erroConfig += erroG/numTests;
				erroMap += erroG/numTests;

			if foraMapa == 1:
				print("Alguma estimativa ficou  fora do mapa")
			foraMapa = 0;
		print(ConfigsName[c] + " " + str(erroConfig) + "__CORR\n\n")
			#print(MapsName[m] + " " + str(erroMap))

font = {'family' : 'sans-serif',
'weight' : 'bold',
'size'   : 10}

matplotlib.rc('font', **font)

#figure(figsize=(8,6))
figure(figsize=(16,9))
legendNames = []
lstyle = ['-', '-','-','-', '-','-','-','-','-']
cstyle = ['r', 'g', 'b','c','m','y','k',('#00ff00'),('#ff0066')]

lstyleCorr = [':',':',':',':',':',':',':',':',':']
cstyleCorr = ['r','g','b','c','m','y','k',('#00ff00'),('#ff0066')]

lstyleMaxError = ['--','--','--','--','--','--','--','--','--']
cstyleMaxError = ['r','g','b','c','m','y','k',('#00ff00'),('#ff0066')]

lstyleMinError = ['-.','-.','-.','-.','-.','-.','-.','-.','-.']
cstyleMinError = ['r','g','b','c','m','y','k',('#00ff00'),('#ff0066')]

for c in range(len(Configs)):
	for m in range(len(Maps)):
		plot(errorXYZ[m][c],color=cstyle[(c%len(cstyle))],linestyle=lstyle[(c%len(lstyle))],linewidth=2) #VOU PLOTSAR SÓ O CORRIGIDO

		if len(ConfigsName) > 1 and len(MapsName) > 1:
			legendNames.append(ConfigsName[c]+MapsName[m])
		elif len(ConfigsName) > 1 and len(MapsName) == 1:			
			legendNames.append(ConfigsName[c] + " Divergencias: " + str(qtdDivergiu[c]))
		elif len(ConfigsName) == 1 and len(MapsName) > 1:
			legendNames.append(MapsName[m])
		elif len(ConfigsName) == 1 and len(MapsName) == 1:
			legendNames.append(ConfigsName[c] + " Divergencias: " + str(qtdDivergiu[c]))
			print(str(qtdDivergiu[c]))		

#GT CORRIGIDO
if printCorrigido == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):					
			plot(errorXYZcorr[m][c],color=cstyle[(c%len(cstyleCorr))],linestyle=lstyle[(c%len(lstyleCorr))],linewidth=2)

#MAX ERROR
if len(ConfigsName) == 1 and len(MapsName) == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):
			plot(maxError[m][c],color=cstyle[2],linestyle=lstyleMaxError[2],linewidth=2)

#MIN ERROR
if len(ConfigsName) == 1 and len(MapsName) == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):
			plot(minError[m][c],color=cstyle[1],linestyle=lstyleMinError[1],linewidth=2)

legend(legendNames)
ax = axes()
ax.set_ylabel("Erro (m)")
ax.set_xlabel("Instante")
ax.set_xlabel("Instante")
ax.set_xlim([0,LenTraj])
ax.set_ylim([0,600])
ax.set_aspect(0.15)
title('(c)')

if len(ConfigsName) == 1:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[0])
elif len(ConfigsName) == 3:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[1] + "_VS_" + ConfigsName[2])#config 0 é o abBRIEF, estou colocand como referência
else:
	savefig("Graficos/" + MapsName[m])
show()

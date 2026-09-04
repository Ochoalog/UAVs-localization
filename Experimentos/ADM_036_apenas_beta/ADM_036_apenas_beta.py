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
#"ADM_35_12_09_2018",
#"ADM_35_17_08_2018",
#"ADM_35_17_12_2018",
#"ADM_35_05_09_2018",
"ADM_35_27_02_2017"
]

MapsName=[
#"ADM_35_12_09_2018",
#"ADM_35_17_08_2018",
#"ADM_35_17_12_2018",
#"ADM_35_05_09_2018",
"ADM_35_27_02_2017"
]

printCorrigido = 1;

################## TRAJ ####################
Traj="traj"

################## LENTRAJ ####################
LenTraj=196

################## CONFIGS ####################
Configs=[
"alfagaussmsk_0",
"alfagaussmsk_0.2",
"alfagaussmsk_0.4",
"alfagaussmsk_0.6",
"alfagaussmsk_0.8",
"alfagaussmsk_1.0" 
]

ConfigsName=[
"alfagaussmsk_0",
"alfagaussmsk_0,2",
"alfagaussmsk_0,4",
"alfagaussmsk_0,6",
"alfagaussmsk_0,8",
"alfagaussmsk_1,0" 
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
gtFile =  open("GT_ADM036_199_394.txt")
count = 0;
for line in gtFile:
	w = line.split()
	gtX.append(float(w[0])*0.188679245)#pixel para metros
	gtY.append(float(w[1])*0.188679245)#pixel para metros
	gtZ.append(float(w[2])) #altura do solo (já está em metros)
	gtT.append(float(w[3])) 

################## READ ANGLES ####################
angFile =  open("traj_angles_ADM036_199_394.txt")
for line in angFile:
	w = line.split()
	roll.append(math.radians(float(w[0])))
	pitch.append(math.radians(float(w[1])))
	yaw.append(math.radians(float(w[2])))
	yawInvertido.append(math.radians(math.fmod((float(w[2]) + 180 + 180 + 360), 360)-180))


	#DEG2RAD(fmod((p.yaw + 180 + 180 + 360), 360)-180);
	#double multExcalaZ = (45/0.188679245);//Multiplicador utilizado p converter de escala p pixeis
    #Pose3d orientacao = droneAngles[currentImage];
    #double dir[4] = {0, 0, (-part.p.z /0.366197183) * (multExcalaZ), 0};//Direcao inicial (Apontando p baixo)[x y z 0], e localizado na origem
    #double dirCamera[4] = {1, 0, 0, 0};

    #//Mover o ponto de origem do centro do drone para a posição da câmera
    #part.p.x += dirCamera[0]*(cos(orientacao.pitch)*cos(orientacao.yawInvertido)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)-cos(orientacao.roll)*sin(orientacao.yawInvertido)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)+sin(orientacao.roll)*sin(orientacao.yawInvertido));
    #part.p.y += dirCamera[0]*(cos(orientacao.pitch)*sin(orientacao.yawInvertido)) + dirCamera[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)+cos(orientacao.roll)*cos(orientacao.yawInvertido)) + dirCamera[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)-sin(orientacao.roll)*cos(orientacao.yawInvertido));
    #part.p.z += ((dirCamera[0]*-sin(orientacao.pitch) + dirCamera[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + dirCamera[2]*(cos(orientacao.roll)*cos(orientacao.pitch)))) * 0,366197183 / 45*0,188679245;

    #//Rotacionar a reta do drone até o chão
    #part.p.x += dir[0]*(cos(orientacao.pitch)*cos(orientacao.yawInvertido)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)-cos(orientacao.roll)*sin(orientacao.yawInvertido)) + dir[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*cos(orientacao.yawInvertido)+sin(orientacao.roll)*sin(orientacao.yawInvertido));
    #part.p.y += dir[0]*(cos(orientacao.pitch)*sin(orientacao.yawInvertido)) + dir[1]*(sin(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)+cos(orientacao.roll)*cos(orientacao.yawInvertido)) + dir[2]*(cos(orientacao.roll)*sin(orientacao.pitch)*sin(orientacao.yawInvertido)-sin(orientacao.roll)*cos(orientacao.yawInvertido));
    #part.p.z += ((dir[0]*-sin(orientacao.pitch) + dir[1]*(sin(orientacao.roll)*cos(orientacao.pitch)) + dir[2]*(cos(orientacao.roll)*cos(orientacao.pitch)))) * 0,366197183 / 45*0,188679245;


multExcalaZ = (45/0.188679245); #Multiplicador utilizado p converter de escala p pixeis
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
					
				meanXAux = math.pow(testX[m][c][x][t]*0.188679245-gtX[t],2)#pixel para metros
				meanYAux = math.pow(testY[m][c][x][t]*0.188679245-gtY[t],2)#pixel para metros
				meanZAux = math.pow(testZ[m][c][x][t]*62/0.330769231-gtZ[t],2)		
				erroGxyzAux = math.sqrt(meanZAux + meanYAux + meanXAux)	
				erroGxyz += erroGxyzAux	
				#erroGxy += math.sqrt(meanYAux + meanXAux)
				
				if t+1 == LenTraj:#Último teste
					#print("t: " + str(t) + " LenTraj: " + str(LenTraj) + "sssssssss")
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

			#Escala:	0,366197183
			#Divisor:	45
			#Correção yaw:	270
			#0,330769231 é a escala dos mapas de ADM a 62 metros de altura
 			#mapa (4453,4772)-(4771,4772) => 60m  = 318px => 60/318 = 0,188679245*100 = 18,8679245
			
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
					dirZ = (-testZ[m][c][x][t] /0.366197183) * (multExcalaZ);#Z do teste, negativo, situado próximo ao chão
					xCorr = testX[m][c][x][t];
					yCorr = testY[m][c][x][t];
					#zCorr = (testZ[m][c][x][t] /0.366197183) * (multExcalaZ);#Convertendo o z de escala para pixeis
					
					#print("zCorr: " + str(zCorr) + " gtZ[t]: " + str(gtZ[t]))

					if testX[m][c][x][t] > 4800 or testY[m][c][x][t] > 4800 or testX[m][c][x][t] < 0 or testY[m][c][x][t] < 0:
						foraMapa = 1;

					xCorr+= dirX*(math.cos(pitch[t])*math.cos(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])-math.cos(roll[t])*math.sin(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])+math.sin(roll[t])*math.sin(yawInvertido[t]));    			
					yCorr += dirX*(math.cos(pitch[t])*math.sin(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])+math.cos(roll[t])*math.cos(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])-math.sin(roll[t])*math.cos(yawInvertido[t]));
	    			#zCorr += ((dirX*-math.sin(pitch[t]) + dirY*(math.sin(roll[t])*math.cos(pitch[t])) + dirZ*(math.cos(roll[t])*math.cos(pitch[t])))) * 0.366197183 / 45 * 0.188679245
					#zCorr += (((dirX*-math.sin(pitch[t]) + dirY*(math.sin(roll[t])*math.cos(pitch[t])) + dirZ*(math.cos(roll[t])*math.cos(pitch[t])))));

					#TESTEX = dirX*(math.cos(pitch[t])*math.cos(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])-math.cos(roll[t])*math.sin(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.cos(yawInvertido[t])+math.sin(roll[t])*math.sin(yawInvertido[t]));   
					#TESTEY = dirX*(math.cos(pitch[t])*math.sin(yawInvertido[t])) + dirY*(math.sin(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])+math.cos(roll[t])*math.cos(yawInvertido[t])) + dirZ*(math.cos(roll[t])*math.sin(pitch[t])*math.sin(yawInvertido[t])-math.sin(roll[t])*math.cos(yawInvertido[t]));
					#TESTEZ = ((dirX*-math.sin(pitch[t]) + dirY*(math.sin(roll[t])*math.cos(pitch[t])) + dirZ*(math.cos(roll[t])*math.cos(pitch[t]))));
					#print("zCorr: " + str(zCorr) + " TESTEZ: " + str(TESTEZ) + " TESTEX: " + str(TESTEX) + " TESTEY: " + str(TESTEY))

					meanXAux = math.pow(xCorr*0.188679245-gtX[t],2)#pixel para metros
					meanYAux = math.pow(yCorr*0.188679245-gtY[t],2)#pixel para metros
					#meanZAux = math.pow(zCorr*62/0.330769231-gtZ[t],2)		
					#meanZAux = math.pow(zCorr * 0.188679245 - gtZ[t],2)#pixel para metros	
					meanZAux = math.pow(testZ[m][c][x][t]*62/0.330769231-gtZ[t],2)#o Z não é corrigido

					erroG += math.sqrt(meanZAux + meanYAux + meanXAux)
					#erroG += math.sqrt(meanYAux + meanXAux)
					#print("zCorr: " + str(zCorr) + " gtZ[t]: " + str(gtZ[t]))

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
		#plot(errorXYZ[m][c],color=cstyle[(c%len(cstyle))],linestyle=lstyle[(c%len(lstyle))],linewidth=2) #VOU PLOTSAR SÓ O CORRIGIDO

		if len(ConfigsName) > 1 and len(MapsName) > 1:
			legendNames.append(ConfigsName[c]+MapsName[m])
		elif len(ConfigsName) > 1 and len(MapsName) == 1:			
			legendNames.append(ConfigsName[c] + " Divergencias: " + str(qtdDivergiu[c]))
		elif len(ConfigsName) == 1 and len(MapsName) > 1:
			legendNames.append(MapsName[m])
		elif len(ConfigsName) == 1 and len(MapsName) == 1:
			legendNames.append(ConfigsName[c] + " Divergencias: " + str(qtdDivergiu[c]))
			print(str(qtdDivergiu[c]))
			#legendNames.append(ConfigsName[c] + " Divergências: " + str(len(qtdDivergiu)))			

#GT CORRIGIDO
if printCorrigido == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):					
			plot(errorXYZcorr[m][c],color=cstyle[(c%len(cstyle))],linestyle=lstyle[(c%len(lstyle))],linewidth=2)
			#plot(errorXYZcorr[m][c],color=cstyle[(c%len(cstyleCorr))],linestyle=lstyleCorr[(c%len(lstyleCorr))],linewidth=2)			
			#plot(errorXY[m][c],color=cstyle[(c%len(cstyleCorr))],linestyle=lstyleCorr[(c%len(lstyleCorr))],linewidth=2)

#MAX ERROR
if len(ConfigsName) == 1 and len(MapsName) == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):
			#plot(maxError[m][c],color=cstyle[(c%len(cstyleMaxError))],linestyle=lstyleMaxError[(c%len(lstyleMaxError))],linewidth=2)
			plot(maxError[m][c],color=cstyle[2],linestyle=lstyleMaxError[2],linewidth=2)

#MIN ERROR
if len(ConfigsName) == 1 and len(MapsName) == 1:
	for c in range(len(Configs)):
		for m in range(len(Maps)):
			#plot(minError[m][c],color=cstyle[(c%len(cstyleMinError))],linestyle=lstyleMinError[(c%len(lstyleMinError))],linewidth=2)
			plot(minError[m][c],color=cstyle[1],linestyle=lstyleMinError[1],linewidth=2)

legend(legendNames)
ax = axes()
ax.set_ylabel("Erro (m)")
ax.set_xlabel("Instante")
ax.set_xlabel("Instante")
ax.set_xlim([0,LenTraj])
ax.set_ylim([0,300])
ax.set_aspect(0.3)
title('(c)')

if len(ConfigsName) == 1:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[0])
elif len(ConfigsName) == 3:
	savefig("Graficos/" + MapsName[m] + "_" + ConfigsName[1] + "_VS_" + ConfigsName[2])#config 0 é o abBRIEF, estou colocand como referência
else:
	savefig("Graficos/" + MapsName[m])
show()

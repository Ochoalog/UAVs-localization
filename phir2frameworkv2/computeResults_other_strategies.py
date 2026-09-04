import numpy as np
import scipy
from scipy import stats
import sys
from pylab import *
import itertools
import os


#if len(sys.argv)!=2:
#    print 'USAGE: python programName folderName'
#    sys.exit(2)

# Parameters
Map="median_new"

Trajs=[
#"traj4_460_mod",
#"traj4_460_mod_off",
#"traj4_460_gt_mod",
#"traj3_mod",
#"traj3_gt_mod",
#"traj3_good",
#"traj3_gt_good",
#"traj3_eq_good",
#"traj3_eq_gt_good",
#"traj2_mod",
#"traj2_mod_off",
#"traj4_460_gt_mod",
"traj4_460_gt_mod_brief",
]
print Trajs

LenTrajs=[
82
#80,
#88,
#133,
#133,
#85,
#85,
#85,
#85,
#79,
#79,
]

#numConfigs=4
#lenConfig=1
Configs=[
"-s BRIEF diff-intensity 19 -bp 100 -blt 0.5 -bmt 4 -bm 10"
]
print Configs

HelpersNames = [
"entropy"
]

Helpers=[[[] for y in xrange(len(HelpersNames)) ] for x in xrange(len(Configs))]
Helpers[0][0] = [
"100p_btl-0.5_btm-4_bm-10"
]

#Limiars = [5]
Limiars = [x for x in xrange(5,50+1,5)]
print Limiars

numTests = 10

# Files
#files = [[[[[] for x in xrange(numTests)] for l in xrange(len(Helpers[0]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]

# Metrics
#particleLog << "trueX trueY meanPX meanPY closestx closesty closestw meanParticleError meanParticleStdev meanError stdevError trueTh meanAngle angleStdev angleError stdevAngleError NEFF elapsedTime\n";

trueX = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
trueY = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanPX = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanPY = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
closestX = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
closestY = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
closestTheta = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
closestw = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanParticleError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanParticleStdev = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
stdevError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
trueTh = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
meanAngle = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
angleStdev = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
angleError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
stdevAngleError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]
timeMethod = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]

closestError = [[[[[] for x in xrange(len(Helpers[c][l]))] for l in xrange(len(Helpers[c]))] for c in xrange(len(Configs))] for t in xrange(len(Trajs))]


#lists = []
#times = []

#timePlots= []
#localizationPlots = []

font = {'family' : 'verdana',
        'weight' : 'bold',
        'size'   : 14}

matplotlib.rc('font', **font)

# Reading files
for t in xrange(len(Trajs)):
#    meanErr=[[0 for l in xrange(len(Helpers[0]))] for c in xrange(len(Configs))]
    meanErr=[[0 for c in xrange(len(Configs))] for l in xrange(len(Helpers[0]))]
    finalMeanErr=[[0 for c in xrange(len(Configs))] for l in xrange(len(Helpers[0]))]

    minIDs = []
    meanIDs = []

    for c in xrange(len(Configs)):

        for l in xrange(len(Helpers[c])):

            figure()
            legendNames = []

            meanValues = []
            minValues = []

            for x in xrange(len(Helpers[c][l])):

                f = os.listdir(Map+"/"+Trajs[t]+"/"+Helpers[c][l][x]+"/")
                first=True

                print Trajs[t]+"/"+Helpers[c][l][x]

                for pathFile in f:
#                    print Trajs[t]+"/"+Helpers[c][l][x]+"/"+pathFile,

                    # Reading log file
                    curFile =  open(Map+"/"+Trajs[t]+"/"+Helpers[c][l][x]+"/"+pathFile)
    #                files[t][c][l][i] = open(Map+"/"+testName+".txt")

                    # Check if file is good
                    count=0
                    for line in curFile:
                        count += 1
                    if count < LenTrajs[t]:
#                        print count," LIXO"
                        continue
                    else:
#                        print count," BOM",
                        trueX[t][c][l][x].append([])
                        trueY[t][c][l][x].append([])
                        meanPX[t][c][l][x].append([])
                        meanPY[t][c][l][x].append([])
                        closestX[t][c][l][x].append([])
                        closestY[t][c][l][x].append([])
                        closestTheta[t][c][l][x].append([])
                        closestw[t][c][l][x].append([])
                        meanParticleError[t][c][l][x].append([])
                        meanParticleStdev[t][c][l][x].append([])
                        meanError[t][c][l][x].append([])
                        stdevError[t][c][l][x].append([])
                        trueTh[t][c][l][x].append([])
                        meanAngle[t][c][l][x].append([])
                        angleStdev[t][c][l][x].append([])
                        angleError[t][c][l][x].append([])
                        stdevAngleError[t][c][l][x].append([])
                        closestError[t][c][l][x].append([])

                    i=len(trueX[t][c][l][x])-1
#                    print i

                    curFile.close()
                    curFile =  open(Map+"/"+Trajs[t]+"/"+Helpers[c][l][x]+"/"+pathFile)

                    for line in curFile:
                        w = line.split()

        #                   0         1               2                 3                4               5
        #particleLog  << "Standard Measurements: " << trueX <<  " " << trueY << " " << meanPX << " " << meanPY << " "
        #                    6                    7                        8                     9
        #             <<  closest.p.x << " " << closest.p.y << " " << closest.p.theta << " " << closest.w << " "
        #                    10                            11                        12                   13
        #             << meanParticleError <<  " " << meanParticleStdev  << " " << meanError << " " << stdevError << " "
        #                    14            15                  16                   17                     18
        #             << trueTh << " " << meanAngle << " " << angleStdev << " " << angleError << " " << stdevAngleError << " "
        #                 19                20
        #             << NEFF << " " << elapsedTime endl;

                        if w[0] == 'Standard':
                            trueX[t][c][l][x][i].append(float(w[2]))
                            trueY[t][c][l][x][i].append(float(w[3]))
                            meanPX[t][c][l][x][i].append(float(w[4]))
                            meanPY[t][c][l][x][i].append(float(w[5]))
                            closestX[t][c][l][x][i].append(float(w[6]))
                            closestY[t][c][l][x][i].append(float(w[7]))
                            closestTheta[t][c][l][x][i].append(float(w[8]))
                            closestw[t][c][l][x][i].append(float(w[9]))
                            meanParticleError[t][c][l][x][i].append(float(w[10]))
                            meanParticleStdev[t][c][l][x][i].append(float(w[11]))
                            meanError[t][c][l][x][i].append(float(w[12]))
                            stdevError[t][c][l][x][i].append(float(w[13]))
                            trueTh[t][c][l][x][i].append(float(w[14]))
                            meanAngle[t][c][l][x][i].append(float(w[15]))
                            angleStdev[t][c][l][x][i].append(float(w[16]))
                            angleError[t][c][l][x][i].append(float(w[17]))
                            stdevAngleError[t][c][l][x][i].append(float(w[18]))
                  
                            closestError[t][c][l][x][i].append(sqrt( (float(w[6])-float(w[2]))*(float(w[6])-float(w[2]))+(float(w[7])-float(w[3]))*(float(w[7])-float(w[3])) ))
                        elif w[0] == 'trueX':
                            continue

                    curFile.close()

                if len(meanError[t][c][l][x]) == 0:
                    continue
                length = min([len(meanError[t][c][l][x][i]) for i in xrange(len(meanError[t][c][l][x]))])  
                print "length",length

                v = [np.mean([meanError[t][c][l][x][i][r] for i in xrange(len(meanError[t][c][l][x]))]) for r in xrange(length)]
                plot(v)
#                meanErr[l][c] = np.mean([np.mean([meanError[t][c][l][x][i][r] for i in xrange(len(meanError[t][c][l][x]))]) for r in xrange(length)])
#                finalMeanErr[l][c] = np.mean([meanError[t][c][l][x][i][length-1] for i in xrange(len(meanError[t][c][l][x]))])
                legendNames.append(Helpers[c][l][x])

                meanValues.append(mean(v))
                minValues.append(min(v))

            minIDs.append(minValues.index(min(minValues)))
            meanIDs.append(meanValues.index(min(meanValues)))

            legend(legendNames)
            ax = axes()  
            ax.set_title(Trajs[t]+"/"+Configs[c]+"/"+HelpersNames[l])
            ax.set_ylabel("Error (pix)")
            ax.set_xlabel("Step")
            savefig(Trajs[t]+"-"+Configs[c])
        # show()
            close()

    figure()
    legendNames = []
    lstyle = ['--', '-.', '-', ':']
    cstyle = ['r', 'g', 'b', 'k']
    for c in xrange(len(Helpers)):
        for l in xrange(len(Helpers[c])):
            length = min([len(meanError[t][c][l][minIDs[c]][i]) for i in xrange(len(meanError[t][c][l][minIDs[c]]))])
            v = [np.mean([meanError[t][c][l][minIDs[c]][i][r] for i in xrange(len(meanError[t][c][l][minIDs[c]]))]) for r in xrange(length)]
            plot(v,linestyle=lstyle[(c*len(lstyle))//len(Helpers)],color=cstyle[(c%len(lstyle))],linewidth=2)
            legendNames.append(Helpers[c][l][minIDs[c]])

    print legendNames
    legend(legendNames)
    ax = axes()
    ax.set_title(Trajs[t]+"-MIN")
    ax.set_ylabel("Error (pix)")
    ax.set_xlabel("Step")
    savefig(Trajs[t])

    figure()
    legendNames = []
    lstyle = ['--', '-.', '-', ':']
    cstyle = ['r', 'g', 'b', 'k']
    for c in xrange(len(Helpers)):
        for l in xrange(len(Helpers[c])):
            length = min([len(meanError[t][c][l][meanIDs[c]][i]) for i in xrange(len(meanError[t][c][l][meanIDs[c]]))])
            v = [np.mean([meanError[t][c][l][meanIDs[c]][i][r] for i in xrange(len(meanError[t][c][l][meanIDs[c]]))]) for r in xrange(length)]
            plot(v,linestyle=lstyle[(c*len(lstyle))//len(Helpers)],color=cstyle[(c%len(lstyle))],linewidth=2)
            legendNames.append(Helpers[c][l][meanIDs[c]])

    print legendNames
    legend(legendNames)
    ax = axes()
    ax.set_title(Trajs[t]+"-MEAN")
    ax.set_ylabel("Error (pix)")
    ax.set_xlabel("Step")
    savefig(Trajs[t])
    show()


#    figure()
##    x = [Limiars for i in xrange(4)] # [4][9]
#    x = [[Limiars[i] for j in xrange(4)] for i in xrange(len(Limiars))] # [9][4]
##    y = [[meanErr[j][i] for j in xrange(len(meanErr))] for i in xrange(len(meanErr[0]))] # [9][4]
#    plot(x,meanErr,'o-')
#    legend(Configs)
#    ax = axes()  
#    ax.set_title(Trajs[t])
#    ax.set_ylabel("Mean Error (pix)")
#    ax.set_xlabel("Threshold")
#    ax.set_ylim(0,800)
#    savefig(Trajs[t]+"-MeanErr")

#    figure()
#    plot(x,finalMeanErr,'o-')
#    legend(Configs)
#    ax = axes()  
#    ax.set_title(Trajs[t])
#    ax.set_ylabel("Final Mean Error (pix)")
#    ax.set_xlabel("Threshold")
#    ax.set_ylim(0,800)
#    savefig(Trajs[t]+"-FinalMeanErr")
##    show()








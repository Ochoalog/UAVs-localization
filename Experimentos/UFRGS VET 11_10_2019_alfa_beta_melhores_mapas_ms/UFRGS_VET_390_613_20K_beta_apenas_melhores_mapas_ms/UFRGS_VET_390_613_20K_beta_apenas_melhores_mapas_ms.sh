#!/bin/bash

numTests=30

execPath="PhiR2Framework"

Maps=( 
"Mapas/UFRGS_VET_V3_12_10_2012.jpg"
"Mapas/UFRGS_VET_V3_13_05_2015.jpg"
"Mapas/UFRGS_VET_V3_11_08_2011.jpg"
"Mapas/UFRGS_VET_V3_16_05_2017.jpg"
)

MapsNames=(
"UFRGS_VET_V3_12_10_2012"
"UFRGS_VET_V3_13_05_2015"
"UFRGS_VET_V3_11_08_2011"
"UFRGS_VET_V3_16_05_2017"
)


#echo "NumMaps" ${#Maps[@]}

#trajsPath="/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/"
trajsPath="/home/mathias/Workspace/Datasets/UFRGS_VET/voo_100m/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"


Configs=(
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 1"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 2"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 4"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 8"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5   -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfavismsk 0  -corrpxpos 0 -betaweight 16"
)

Outputs=(  
"abBRIEF__betaweight_0" 
"betaweight_1" 
"betaweight_2"
"betaweight_4"
"betaweight_8"
"betaweight_16" 
)

for t in "${!Trajs[@]}"
do
        traj=${Trajs[$t]}
	for m in "${!Maps[@]}" 
	do
		map=${Maps[$m]}
                
		for c in "${!Configs[@]}"
		do
			config="${Configs[$c]}"

			#####for test in `seq 1 $numTests`  comentei pois assime stava rodando numTests * numTests vezes
			#####do            

			resultPath="/home/mathias/Workspace/Experimentos/UFRGS VET 11_10_2019_alfa_beta_melhores_mapas_ms/UFRGS_VET_390_613_20K_beta_apenas_melhores_mapas_ms/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

				# Create dir for this test
				if [ ! -d "$resultPath" ] 
				then
				mkdir -p "$resultPath"
				fi
                                 

				# run test

				echo "Running $traj $config ${MapsNames[$m]} ($test)"
				count=`ls "$resultPath" 2>/dev/null | wc -l`

				echo ""
				echo " -quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))"
				echo ""		

				./$execPath -quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))

				#            echo ./$execPath -quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))
				#            # Move log files to Results
				#            ls -t Logs | head -n 1 | while read f
				#            do
				#                count=`ls "Results/${MapsNames[$m]}/$traj/$config $lim/" 2>/dev/null | wc -l`
				#                mv "Logs/$f" "Results/${MapsNames[$m]}/$traj/$config $lim/$count.txt"
				#            done
			#####done
		done
	done 
done 

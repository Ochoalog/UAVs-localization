#!/bin/bash

numTests=30

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_28_04_2017.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_18_03_2015.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_12_01_2017.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_12_10_2012.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_13_05_2015.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_11_08_2011.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_16_05_2017.jpg"

#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_02_04_2015.jpg"
"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_17_12_2002.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_23_08_2016.jpg"
#"/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_18_09_2018.jpg"
)

MapsNames=(
#"UFRGS_VET_V3_28_04_2017"
#"UFRGS_VET_V3_18_03_2015"
#"UFRGS_VET_V3_12_01_2017"
#"UFRGS_VET_V3_12_10_2012"
#"UFRGS_VET_V3_13_05_2015"
#"UFRGS_VET_V3_11_08_2011"
#"UFRGS_VET_V3_16_05_2017"

#"UFRGS_VET_V3_02_04_2015"
"UFRGS_VET_V3_17_12_2002"
#"UFRGS_VET_V3_23_08_2016"
#"UFRGS_VET_V3_18_09_2018"
)


#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"


Configs=(
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 0 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 0.2 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 0.4 -cmppar 0 -corrpxpos 0 -betaweight 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 0.6 -cmppar 0 -corrpxpos 0 -betaweight 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 0.8 -cmppar 0 -corrpxpos 0 -betaweight 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613 -alfagaussmsk 1.0 -cmppar 0 -corrpxpos 0 -betaweight 0"
)

Outputs=(  
#"alfagaussmsk_0" 

#"alfagaussmsk_0,2" 

#"alfagaussmsk_0,4" 

"alfagaussmsk_0,6"  

"alfagaussmsk_0,8"  

"alfagaussmsk_1,0" 
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
                                resultPath="/home/phi/Dropbox/experimentos/UFRGS VET/UFRGS_VET_390_613_50K_apenas_alfa_correcao_VI_escalado/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

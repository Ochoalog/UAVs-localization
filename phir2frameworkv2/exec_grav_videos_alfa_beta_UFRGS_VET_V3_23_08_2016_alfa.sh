#!/bin/bash

numTests=1

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"
 
Maps=( 
#"/home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_02_04_2015.jpg"
#"/home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_17_12_2002.jpg"
"/home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_23_08_2016.jpg"
#"/home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/Mapa/UFRGS_VET_V3_18_09_2018.jpg"
)

MapsNames=(
#"UFRGS_VET_V3_02_04_2015"
#"UFRGS_VET_V3_17_12_2002"
"UFRGS_VET_V3_23_08_2016"
#"UFRGS_VET_V3_18_09_2018"
)


#echo "NumMaps" ${#Maps[@]}
trajsPath="/home/raziel/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"



Configs=(
#ALFA 20k
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.2 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.4 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.6 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.8 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1.0 -cmppar 0 -corrpxpos 0 -betaweight 0 "

#ALFA 50k
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.2 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.4 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.6 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 0.8 -cmppar 0 -corrpxpos 0 -betaweight 0 "
 
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1.0 -cmppar 0 -corrpxpos 0 -betaweight 0 "


#BETA 20k
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 0 "

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 1"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 2"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 4"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 8"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 16"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 32"


#BETA 50k
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 0 "

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 1"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 2"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 4"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 8"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 16"
 
#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000  -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 405 -alfagaussmsk 1 -cmppar 0 -corrpxpos 0 -betaweight 32"
)

Outputs=( 
"alfagaussmsk_0_20k"
"alfagaussmsk_0,2_20k"
"alfagaussmsk_0,4_20k"
"alfagaussmsk_0,6_20k"
"alfagaussmsk_0,8_20k"
"alfagaussmsk_1_20k"

"alfagaussmsk_0_50k"
"alfagaussmsk_0,2_50k"
"alfagaussmsk_0,4_50k"
"alfagaussmsk_0,6_50k"
"alfagaussmsk_0,8_50k"
"alfagaussmsk_1_50k"
 
#"betaweight_0_20k" 
#"betaweight_1_20k" 
#"betaweight_2_20k"
#"betaweight_4_20k"
#"betaweight_8_20k"
#"betaweight_16_20k"
#"betaweight_32_20k"

#"betaweight_0_50k" 
#"betaweight_1_50k" 
#"betaweight_2_50k"
#"betaweight_4_50k"
#"betaweight_8_50k"
#"betaweight_16_50k"
#"betaweight_32_50k"
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
                                resultPath="/home/raziel/Experimentos/UFRGS VET/UFRGS_VET_alfa_beta_videos/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

				./$execPath -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))

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

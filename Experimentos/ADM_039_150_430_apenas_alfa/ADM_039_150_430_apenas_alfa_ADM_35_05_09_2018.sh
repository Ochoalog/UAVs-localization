#!/bin/bash

numTests=30

execPath="../build-PhiR2Framework-Desktop_Qt_5_12_0_GCC_64bit9-Release/PhiR2Framework"

Maps=( 
#"/home/mathias/Workspace/Datasets/ADM2019/0036/Mapas/ADM_35_12_09_2018.jpg"
#"/home/mathias/Workspace/Datasets/ADM2019/0036/Mapas/ADM_35_17_08_2018.jpg"
#"/home/mathias/Workspace/Datasets/ADM2019/0036/Mapas/ADM_35_17_12_2018.jpg"
"/home/mathias/Workspace/Datasets/ADM2019/0036/Mapas/ADM_35_05_09_2018.jpg"
#"/home/mathias/Workspace/Datasets/ADM2019/0036/Mapas/ADM_35_27_02_2017.jpg"
)

MapsNames=(
#"ADM_35_12_09_2018"
#"ADM_35_17_08_2018"
#"ADM_35_17_12_2018"
"ADM_35_05_09_2018"
#"ADM_35_27_02_2017"
)

#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/mathias/Workspace/Datasets/ADM2019/0039/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 0 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 0.2 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 0.4 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 0.6 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 0.8 -cmppar 0 -corrpxpos 0 -betaweight 0"

#"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 140 -iniexec 150 -fimds 430 -fiexec 430 -alfagaussmsk 1.0 -cmppar 0 -corrpxpos 0 -betaweight 0"
)

Outputs=(     
"alfagaussmsk_0" 

#"alfagaussmsk_0,2" 

#"alfagaussmsk_0,4" 

#"alfagaussmsk_0,6"  

#"alfagaussmsk_0,8"  

#"alfagaussmsk_1,0"   
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
                resultPath="/home/mathias/Workspace/Experimentos/ADM/0039/ADM_039_150_430_apenas_alfa/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

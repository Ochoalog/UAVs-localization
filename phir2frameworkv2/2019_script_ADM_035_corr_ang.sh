#!/bin/bash

numTests=15

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_12_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_13_06_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_08_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_12_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_18_11_2013.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_19_09_2010.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_23_07_2016.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_27_02_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_31_07_2016.jpg"
)

MapsNames=(
"ADM_35_12_09_2018.jpg",
"ADM_35_13_06_2017.jpg",
"ADM_35_17_08_2018.jpg",
"ADM_35_17_12_2018.jpg",
"ADM_35_18_11_2013.jpg",
"ADM_35_19_09_2010.jpg",
"ADM_35_23_07_2016.jpg",
"ADM_35_27_02_2017.jpg",
"ADM_35_31_07_2016.jpg"
)


#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 0 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 1 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 0 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 1 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 1 -corrpxpos 0"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 0 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 1 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 1"


"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 7 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 7 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 1 -corrpxpos 0"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 7 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 1"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 7 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 7 -fit 0 -inids 85 -iniexec 103 -fimds 498 -fiexec 387 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"
)

Outputs=(  
"abBRIEF_6_50k__pdm_1__cmppar_0__corrpxpos_0"
"abBRIEF_6_50k__pdm_1__cmppar_1__corrpxpos_0"  
"abBRIEF_6_50k__pdm_1__cmppar_0__corrpxpos_1"

"MSabBRIEF_6_50k__pdm_7__multgaussmsk_0.25__cmppar_0__corrpxpos_0"   
"MSabBRIEF_6_50k__pdm_7__multgaussmsk_0.25__cmppar_1__corrpxpos_0"
"MSabBRIEF_6_50k__pdm_7__multgaussmsk_0.25__cmppar_0__corrpxpos_1"

"MSabBRIEF_6_50k__pdm_7__multgaussmsk_0.5__cmppar_0__corrpxpos_0"  
"MSabBRIEF_6_50k__pdm_7__multgaussmsk_1__cmppar_0__corrpxpos_0"  
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
                                resultPath="/home/phi/Dropbox/experimentos/ADM 2019/0035/ADM_035_Correcao_orientacao/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

#!/bin/bash

numTests=15

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_05_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_12_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_13_06_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_17_08_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_17_12_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_18_11_2013.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_19_09_2010.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_23_07_2016.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_27_02_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_28_02_2006.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/Mapas/ADM_35_31_07_2016.jpg"
)

MapsNames=(
"ADM_35_05_09_2018.jpg"
"ADM_35_12_09_2018.jpg"
"ADM_35_13_06_2017.jpg"
"ADM_35_17_08_2018.jpg"
"ADM_35_17_12_2018.jpg"
"ADM_35_18_11_2013.jpg"
"ADM_35_19_09_2010.jpg"
"ADM_35_23_07_2016.jpg"
"ADM_35_27_02_2017.jpg"
"ADM_35_28_02_2006.jpg"
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
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 85 -iniexec 101 -fimds 498 -fiexec 400 -multgaussmsk 0.1"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 85 -iniexec 101 -fimds 498 -fiexec 400 -multgaussmsk 0.50"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 85 -iniexec 101 -fimds 498 -fiexec 400 -multgaussmsk 0.75"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 85 -iniexec 101 -fimds 498 -fiexec 400 -multgaussmsk 0.90"
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 85 -iniexec 101 -fimds 498 -fiexec 400 -multgaussmsk 1"
)


Outputs=(     
"mndvw_6_20k_pdm_51_OK_gausmsk_010" # MSC  
"mndvw_6_20k_pdm_51_OK_gausmsk_050" # MSC
"mndvw_6_20k_pdm_51_OK_gausmsk_075" # MSC  
"mndvw_6_20k_pdm_51_OK_gausmsk_090" # MSC  
"mndvw_6_20k_pdm_51_OK_gausmsk_1" # MSC    
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
                                resultPath="/home/phi/Dropbox/experimentos/ADM 2019/0035/ADM_035_MapasTerra/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

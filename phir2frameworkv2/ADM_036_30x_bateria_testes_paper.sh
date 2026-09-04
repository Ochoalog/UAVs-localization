#!/bin/bash

numTests=30

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_12_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_08_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_12_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_05_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_27_02_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_27_02_2017.jpg"
)

MapsNames=(
"ADM_35_12_09_2018"
"ADM_35_17_08_2018"
"ADM_35_17_12_2018"
"ADM_35_05_09_2018"
"ADM_35_27_02_2017"
"ADM_35_27_02_2017"
)


#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0036/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"


Configs=(
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 0 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 1 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.5 -cmppar 0 -corrpxpos 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.75 -cmppar 0 -corrpxpos 0"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 1.0 -cmppar 0 -corrpxpos 0"





"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.25 -cmppar 0 -corrpxpos 0 -msimp 2 -bfd 6 -bfsc 5 -bfss 5"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.5 -cmppar 0 -corrpxpos 0 -msimp 2 -bfd 6 -bfsc 5 -bfss 5"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 0.75 -cmppar 0 -corrpxpos 0 -msimp 2 -bfd 6 -bfsc 5 -bfss 5"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 50000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.1 -maxsc 3 -mappp 1 -pdm 51 -fit 0 -inids 115 -iniexec 199 -fimds 456 -fiexec 394 -multgaussmsk 1.0 -cmppar 0 -corrpxpos 0 -msimp 2 -bfd 6 -bfsc 5 -bfss 5"
)

Outputs=(  
"abBRIEF_6_50k__pdm_1"

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.25"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.5"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.75"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_1.0" 


"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.25_msimp_2__bfd_6__bfsc_5_bfss_5"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.5_msimp_2__bfd_6__bfsc_5_bfss_5"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_0.75_msimp_2__bfd_6__bfsc_5_bfss_5"  

"MSabBRIEF_6_50k__pdm_51__multgaussmsk_1.0_msimp_2__bfd_6__bfsc_5_bfss_5"
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
                                resultPath="/home/phi/Dropbox/experimentos/ADM 2019/0036/ADM_036_30x_bateria_testes_paper/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

#!/bin/bash

numTests=15

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_02_02_2014.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_03_03_2014.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_05_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_08_09_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_11_04_2011.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_12_07_2013.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_12_09_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_13_06_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_15_05_2012.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_08_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_17_12_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_18_08_2013.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_18_11_2013.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_19_01_2014.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_19_09_2010.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_19_10_2010.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_20_06_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_23_07_2016.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_24_11_2016.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_27_01_2014.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_27_02_2017.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_28_02_2006.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_28_06_2015.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_28_12_2018.jpg"
"/home/phi/Documents/Datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0035/Mapas/ADM_35_31_07_2016.jpg"
)

MapsNames=(
"ADM_35_02_02_2014"
"ADM_35_03_03_2014"
"ADM_35_05_09_2018"
"ADM_35_08_09_2017"
"ADM_35_11_04_2011"
"ADM_35_12_07_2013"
"ADM_35_12_09_2018"
"ADM_35_13_06_2017"
"ADM_35_15_05_2012"
"ADM_35_17_08_2018"
"ADM_35_17_12_2018"
"ADM_35_18_08_2013"
"ADM_35_18_11_2013"
"ADM_35_19_01_2014"
"ADM_35_19_09_2010"
"ADM_35_19_10_2010"
"ADM_35_20_06_2018"
"ADM_35_23_07_2016"
"ADM_35_24_11_2016"
"ADM_35_27_01_2014"
"ADM_35_27_02_2017"
"ADM_35_28_02_2006"
"ADM_35_28_06_2015"
"ADM_35_28_12_2018"
"ADM_35_31_07_2016"
)

#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/phi/Documents/Datasets/Sequoia_Matrice100_05_02_2019/voo_100m/"
Trajs=(
"traj"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 1 -mindviw 1 -msc 0 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 1 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 6 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613"

"-s BRIEF diff-cie2000 15 -bp 256 -blt 0.5 -bmt 4 -bm 10 -par 20000 -mndvw 1 -mindviw 1 -msc 1 -ndvit 1 -minsc 0.5 -maxsc 5 -msimp 0 -mappp 1 -pdm 51 -fit 0 -inids 186 -iniexec 390 -fimds 719 -fiexec 613"
)

Outputs=(     
"mndvw_1_20k_pdm_1"  # Gaussian Roulette-wheel (abBRIEF)
"mndvw_6_20k_pdm_51" # NDVI Roulette-wheel + Gaussian Roulette-wheel + NDVI Weight
"mndvw_1_20k_pdm_51" # NDVI Roulette-wheel + Gaussian Roulette-wheel       
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
                                resultPath="/home/phi/Dropbox/ADM_All_Maps/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

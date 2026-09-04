#!/bin/bash

numTests=1

execPath="../build-PhiR2Framework-Desktop_Qt_5_5_1_GCC_64bit-Reease/PhiR2Framework"

Maps=( 
"/home/mathias/Documents/Datasets/voo_maicon/Mapas_AdM/globalmap1.png"
"/home/mathias/Documents/Datasets/voo_maicon/Mapas_AdM/globalmap2.png"
"/home/mathias/Documents/Datasets/voo_maicon/Mapas_AdM/globalmap3.png"
"/home/mathias/Documents/Datasets/voo_maicon/Mapas_AdM/globalmap4.png"
"/home/mathias/Documents/Datasets/voo_maicon/Mapas_AdM/globalmap5.png"
)

MapsNames=(
"globalmap1"	
"globalmap2"
"globalmap3"
"globalmap4"
"globalmap5"
)

#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/mathias/Documents/Datasets/voo_maicon/"
Trajs=(
"traj3_alto"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-cie2000 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
)

Outputs=(
"LAB_500_1"
"LAB_500_2"
"LAB_500_3"
"LAB_500_4"
"LAB_500_5"
"LAB_500_6"
"LAB_500_7"
"LAB_500_8"
"LAB_500_9"
"LAB_500_10"
"LAB_500_11"
"LAB_500_12"
"LAB_500_13"
"LAB_500_14"
"LAB_500_15"
"LAB_500_16"
"LAB_500_17"
"LAB_500_18"
"LAB_500_19"
"LAB_500_20"
"LAB_500_21"
"LAB_500_22"
"LAB_500_23"
"LAB_500_24"
"LAB_500_25"
"LAB_500_26"
"LAB_500_27"
"LAB_500_28"
"LAB_500_29"
"LAB_500_30"
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

			for test in `seq 1 $numTests`
			do                
				resultPath="/home/mathias/Documents/Results_MasterThesis/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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
			done
		done
	done 
done 

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
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-rgb 10 -bp 500 -blt 0.5 -bmt 4 -bm 10"
)

Outputs=(
"gray_500_1"
"gray_500_2"
"gray_500_3"
"gray_500_4"
"gray_500_5"
"gray_500_6"
"gray_500_7"
"gray_500_8"
"gray_500_9"
"gray_500_10"
"gray_500_11"
"gray_500_12"
"gray_500_13"
"gray_500_14"
"gray_500_15"
"gray_500_16"
"gray_500_17"
"gray_500_18"
"gray_500_19"
"gray_500_20"
"gray_500_21"
"gray_500_22"
"gray_500_23"
"gray_500_24"
"gray_500_25"
"gray_500_26"
"gray_500_27"
"gray_500_28"
"gray_500_29"
"gray_500_30"
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
				resultPath="/home/mathias/Documents/Results/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

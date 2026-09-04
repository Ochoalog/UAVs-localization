#!/bin/bash
##########-e /home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap1.png -t /home/mathias/Documents/voo_maicon/traj2_alto.txt -s MI diff-intensity 15
numTests=1

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap6.png"
"/home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap2.png"
"/home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap3.png"
"/home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap4.png"
"/home/mathias/Documents/voo_maicon/Mapas_AdM/globalmap5.png"
)

MapsNames=(
"globalmap6"	
"globalmap2"
"globalmap3"
"globalmap4"
"globalmap5"
)

#echo "NumMaps" ${#Maps[@]}
#/home/mathias/Documentos/Datasets/voo_maicon/
trajsPath="/home/mathias/Documents/voo_maicon/"
Trajs=(
"traj2_alto"
)

#Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
"-s MI diff-intensity 15"
)

Outputs=(
"MI_500_1"
"MI_500_2"
"MI_500_3"
"MI_500_4"
"MI_500_5"
"MI_500_6"
"MI_500_7"
"MI_500_8"
"MI_500_9"
"MI_500_10"
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
				resultPath="/home/mathias/Documents/Results_MI_10_500/${MapsNames[$m]}/$traj/${Outputs[$c]}/"

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

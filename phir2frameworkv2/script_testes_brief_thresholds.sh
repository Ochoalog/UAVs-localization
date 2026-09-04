#!/bin/bash

numTests=$1

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
"/home/diego/ProjetoLocalizacao/median_new"
)

MapsNames=(
"median_new"
)

#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/diego/ProjetoLocalizacao/ufrgs_dronao_28ago/"
Trajs=(
"traj4_460_gt_mod_brief"
)

Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 3 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 5 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 7 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 10 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 3 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 5 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 7 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 10 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 3 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 5 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 7 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 10 -bm 10"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 3 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 4 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 5 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 7 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.45 -bmt 10 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 3 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 4 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 5 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 7 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.5 -bmt 10 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 3 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 4 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 5 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 7 -bm 30"
"-s BRIEF diff-intensity 19 -bp 200 -blt 0.55 -bmt 10 -bm 30"
)

Outputs=(
"thresholds_btl-0.45_btm-3_bm-10"
"thresholds_btl-0.45_btm-4_bm-10"
"thresholds_btl-0.45_btm-5_bm-10"
"thresholds_btl-0.45_btm-7_bm-10"
"thresholds_btl-0.45_btm-10_bm-10"
"thresholds_btl-0.5_btm-3_bm-10"
"thresholds_btl-0.5_btm-4_bm-10"
"thresholds_btl-0.5_btm-5_bm-10"
"thresholds_btl-0.5_btm-7_bm-10"
"thresholds_btl-0.5_btm-10_bm-10"
"thresholds_btl-0.55_btm-3_bm-10"
"thresholds_btl-0.55_btm-4_bm-10"
"thresholds_btl-0.55_btm-5_bm-10"
"thresholds_btl-0.55_btm-7_bm-10"
"thresholds_btl-0.55_btm-10_bm-10"
"thresholds_btl-0.45_btm-3_bm-30"
"thresholds_btl-0.45_btm-4_bm-30"
"thresholds_btl-0.45_btm-5_bm-30"
"thresholds_btl-0.45_btm-7_bm-30"
"thresholds_btl-0.45_btm-10_bm-30"
"thresholds_btl-0.5_btm-3_bm-30"
"thresholds_btl-0.5_btm-4_bm-30"
"thresholds_btl-0.5_btm-5_bm-30"
"thresholds_btl-0.5_btm-7_bm-30"
"thresholds_btl-0.5_btm-10_bm-30"
"thresholds_btl-0.55_btm-3_bm-30"
"thresholds_btl-0.55_btm-4_bm-30"
"thresholds_btl-0.55_btm-5_bm-30"
"thresholds_btl-0.55_btm-7_bm-30"
"thresholds_btl-0.55_btm-10_bm-30"
)


#for test in `seq 1 $numTests`
#do
for m in "${!Maps[@]}" #for m in "${Maps[@]}"
do
    map=${Maps[$m]}
    for c in "${!Configs[@]}"
    do
       config="${Configs[$c]}"
	   
       for traj in "${Trajs[@]}"
       do
            resultPath="Results/${MapsNames[$m]}/$traj/${Outputs[$c]}/"
			
            # Create dir for this test
            if [ ! -d "$resultPath" ] 
            then
                mkdir -p "$resultPath"
            fi 

			# run test
			
           # echo "Running $traj $config"
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
#done   


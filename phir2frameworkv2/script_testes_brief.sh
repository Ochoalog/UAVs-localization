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
#"traj4_460_mod_off"
#"traj4_460_gt_mod"
"traj4_460_gt_mod_brief"
#"traj3_mod"
#"traj3_gt_mod"
#"traj3_good"
#"traj3_gt_good"
#"traj3_eq_good"
#"traj3_eq_gt_good"
#"traj2_mod_off"
#"traj2_gt_mod"
)

Trajs=("${Trajs[$2]}")
#echo "${Trajs[@]}"

Configs=(
"-s BRIEF diff-intensity 19 -bp 1000 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 19 -bp 1500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 19 -bp 2000 -blt 0.5 -bmt 4 -bm 10"
)

Outputs=(
"1000p_btl-0.5_btm-4_bm-10"
"1500p_btl-0.5_btm-4_bm-10"
"2000p_btl-0.5_btm-4_bm-10"
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
			
     		#./$execPath -quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))
			
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


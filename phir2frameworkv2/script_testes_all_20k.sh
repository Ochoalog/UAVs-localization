#!/bin/bash

numTests=$1

execPath="../../../../phir2framework_diego/build-PhiR2Framework-Desktop_Qt_5_5_1_GCC_64bit-Release_20k/PhiR2Framework"

Maps=(
"/home/mathias/Documents/Datasets/ufrgs_google_longterm/adjusted/median_new"
)

MapsNames=(
"median_new_20k"
)

trajsPath="/home/mathias/Documents/Datasets/ufrgs_dronao_28ago/trajectories/"
Trajs=(
"gt_traj2"
"gt_traj3"
"gt_traj4"
)

Configs=(
"-s BRIEF diff-intensity 15 -bp 50 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 15 -bp 100 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 15 -bp 200 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 15 -bp 500 -blt 0.5 -bmt 4 -bm 10"
"-s BRIEF diff-intensity 15 -bp 1000 -blt 0.5 -bmt 4 -bm 10"

#"-s unscented diff-cie2000 4 -unp 50"
#"-s unscented diff-cie2000 4 -unp 100"
#"-s unscented diff-cie2000 4 -unp 200"
#"-s unscented diff-cie2000 4 -unp 500"
#"-s unscented diff-cie2000 4 -unp 1000"

#"-s unscented diff-intensity 15 -unp 50"
#"-s unscented diff-intensity 15 -unp 100"
#"-s unscented diff-intensity 15 -unp 200"
#"-s unscented diff-intensity 15 -unp 500"
#"-s unscented diff-intensity 15 -unp 1000"

#"-s unscented diff-rgb 25 -unp 50"
#"-s unscented diff-rgb 25 -unp 100"
#"-s unscented diff-rgb 25 -unp 200"
#"-s unscented diff-rgb 25 -unp 500"
#"-s unscented diff-rgb 25 -unp 1000"

#"-s BRIEF diff-rgb 25 -bp 200 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-cie2000 4 -unp 200"
#"-s BRIEF diff-rgb 25 -bp 500 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-cie2000 4 -unp 500"
#"-s BRIEF diff-rgb 25 -bp 1000 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-cie2000 4 -unp 1000"

#"-s BRIEF diff-intensity 15 -bp 200 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-intensity 15 -unp 200"
#"-s BRIEF diff-intensity 15 -bp 500 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-intensity 15 -unp 500"
#"-s BRIEF diff-intensity 15 -bp 1000 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-intensity 15 -unp 1000"

#"-s BRIEF diff-rgb 25 -bp 200 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-rgb 25 -unp 200"
#"-s BRIEF diff-rgb 25 -bp 500 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-rgb 25 -unp 500"
#"-s BRIEF diff-rgb 25 -bp 1000 -blt 0.5 -bmt 4 -bm 10 -s unscented diff-rgb 25 -unp 1000"
)

Outputs=(./
"s_BRIEF_diff-intensity_15_-bp_50_-blt_0.5_-bmt_4_-bm_10"
"s_BRIEF_diff-intensity_15_-bp_100_-blt_0.5_-bmt_4_-bm_10"
"s_BRIEF_diff-intensity_15_-bp_200_-blt_0.5_-bmt_4_-bm_10"
"s_BRIEF_diff-intensity_15_-bp_500_-blt_0.5_-bmt_4_-bm_10"
"s_BRIEF_diff-intensity_15_-bp_1000_-blt_0.5_-bmt_4_-bm_10"

#"s_unscented_diff-cie2000_4_-unp_50"
#"s_unscented_diff-cie2000_4_-unp_100"
#"s_unscented_diff-cie2000_4_-unp_200"
#"s_unscented_diff-cie2000_4_-unp_500"
#"s_unscented_diff-cie2000_4_-unp_1000"

#"s_unscented_diff-intensity_15_-unp_50"
#"s_unscented_diff-intensity_15_-unp_100"
#"s_unscented_diff-intensity_15_-unp_200"
#"s_unscented_diff-intensity_15_-unp_500"
#"s_unscented_diff-intensity_15_-unp_1000"

#"s_unscented_diff-rgb_25_-unp_50"
#"s_unscented_diff-rgb_25_-unp_100"
#"s_unscented_diff-rgb_25_-unp_200"
#"s_unscented_diff-rgb_25_-unp_500"
#"s_unscented_diff-rgb_25_-unp_1000"

#"s_BRIEF_diff-rgb_25_-bp_200_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-cie2000_4_-unp_200"
#"s_BRIEF_diff-rgb_25_-bp_500_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-cie2000_4_-unp_500"
#"s_BRIEF_diff-rgb_25_-bp_1000_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-cie2000_4_-unp_1000"

#"s_BRIEF_diff-intensity_15_-bp_200_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-intensity_15_-unp_200"
#"s_BRIEF_diff-intensity_15_-bp_500_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-intensity_15_-unp_500"
#"s_BRIEF_diff-intensity_15_-bp_1000_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-intensity_15_-unp_1000"

#"s_BRIEF_diff-rgb_25_-bp_200_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-rgb_25_-unp_200"
#"s_BRIEF_diff-rgb_25_-bp_500_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-rgb_25_-unp_500"
#"s_BRIEF_diff-rgb_25_-bp_1000_-blt_0.5_-bmt_4_-bm_10_-s_unscented_diff-rgb_25_-unp_1000"
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


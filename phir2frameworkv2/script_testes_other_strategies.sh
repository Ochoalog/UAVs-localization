#!/bin/bash

numTests=$1

execPath="/home/mathias/Dropbox/Mestrado/Localização/build-PhiR2Framework-Desktop-Debug/PhiR2Framework"

Maps=(
"/home/mathias/Documentos/Datasets/ufrgs_google_longterm/adjusted/median_goodplusnew"
)
MapsNames=(
"median_goodplusnew"
)
#echo "NumMaps" ${#Maps[@]}

trajsPath="/home/mathias/Documentos/Datasets/ufrgs_dronao_28ago/"
Trajs=(
"traj4_460_mod_off"
"traj4_460_gt_mod"
#"traj3_mod"
#"traj3_gt_mod"
#"traj3_good"
#"traj3_gt_good"
#"traj3_eq_good"
#"traj3_eq_gt_good"
"traj2_mod_off"
"traj2_gt_mod"
)

Trajs=("${Trajs[$2]}")
echo "${Trajs[@]}"

#numConfigs=4
#lenConfig=1
Configs=(
#"-s color diff-intensity 50 -s density diff-intensity 35.640406 circular 10"
#"-s color diff-intensity 50 -s density diff-intensity 35.640406 circular 20"  
#"-s color diff-intensity 50 -s density diff-intensity 35.640406 inverted 5"
#"-s color diff-intensity 50 -s density diff-intensity 35.640406 gaussian 5"
#"-s color diff-intensity 25 -s density diff-intensity 35.640406 circular 10"
#"-s color diff-intensity 25 -s density diff-intensity 35.640406 circular 20"  
#"-s color diff-intensity 25 -s density diff-intensity 35.640406 inverted 5"
#"-s color diff-intensity 25 -s density diff-intensity 35.640406 gaussian 5"
#"-s color diff-cmc1984 25 -s density diff-cmc1984 24.400485 circular 10"
#"-s color diff-cmc1984 25 -s density diff-cmc1984 24.400485 circular 20"
#"-s color diff-cmc1984 25 -s density diff-cmc1984 24.400485 gaussian 10"
#"-s color diff-cmc1984 25 -s density diff-cmc1984 24.400485 inverted 10"
#"-s color diff-cmc1984 40 -s density diff-cmc1984 24.400485 circular 10"
#"-s color diff-cmc1984 40 -s density diff-cmc1984 24.400485 circular 20"
#"-s color diff-cmc1984 40 -s density diff-cmc1984 24.400485 gaussian 10"
#"-s color diff-cmc1984 40 -s density diff-cmc1984 24.400485 inverted 10"
#"-s color diff-rgb 15 -s density diff-rgb 48.630549 circular 10"
#"-s color diff-rgb 15 -s density diff-rgb 63.503281 circular 10"
#"-s color diff-rgb 15 -s density diff-rgb 63.503281 gaussian 5"
#"-s color diff-rgb 15 -s density diff-rgb 63.503281 inverted 5"
#"-s color diff-rgb 40 -s density diff-rgb 48.630549 circular 10"
#"-s color diff-rgb 40 -s density diff-rgb 63.503281 circular 10"
#"-s color diff-rgb 40 -s density diff-rgb 63.503281 gaussian 5"
#"-s color diff-rgb 40 -s density diff-rgb 63.503281 inverted 5"
#"-s color diff-cie2000 15 -s density diff-cie2000 13.889990 circular 10"
#"-s color diff-cie2000 15 -s density diff-cie2000 13.889990 circular 20"
#"-s color diff-cie2000 15 -s density diff-cie2000 13.889990 inverted 10"
#"-s color diff-cie2000 15 -s density diff-cie2000 13.889990 gaussian 10"
#"-s color diff-cie2000 20 -s density diff-cie2000 13.889990 circular 10"
#"-s color diff-cie2000 20 -s density diff-cie2000 13.889990 circular 20"
#"-s color diff-cie2000 20 -s density diff-cie2000 13.889990 inverted 10"
#"-s color diff-cie2000 20 -s density diff-cie2000 13.889990 gaussian 10"
#"-s color diff-intensity 25 -s entropy diff-intensity 2 circular 10"
#"-s color diff-intensity 25 -s entropy diff-intensity 8 circular 10"
#"-s color diff-intensity 25 -s entropy diff-intensity 8 gaussian 10"
#"-s color diff-intensity 25 -s entropy diff-intensity 8 inverted 10"
#"-s color diff-intensity 50 -s entropy diff-intensity 2 circular 10"
#"-s color diff-intensity 50 -s entropy diff-intensity 8 circular 10"
#"-s color diff-intensity 50 -s entropy diff-intensity 8 gaussian 10"
#"-s color diff-intensity 50 -s entropy diff-intensity 8 inverted 10"
#"-s color diff-cie2000 15 -s entropy diff-cie1976 2 circular 10"
#"-s color diff-cie2000 15 -s entropy diff-cie1976 4 circular 10"
#"-s color diff-cie2000 15 -s entropy diff-cie1976 8 circular 10"
#"-s color diff-cie2000 15 -s entropy diff-cie1976 8 gaussian 10"
#"-s color diff-cie2000 15 -s entropy diff-cie1976 8 inverted 10"
#"-s color diff-cie2000 20 -s entropy diff-cie1976 2 circular 10"
#"-s color diff-cie2000 20 -s entropy diff-cie1976 4 circular 10"
#"-s color diff-cie2000 20 -s entropy diff-cie1976 8 circular 10"
#"-s color diff-cie2000 20 -s entropy diff-cie1976 8 gaussian 10"
#"-s color diff-cie2000 20 -s entropy diff-cie1976 8 inverted 10"
#"-s color diff-rgb 15 -s entropy diff-rgb 2 circular 10"
#"-s color diff-rgb 15 -s entropy diff-rgb 8 circular 10"
#"-s color diff-rgb 15 -s entropy diff-rgb 8 gaussian 10"
#"-s color diff-rgb 15 -s entropy diff-rgb 8 inverted 10"
#"-s color diff-rgb 40 -s entropy diff-rgb 2 circular 10"
#"-s color diff-rgb 40 -s entropy diff-rgb 8 circular 10"
#"-s color diff-rgb 40 -s entropy diff-rgb 8 gaussian 10"
#"-s color diff-rgb 40 -s entropy diff-rgb 8 inverted 10"
"-s color diff-intensity 25 -s mi diff-intensity 2 circular 10"
"-s color diff-intensity 25 -s mi diff-intensity 8 circular 10"
"-s color diff-intensity 50 -s mi diff-intensity 2 circular 10"
"-s color diff-intensity 50 -s mi diff-intensity 8 circular 10"
"-s color diff-cie2000 15 -s mi diff-cie1976 2 circular 10"
"-s color diff-cie2000 15 -s mi diff-cie1976 8 circular 10"
"-s color diff-cie2000 20 -s mi diff-cie1976 2 circular 10"
"-s color diff-cie2000 20 -s mi diff-cie1976 8 circular 10"
"-s color diff-rgb 15 -s mi diff-rgb 2 circular 10"
"-s color diff-rgb 15 -s mi diff-rgb 8 circular 10"
"-s color diff-rgb 40 -s mi diff-rgb 2 circular 10"
"-s color diff-rgb 40 -s mi diff-rgb 8 circular 10"
)

Outputs=(
#"s_color_diff-intensity_50_-s_density_diff-intensity_35.640406_circular_10"
#"s_color_diff-intensity_50_-s_density_diff-intensity_35.640406_circular_20"
#"s_color_diff-intensity_50_-s_density_diff-intensity_35.640406_inverted_5"
#"s_color_diff-intensity_50_-s_density_diff-intensity_35.640406_gaussian_5"
#"s_color_diff-intensity_25_-s_density_diff-intensity_35.640406_circular_10"
#"s_color_diff-intensity_25_-s_density_diff-intensity_35.640406_circular_20"
#"s_color_diff-intensity_25_-s_density_diff-intensity_35.640406_inverted_5"
#"s_color_diff-intensity_25_-s_density_diff-intensity_35.640406_gaussian_5"
#"s_color_diff-cmc1984_25_-s_density_diff-cmc1984_24.400485_circular_10"
#"s_color_diff-cmc1984_25_-s_density_diff-cmc1984_24.400485_circular_20"
#"s_color_diff-cmc1984_25_-s_density_diff-cmc1984_24.400485_gaussian_10"
#"s_color_diff-cmc1984_25_-s_density_diff-cmc1984_24.400485_inverted_10"
#"s_color_diff-cmc1984_40_-s_density_diff-cmc1984_24.400485_circular_10"
#"s_color_diff-cmc1984_40_-s_density_diff-cmc1984_24.400485_circular_20"
#"s_color_diff-cmc1984_40_-s_density_diff-cmc1984_24.400485_gaussian_10"
#"s_color_diff-cmc1984_40_-s_density_diff-cmc1984_24.400485_inverted_10"
#"s_color_diff-rgb_15_-s_density_diff-rgb_48.630549_circular_10"
#"s_color_diff-rgb_15_-s_density_diff-rgb_63.503281_circular_10"
#"s_color_diff-rgb_15_-s_density_diff-rgb_63.503281_gaussian_5"
#"s_color_diff-rgb_15_-s_density_diff-rgb_63.503281_inverted_5"
#"s_color_diff-rgb_40_-s_density_diff-rgb_48.630549_circular_10"
#"s_color_diff-rgb_40_-s_density_diff-rgb_63.503281_circular_10"
#"s_color_diff-rgb_40_-s_density_diff-rgb_63.503281_gaussian_5"
#"s_color_diff-rgb_40_-s_density_diff-rgb_63.503281_inverted_5"
#"s_color_diff-cie2000_15_-s_density_diff-cie2000_13.889990_circular_10"
#"s_color_diff-cie2000_15_-s_density_diff-cie2000_13.889990_circular_20"
#"s_color_diff-cie2000_15_-s_density_diff-cie2000_13.889990_inverted_10"
#"s_color_diff-cie2000_15_-s_density_diff-cie2000_13.889990_gaussian_10"
#"s_color_diff-cie2000_20_-s_density_diff-cie2000_13.889990_circular_10"
#"s_color_diff-cie2000_20_-s_density_diff-cie2000_13.889990_circular_20"
#"s_color_diff-cie2000_20_-s_density_diff-cie2000_13.889990_inverted_10"
#"s_color_diff-cie2000_20_-s_density_diff-cie2000_13.889990_gaussian_10"
#"s_color_diff-intensity_25_-s_entropy_diff-intensity_2_circular_10"
#"s_color_diff-intensity_25_-s_entropy_diff-intensity_8_circular_10"
#"s_color_diff-intensity_25_-s_entropy_diff-intensity_8_gaussian_10"
#"s_color_diff-intensity_25_-s_entropy_diff-intensity_8_inverted_10"
#"s_color_diff-intensity_50_-s_entropy_diff-intensity_2_circular_10"
#"s_color_diff-intensity_50_-s_entropy_diff-intensity_8_circular_10"
#"s_color_diff-intensity_50_-s_entropy_diff-intensity_8_gaussian_10"
#"s_color_diff-intensity_50_-s_entropy_diff-intensity_8_inverted_10"
#"s_color_diff-cie2000_15_-s_entropy_diff-cie1976_2_circular_10"
#"s_color_diff-cie2000_15_-s_entropy_diff-cie1976_4_circular_10"
#"s_color_diff-cie2000_15_-s_entropy_diff-cie1976_8_circular_10"
#"s_color_diff-cie2000_15_-s_entropy_diff-cie1976_8_gaussian_10"
#"s_color_diff-cie2000_15_-s_entropy_diff-cie1976_8_inverted_10"
#"s_color_diff-cie2000_20_-s_entropy_diff-cie1976_2_circular_10"
#"s_color_diff-cie2000_20_-s_entropy_diff-cie1976_4_circular_10"
#"s_color_diff-cie2000_20_-s_entropy_diff-cie1976_8_circular_10"
#"s_color_diff-cie2000_20_-s_entropy_diff-cie1976_8_gaussian_10"
#"s_color_diff-cie2000_20_-s_entropy_diff-cie1976_8_inverted_10"
#"s_color_diff-rgb_15_-s_entropy_diff-rgb_2_circular_10"
#"s_color_diff-rgb_15_-s_entropy_diff-rgb_8_circular_10"
#"s_color_diff-rgb_15_-s_entropy_diff-rgb_8_gaussian_10"
#"s_color_diff-rgb_15_-s_entropy_diff-rgb_8_inverted_10"
#"s_color_diff-rgb_40_-s_entropy_diff-rgb_2_circular_10"
#"s_color_diff-rgb_40_-s_entropy_diff-rgb_8_circular_10"
#"s_color_diff-rgb_40_-s_entropy_diff-rgb_8_gaussian_10"
#"s_color_diff-rgb_40_-s_entropy_diff-rgb_8_inverted_10"
"s_color_diff-intensity_25_-s_mi_diff-intensity_2_circular_10"
"s_color_diff-intensity_25_-s_mi_diff-intensity_8_circular_10"
"s_color_diff-intensity_50_-s_mi_diff-intensity_2_circular_10"
"s_color_diff-intensity_50_-s_mi_diff-intensity_8_circular_10"
"s_color_diff-cie2000_15_-s_mi_diff-cie1976_2_circular_10"
"s_color_diff-cie2000_15_-s_mi_diff-cie1976_8_circular_10"
"s_color_diff-cie2000_20_-s_mi_diff-cie1976_2_circular_10"
"s_color_diff-cie2000_20_-s_mi_diff-cie1976_8_circular_10"
"s_color_diff-rgb_15_-s_mi_diff-rgb_2_circular_10"
"s_color_diff-rgb_15_-s_mi_diff-rgb_8_circular_10"
"s_color_diff-rgb_40_-s_mi_diff-rgb_2_circular_10"
"s_color_diff-rgb_40_-s_mi_diff-rgb_8_circular_10"
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
#            echo "$resultPath"
            # Create dir for this test
            if [ ! -d "$resultPath" ] 
            then
                mkdir -p "$resultPath"
            fi 

            # run test
            echo "Running $traj $config"
            count=`ls "$resultPath" 2>/dev/null | wc -l`
            echo "-quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))"		
#            ./$execPath -quiet -e $map -t $trajsPath$traj.txt $config -o "$resultPath" $count $(($count+$numTests-1))
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


#!/bin/bash

numTests=$1

execPath="../build-PhiR2Framework-Desktop-Release/PhiR2Framework"

Maps=( 
#"/home/mathias/Documentos/Datasets/ufrgs_google_longterm/adjusted/median_goodplusnew"
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
echo "${Trajs[@]}"

#numConfigs=4
#lenConfig=1
Configs=(
"unscented diff-intensity"
)

Limiar=(
1
)

Pairs=(
100 200 300 400 500 600 700 800 900 1000 1500 2000
)

for test in `seq 1 $numTests`
do
    for m in "${!Maps[@]}" #for m in "${Maps[@]}"
    do
        map=${Maps[$m]}
        for traj in "${Trajs[@]}"
        do
            for lim in "${Limiar[@]}"
            do
        #        for c in `seq 1 $numConfigs`
                for config in "${Configs[@]}"
                do

                    # Create dir for this test
                    if [ ! -d "Results/${MapsNames[$m]}/$traj/$config $lim" ] 
                    then
                        mkdir -p "Results/${MapsNames[$m]}/$traj/$config $lim"
                    fi 

                    # run test
                    echo "Running $traj -s $config $lim ($test)"
					#echo ./$execPath -e $map -t $trajsPath$traj.txt -s $config $lim
                    ./$execPath -quiet -e $map -t $trajsPath$traj.txt -s $config $lim

                    # Move log files to Results
                    ls -t Logs | head -n 1 | while read f
                    do
                        count=`ls "Results/${MapsNames[$m]}/$traj/$config $lim/" 2>/dev/null | wc -l`
                        mv "Logs/$f" "Results/${MapsNames[$m]}/$traj/$config $lim/$count.txt"
                    done
                done
            done 
        done
    done 
done   


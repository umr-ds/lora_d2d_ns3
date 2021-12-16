#!/bin/bash

# Start CORE gui if $DISPLAY is set
if [ -n "$RUN" ]; then
    echo "# Starting experiments from path $RUN"
    source "$RUN"

    exp_folder=$(date +"%Y-%m-%d_%H%M%S")
    log_path="/results/$exp_folder"

    mkdir -p "$log_path"


    for nodes in "${NODES[@]}"; do
        for area in "${AREA[@]}"; do
            for freq in "${FREQ[@]}"; do
                for bps in "${BPS[@]}"; do
                    for sps in "${SPS[@]}"; do
                        for bw in "${BW[@]}"; do
                            for payload in "${PAYLOAD_SIZE[@]}"; do
                                for msg_per_node in "${MSG_PER_NODE[@]}"; do
                                    name="${nodes}_${area}_${freq}_${bps}_${sps}_${bw}_${payload}_${msg_per_node}"

                                    echo "# Starting $name"
                                    cmd="./waf --run=\"lora_d2d
                                        --nodes=${nodes}
                                        --area=${area}
                                        --freq=${freq}
                                        --bps=${bps}
                                        --sps=${sps}
                                        --bw=${bw}
                                        --payload_size=${payload}
                                        --msg=${msg_per_node}
                                        --sim_time=${SIM_TIME}
                                        --iter=${RUNS}\" > \"$log_path/$name.err\" 2>\"$log_path/$name.csv\""

                                    sem -j+0 bash -c "'$cmd'"

                                    echo "# `ps aux | grep "python3 ./waf" | wc -l` parallel jobs running"
                                done
                            done
                        done
                    done
                done
            done
        done
    done

    while true; do echo "# Waiting for `ps aux | grep "python3 ./waf" | wc -l` jobs."; sleep 5; done &
    statuspid="$!"

    sem --wait
    kill "$statuspid"
    echo "Done."

else
    echo "# Dropping into bash"
    bash
fi

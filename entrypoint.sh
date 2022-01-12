#!/bin/bash

# Start CORE gui if $DISPLAY is set
if [ -n "$RUN" ]; then
    echo "# Starting experiments from path $RUN"
    source "$RUN"

    if [ -z ${LOG_PATH+x} ];
    then
    	exp_folder=$(date +"%Y-%m-%d_%H%M%S")
    	LOG_PATH="/results/$exp_folder"
    fi

    mkdir -p "$LOG_PATH"

    parallelism=$(( $(nproc) - $(nproc) * 10 / 100 ))
    echo "Will use $parallelism for experiments."

    PARAM_COMBINATIONS=$(( ${#BWS[@]} - 1 ))
    echo "Found $PARAM_COMBINATIONS combinations"

    END=$(( 35039 + RUNS - 1))
    echo "Will repeat $RUNS runs (from 35039 to $END)"

    for nodes in "${NODES[@]}"; do
        for area in "${AREA[@]}"; do
            for freq in "${FREQ[@]}"; do
                for msg_per_node in "${MSG_PER_NODE[@]}"; do
                    for index in $(seq 0 $PARAM_COMBINATIONS); do
                        bw=${BWS[$index]}
                        sps=${SPS[$index]}
                        bps=${BPS[$index]}
                        payload=${PAYLOAD[$index]}

			for seed in $(seq 35039 $END); do

				name="${nodes}_${area}_${freq}_${bps}_${sps}_${bw}_${payload}_${msg_per_node}_${seed}"
				output_path="$LOG_PATH/$name.log"

				if [[ -f "$output_path" ]]; then
				    echo "$output_path exists."
				    continue
				fi

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
				    --seed=${seed}\" > \"$LOG_PATH/$name.err\" 2> \"$output_path\""

				sem -j $parallelism bash -c "'$cmd'"

				echo "# `ps aux | grep "python3 ./waf" | wc -l` parallel jobs running"
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

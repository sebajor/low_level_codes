#!/bin/bash
#if one cmd fails exit
set -e

data_dir="/home/interferometer/data"
corr_file="/home/interferometer/sandbox/masked_correlator/build/correlation"

binary="/home/interferometer/sandbox/masked_correlator/build/phase_correlator"
flo=1953000000.0
gain=20
batch_integration=1
window=50
bin_cmd="$binary -flo $flo -gain $gain -batch_integration $batch_integration -window $window"

###
###
###

today=$(date '+%Y%m%d_%H%M%S')

# If correlation file exists, archive it
if [ -f "$corr_file" ]; then

    n=1

    while [ -e "$data_dir/${today}_$n" ]; do
        n=$((n+1))
    done

    dest="$data_dir/${today}_$n"

    echo "Archiving old correlation file to $dest"

    mv "$corr_file" "$dest"
    echo "$corr_file" "$dest"


    # Optional remote sync
fi

#rsync -av "$dest" pi@10.0.33.98:/home/pi/

echo "Starting correlator..."
echo $bin_cmd
#exec "$bin_cmd"
$bin_cmd

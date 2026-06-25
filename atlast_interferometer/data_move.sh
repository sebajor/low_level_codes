dest_dir=/home/interferometer/data/
dest=$dest_dir$(date '+%Y%m%d')
sour=/home/interferometer/sandbox/masked_correlator/build/correlation

remote_ip=10.0.17.199
remote_user="forecast"
remote_loc="/home/forecast/meteoscope/Scripts/AtLAST/raw_data_site2/"

####
####
####
echo "Moving $sour to $dest"
mv $sour $dest

echo "copy the data to $remote_user@$remote_ip:$remote_loc"
rsync -av --inplace $dest_dir $remote_user@$remote_ip:$remote_loc

#!/bin/bash

# Directory with raw data files
DATA_DIR="/home/interferometer/data/"

# Find and delete files older than 7 days
find "$DATA_DIR" -type f -mtime +6 -exec rm -f {} \;

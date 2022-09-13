#!/bin/bash




op_file=${1:-"/tmp/trace_data_dir/op_trace_human_keys_remap.txt"}

block_file=${2:-"/tmp/trace_data_dir/block_trace_human_file"}

io_file=${3:-"/tmp/trace_data_dir/abs_time_io_trace_res"}



put_op_file="/tmp/trace_data_dir/op_put.txt"

block_compaction_file="/tmp/trace_data_dir/block_compact.txt"

io_append_file="/tmp/trace_data_dir/io_put.txt"


awk '{if($3 == 1) {print}}' $op_file > $put_op_file

echo "op done"

# block cache caller ? how about compaction ? 
awk -F ',' '{if($9 == 10) {print}}' $block_file > $block_compaction_file

echo "block done"

awk '{if($2 == 1)  {print}}' $io_file > $io_append_file

echo "io done"


# cache miss ? 



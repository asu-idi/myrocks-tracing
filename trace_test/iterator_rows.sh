





op_file="/tmp/trace_data_dir/op_trace_human_keys_remap.txt"


block_cache_file="/tmp/trace_data_dir/block_trace_human_file"


io_file="/tmp/trace_data_dir/abs_time_io_trace_res"


op_out=${1:-"/tmp/trace_data_dir/iterator_row.txt"}

block_out=${2:-"/tmp/trace_data_dir/block_iterator.txt"}

io_out=${3:-"/tmp/trace_data_dir/io_prefetch.txt"}

op_type=${4:-6}

block_caller_type=${5:-"3"}

io_type=${6:-"9"}


# iterator op : 6
awk -v op_type="$op_type" '{if($3 == op_type) {print}}' $op_file > $op_out

echo "op done"


awk -v block_type="$block_caller_type" \
  -F ',' '{if($9 == block_type) {print}}' $block_cache_file > $block_out


echo "block done"


awk -v io_type="$io_type" '{if($2 == io_type) {print}}' \
  $io_file > $io_out

echo "io done"

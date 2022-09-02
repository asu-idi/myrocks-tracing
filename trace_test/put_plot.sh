




op_file="/tmp/trace_data_dir/op_put.txt"

block_file="/tmp/trace_data_dir/block_compact.txt"


io_file="/tmp/trace_data_dir/io_put.txt"

tr_put_file="/tmp/block_tr_put.txt"

cat $block_file | tr ','  ' ' > $tr_put_file

output_file=${1:-"/tmp/trace_data_dir/put_output.png"}
gnuplot -e "op_file='$op_file'" \
  -e "block_file='$tr_put_file'" \
  -e "io_file='$io_file'" \
  -e "output_file='$output_file'" \
  plot.gp


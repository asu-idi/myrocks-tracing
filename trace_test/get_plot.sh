

op_file=${1:-"/tmp/trace_data_dir/op_get_row.txt"}

block_file=${2:-"/tmp/trace_data_dir/block_get_row.txt"}

my_io_file=${3:-"/tmp/trace_data_dir/io_get_row.txt"}

output_file=${4:-"/tmp/trace_data_dir/get_output.png"}

tr_block_file="/tmp/block_get_tr.txt"

# cat $block_file | tr ',' ' ' > $tr_block_file


# echo "io file is $my_io_file "

# echo "$block_file"

gnuplot -e "op_file='$op_file'" \
  -e "block_file='$tr_block_file'" \
  -e "io_file='$my_io_file'" \
  -e "output_file='$output_file'" \
  plot.gp
  

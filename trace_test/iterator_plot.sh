





op_file=${1:-"/tmp/trace_data_dir/iterator_row.txt"}

block_file=${2:-"/tmp/trace_data_dir/block_iterator.txt"}

io_file=${3:-"/tmp/trace_data_dir/io_prefetch.txt"}




tr_iter_file="/tmp/block_tr_iter.txt"


cat $block_file | tr ',' ' ' > $tr_iter_file

output_plt="/tmp/trace_data_dir/iter_block_id.png"

gnuplot -e "op_file='$op_file'" \
  -e "block_file='$tr_iter_file'" \
  -e "io_file='$io_file'" \
  -e "output_file='$output_plt'" \
  plot.gp


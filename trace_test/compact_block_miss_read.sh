



block_compact_miss="/tmp/trace_data_dir/block_compact_tr.txt"

op_get_file="/tmp/trace_data_dir/op_get_row.txt"


io_read_file="/tmp/trace_data_dir/io_get_row.txt"


output_png="/tmp/trace_data_dir/compact_miss_read.png"



gnuplot -e "op_file='$op_get_file'"\
  -e "block_file='$block_compact_miss'" \
  -e "io_file='$io_read_file'" \
  -e "output_file='$output_png'" \
  plot.gp





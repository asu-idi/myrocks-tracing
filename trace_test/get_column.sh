

op_file="/tmp/trace_data_dir/op_trace-human_readable_trace.txt"
op_get_file="/tmp/trace_data_dir/op_get_row.txt"

awk '{if($2 == 0) {print}}' $op_file > $op_get_file

# we don't have latency data for op get
# we only get the timestamp of it.

# wc -l $op_get_file

echo "op done"

# block
block_file="/tmp/trace_data_dir/block_trace_human_file"
block_get_file="/tmp/trace_data_dir/block_get_row.txt"

awk -F ','  '{if($9 == 1) {print}}' $block_file > $block_get_file


echo "block done"
#io get

io_file="/tmp/trace_data_dir/abs_time_io_trace_res"
io_get_file="/tmp/trace_data_dir/io_get_row.txt"
awk '{if($14 == "Read") {print}}' $io_file > $io_get_file 

echo "io done"



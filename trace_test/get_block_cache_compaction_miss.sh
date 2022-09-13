

comp_block_cache_miss_file="/tmp/trace_data_dir/block_compact.txt"


miss_compaction_file="/tmp/trace_data_dir/block_compact_miss.txt"


awk -F ',' '{if($14 == 0) {print }}'\
  $comp_block_cache_miss_file  > \
  $miss_compaction_file


#block file transform

output_file="/tmp/trace_data_dir/block_compact_tr.txt"

cat $miss_compaction_file |  tr ',' ' ' > $output_file


# block cache miss in compaction.

# read file latency







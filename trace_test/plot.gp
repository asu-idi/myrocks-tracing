


if(!exists("op_file")) {
    print("op file not exists")
    exit;
  }

if(!exists("block_file")) {
    print("block file not exists")
    exit;
  }


if(!exists("io_file")) {
    print("io file not exists")
    exit;
  }


print("io file".io_file)
print("block file is".block_file)
print("op file".op_file)

set terminal  png size 800, 400

set output output_file
set xlabel "time(ms)"

set y2tics

set multiplot


set key left

set size 1, 0.5
set origin 0, 0

#set y2range [-1:2]
plot io_file using 1:17 title "io file latency" w dots, \
 block_file using 1:2 title "block id" w dots axes x1y2

set key left
set size 1, 0.5
set origin 0.0, 0.5


#set y2range [-1:2]
plot op_file using  5:1 title "op keys  " w dots, \
 block_file using 1:2 title "block id" w dots axes x1y2

unset multiplot


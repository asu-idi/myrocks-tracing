#!/bin/perl

use 5.010;
use Data::Dumper;
$block_cache_file = "/tmp/trace_data_dir/block_trace_human_file";

my %callers = ( 1 => 'Get', 
  2 => 'MultiGet',
  3 => 'Iterator',
  4 => 'ApproximateSize',
  5 => 'Checksum',
  6 => 'dumptool',
  7 => 'sstingestion',
  8 => 'Repair',
  9 => 'prefetch',
  10 => 'Compaction',
  11 => 'CompactionRefill',
 12 => 'Flush',
 13 => 'SSTFileReader',
 14 => 'Uncategorized');


# sort in the order of
# caller type, and then hit , and then timestamp numeric.

@sort_output = qx(awk -F ',' '{print \$9,\$14}'  $block_cache_file  \\
| sort -t ',' -k1 \\
| uniq -c);

print "@sort_output\n";
my  %callers_hash = ();
foreach (@sort_output)  {
  my @strs = split(' ',  $_);
  $count = @strs[0];
  $caller = @strs[1];
  $hit = @strs[2];
  # if(!exists $callers_hash{$caller}) {
    # $callers_hash{$caller}  = @array; 
    push @{$callers_hash{$caller}}, \@strs;

    # print "$callers_hash{$caller}[0]\n";
  # } else {
  # }

}


foreach my $val (values %callers_hash) {
  my @arrs = @{$val};
  my $size = @arrs;
  # print "size is $size\n";
  my $count_sum = 0;
  foreach (@arrs) {
    my @arr = @{$_};
    my $count = @arr[0];
    $count_sum += $count;
    print "@arr  \n";
  }


  foreach (@arrs) {
    my @arr = @{$_};
    my $count = @arr[0];
    my $caller = @arr[1];
    my $hit = @arr[2];
    my $ratio = $count / $count_sum;
    print "$callers{$caller} -  $hit : $ratio\n";
  }


  print "sum: $count_sum\n";
  # print "@{@arrs[0]}\n";
  # print "@{@{$val}[0]}[0]\n";
}

# say Dumper \%callers_hash;


exit;


@block_caller = qx( awk -F ',' '{print \$9}'  $block_cache_file  \\
| sort  -k1 \\
| uniq -c  );


# $sum = qx(print @block_caller | awk '{s += \$1} END {print s}');
$sum = 0;
foreach (@block_caller) {
  print "$_\n";
  @strs = split(' ', $_);
  $caller_count = @strs[0];
  $sum += $caller_count;
}




print "sum is $sum\n ";
foreach (@block_caller) {
  @strs = split(' ', $_);
  $count = @strs[0];
  printf ("%s: %f\n", $callers{"@strs[1]"}, $count / $sum);
}
# print "$sum\n";

# @strs = split(' ',@block_caller[0]);

# print "@strs[0]\n"


# we want to know the percentage  of each caller type

# we want the hit rate within each caller type
# I will build a hash for this .

# print "@sort_output\n";



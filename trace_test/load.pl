#!/bin/perl

sub generate {
  @match_tbls = `ls /home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen/*.tbl`;

# print "@match_tbls";
  my @base_file = ();
  foreach (@match_tbls )  {
   $basename = `basename $_`; 
   $basename = substr $basename, 0, -5 ;
   # print "base name is $basename\n";
   push(@base_file, $basename);
  }

# print "files are @base_file\n";



  my @load_sqls = ();
  $tbl_dir = "/home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen";
  foreach (@base_file) {
    $file = "$tbl_dir/$_.tbl";
    $sql = "LOAD DATA LOCAL INFILE $file into TABLE $_ FIELDS TERMINATED BY '|';";
    print "$sql\n";
  }
}

@tbls = generate();
print "@tbls";

# for (my $i =0; $i <= $#base_file; $i++) {
#   $sql = "LOAD DATA LOCAL  INFILE  "
#   print "$match_tbls[$i]  $base_file[$i]\n";
# }

# foreach (@base_file) {
#   $sql = "LOAD DATA LOCAL INFILE "
# }

# print "match tbls are $match_tbls\n";


# $tbls = `awk -f copy_into.awk $tbl_dir/*`;

# print "res is \n $tbls\n";

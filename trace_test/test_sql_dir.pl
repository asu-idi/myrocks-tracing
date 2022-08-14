#!/bin/perl




$tbl_dir = "/home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen";
$sql_dir = "$tbl_dir/queries";

@sql_files = `ls $sql_dir`;

foreach(@sql_files) {
  chomp($_);
  print "sql file is $_\n";
}

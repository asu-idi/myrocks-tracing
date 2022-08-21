#!/bin/perl

# sub generate {
#   @match_tbls = `ls /home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen/*.tbl`;

# # print "@match_tbls";
#   my @base_file = ();
#   foreach (@match_tbls )  {
#    $basename = `basename $_`; 
#    $basename = substr $basename, 0, -5 ;
#    # print "base name is $basename\n";
#    push(@base_file, $basename);
#   }

# # print "files are @base_file\n";



#   my @load_sqls = ();
#   $tbl_dir = "/home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen";
#   foreach (@base_file) {
#     $file = "$tbl_dir/$_.tbl";
#     $sql = "LOAD DATA LOCAL INFILE $file into TABLE $_ FIELDS TERMINATED BY '|';";
#     print "$sql\n";
#   }
# }

# @tbls = generate();
# print "@tbls";

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





# $hostname = `hostname`;
# chomp($hostname);
# $pid_file = "$db_dir/$hostname.pid";
# $server_pid = `cat $pid_file`;
# chomp($server_pid);

$schema_path = "/home/bily/mysql-5.6/trace_test/tpch_mysql_schema.sql";
my $table_res = `mysql -u root test < "$schema_path"`;
# if ($?) {
#   print "table create fail\n $table_res";
#   kill 9,$server_pid;
#   exit;
# }


$tables = `mysql -u root -e "use test; show tables;"`;
print "tables are \n$tables\n";

# should load all the tables into the database
sub generate_load_sqls {

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
    $uppercase_table_name  = uc $_;
    $sql = "LOAD DATA LOCAL INFILE '$file' into TABLE $uppercase_table_name FIELDS TERMINATED BY '|';";
    push(@load_sqls, $sql);
    # print "$sql\n";

  }

  return @load_sqls;

} 
@load_sqls = generate_load_sqls();
foreach (@load_sqls) {
  print "sql is $_\n";
  $load_outpt = `mysql -u root -e "SET session rocksdb_bulk_load=1; use test; $_;"`;
}
# $load_output = `mysql -u root -e "SET session rocksdb_bulk_load=1; use test; LOAD DATA LOCAL INFILE '~/tpc-h/TPC-H_Tools_v3.0.0/dbgen/lineitem.tbl' INTO TABLE lineitem FIELDS TERMINATED BY '|';"`;

# $alter_res = `mysql -u root test < tpch_mysql_schema_alter.sql`;
# print "aterl result is $alter_res\n";

$line_count = `mysql -u root -e "use test; select count(*) from LINEITEM;"`;

print "number of lines is $line_count";
# print $load_outpt;




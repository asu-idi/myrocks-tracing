#!/usr/bin/perl




#install db 

$default_file = "/home/bily/mysql-5.6/trace_test/my.cnf";
$mysql_install_exe = "/home/bily/mysql-5.6/scripts/mysql_install_db";
$mysql_install_dir = "/usr/local/mysql";

$db_dir =  "/home/bily/mysql-5.6/test_data";

`rm -rf $db_dir`;

$install_res = `$mysql_install_exe --basedir=$mysql_install_dir --defaults-file=$default_file`;
if ($?) {
  print "install fail \n $install_res";
  exit;
} 

print "install successfully\n";
# start server
# $server_pid = fork;
# if ($server_pid == 0) {
  # this is the child process
# my $start_res = `mysqld_safe --defaults-file=$default_file `;
# if ($?) {
#   print " start server failed \n $start_res";
#   exit;
# }

# }

$pid = fork;
if($pid == 0) {
  exec("mysqld_safe --defaults-file=$default_file ");
  # I just assume it will work.
}
# create tables;

# kill 9, $server_pid;
system("sleep 1");
# my $show_tables = `mysql -u root -e "use mysql; show tables;"`;
# print "tables are $show_tables\n";

$hostname = `hostname`;
chomp($hostname);
$pid_file = "$db_dir/$hostname.pid";
$server_pid = `cat $pid_file`;
chomp($server_pid);

$schema_path = "/home/bily/mysql-5.6/trace_test/tpch_schema.sql";
my $table_res = `mysql -u root test \< $schema_path`;
if ($?) {
  print "table create fail\n $table_res";
  kill 9,$server_pid;
  exit;
}


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
    $sql = "LOAD DATA LOCAL INFILE '$file' into TABLE $_ FIELDS TERMINATED BY '|';";
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


$line_count = `mysql -u root -e "use test; select count(*) from lineitem;"`;

print "number of lines is $line_count";
# print $load_outpt;




print "pid file is $pid_file\n";
print "server id is $server_pid\n";
kill 9,$server_pid;
print "kill server $server_pid seccessfully\n";
exit;



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

print "pid file is $pid_file\n";
print "server id is $server_pid\n";
kill 9,$server_pid;
print "kill server $server_pid seccessfully\n";
exit;



$load_output = `mysql -u root -e "use test; LOAD DATA LOCAL INFILE '~/tpc-h/TPC-H_Tools_v3.0.0/dbgen/lineitem.tbl.u1' INTO TABLE LINEITEM FIELDS TERMINATED BY '|';"`;


print $load_output;

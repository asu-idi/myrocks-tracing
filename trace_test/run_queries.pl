#!/usr/bin/perl


sub start_server {
  $pid = fork;
  if($pid == 0) {
    $cnf = "/home/bily/mysql-5.6/trace_test/my.cnf";
    $start_res = `mysqld_safe --defaults-file=$cnf &`;
    print "$start_res\n";
  }

}

start_server();

# run all the queries
system("sleep 3");
$start_time = time();

$tbl_dir = "/home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen";
$sql_dir = "$tbl_dir/queries";

@sql_files = `ls $sql_dir`;

foreach(@sql_files) {
  chomp($_);

  print "sql file is $_";
  
  $query_res = `mysql -u root test < $sql_dir/$_`;
  print "query res is $query_res\n";
}

# @sql_files = `ls $tbl_dir`


$end_time = time();



$time_elapse = $end_time - $start_time;
print "time used for all query is $time_elapse";



sub shut_server {
  $server_pid = `cat /home/bily/mysql-5.6/test_data/WIN-6O1J2FL7BBQ.pid`;
  chomp($server_pid);
  kill 9, $server_pid;
}


shut_server();

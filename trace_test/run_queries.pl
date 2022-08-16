#!/usr/bin/perl


sub start_server {
  $pid = fork;
  if($pid == 0) {
    $cnf = "/home/bily/mysql-5.6/trace_test/my.cnf";
    $start_res = `mysqld_safe --defaults-file=$cnf &`;
    print "$start_res\n";
    exit;
  }


}

sub shut_server {
  $server_pid = `cat /home/bily/mysql-5.6/test_data/WIN-6O1J2FL7BBQ.pid`;
  chomp($server_pid);
  kill 9, $server_pid;

  print "server shut down \n";
}





# start_server();

$start_query = @ARGV[0];
$end_query = @ARGV[1];
# run all the queries
# system("sleep 5");
$start_time = time();

# $tbl_dir = "/home/bily/tpc-h/TPC-H_Tools_v3.0.0/dbgen";
# $sql_dir = "$tbl_dir/queries";

# @sql_files = `ls $sql_dir`;

# $qgen_exe = "$tbl_dir/qgen";


# print "start:$start_query, end:$end_query \n";


# for ($i=$start_query; $i <= $end_query ; $i++) {
#  $sql = `DSS_CONFIG=$tbl_dir DSS_QUERY=$tbl_dir/queries $qgen_exe -d -c $i`;
#  # print  "sql is \n $sql\n";
#  $cmd =  "mysql -u root -e \"use test; $sql;\"";
#  print "$cmd\n";
#  `$cmd`;
#  # `mysql -u root -e "use test; $sql"`;
#  # `$cmd`;

# }
# $all_sqls = `DSS_CONFIG=$tbl_dir DSS_QUERY=$tbl_dir/queries $qgen_exe -d -c 1`;
# print "$all_sqls\n";


# `mysql -u root test < $all_sqls`;

# print "gen sqls are \n $all_sqls";
# foreach(@sql_files) {
#   chomp($_);

#   print "sql file is $_";
#   
#   $query_res = `mysql -u root test < $sql_dir/$_`;
#   print "query res is $query_res\n";
# }

# @sql_files = `ls $tbl_dir`
$sql_queries_dir = "/home/bily/mysql-5.6/queries-tpch-dbgen-mysql" ;

@sql_files  =  `ls $sql_queries_dir/*.sql`;

foreach (@sql_files) {
  chomp($_);
  $cur_q_start = time();
  $q_res = `mysql -u root test < $_`;
  $cur_q_end = time();
  $cur_elapse = $cur_q_end - $cur_q_start;
  print "query result is $q_res\n";
  print "time for $_ is $cur_elapse";
}

$end_time = time();



$time_elapse = $end_time - $start_time;
print "time used for all query is $time_elapse s \n";


# shut_server();


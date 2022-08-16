#!/bin/perl


$cnf = "/home/bily/mysql-5.6/trace_test/my.cnf";


$pid = fork;

if($pid == 0) {
  exec("mysqld_safe --defaults-file=$cnf");
  exit;
}

#!/bin/perl


$host = `hostname`;
chomp($host);

$pid_file = "/home/bily/mysql-5.6/test_data/$host.pid";

$pid = `cat $pid_file`;
chomp($pid);


kill 9,$pid;



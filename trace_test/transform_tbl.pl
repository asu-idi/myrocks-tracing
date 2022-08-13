#!/usr/bin/perl




# $tbl_path = "~/tpc-h/TPC-H_Tools_v3.0.0/dbgen/lineitem.tbl.u1";
$tbl_path = "/tmp/data.txt";



$res = `awk -f copy_into.awk $tbl_path`;


print "res is $res";

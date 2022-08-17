#!/usr/bin/perl




#install db 
sub get_dir {
  use Cwd 'abs_path';
  use File::Basename;

  my $abs_path = abs_path(__FILE__);
  # print "abs path $abs_path\n";


  ($name, $path, $suffix) = fileparse($abs_path, @suffixlist);
  # print "$name \n $path \n ";
  return $path;


}

$dir_path = get_dir();

$default_file = "$dir_path/my.cnf";
$mysql_install_exe = "/usr/local/mysql/scripts/mysql_install_db";
$mysql_install_dir = "/usr/local/mysql";

$db_dir =  "$dir_path/test_data";

`rm -rf $db_dir`;

$cmd  ="$mysql_install_exe --basedir=$mysql_install_dir --defaults-file=$default_file";
print "cmd is $cmd\n";
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

# $pid = fork;
# if($pid == 0) {
#   exec("mysqld_safe --defaults-file=$default_file &");
#   exit;
#   # I just assume it will work.
# }

# create tables;

# kill 9, $server_pid;
# system("sleep 1");
# my $show_tables = `mysql -u root -e "use mysql; show tables;"`;
# print "tables are $show_tables\n";


# print "pid file is $pid_file\n";
# print "server id is $server_pid\n";
# kill 9,$server_pid;
# print "kill server $server_pid seccessfully\n";
# exit;



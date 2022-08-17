#!/bin/perl


use Cwd;


use File::Basename;


# my $dirname = dirname(__FILE__);
# print "dir name is $dirname\n";

# use File::Spec;
# $path = File::Spec->rel2abs(__FILE__);
# print "path is $path \n";

# my $dir = getcwd;
# print "dir is $dir\n" ;

sub get_dir {
  use Cwd 'abs_path';

  my $abs_path = abs_path(__FILE__);
  # print "abs path $abs_path\n";


  ($name, $path, $suffix) = fileparse($abs_path, @suffixlist);
  # print "$name \n $path \n ";
  return $path;


}

$path = get_dir();

$cnf = "$path/my.cnf";


$pid = fork;

if($pid == 0) {
  exec("mysqld_safe --defaults-file=$cnf");
  exit;
}

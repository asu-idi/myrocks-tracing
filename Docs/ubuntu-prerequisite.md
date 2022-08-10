


The packages you need to install on ubuntu20 to build 
your myrocks successfully.

1. 
Follow the wiki first 
https://github.com/facebook/mysql-5.6/wiki/Build-Steps

2. 
`sudo apt-get -y install libzstd1 libzstd-dev`

A little bit different than the command in the wiki


3.
`sudo apt install lz4`

`sudo apt install liblz4-dev`


4. ncurses packages miss
`sudo apt install libncurses5-dev`


5. libreadline miss
`sudo apt install libreadline-dev`


6.  cap_value_t  not exist
` sudo apt install libcap-dev`

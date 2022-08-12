


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
You will meet this compile error if you haven't install the libcap-dev
package.
And if you run cmake before this step, you will need to clean the cmake files and MakeFile to build the Makefile again so that you can compile successfully..

I doubt that  it's because some `#define XX` variables is controlled by 
cmake and is written into Makefile to control the compile.
` sudo apt install libcap-dev`



7. cmake

8. make -j16


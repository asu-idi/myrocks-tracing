
sysbench_dir="/home/bily/sysbench/sysbench/src/lua"


# pushd $sysbench_dir 


# lua_file=`ls  | grep 'oltp.*lua'`

# popd

./install.pl


./start_server.pl

pushd $sysbench_dir

sleep 3


# while read -r file; do 


# done <<< ""

sysbench ./oltp_read_write.lua --mysql-port=3306 \
  --mysql-user=root --mysql-db=test \
  --tables=8 --table_size=1000000 \
  --mysql_storage_engine=rocksdb \
  --mysql-socket=/tmp/mysql.sock \
  --threads=64 prepare


sysbench ./oltp_read_write.lua --mysql-port=3306 \
  --mysql-user=root --mysql-db=test \
  --tables=8 --table_size=1000000 \
  --mysql_storage_engine=rocksdb \
  --mysql-socket=/tmp/mysql.sock \
  --report-interval=3 \
  --time=600 \
  --threads=64 run
popd

./shut_server.pl


sysbench_dir="/home/bily/sysbench/sysbench/src/lua"

./install.pl


./start_server.pl

pushd $sysbench_dir

sleep 3


sysbench ./oltp_read_write.lua --mysql-port=3306 \
  --mysql-user=root --mysql-db=test \
  --tables=4 --table_size=1000000 \
  --mysql_storage_engine=rocksdb \
  --mysql-socket=/tmp/mysql.sock \
  --threads=64 prepare


sysbench ./oltp_read_write.lua --mysql-port=3306 \
  --mysql-user=root --mysql-db=test \
  --tables=4 --table_size=1000000 \
  --mysql_storage_engine=rocksdb \
  --mysql-socket=/tmp/mysql.sock \
  --report-interval=3 \
  --time=60 \
  --threads=64 run
popd

./shut_server.pl

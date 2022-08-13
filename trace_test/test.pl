




$load_output = `mysql -u root -e "use test; LOAD DATA LOCAL INFILE '~/tpc-h/TPC-H_Tools_v3.0.0/dbgen/lineitem.tbl.u1' INTO TABLE LINEITEM FIELDS TERMINATED BY '|';"`;


print $load_output;

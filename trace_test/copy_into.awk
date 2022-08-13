
BEGIN{
  regex="[^/]+.tbl.u1"
}
{
  print regex
  if(match($2, regex)) {
    table=substr($2, RSTART, RLENGTH-7);
    print "LOAD DATA INFILE " $2 " INTO TABLE " table  \
          " FIELDS SEPERATED BY '|' ;"
    # print "COPY " $1 " RECORDS INTO " table " from '"$2"' \
          # USING DELIMITERS '|', '|\\n'; "
    }
  }

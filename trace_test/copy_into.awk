
BEGIN{
  regex="[^/]+.tbl"
}
{
  print regex
  if(match($2, regex)) {
    table=substr($2, RSTART, RLENGTH-4);
    print "LOAD DATA INFILE " $2 " INTO TABLE " table  \
          " FIELDS SEPERATED BY '|' ;"
    # print "COPY " $1 " RECORDS INTO " table " from '"$2"' \
          # USING DELIMITERS '|', '|\\n'; "
    }
  }

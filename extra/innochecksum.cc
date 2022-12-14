/*
   Copyright (c) 2005, 2016, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*
  InnoDB offline file checksum utility.  85% of the code in this utility
  is included from the InnoDB codebase.

  The final 15% was originally written by Mark Smith of Danga
  Interactive, Inc. <junior@danga.com>

  Published with a permission.
*/

#include <my_config.h>
#include <my_global.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef __WIN__
# include <unistd.h>
#endif
#include <my_getopt.h>
#include <m_string.h>
#include <welcome_copyright_notice.h> /* ORACLE_WELCOME_COPYRIGHT_NOTICE */
#include <string.h>
#include <dirent.h>
#include <string>

/* Only parts of these files are included from the InnoDB codebase.
The parts not included are excluded by #ifndef UNIV_INNOCHECKSUM. */

#include "univ.i"                /*  include all of this */

#define FLST_BASE_NODE_SIZE (4 + 2 * FIL_ADDR_SIZE)
#define FLST_NODE_SIZE (2 * FIL_ADDR_SIZE)
#define FSEG_PAGE_DATA FIL_PAGE_DATA
#define MLOG_1BYTE (1)

#include "ut0ut.h"
#include "ut0byte.h"
#include "mach0data.h"
#include "fsp0types.h"
#include "rem0rec.h"
#include "buf0checksum.h"        /* buf_calc_page_*() */
#include "fil0fil.h"             /* FIL_* */
#include "page0page.h"           /* PAGE_* */
#include "page0zip.h"            /* page_zip_*() */
#include "trx0undo.h"            /* TRX_* */
#include "fsp0fsp.h"             /* fsp_flags_get_page_size() &
                                    fsp_flags_get_zip_size() */
#include "ut0crc32.h"            /* ut_crc32_init() */

#ifdef UNIV_NONINL
# include "fsp0fsp.ic"
# include "mach0data.ic"
# include "ut0rnd.ic"
#endif

#undef max
#undef min

#include <unordered_map>
#include <unordered_set>

typedef std::unordered_set<std::string> TOKEN_SET_TYPE;

/* Global variables */
static my_bool verbose;
static my_bool debug;
static my_bool skip_corrupt;
static my_bool just_count;
static ullint start_page;
static ullint end_page;
static ullint do_page;
static my_bool use_end_page;
static my_bool do_one_page;
static my_bool per_page_details;
static my_bool do_leaf;
static ulong n_merge;
ulong srv_page_size;              /* replaces declaration in srv0srv.c */
static ulong physical_page_size;  /* Page size in bytes on disk. */
static ulong logical_page_size;   /* Page size when uncompressed. */
static my_bool recursive;
static char* data_dir;
static char* databases;
static char* tables;
static ullint space;
static bool found_space;

int n_undo_state_active;
int n_undo_state_cached;
int n_undo_state_to_free;
int n_undo_state_to_purge;
int n_undo_state_prepared;
int n_undo_state_other;
int n_undo_insert, n_undo_update, n_undo_other;
int n_bad_checksum;
int n_fil_page_index;
int n_fil_page_undo_log;
int n_fil_page_inode;
int n_fil_page_ibuf_free_list;
int n_fil_page_allocated;
int n_fil_page_ibuf_bitmap;
int n_fil_page_type_sys;
int n_fil_page_type_trx_sys;
int n_fil_page_type_fsp_hdr;
int n_fil_page_type_allocated;
int n_fil_page_type_xdes;
int n_fil_page_type_blob;
int n_fil_page_type_zblob;
int n_fil_page_type_dblwr_header;
int n_fil_page_type_other;

int n_fil_page_max_index_id;

#define SIZE_RANGES_FOR_PAGE 10
#define NUM_RETRIES 3
#define DEFAULT_RETRY_DELAY 1000000

struct per_page_stats {
  ulint n_recs;
  ulint data_size;
  ulint left_page_no;
  ulint right_page_no;
  per_page_stats(ulint n, ulint data, ulint left, ulint right) :
      n_recs(n), data_size(data), left_page_no(left), right_page_no(right) {}
};

struct per_index_stats {
  unsigned long long pages;
  unsigned long long leaf_pages;
  ulint first_leaf_page;
  ulint count;
  ulint free_pages;
  ulint max_data_size;
  unsigned long long total_n_recs;
  unsigned long long total_data_bytes;

  /*!< first element for empty pages,
  last element for pages with more than logical_page_size */
  unsigned long long pages_in_size_range[SIZE_RANGES_FOR_PAGE+2];

  std::unordered_map<ulint, per_page_stats> leaves;

  per_index_stats():pages(0), leaf_pages(0), first_leaf_page(0),
                    count(0), free_pages(0), max_data_size(0), total_n_recs(0),
                    total_data_bytes(0)
  {
    memset(pages_in_size_range, 0, sizeof(pages_in_size_range));
  }
};

std::unordered_map<unsigned long long, per_index_stats> index_ids;

/* Get the page size of the filespace from the filespace header. */
static
my_bool
get_page_size(
/*==========*/
  FILE*  f,                     /*!< in: file pointer, must be open
                                         and set to start of file */
  ::byte* buf,                    /*!< in: buffer used to read the page */
  ulong* logical_page_size,     /*!< out: Logical/Uncompressed page size */
  ulong* physical_page_size,    /*!< out: Physical/Commpressed page size */
  bool* compressed,             /*!< out: whether the tablespace is
                                compressed */
  ulint *ptr_space_id)          /*!< out: space id */
{
  ulong flags;

  ulong bytes= ulong(fread(buf, 1, UNIV_PAGE_SIZE_MIN, f));

  if (ferror(f))
  {
    perror("Error reading file header");
    return FALSE;
  }

  if (bytes != UNIV_PAGE_SIZE_MIN)
  {
    fprintf(stderr, "Error; Was not able to read the minimum page size ");
    fprintf(stderr, "of %d bytes.  Bytes read was %lu\n", UNIV_PAGE_SIZE_MIN, bytes);
    return FALSE;
  }

  rewind(f);

  *ptr_space_id = mach_read_from_4(buf + FIL_PAGE_DATA + FSP_SPACE_ID);
  flags = mach_read_from_4(buf + FIL_PAGE_DATA + FSP_SPACE_FLAGS);

  /* srv_page_size is used by InnoDB code as UNIV_PAGE_SIZE */
  srv_page_size = *logical_page_size = fsp_flags_get_page_size(flags);

  /* fsp_flags_get_zip_size() will return zero if not compressed. */
  *physical_page_size = fsp_flags_get_zip_size(flags);
  *compressed = (*physical_page_size != 0);
  if (*physical_page_size == 0)
    *physical_page_size= *logical_page_size;

  return TRUE;
}

#ifdef __WIN__
/***********************************************//*
 @param		[in] error	error no. from the getLastError().

 @retval error message corresponding to error no.
*/
static
char*
win32_error_message(
	int	error)
{
	static char err_msg[1024] = {'\0'};
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)err_msg, sizeof(err_msg), NULL );

	return (err_msg);
}
#endif /* __WIN__ */

/***********************************************//*
 @param [in] name	name of file.
 @retval file pointer; file pointer is NULL when error occured.
*/

FILE*
open_file(
	const char*	name)
{
	int	fd;		/* file descriptor. */
	FILE*	fil_in;
#ifdef __WIN__
	HANDLE		hFile;		/* handle to open file. */
	DWORD		access;		/* define access control */
	int		flags;		/* define the mode for file
					descriptor */

	access = GENERIC_READ;
	flags = _O_RDONLY | _O_BINARY;
	hFile = CreateFile(
			(LPCTSTR) name, access, 0L, NULL,
			OPEN_EXISTING, NULL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		/* print the error message. */
		fprintf(stderr, "Filename::%s %s\n",
			win32_error_message(GetLastError()));

			return (NULL);
		}

	/* get the file descriptor. */
	fd= _open_osfhandle((intptr_t)hFile, flags);
#else /* __WIN__ */

	int	create_flag;
	create_flag = O_RDONLY;

	fd = open(name, create_flag);
	if ( -1 == fd) {
		perror("open");
		return (NULL);
	}

#endif /* __WIN__ */

	fil_in = fdopen(fd, "rb");

	return (fil_in);
}

/* command line argument to do page checks (that's it) */
/* another argument to specify page ranges... seek to right spot and go from there */

static struct my_option innochecksum_options[] =
{
  {"help", '?', "Displays this help and exits.",
    0, 0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"info", 'I', "Synonym for --help.",
    0, 0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"version", 'V', "Displays version information and exits.",
    0, 0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"verbose", 'v', "Verbose (prints progress every 5 seconds).",
    &verbose, &verbose, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"debug", 'd', "Debug mode (prints checksums for each page, implies verbose).",
    &debug, &debug, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"skip-corrupt", 'u', "Skip corrupt pages.",
    &skip_corrupt, &skip_corrupt, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"count", 'c', "Print the count of pages in the file.",
    &just_count, &just_count, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"start_page", 's', "Start on this page number (0 based).",
    &start_page, &start_page, 0, GET_ULL, REQUIRED_ARG,
    0, 0, ULONGLONG_MAX, 0, 1, 0},
  {"end_page", 'e', "End at this page number (0 based).",
    &end_page, &end_page, 0, GET_ULL, REQUIRED_ARG,
    0, 0, ULONGLONG_MAX, 0, 1, 0},
  {"page", 'p', "Check only this page (0 based).",
    &do_page, &do_page, 0, GET_ULL, REQUIRED_ARG,
    0, 0, ULONGLONG_MAX, 0, 1, 0},
  {"per_page_details", 'i', "Print out per-page detail information.",
    &per_page_details, &per_page_details, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0}
    ,
  {"leaf", 'l', "Examine leaf index pages",
    &do_leaf, &do_leaf, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"merge", 'm', "leaf page count if merge given number of consecutive pages",
   &n_merge, &n_merge, 0, GET_ULONG, REQUIRED_ARG,
   0, 0, (longlong)10L, 0, 1, 0},
  {"recursive", 'r', "if specified, innochecksum will check all databases in "
    "the directory specified by -D (--data-dir). "
    "Must be used together with -D.",
    &recursive, &recursive, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},
  {"data-dir", 'D', "Specify the data directory to search for tables.",
    &data_dir, &data_dir, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},
  {"databases", 0, "Check all tables in the specified databases "
    "(comma separated). Must be used together with -D.",
    &databases, &databases, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},
  {"tables", 0, "Check the specfied tables (comma separated). "
    "Must be used together with -D and --databases.",
    &tables, &tables, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},
  {"space", 'S', "Check the table of this space id. "
    "Must be used together with -D and either -r or --databases",
    &space, &space, 0, GET_ULL, REQUIRED_ARG, 0, 0, ULONGLONG_MAX, 0, 1, 0},
  {0, 0, 0, 0, 0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0}
};

static void print_version(void)
{
  printf("%s Ver %s, for %s (%s)\n",
         my_progname, INNODB_VERSION_STR,
         SYSTEM_TYPE, MACHINE_TYPE);
}

static void usage(void)
{
  print_version();
  puts(ORACLE_WELCOME_COPYRIGHT_NOTICE("2000"));
  printf("InnoDB offline file checksum utility.\n");
  printf("Usage: %s [-c] [-s <start page>] [-e <end page>] [-p <page>] [-v] "
         "[-d] [-r] [-D <data directory>] [--databases=<db1,db2>] "
         "[--tables=<tb1,tb2>] [-S <space id>] <filename>\n", my_progname);
  my_print_help(innochecksum_options);
  my_print_variables(innochecksum_options);
}

extern "C" my_bool
innochecksum_get_one_option(
/*========================*/
  int optid,
  const struct my_option *opt MY_ATTRIBUTE((unused)),
  char *argument MY_ATTRIBUTE((unused)))
{
  switch (optid) {
  case 'd':
    verbose=1;	/* debug implies verbose... */
    break;
  case 'e':
    use_end_page= 1;
    break;
  case 'p':
    end_page= start_page= do_page;
    use_end_page= 1;
    do_one_page= 1;
    break;
  case 'V':
    print_version();
    exit(0);
    break;
  case 'I':
  case '?':
    usage();
    exit(0);
    break;
  }
  return 0;
}

static int get_options(
/*===================*/
  int *argc,
  char ***argv)
{
  int ho_error;

  if ((ho_error=handle_options(argc, argv, innochecksum_options, innochecksum_get_one_option)))
    exit(ho_error);

  /* If data_dir is not specified, the next arg must be the filename */
  if (!*argc && (!data_dir || !*data_dir))
  {
    fprintf(stderr, "Error: data directory or file name missing\n");
    usage();
    return 1;
  }
  return 0;
} /* get_options */

/* Parse database names and table names separated by comma */
static void parse_name_tokens(TOKEN_SET_TYPE &hs, char *cstr)
{
  char *token = strtok(cstr, ",");
  while (token)
  {
    hs.insert(token);
    token = strtok(NULL, ",");
  }
}

/*********************************************************************//**
Gets the file page type.
@return type; NOTE that if the type has not been written to page, the
return value not defined */
ulint
fil_page_get_type(
/*==============*/
       uchar*  page)   /*!< in: file page */
{
       return(mach_read_from_2(page + FIL_PAGE_TYPE));
}

/**************************************************************//**
Gets the index id field of a page.
@return        index id */
ib_uint64_t
btr_page_get_index_id(
/*==================*/
       uchar*  page)   /*!< in: index page */
{
       return(mach_read_from_8(page + PAGE_HEADER + PAGE_INDEX_ID));
}

/********************************************************//**
Gets the next index page number.
@return	next page number */
ulint
btr_page_get_next(
/*==============*/
  const page_t* page) /*!< in: index page */
{
  return(mach_read_from_4(page + FIL_PAGE_NEXT));
}

/********************************************************//**
Gets the previous index page number.
@return	prev page number */
ulint
btr_page_get_prev(
/*==============*/
  const page_t* page) /*!< in: index page */
{
  return(mach_read_from_4(page + FIL_PAGE_PREV));
}

void
parse_page(
/*=======*/
  uchar* page, /* in: buffer page */
  uchar* xdes) /* in: extend descriptor page */
{
       ib_uint64_t id;
       ulint x;
       ulint n_recs;
       ulint page_no;
       ulint left_page_no;
       ulint right_page_no;
       ulint data_bytes;
       int is_leaf;
       int size_range_id;

       switch (fil_page_get_type(page)) {
       case FIL_PAGE_INDEX:
               n_fil_page_index++;
               id = btr_page_get_index_id(page);
               n_recs = page_get_n_recs(page);
               page_no = page_get_page_no(page);
               left_page_no = btr_page_get_prev(page);
               right_page_no = btr_page_get_next(page);
               data_bytes = page_get_data_size(page);
               is_leaf = page_is_leaf(page);
               size_range_id = (data_bytes * SIZE_RANGES_FOR_PAGE
                                + logical_page_size - 1) /
                                logical_page_size;
               if (size_range_id > SIZE_RANGES_FOR_PAGE + 1) {
                 /* data_bytes is bigger than logical_page_size */
                 size_range_id = SIZE_RANGES_FOR_PAGE + 1;
               }
               if (per_page_details) {
                 printf("index %lu page %lu leaf %u n_recs %lu data_bytes %lu"
                         "\n", id, page_no, is_leaf, n_recs, data_bytes);
               }
               /* update per-index statistics */
               {
                 if (index_ids.count(id) == 0) {
                   index_ids.insert(std::make_pair(id, per_index_stats()));
                 }
                 per_index_stats &index = index_ids.find(id)->second;
                 uchar* des = xdes + XDES_ARR_OFFSET
                   + XDES_SIZE * ((page_no & (physical_page_size - 1))
                                  / FSP_EXTENT_SIZE);
                 if (xdes_get_bit(des, XDES_FREE_BIT,
                                  page_no % FSP_EXTENT_SIZE)) {
                   index.free_pages++;
                   return;
                 }
                 index.pages++;
                 if (is_leaf) {
                   index.leaf_pages++;
                   if (data_bytes > index.max_data_size) {
                     index.max_data_size = data_bytes;
                   }
                   index.leaves.insert(
                     std::make_pair(page_no,
                                    per_page_stats(n_recs, data_bytes,
                                                   left_page_no,
                                                   right_page_no)));
                   if (left_page_no == ULINT32_UNDEFINED) {
                     index.first_leaf_page = page_no;
                     index.count++;
                   }
                 }
                 index.total_n_recs += n_recs;
                 index.total_data_bytes += data_bytes;
                 index.pages_in_size_range[size_range_id] ++;
               }

               break;
       case FIL_PAGE_UNDO_LOG:
               if (per_page_details) {
                       printf("FIL_PAGE_UNDO_LOG\n");
               }
               n_fil_page_undo_log++;
               x = mach_read_from_2(page + TRX_UNDO_PAGE_HDR +
                                    TRX_UNDO_PAGE_TYPE);
               if (x == TRX_UNDO_INSERT)
                       n_undo_insert++;
               else if (x == TRX_UNDO_UPDATE)
                       n_undo_update++;
               else
                       n_undo_other++;

               x = mach_read_from_2(page + TRX_UNDO_SEG_HDR + TRX_UNDO_STATE);
               switch (x) {
                       case TRX_UNDO_ACTIVE: n_undo_state_active++; break;
                       case TRX_UNDO_CACHED: n_undo_state_cached++; break;
                       case TRX_UNDO_TO_FREE: n_undo_state_to_free++; break;
                       case TRX_UNDO_TO_PURGE: n_undo_state_to_purge++; break;
                       case TRX_UNDO_PREPARED: n_undo_state_prepared++; break;
                       default: n_undo_state_other++; break;
               }
               break;
       case FIL_PAGE_INODE:
               if (per_page_details) {
                       printf("FIL_PAGE_INODE\n");
               }
               n_fil_page_inode++;
               break;
       case FIL_PAGE_IBUF_FREE_LIST:
               if (per_page_details) {
                       printf("FIL_PAGE_IBUF_FREE_LIST\n");
               }
               n_fil_page_ibuf_free_list++;
               break;
       case FIL_PAGE_TYPE_ALLOCATED:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_ALLOCATED\n");
               }
               n_fil_page_type_allocated++;
               break;
       case FIL_PAGE_IBUF_BITMAP:
               if (per_page_details) {
                       printf("FIL_PAGE_IBUF_BITMAP\n");
               }
               n_fil_page_ibuf_bitmap++;
               break;
       case FIL_PAGE_TYPE_SYS:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_SYS\n");
               }
               n_fil_page_type_sys++;
               break;
       case FIL_PAGE_TYPE_TRX_SYS:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_TRX_SYS\n");
               }
               n_fil_page_type_trx_sys++;
               break;
       case FIL_PAGE_TYPE_FSP_HDR:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_FSP_HDR\n");
               }
               memcpy(xdes, page, physical_page_size);
               n_fil_page_type_fsp_hdr++;
               break;
       case FIL_PAGE_TYPE_XDES:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_XDES\n");
               }
               memcpy(xdes, page, physical_page_size);
               n_fil_page_type_xdes++;
               break;
       case FIL_PAGE_TYPE_BLOB:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_BLOB\n");
               }
               n_fil_page_type_blob++;
               break;
       case FIL_PAGE_TYPE_ZBLOB:
       case FIL_PAGE_TYPE_ZBLOB2:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_ZBLOB/2\n");
               }
               n_fil_page_type_zblob++;
               break;
       case FIL_PAGE_TYPE_DBLWR_HEADER:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_DBLWR_HEADER\n");
               }
               n_fil_page_type_dblwr_header++;
               break;
       default:
               if (per_page_details) {
                       printf("FIL_PAGE_TYPE_OTHER\n");
               }
               n_fil_page_type_other++;
       }
}

void print_index_leaf_stats(unsigned long long id, const per_index_stats& index)
{
  ulint page_no = index.first_leaf_page;
  auto it_page = index.leaves.find(page_no);
  printf("\nindex: %llu leaf page stats: n_pages = %llu\n",
         id, index.leaf_pages);
  printf("page_no\tdata_size\tn_recs\n");
  while (it_page != index.leaves.end()) {
    const per_page_stats& stat = it_page->second;
    printf("%lu\t%lu\t%lu\n", it_page->first, stat.data_size, stat.n_recs);
    page_no = stat.right_page_no;
    it_page = index.leaves.find(page_no);
  }
}

void defrag_analysis(unsigned long long id, const per_index_stats& index)
{
  // TODO: make it work for compressed pages too
  auto it = index.leaves.find(index.first_leaf_page);
  ulint n_pages = 0;
  ulint n_leaf_pages = 0;
  while (it != index.leaves.end()) {
    ulint data_size_total = 0;
    for (ulong i = 0; i < n_merge; i++) {
      const per_page_stats& stat = it->second;
      n_leaf_pages ++;
      data_size_total += stat.data_size;
      it = index.leaves.find(stat.right_page_no);
      if (it == index.leaves.end()) {
        break;
      }
    }
    if (index.max_data_size) {
      n_pages += data_size_total / index.max_data_size;
      if (data_size_total % index.max_data_size != 0) {
        n_pages += 1;
      }
    }
  }
  if (index.leaf_pages) {
    printf("count = %lu free = %lu\n", index.count, index.free_pages);
    printf("%llu\t\t%llu\t\t%lu\t\t%lu\t\t%lu\t\t%.2f\t%lu\n",
           id, index.leaf_pages, n_leaf_pages, n_merge, n_pages,
           1.0 - (double)n_pages / (double)n_leaf_pages, index.max_data_size);
  }
}

void print_leaf_stats()
{
  printf("\n**************************************************\n");
  printf("index_id\t#leaf_pages\t#actual_leaf_pages\tn_merge\t"
         "#leaf_after_merge\tdefrag\n");
  for (auto it = index_ids.begin(); it != index_ids.end(); it++) {
    const per_index_stats& index = it->second;
    if (verbose) {
      print_index_leaf_stats(it->first, index);
    }
    if (n_merge) {
      defrag_analysis(it->first, index);
    }
  }
}

void
print_stats()
/*========*/
{
       unsigned long long i;

       printf("%d\tbad checksum\n", n_bad_checksum);
       printf("%d\tFIL_PAGE_INDEX\n", n_fil_page_index);
       printf("%d\tFIL_PAGE_UNDO_LOG\n", n_fil_page_undo_log);
       printf("%d\tFIL_PAGE_INODE\n", n_fil_page_inode);
       printf("%d\tFIL_PAGE_IBUF_FREE_LIST\n", n_fil_page_ibuf_free_list);
       printf("%d\tFIL_PAGE_TYPE_ALLOCATED\n", n_fil_page_type_allocated);
       printf("%d\tFIL_PAGE_IBUF_BITMAP\n", n_fil_page_ibuf_bitmap);
       printf("%d\tFIL_PAGE_TYPE_SYS\n", n_fil_page_type_sys);
       printf("%d\tFIL_PAGE_TYPE_TRX_SYS\n", n_fil_page_type_trx_sys);
       printf("%d\tFIL_PAGE_TYPE_FSP_HDR\n", n_fil_page_type_fsp_hdr);
       printf("%d\tFIL_PAGE_TYPE_XDES\n", n_fil_page_type_xdes);
       printf("%d\tFIL_PAGE_TYPE_BLOB\n", n_fil_page_type_blob);
       printf("%d\tFIL_PAGE_TYPE_ZBLOB\n", n_fil_page_type_zblob);
       printf("%d\tFIL_PAGE_TYPE_DBLWR_HEADER\n",
              n_fil_page_type_dblwr_header);
       printf("%d\tother\n", n_fil_page_type_other);
       printf("%d\tmax index_id\n", n_fil_page_max_index_id);
       printf("undo type: %d insert, %d update, %d other\n",
               n_undo_insert, n_undo_update, n_undo_other);
       printf("undo state: %d active, %d cached, %d to_free, %d to_purge,"
               " %d prepared, %d other\n", n_undo_state_active,
               n_undo_state_cached, n_undo_state_to_free,
               n_undo_state_to_purge, n_undo_state_prepared,
               n_undo_state_other);

       printf("index_id\t#pages\t\t#leaf_pages\t#recs_per_page"
               "\t#bytes_per_page\n");
       for (auto it = index_ids.begin(); it != index_ids.end(); it++) {
         const per_index_stats& index = it->second;
         printf("%lld\t\t%lld\t\t%lld\t\t%lld\t\t%lld\n",
                it->first, index.pages, index.leaf_pages,
                index.total_n_recs / index.pages,
                index.total_data_bytes / index.pages);
       }
       printf("\n");
       printf("index_id\tpage_data_bytes_histgram(empty,...,oversized)\n");
       for (auto it = index_ids.begin(); it != index_ids.end(); it++) {
         printf("%lld\t", it->first);
         const per_index_stats& index = it->second;
         for (i = 0; i < SIZE_RANGES_FOR_PAGE+2; i++) {
           printf("\t%lld", index.pages_in_size_range[i]);
         }
         printf("\n");
       }
       if (do_leaf) {
         print_leaf_stats();
       }
}

/* Opens the file and return the file handler.
 * If psize is provided, it also get and save the file size in psize.
 */
FILE * check_file_open(const char *filename, size_t *psize)
{
  FILE* f = NULL;                      /* our input file */

  /* stat, to get file size. */
#ifdef __WIN__
  struct _stat64 st;
#else
  struct stat st;
#endif

  /* The file name is not optional */
  if (*filename == '\0')
  {
    fprintf(stderr, "Error; File name missing\n");
    return 0;
  }

  /* if psize is provide, stat the file to get the file size */
  if (psize)
  {
    /* stat the file to get size */
#ifdef __WIN__
    if (_stat64(filename, &st))
#else
    if (stat(filename, &st))
#endif
    {
      fprintf(stderr, "Error; %s cannot be found\n", filename);
      return 0;
    }
    *psize= st.st_size;
  }

  /* Open the file for reading */
  f= open_file(filename);
  if (f == NULL) {
    return 0;
  }

  return f;
}

/* Checksum one data file */
int check_file(FILE *f, unsigned long long size,
               const char *filename,  /* for verbose info */
               const char *tablename) /* for print only */
{
  unsigned char big_buf[UNIV_PAGE_SIZE_MAX*2]; /* Buffer to store pages read */
  unsigned char *buf = (unsigned char*)ut_align_down(big_buf
                       + UNIV_PAGE_SIZE_MAX, UNIV_PAGE_SIZE_MAX);
  unsigned char big_xdes[UNIV_PAGE_SIZE_MAX*2];
  unsigned char *xdes = (unsigned char*)ut_align_down(big_xdes
                       + UNIV_PAGE_SIZE_MAX, UNIV_PAGE_SIZE_MAX);
                                 /* Make sure the page is aligned */
  ulong bytes;                   /* bytes read count */
  ulint ct;                      /* current page number (0 based) */
  time_t now;                    /* current time */
  time_t lastt;                  /* last time */
  ulint oldcsum, oldcsumfield, csum, csumfield, logseq, logseqfield;
  dual_crc crc32s;               /* struct for the two forms of crc32c */

                                 /* ulints for checksum storage */
  ulint pages;                   /* number of pages in file */
  off_t offset= 0;
  bool compressed;

  ulint space_id = 0;
  if (!get_page_size(f, buf, &logical_page_size, &physical_page_size,
                     &compressed, &space_id))
  {
    return 1;
  }

  /* filter by space id */
  found_space = (space && space == space_id);
  if (space && space != space_id)
    return 0;

  if (tablename)
    printf("Checking table '%s':\n", tablename);

  if (compressed)
  {
    printf("Table is compressed\n");
    printf("Key block size is %lu\n", physical_page_size);
  }
  else
  {
    printf("Table is uncompressed\n");
    printf("Page size is %lu\n", physical_page_size);
  }

  if (space_id) /* not all files has space id */
    printf("Table space id is %lu\n", space_id);

  pages= (ulint) (size / physical_page_size);

  if (just_count)
  {
    if (verbose)
      printf("Number of pages: ");
    printf("%lu\n", pages);
    return 0;
  }
  else if (verbose)
  {
    printf("file %s = %llu bytes (%lu pages)...\n", filename, size, pages);
    if (do_one_page)
      printf("InnoChecksum; checking page %llu\n", do_page);
    else
      printf("InnoChecksum; checking pages in range %llu to %llu\n", start_page, use_end_page ? end_page : (pages - 1));
  }

#ifdef UNIV_LINUX
  if (posix_fadvise(fileno(f), 0, 0, POSIX_FADV_SEQUENTIAL) ||
      posix_fadvise(fileno(f), 0, 0, POSIX_FADV_NOREUSE))
  {
    perror("posix_fadvise failed");
  }
#endif

  /* seek to the necessary position */
  if (start_page)
  {

    offset= (off_t)start_page * (off_t)physical_page_size;

#ifdef __WIN__
	if (_fseeki64(f, offset, SEEK_SET)) {
#else
	if (fseeko(f, offset, SEEK_SET)) {
#endif /* __WIN__ */
	perror("Error; Unable to seek to necessary offset");
	return 1;
    }
  }

  /* main checksumming loop */
  ct= start_page;
  lastt= 0;
  while (!feof(f))
  {
    int page_ok = 1;

    bytes= ulong(fread(buf, 1, physical_page_size, f));
    if (!bytes && feof(f))
    {
      print_stats();
      return 0;
    }

    if (ferror(f))
    {
      fprintf(stderr, "Error reading %lu bytes", physical_page_size);
      perror(" ");
      return 1;
    }
    if (bytes != physical_page_size)
    {
      fprintf(stderr, "Error; bytes read (%lu) doesn't match page size (%lu)\n", bytes, physical_page_size);
      print_stats();
      return 1;
    }

    if (compressed) {
      /* compressed pages */
      if (!page_zip_verify_checksum(buf, physical_page_size)) {
        fprintf(stderr, "Fail; page %lu invalid (fails compressed page checksum).\n", ct);
        if (!skip_corrupt)
          return 1;
        page_ok = 0;
      }
    } else if (FIL_PAGE_TYPE_DBLWR_HEADER != fil_page_get_type(buf)) {
      /* check the "stored log sequence numbers" */
      logseq= mach_read_from_4(buf + FIL_PAGE_LSN + 4);
      logseqfield= mach_read_from_4(buf + logical_page_size - FIL_PAGE_END_LSN_OLD_CHKSUM + 4);
      if (debug)
        printf("page %lu: log sequence number: first = %lu; second = %lu\n", ct, logseq, logseqfield);
      if (logseq != logseqfield)
      {
        fprintf(stderr, "Fail; page %lu invalid (fails log sequence number check)\n", ct);
        if (!skip_corrupt) return 1;
        page_ok = 0;
      }
      /* check old method of checksumming */
      oldcsum= buf_calc_page_old_checksum(buf);
      oldcsumfield= mach_read_from_4(buf + logical_page_size - FIL_PAGE_END_LSN_OLD_CHKSUM);
      crc32s= buf_calc_page_crc32fb(buf);
      if (debug)
        printf("page %lu: old style: calculated = %lu; recorded = %lu\n", ct, oldcsum, oldcsumfield);
      if (oldcsumfield != mach_read_from_4(buf + FIL_PAGE_LSN)
          && oldcsumfield != oldcsum && crc32s.crc32c != oldcsumfield
          && crc32s.crc32cfb != oldcsumfield)
      {
        fprintf(stderr, "Fail;  page %lu invalid (fails old style checksum)\n", ct);
        if (!skip_corrupt) return 1;
        page_ok = 0;
      }
      /* now check the new method */
      csum= buf_calc_page_new_checksum(buf);
      csumfield= mach_read_from_4(buf + FIL_PAGE_SPACE_OR_CHKSUM);
      if (debug)
        printf("page %lu: new style: calculated = %lu; crc32c = %u; crc32cfb = %u; recorded = %lu\n",
               ct, csum, crc32s.crc32c, crc32s.crc32cfb, csumfield);
      if (csumfield != 0 && crc32s.crc32c != csumfield && crc32s.crc32cfb != csumfield && csum != csumfield)
      {
        fprintf(stderr, "Fail; page %lu invalid (fails innodb and crc32 checksum)\n", ct);
        if (!skip_corrupt) return 1;
        page_ok = 0;
      }
    }

    /* end if this was the last page we were supposed to check */
    if (use_end_page && (ct >= end_page))
    {
      print_stats();
      return 0;
    }

    if (per_page_details)
    {
      printf("page %ld ", ct);
    }

    ct++;

    if (!page_ok)
    {
      if (per_page_details)
      {
        printf("BAD_CHECKSUM\n");
      }
      n_bad_checksum++;
      continue;
    }

    parse_page(buf, xdes);

    /* progress printing */
    if (verbose)
    {
      if (ct % 10000 == 0)
      {
        now= time(0);
        if (!lastt) lastt= now;
        if (now - lastt >= 1)
        {
          fprintf(stderr, "page %lu okay: %.3f%% done\n", (ct - 1), (float) ct / pages * 100);
          lastt= now;
        }
      }
    }
  }
  print_stats();

  return 0;
}

/* Recursively check the data files in the directory.
 * If filter is provided, only check the tables in the filter
 *
 * The function will return 1 (error) only if any table fails the checksum,
 * i.e. if check_file fails
 */
int check_dir(string &database, TOKEN_SET_TYPE *filter)
{
  DIR *p_dir;
  struct dirent *p_dirent;

  string db_dir = data_dir;
  db_dir += FN_LIBCHAR;
  db_dir += database;
  p_dir = opendir(db_dir.c_str());
  if (!p_dir)
  {
    fprintf(stderr, "Error: cannot open directory '%s'\n", db_dir.c_str());
    return 0;
  }

  string fn, tablespace, tablename;
  while ((p_dirent = readdir(p_dir)))
  {
    if (strcmp(p_dirent->d_name, ".") == 0 ||
        strcmp(p_dirent->d_name, "..") == 0)
      continue;

    // get physical file full name
    fn = p_dirent->d_name;
    std::size_t pos = fn.find_last_of(".");
    // get tablespace (file) name without the extension (e.g. .ibd)
    tablespace = fn.substr(0, pos);
    // get the table name. In case of partition tables, the table name is
    // appended by "#P#p[0-9]"
    tablename = tablespace.substr(0, tablespace.find_first_of("#"));
    /* check if the data file is ibd type and if it is in filter */
    if (fn.substr(pos + 1) != "ibd" ||
        (filter && filter->find(tablename) == filter->end()))
        continue;

    /* full file path */
    fn = db_dir + FN_LIBCHAR + fn;
    struct stat statinfo;
    if (stat(fn.c_str(), &statinfo))
    {
      // If the file cannot be read, skip it
      fprintf(stderr, "Error; %s cannot be found\n", fn.c_str());
      continue;
    }
    if (S_ISREG(statinfo.st_mode))
    {
      // get "database/table" string for printing purpose
      tablespace = database + FN_LIBCHAR + tablespace;
      FILE *f = check_file_open(fn.c_str(), NULL);
      if (f && check_file(f, statinfo.st_size, fn.c_str(), tablespace.c_str()))
      {
        fclose(f);
        closedir(p_dir);
        return 1;
      }
      else if (f)
      {
        fclose(f);
        // if we found the space id, stop
        if (found_space)
          break;
      }
    }
  }

  closedir(p_dir);
  return 0;
}

int check_recursive()
{
  DIR *p_dir;
  struct dirent *p_dirent;

  TOKEN_SET_TYPE hs_dbs;
  TOKEN_SET_TYPE hs_tbs;

  // Parse --databases param
  if (databases && *databases)
  {
    parse_name_tokens(hs_dbs, databases);
    if (!hs_dbs.empty() && recursive)
    {
      fprintf(stderr, "Warning: databases and recursive both are specified. "
              "Recursive will be ignored\n");
      recursive = 0;
    }
  }

  if (hs_dbs.empty() && !recursive)
  {
    fprintf(stderr, "Error: either '--databases' or '--recursive' must be "
            "specified");
    return 1;
  }

  // Parse --tables param
  if (tables && *tables)
    parse_name_tokens(hs_tbs, tables);

  if (!hs_dbs.empty() && !hs_tbs.empty())
  {
    // Both --databases and --tables are not empty
    // check all the combanation of databases and tables
    for (auto db: hs_dbs)
    {
      for (auto tbl: hs_tbs)
      {
        string tn = db + FN_LIBCHAR + tbl;
        string fn = data_dir;
        fn += FN_LIBCHAR;
        fn += tn + ".ibd";
        size_t size;
        FILE *f = check_file_open(fn.c_str(), &size);
        // If the DB+Table leads to a valid file, check it
        if (f && check_file(f, size, fn.c_str(), tn.c_str()))
        {
          fclose(f);
          return 1;
        }
        else if (f)
        {
          fclose(f);
          // if we found the space id, stop
          if (found_space)
            return 0;
        }
      }
    }
  }
  else if (!hs_dbs.empty())
  {
    // Only --databases is specified
    // check all tables in the specified databases
    for (auto db: hs_dbs)
    {
      if (check_dir(db, NULL))
        return 1;
      else if (found_space)
        return 0;
    }
  }
  else
  {
    // either --databases is empty or both --databases and --tables are empty
    fprintf(stderr, "Warning: innochecksum is going to check every database "
           "on the server. This may take a while ... \n");

    p_dir = opendir(data_dir);
    if (!p_dir)
    {
      fprintf(stderr, "Error: cannot open data directory '%s'\n", data_dir);
      return 1;
    }

    string db, dn;
    struct stat statinfo;
    int ret = 0;
    while ((p_dirent = readdir(p_dir)))
    {
      if (strcmp(p_dirent->d_name, ".") == 0 ||
          strcmp(p_dirent->d_name, "..") == 0)
        continue;

      dn = data_dir;
      dn += FN_LIBCHAR;
      dn += p_dirent->d_name;
      if (stat(dn.c_str(), &statinfo))
      {
        // If we cannot read into the file/directory, skip it
        fprintf(stderr, "Error; %s cannot be found\n", dn.c_str());
        continue;
      }
      if (S_ISDIR(statinfo.st_mode))
      {
        db = p_dirent->d_name;
        if (check_dir(db, hs_tbs.empty()? NULL : &hs_tbs))
        {
          ret = 1;
          break;
        }
        else if (found_space)
        {
          // if we found the space id, stop
          ret = 0;
          break;
        }
      }
    }
    closedir(p_dir);
    return ret;
  }

  return 0;
}

int main(int argc, char **argv)
{
  printf("InnoDB offline file checksum utility.\n");

  ut_crc32_init();

  MY_INIT(argv[0]);

  if (get_options(&argc,&argv))
    return 1;

  if (verbose)
    my_print_variables(innochecksum_options);

  // check single file
  if (*argv)
  {
    if (data_dir)
      fprintf(stderr, "Warning: file name and data-dir both are provided. "
              "data-dir will be ignored.\n");
    /* size of file (has to be 64 bits) */
    size_t size;
    FILE *f = check_file_open(*argv, &size);
    if (f && check_file(f, size, *argv, NULL))
    {
      fclose(f);
      return 1;
    }
    else if (f)
    {
      fclose(f);
      return 0;
    }
    return 1;
  }

  // recursively check data-dir
  if (check_recursive())
    return 1;

  return 0;
}

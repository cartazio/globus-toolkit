/*
 * myproxy_creds.c
 *
 * Routines for storing and retrieving credentials.
 *
 * See myproxy_creds.h for documentation.
 */

#include "myproxy_creds.h"
#include "my_utility.h"
#include "myproxy_server.h"

#include "verror.h"
#include "string_funcs.h"

#include "sslutil.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

struct myproxy_database mydbase; //database structure

extern char *mydsn, *myuid, *mypwd;
   char *dbase_name; //database name

/*
 * Doesn't always seem to be define in <unistd.h>
 */
char *crypt(const char *key, const char *salt);

/* Files should only be readable by me */
#define FILE_MODE               0600

/* Where to store our files */
#ifndef CREDS_STORAGE_DIRECTORY
#define CREDS_STORAGE_DIRECTORY         MYPROXY_SERVER_DIR "/store/"
#endif /* CREDS_STORAGE_DIRECTORY */

/**********************************************************************
 *
 * Internal variables
 *
 */

char *storage_dir = CREDS_STORAGE_DIRECTORY;

/**********************************************************************
 *
 * Internal functions
 *
 */

/*
 * mystrdup()
 *
 * Wrapper around strdup()
 */
static char *
mystrdup(const char *string)
{
    char *dup = NULL;
    
    assert(string != NULL);
    
    dup = strdup(string);
    
    if (dup == NULL)
    {
        verror_put_errno(errno);
        //verror_put_string("strdup() failed");
    }
    
    return dup;
}


/*
 * file_exists()
 *
 * Check for existance of a file.
 *
 * Returns 1 if exists, 0 if not, -1 on error.
 */
static int
file_exists(const char *path)
{
    struct stat					statbuf;
    int						return_value = -1;

    if (path == NULL)
    {
	verror_put_errno(EINVAL);
	return -1;
    }
    
    if (stat(path, &statbuf) == -1)
    {
	switch (errno)
	{
	  case ENOENT:
	  case ENOTDIR:
	    /* File does not exist */
	    return_value = 0;
	    break;
	    
	  default:
	    /* Some error */
	    return_value = -1;
	    break;
	}
    }
    else
    {
	/* File exists */
	return_value = 1;
    }
    
    return return_value;
}


/********************************************************
* initialize tables                                     *
*********************************************************/
int my_init_table(SQLHDBC hdbc, SQLHSTMT hstmt)
{
  SQLRETURN   rc;

  printf("\nmy_init_table:\n");

#ifdef CREATE_DATABASE
  printf ("\ncreating database:\n");
  rc = SQLPrepare (hstmt, "create database if not exists ?", SQL_NTS);
  mystmt (hstmt, rc);

  rc = SQLBindParameter (hstmt, 1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, "mydsn", strlen ("mydsn") , NULL);
  mystmt (hstmt, rc);

   rc = SQLExecute (hstmt);
  mycon(hdbc,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
  mycon(hdbc,rc);

  /* use database*/
  rc = SQLPrepare (hstmt, "USE DATABASE ?", SQL_NTS);
  mystmt (hstmt, rc);

  rc = SQLBindParameter (hstmt, 1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, dbase_name, strlen (dbase_name) , NULL);
  mystmt (hstmt, rc);

   rc = SQLExecute (hstmt);
  mycon(hdbc,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
  mycon(hdbc,rc);
#endif

    /* create the table 'main' */
  printf(" creating table 'main'\n");

    rc = SQLExecDirect(hstmt,"CREATE TABLE IF NOT EXISTS main(\
                                owner VARCHAR(255) ,\
                                passphrase VARCHAR(255),\
				username VARCHAR(255) NOT NULL, \
                                lifetime INT (10) UNSIGNED, \
                                retrievers VARCHAR(255), \
                                renewers VARCHAR(255), \
                                cred_name VARCHAR(200) NOT NULL, \
                                cred_desc TEXT, \
                                credentials TEXT, \
				PRIMARY KEY (username, cred_name))", SQL_NTS);
    mystmt(hstmt,rc);

    /* commit the transaction*/
    rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
    mycon(hdbc,rc);

   return 0;
}

/* Retrieve data		                          *
*********************************************************/
int my_retrieve(SQLHDBC hdbc, SQLHSTMT hstmt)
{
  SQLRETURN   rc;
  SQLINTEGER  id, lifetime;
  SQLCHAR owner[MAX_VARCHAR_LEN];
  SQLCHAR  passphrase[MAX_VARCHAR_LEN], retrievers[MAX_VARCHAR_LEN], renewers[MAX_VARCHAR_LEN];
  SQLCHAR credname[CRED_NAME_LEN], cred_desc[MAX_TEXT_LEN];

   printf ("mpi-0");
  printf("\nmy_retrieve:\n");

   rc = SQLBindCol(hstmt,1, SQL_C_CHAR, owner,  
                      MAX_VARCHAR_LEN,NULL);
   mystmt(hstmt,rc);
   
   rc = SQLBindCol(hstmt,2, SQL_C_CHAR, credname,  
                      CRED_NAME_LEN,NULL);
   mystmt(hstmt,rc);
    
   rc = SQLBindCol(hstmt,3, SQL_C_CHAR, cred_desc,  
                      MAX_TEXT_LEN,NULL);
   mystmt(hstmt,rc);
    
   printf ("mpi-9");
   rc = SQLFetch (hstmt);

   /*if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)  //failed
	return -1;
*/
   mystmt (hstmt,rc);
   printf ("mpi-10");

   mydbase.owner = strdup (owner);
   mydbase.credname = strdup (credname);
   mydbase.cred_desc = strdup (cred_desc);
   return 0;
}


/* Insert data using parameters                          *
*********************************************************/
int my_param_insert(SQLHDBC hdbc, SQLHSTMT hstmt)
{
  SQLRETURN   rc;
  SQLINTEGER  id;
  SQLCHAR     name[50];

   printf ("mpi-0");
  printf("\nmy_param_insert:\n");

    /* prepare the insert statement with parameters */
    if (mydbase.force_dbase_write)  
       rc = SQLPrepare(hstmt,"REPLACE main SET owner=?,passphrase=?,lifetime=?, retrievers=?, renewers=?, cred_name=?, cred_desc=?, credentials=?, username=?",SQL_NTS);   //force database write
    else
       rc = SQLPrepare(hstmt,"INSERT INTO main (owner,passphrase,lifetime, retrievers, renewers, cred_name, cred_desc, credentials, username) VALUES (?,?,?,?,?,?,?,?,?)",SQL_NTS);

    mystmt(hstmt,rc);
    
   printf ("mpi-1");

   rc = SQLBindParameter (hstmt, 1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.owner, strlen(mydbase.owner) , NULL);
   mystmt (hstmt, rc);
    
   printf ("mpi-2");
   rc = SQLBindParameter (hstmt, 2, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.passphrase, strlen (mydbase.passphrase), NULL);
   mystmt (hstmt, rc);

   printf ("mpi-3");
   rc = SQLBindParameter (hstmt, 3, SQL_PARAM_INPUT, SQL_INTEGER, SQL_C_ULONG, 10, 0, &(mydbase.lifetime) , sizeof(mydbase.lifetime), NULL);
   mystmt (hstmt, rc);

   printf ("mpi-4");
   if (mydbase.retrievers == NULL)
        rc = SQLBindParameter (hstmt, 4, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, "", 0, NULL);
   else
        rc = SQLBindParameter (hstmt, 4, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.retrievers, strlen (mydbase.retrievers), NULL);

   mystmt (hstmt, rc);

   printf ("mpi-5");
   if (mydbase.renewers == NULL)
       rc = SQLBindParameter (hstmt, 5, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, "",0, NULL);
   else
       rc = SQLBindParameter (hstmt, 5, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.renewers, strlen (mydbase.renewers), NULL);

   mystmt (hstmt, rc);

   printf ("mpi-6");
   if (mydbase.credname == NULL)
       rc = SQLBindParameter (hstmt,6, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, "", 0, NULL);
   else
       rc = SQLBindParameter (hstmt,6, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.credname, strlen (mydbase.credname), NULL);
   mystmt (hstmt, rc);

   printf ("mpi-7");
   if (mydbase.cred_desc == NULL)
       rc = SQLBindParameter (hstmt, 7, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 65535, 0, "", 0, NULL);
   else
       rc = SQLBindParameter (hstmt, 7, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 65535, 0, mydbase.cred_desc, strlen(mydbase.cred_desc), NULL);
   mystmt (hstmt, rc);

   printf ("mpi-8");
   if (mydbase.credentials == NULL)
       rc = SQLBindParameter (hstmt, 8, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 65535, 0, "",0, NULL);
   else
       rc = SQLBindParameter (hstmt, 8, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 65535, 0, mydbase.credentials, strlen (mydbase.credentials), NULL);
   mystmt (hstmt, rc);

       rc = SQLBindParameter (hstmt, 9, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 65535, 0, mydbase.username, strlen (mydbase.username), NULL);

   printf ("mpi-9");
   rc = SQLExecute (hstmt);
   mystmt (hstmt, rc);
   printf ("mpi-10");

    /* Free statement param resorces */
    rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
    mystmt(hstmt,rc);

    /* Free statement cursor resorces */
    rc = SQLFreeStmt(hstmt, SQL_CLOSE);
    mystmt(hstmt,rc);

    /* commit the transaction */
    rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
    mycon(hdbc,rc);

    return 0;
}

/*
 * check_storage_directory()
 *
 * Check for existance and permissions on given storage directory.
 *
 * Returns 0 if ok, -1 on error.
 */
static int
check_storage_directory(const char *path)
{
    struct stat statbuf;
    int return_code = -1;
    
    
    if (stat(path, &statbuf) == -1)
    {
        verror_put_errno(errno);
        verror_put_string("could not stat directory %s", path);
        goto error;
    }
    
    if (!S_ISDIR(statbuf.st_mode))
    {
        verror_put_string("%s is not a directory", path);
        goto error;
    }
    
    /* Make sure it's owned by me */
    if (statbuf.st_uid != getuid())
    {
        verror_put_string("bad ownership on %s", path);
        goto error;
    }
    
    /* Make sure it's not readable or writable by anyone else */
    if ((statbuf.st_mode & S_IRWXG) ||
        (statbuf.st_mode & S_IRWXO))
    {
        verror_put_string("bad permissions on %s", path);
        goto error;
    }
    
    /* Success */
    return_code = 0;
    
  error:
    return return_code;
}


/*
 * sterilize_string
 *
 * Walk through a string and make sure that is it acceptable for using
 * as part of a path.
 */
void
sterilize_string(char *string)
{
    /* Characters to be removed */
    char *bad_chars = "/";
    /* Character to replace any of above characters */
    char replacement_char = '-';
    
    assert(string != NULL);
    
    /* No '.' as first character */
    if (*string == '.')
    {
        *string = replacement_char;
    }
    
    /* Replace any bad characters with replacement_char */
    while (*string != '\0')
    {
        if (strchr(bad_chars, *string) != NULL)
        {
            *string = replacement_char;
        }

        string++;
    }

    return;
}

static char *
strmd5(const char *s, unsigned char *digest)
{
    MD5_CTX md5;
    unsigned char   d[16];
    int     i;
    char mbuf[33];

    MD5_Init(&md5);
    MD5_Update(&md5,s,strlen(s));
    MD5_Final(d,&md5);

    if (digest) 
       memcpy(digest,d,sizeof(d));
    for (i=0; i<16; i++) {
       int     dd = d[i] & 0x0f;
       mbuf[2*i+1] = dd<10 ? dd+'0' : dd-10+'a';
       dd = d[i] >> 4;
       mbuf[2*i] = dd<10 ? dd+'0' : dd-10+'a';
    }
    mbuf[32] = 0;
    return mystrdup(mbuf);
}
        
    
/*
 * get_storage_locations()
 *
 * Given an user name return the path where the credentials for that
 * username should be stored and the path where data about the credentials
 * should be stored.
 *
 * Return 0 on success, -1 on error.
 */
static int
get_storage_locations(const char *username,
                      char *creds_path,
                      const int creds_path_len,
                      char *data_path,
                      const int data_path_len)
{
    int return_code = -1;
    char *sterile_username = NULL;
    const char *creds_suffix = ".creds";
    const char *data_suffix = ".data";
    
    assert(username != NULL);
    assert(creds_path != NULL);
    assert(data_path != NULL);
    assert(storage_dir != NULL);
    
/*    if (check_storage_directory(storage_dir) == -1)
    {
        goto error;
    }
  */  
    if (strchr(username, '/')) {
       sterile_username = strmd5(username, NULL);
       if (sterile_username == NULL)
	  goto error;
    } else {
       sterile_username = mystrdup(username);

       if (sterile_username == NULL)
       {
	   goto error;
       }
       
       sterilize_string(sterile_username);
    }
    
    creds_path[0] = '\0';
    
    if (concatenate_strings(creds_path, creds_path_len, storage_dir,
			    "/", sterile_username, creds_suffix, NULL) == -1)
    {
        verror_put_string("Internal error: creds_path too small: %s line %s",
                          __FILE__, __LINE__);
        goto error;
    }

    data_path[0] = '\0';
    
    if (concatenate_strings(data_path, data_path_len, storage_dir,
			    "/", sterile_username, data_suffix, NULL) == -1)
    {
        verror_put_string("Internal error: data_path too small: %s line %s",
                          __FILE__, __LINE__);
        goto error;
    }

    /* Success */
    return_code = 0;

  error:
    if (sterile_username != NULL)
    {
        free(sterile_username);
    }
    
    return return_code;
}

/*
 * freedbase ()
 *
 * free dbase structure
 *
*/

void freedbase (struct myproxy_database *pdbase)
{
  if (pdbase== NULL)
     return;

  if (pdbase->owner != NULL)
     free(pdbase->owner);
    
  if (pdbase->passphrase != NULL)
     free(pdbase->passphrase);
    
  if (pdbase->retrievers != NULL)
     free(pdbase->retrievers);
    
  if (pdbase->renewers != NULL)
     free(pdbase->renewers);
    
  if (pdbase->credname != NULL)
     free(pdbase->credname);
    
  if (pdbase->cred_desc != NULL)
     free(pdbase->cred_desc);
   /* 
  if (pdbase->credentials != NULL)
     free(pdbase->credentials);
*/
  return;
}

/*
 * copy_credential_to_file()
 *
 * Copy credential in database to a file. The GSI delegation function requires the credential 
 * to be in a file.
 *
 * Returns 0 on success, -1 or error.
 */
int 
copy_credential_to_file(struct myproxy_creds *creds, char *filename)
{
    char buffer[MAX_TEXT_LEN];
    int dst_fd = -1;	
    int dst_flags = O_WRONLY | O_CREAT | O_TRUNC;
    mode_t data_file_mode = FILE_MODE;
    char creds_path[MAXPATHLEN];
    char data_path[MAXPATHLEN]; //+4 to add a suffix 
    SQLHENV    henv;
    SQLHDBC    hdbc;
    SQLHSTMT   hstmt;
    SQLRETURN   rc;
    int cred_length;
    int return_code = -1;
    char *tmp_data_file;

    if (get_storage_locations(creds->username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
        goto error;
    }
    

    mydsn = strdup (dbase_name);
    myuid = strdup ("root");
    mypwd = strdup ("");
   /*
    * connect to MySQL server
    */ 
    if (myconnect(&henv,&hdbc,&hstmt) < 0)
	return -1;

    /*
     * retrieve data
    */

   rc = SQLPrepare (hstmt, "Select credentials from main where username=? and cred_name=?", SQL_NTS);
   mystmt (hstmt,rc);
   rc = SQLBindParameter (hstmt, 1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, creds->username, strlen(creds->username), NULL);
   mystmt (hstmt, rc);
   rc = SQLBindParameter (hstmt, 2, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, creds->credname, strlen(creds->credname), NULL);
   mystmt (hstmt, rc);

   rc = SQLExecute (hstmt);
   mycon(hdbc,rc);

   rc = SQLBindCol(hstmt,1, SQL_C_CHAR, buffer, 
                      MAX_TEXT_LEN,NULL);
   mystmt(hstmt,rc);
  
   rc = SQLFetch (hstmt);

   if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)  //failed
	goto error;

   cred_length = strlen (buffer); // get credential size
   if (cred_length <= 0)
	goto error;
 
   /* Copy credential to file */

    printf ("opening %s for writing ..", data_path);
    dst_fd = open(data_path, dst_flags, data_file_mode);
   
    if (dst_fd == -1)
    {
        verror_put_errno(errno);
        verror_put_string("opening %s for writing", data_path);
        goto error;
    }
    if (write(dst_fd, buffer, cred_length) == -1)
    {
        verror_put_errno(errno);
        verror_put_string("writing %s", data_path);
        goto error;
    }

	strcpy (filename, data_path);  //copy filename
	return_code = 0;  //success

    error:;
    /*
     * disconnect from the server, by freeing all resources
    */
    mydisconnect(&henv,&hdbc,&hstmt);

	return return_code;
}

/*
 * copy_file()
 *
 * Copy source to destination, creating destination if needed.
 * Set permissions on destination to given mode.
 *
 * Returns 0 on success, -1 on error.
 */
static int
copy_file(const char *source,
          const char *dest,
          const mode_t mode)
{

    int src_fd = -1;
    int dst_fd = -1;
    int src_flags = O_RDONLY;
    int dst_flags = O_WRONLY | O_CREAT | O_TRUNC;
    char buffer[2048];
    int bytes_read;
    int return_code = -1;
    
    assert(source != NULL);
    assert(dest != NULL);
    src_fd = open(source, src_flags);
    if (src_fd == -1)
    {
        verror_put_errno(errno);
        verror_put_string("opening %s for reading", source);
        goto error;
    }

#ifdef NOTREQUIRED    
    dst_fd = open(dest, dst_flags, mode);
    
    if (dst_fd == -1)
    {
        verror_put_errno(errno);
        verror_put_string("opening %s for writing", dest);
        goto error;
    }
#endif    

	memset (mydbase.credentials, 0, MAX_TEXT_LEN);
    do 
    {
	static int size = 0;
        bytes_read = read(src_fd, buffer, sizeof(buffer)-1);
	buffer[bytes_read]='\0';
        if (bytes_read == -1)
        {
            verror_put_errno(errno);
            verror_put_string("reading %s", source);
            goto error;
        }

	size += bytes_read;
	
        if (bytes_read != 0)
        {
	if (size != bytes_read)
		strcat (mydbase.credentials, buffer);
	else
		strcpy (mydbase.credentials,buffer);
#ifdef NOTREQUIRED
            if (write(dst_fd, buffer, bytes_read) == -1)
            {
                verror_put_errno(errno);
                verror_put_string("writing %s", dest);
                goto error;
            }
#endif
      }
    }
    while (bytes_read > 0);
  
    /* Success */
    return_code = 0;
        
  error:
    if (src_fd != -1)
    {
        close(src_fd);
    }
#ifdef NOTREQUIRED    
    if (dst_fd != -1)
    {
        close(dst_fd);

        if (return_code == -1)
        {
            unlink(dest);
        }
    }
    
#endif 
    return return_code;
}

/*
 * write_data_file()
 *
 * Write the data in the myproxy_creds() structure out the the
 * file name given, creating it if needed with the given mode.
 *
 * Returns 0 on success, -1 on error.
 */
static int
write_data_file(const struct myproxy_creds *creds,
                const char *data_file_path,
                const mode_t data_file_mode)
{

    int data_fd = -1;
    FILE *data_stream = NULL;
    int data_file_open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    int return_code = -1;
        char *tmp1;
#ifdef NOTREQUIRED
    
    /*
     * Open with open() first to minimize any race condition with
     * file permissions.
     */
    data_fd = open(data_file_path, data_file_open_flags, data_file_mode);
    
    if (data_fd == -1)
    {
        verror_put_errno(errno);
        verror_put_string("opening storage file %s", data_file_path);
        goto error;
    }

    /* Now open as stream for easier IO */
    data_stream = fdopen(data_fd, "w");
    
    if (data_stream == NULL)
    {
        verror_put_errno(errno);
        verror_put_string("reopening storage file %s", data_file_path);
        goto error;
    }
#endif 

    /* Write out all the extra data associated with these credentials 
     * support for crypt() added btemko /6/16/00
     * Please, don't try to free tmp1 - crypt() uses one 
     * static string space, a la getenv()
     */
    tmp1=(char *)crypt(creds->passphrase, 
	&creds->owner_name[strlen(creds->owner_name)-3]);

    mydbase.owner = strdup (creds->owner_name);
//    fprintf(data_stream, "OWNER=%s\n", creds->owner_name);
    mydbase.passphrase = strdup (tmp1);
 //   fprintf(data_stream, "PASSPHRASE=%s\n", tmp1);
    mydbase.lifetime = creds->lifetime;
//    fprintf(data_stream, "LIFETIME=%d\n", creds->lifetime);

    mydbase.username = strdup (creds->username);

    if (creds->retrievers != NULL)
    {
//        fprintf(data_stream, "RETRIEVERS=%s\n", creds->retrievers);
        mydbase.retrievers = strdup (creds->retrievers);
    }
    else
	mydbase.retrievers = NULL;

    if (creds->renewers != NULL)
    {	
//        fprintf(data_stream, "RENEWERS=%s\n", creds->renewers);
        mydbase.renewers = strdup (creds->renewers);
    }
    else
	mydbase.renewers = NULL;

   mydbase.credname = strdup (creds->credname);
   mydbase.cred_desc = strdup (creds->cred_desc);
    mydbase.force_dbase_write = creds->force_dbase_write;

//    fprintf(data_stream, "END_OPTIONS\n");

    fflush(data_stream);
    
    /* Success */
    return_code = 0;
    
  error:
    if (data_fd != -1)
    {
        close(data_fd);
        
        if (return_code == -1)
        {
            unlink(data_file_path);
        }
    }

    if (data_stream != NULL)
    {
        fclose(data_stream);
    }
    
    return return_code;
}

struct dbase_info
{
  char *owner;
  char *credname;
  char *cred_desc;
};

/*
 * read_data_file()
 *
 * Read the data contained in the given data file and fills in the
 * given creds structure.
 *
 * Returns 0 on success, -1 on error.
 */
static int
read_data_file(struct myproxy_creds *creds,
               const char *datafile_path)
{
    FILE *data_stream = NULL;
    char *data_stream_mode = "r";
    int done = 0;
    int line_number = 0;
    int return_code = -1;
    struct dbase_info *dbase_recs;
    int num_recs,i;
    //char *pchr;
    
    assert(creds != NULL);
    assert(datafile_path != NULL);
    
    data_stream = fopen(datafile_path, data_stream_mode);
    
    if (data_stream == NULL)
    {
        verror_put_errno(errno);
        verror_put_string("opening %s for reading", datafile_path);
        goto error;
    }

    while (!done)
    {
        char buffer[512];
        char *variable;
        char *value;
        int len;
        
        if (fgets(buffer, sizeof(buffer), data_stream) == NULL)
        {
            int errno_save = errno;
            
            if (feof(data_stream))
            {
                verror_put_string("unexpected EOF reading %s", datafile_path);
                goto error;
            }
            else
            {
                verror_put_errno(errno_save);
                verror_put_string("reading %s", datafile_path);
                goto error;
            }
            /* Not reached */
        }

        /* Remove carriage return from credentials */
        len = strlen(buffer);
        
        if (buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }

        line_number++;
        
        variable = buffer;
        
        value = strchr(buffer, '=');
        
        if (value != NULL)
        {
            /* NUL-terminate variable name */
            *value = '\0';

            /* ...and advance value to point at value */
            value++;
        }

        if (strcmp(variable, "END_OPTIONS") == 0) 
        {
            done = 1;
            break;
        }
        
        /* Everything else requires values to be non-NULL */
        if (value == NULL)
        {
            verror_put_string("malformed line: %s line %d",
                              datafile_path, line_number);
            goto error;
        }
        
        if (strcmp(variable, "OWNER") == 0)
        {
            creds->owner_name = mystrdup(value);
            
            if (creds->owner_name == NULL)
            {
                goto error;
            }
            continue;
        }

        if (strcmp(variable, "PASSPHRASE") == 0)
        {
            creds->passphrase = mystrdup(value);
            
            if (creds->passphrase == NULL)
            {
                goto error;
            }
            continue;
        }
        
        if (strcmp(variable, "RETRIEVERS") == 0)
        {
            creds->retrievers = mystrdup(value);
            
            if (creds->retrievers == NULL)
            {
                goto error;
            }
            continue;
        }
        
        if (strcmp(variable, "RENEWERS") == 0)
        {
            creds->renewers = mystrdup(value);
            
            if (creds->renewers == NULL)
            {
                goto error;
            }
            continue;
        }
        
        if (strcmp(variable, "LIFETIME") == 0)
        {
            creds->lifetime = (int) strtol(value, NULL, 10);
            
            continue;
        }
        
        /* Unrecognized varibale */
        verror_put_string("unrecognized line: %s line %d",
                          datafile_path, line_number);
        goto error;
    }

    //dbase_recs = (struct dbase_info *) malloc (sizeof (struct dbase_info));
    //num_recs = read_from_database_for_info(&dbase_recs);

//    pchr = read_from_database(&dbase_recs);

/*    for (i = 0; i < num_recs; i++)
	printf ("%s\t%s\t%s\n-------------------\n",(dbase_recs+i)->owner, dbase_recs[i].credname, dbase_recs[i].cred_desc);
*/
    //printf ("%s",pchr);
    /* Success */
    return_code = 0;
    
  error:
    if (data_stream != NULL)
    {
        fclose(data_stream);
    }
    
    return return_code;
}

/*
 * Reads all records and returns a pointer to a character string having the record data (owner, credential name , credential description) 
 */

char *read_from_database_for_info()
{
  SQLHENV    henv;
  SQLHDBC    hdbc;
  SQLHSTMT   hstmt;
  SQLINTEGER narg;
  SQLRETURN   rc;
  unsigned long size;
  int index = -1;
  char *data;

    mydsn = strdup (dbase_name);
    myuid = strdup ("root");
    mypwd = strdup ("");
   /*
    * connect to MySQL server
    */ 
    if (myconnect(&henv,&hdbc,&hstmt) < 0)
	return NULL;

    /*
     * retrieve data
    */

   data = NULL;
   rc = SQLExecDirect (hstmt, "Select owner, cred_name, cred_desc from main", SQL_NTS);
   if (mystmt_wrap (hstmt,rc) < 0)
	return NULL;

    memset (&mydbase, 0, sizeof (mydbase));
    while (my_retrieve(hdbc, hstmt) == 0)
    {
	
	int len = strlen (mydbase.owner)+strlen (mydbase.credname)+strlen(mydbase.cred_desc);	
	static int tot_len = 0;

	tot_len += len+3;  //3 extra characters for new lines. 

	data = realloc (data, tot_len);

	if (tot_len == len+3)  // indicates first record
 		strcpy (data, mydbase.owner);
	else
		strcat (data, mydbase.owner);	
	strcat (data, "\t");
	strcat (data, mydbase.credname);
	strcat (data, "\t");
	strcat (data, mydbase.cred_desc);
	strcat (data, "\t");

	if (mydbase.owner != NULL)
		free (mydbase.owner);
	if (mydbase.credname != NULL)
		free (mydbase.credname);
	if (mydbase.cred_desc != NULL)
		free (mydbase.cred_desc);
    }

	data[strlen(data)-1] = '\0';	//avoid last tab
    /*
     * disconnect from the server, by freeing all resources
    */
    mydisconnect(&henv,&hdbc,&hstmt);

    return data;
}

/*
 * Retrieve credential data from database for given username and credential name
 */

int retrieve_from_database_given_username_credname(char *username, char *credname, struct myproxy_creds *pcreds)
{
	SQLHENV    henv;
	SQLHDBC    hdbc;
	SQLHSTMT   hstmt;
	SQLINTEGER narg;
	SQLRETURN   rc;
	char owner[MAX_VARCHAR_LEN], retrievers[MAX_VARCHAR_LEN], renewers[MAX_VARCHAR_LEN], passphrase[MAX_VARCHAR_LEN];
	unsigned long size;
	int index = -1;
	char *data;
	//char credential[MAX_TEXT_LEN];

	mydsn = strdup (dbase_name);
	myuid = strdup ("root");
	mypwd = strdup ("");
	/*
	 * connect to MySQL server
	 */ 
	if (myconnect(&henv,&hdbc,&hstmt) < 0)
		return -1;

	/* Get size */

	/*
	 * retrieve data
	 */

	data = NULL;
   rc = SQLPrepare(hstmt, "Select owner, lifetime, retrievers, renewers, passphrase from main where (username=? and cred_name=?)", SQL_NTS);
   mystmt (hstmt,rc);

   rc = SQLBindParameter (hstmt, 1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, username, strlen(username), NULL);
   mystmt (hstmt, rc);
   rc = SQLBindParameter (hstmt, 2, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, credname, strlen(credname), NULL);
   mystmt (hstmt, rc);

   rc = SQLExecute (hstmt);
   mycon(hdbc,rc);

   rc = SQLBindCol(hstmt,1, SQL_C_CHAR, owner, 
                      MAX_VARCHAR_LEN,NULL);
   mystmt(hstmt,rc);
   
   rc = SQLBindCol(hstmt,2, SQL_C_ULONG, &pcreds->lifetime, 
                      sizeof (pcreds->lifetime),NULL);
   mystmt(hstmt,rc);
   
   rc = SQLBindCol(hstmt,3, SQL_C_CHAR, retrievers, 
                      MAX_VARCHAR_LEN,NULL);
   mystmt(hstmt,rc);
   
   rc = SQLBindCol(hstmt,4, SQL_C_CHAR, renewers, 
                      MAX_VARCHAR_LEN,NULL);
   mystmt(hstmt,rc);
   
   rc = SQLBindCol(hstmt,5, SQL_C_CHAR, passphrase, 
                      MAX_VARCHAR_LEN,NULL);
   mystmt(hstmt,rc);
   
   /*rc = SQLBindCol(hstmt,5, SQL_C_CHAR, credential, 
                      MAX_TEXT_LEN,NULL);
   mystmt(hstmt,rc);
   */
   rc = SQLFetch (hstmt);

   if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)  //failed
   {
	myproxy_log ("Credential fetch failed");
	return -1;
   }

   pcreds->owner_name = strdup (owner);
   pcreds->retrievers = strdup (retrievers);
   pcreds->renewers = strdup (renewers);
   pcreds->passphrase = strdup (passphrase);

    /*
     * disconnect from the server, by freeing all resources
    */
    mydisconnect(&henv,&hdbc,&hstmt);

    return 0;
}

int write_to_database()
{
  SQLHENV    henv;
  SQLHDBC    hdbc;
  SQLHSTMT   hstmt;
  SQLINTEGER narg;
  int retcode = -1;

    mydsn = strdup (dbase_name); // connect to default database
    myuid = strdup ("root");
    mypwd = strdup ("");

   
   /*
    * connect to MySQL server
    */ 
    if (myconnect(&henv,&hdbc,&hstmt) < 0)
	return retcode;

    /* 
     * initialize table
    */
    if (my_init_table(hdbc, hstmt) < 0)
	goto error;

    /*
     * insert data using parameters
    */
    if (my_param_insert(hdbc, hstmt) < 0)
	goto error;

	retcode = 0;
    /*
     * disconnect from the server, by freeing all resources
    */
    error:
    mydisconnect(&henv,&hdbc,&hstmt);

   return retcode;

}

/**********************************************************************
 *
 * API routines
 *
 */

int
myproxy_creds_store(const struct myproxy_creds *creds)
{
    char creds_path[MAXPATHLEN] = "";
    char data_path[MAXPATHLEN] = "";
    mode_t data_file_mode = FILE_MODE;
    mode_t creds_file_mode = FILE_MODE;
    int return_code = -1;
   
	printf ("Myproxy_creds_store entered\n"); 
    if ((creds == NULL) ||
        (creds->username == NULL) ||
        (creds->passphrase == NULL) ||
        (creds->owner_name == NULL) ||
        (creds->location == NULL) ||
        (creds->restrictions != NULL))
    {
        verror_put_errno(EINVAL);
        return -1;
    }

//#ifdef FORLATER  
    if (get_storage_locations(creds->username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
        goto error;
    }
//#endif
    /*
     * If credentials already exist for this username then we need
     * to check to make sure new credentials have the same owner.
     */

    printf ("Hi - 1\n");
    memset (&mydbase, 0, sizeof (mydbase));

    //data_path = strdup ("junk");  //temporary
    //creds_path = strdup ("junk");

    // the following 2 functions fill in the mydbase structure too
    if (write_data_file(creds, data_path, data_file_mode) == -1) // info about credential
    {
        goto error;
    }
    printf ("Hi - 2\n");

    if (copy_file(creds->location, creds_path, creds_file_mode) == -1) // credential
    {
        goto error;
    }

    printf ("Hi - 3\n");
    if (write_to_database () < 0)
	goto error;

    printf ("Hi - 4\n");
    freedbase (&mydbase);

    /* Success */
    return_code = 0;
    
  error:
    /* XXX */
    /* Remove files on error */
    if (return_code == -1)
    {
        unlink(data_path);
        unlink(creds_path);
    }

    return return_code;
}

int
myproxy_creds_fetch_entry(char *username, char *credname, struct myproxy_creds *creds)
{
   char creds_path[MAXPATHLEN];
   char data_path[MAXPATHLEN];

   if (username == NULL || creds == NULL) {
      verror_put_errno(EINVAL);
      return -1;
   }

   if (get_storage_locations(username,
	                     creds_path, sizeof(creds_path),
			     data_path, sizeof(data_path)) == -1)
      return -1;

   //if (read_data_file(creds, data_path) == -1)
     // return -1;

   if (retrieve_from_database_given_username_credname (username, credname, creds) == -1)
	return -1;

   creds->username = mystrdup(username);
   creds->location = mystrdup(creds_path);
   creds->restrictions = NULL;
   return 0;
}


/* retrieves credential for the user name and credential name*/
int
myproxy_creds_retrieve(struct myproxy_creds *creds)
{
    char creds_path[MAXPATHLEN];
    char data_path[MAXPATHLEN];
    struct myproxy_creds retrieved_creds;
    int return_code = -1;
        char *tmp1=NULL;
    
    if ((creds == NULL) ||
        (creds->username == NULL) ||
        (creds->passphrase == NULL))
    {
        verror_put_errno(EINVAL);
        return -1;
    }

    memset(&retrieved_creds, 0, sizeof(retrieved_creds));
   
/*
    if (get_storage_locations(creds->username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
        goto error;
    }

    if (read_data_file(&retrieved_creds, data_path) == -1)
    {
	verror_put_string("can't read credentials");
        goto error;
    }
*/

   if ( retrieve_from_database_given_username_credname(creds->username, creds->credname, &retrieved_creds) == -1)
	goto error;

    /* Copy creds */
    if (creds->owner_name != NULL)
       free(creds->owner_name);
    if (creds->location != NULL)
       free(creds->location);
    creds->owner_name = mystrdup(retrieved_creds.owner_name);
    //creds->location = mystrdup(creds_path);
    creds->lifetime = retrieved_creds.lifetime;
    creds->restrictions = NULL;
    
    if ((creds->owner_name == NULL)) //||
       // (creds->location == NULL))
    {
        goto error;
    }
   
    if (retrieved_creds.retrievers == NULL || strlen(retrieved_creds.retrievers) == 0)
       creds->retrievers = NULL;
    else
       creds->retrievers = mystrdup(retrieved_creds.retrievers);

    if (retrieved_creds.renewers != NULL || strlen(retrieved_creds.renewers) == 0)
       creds->renewers = NULL;
    else
       creds->renewers = mystrdup(retrieved_creds.renewers);
 
    /* Success */
    return_code = 0;
    
  error:
    if (return_code < 0)
    {
        /*
         * Don't want to free username or passphrase as caller supplied
         * these.
         */
        if (creds->owner_name != NULL)
        {
            free(creds->owner_name);
            creds->owner_name = NULL;
        }
        if (creds->location != NULL)
        {
            free(creds->location);
            creds->location = NULL;
        }
        creds->lifetime = 0;
    }

    myproxy_creds_free_contents(&retrieved_creds);
    
    return return_code;
}


int
myproxy_creds_exist(const char *username)
{
    char creds_path[MAXPATHLEN] = "";
    char data_path[MAXPATHLEN] = "";
    int rc;

    if (username == NULL)
    {
	verror_put_errno(EINVAL);
	return -1;
    }

    if (get_storage_locations(username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
	return -1;
    }

    rc = file_exists(creds_path);
    
    switch(rc)
    {
      case 0:
	/* File does not exist */
	return 0;

      case 1:
	/* File exists, keep checking */
	break;
	
      case -1:
	/* Error */
	return -1;

      default:
	/* Should not be here */
	verror_put_string("file_exists(%s) return unknown value (%d)",
			  creds_path, rc);
	return -1;
    }

    rc = file_exists(data_path);
    
    switch(rc)
    {
      case 0:
	/* File does not exist */
	return 0;

      case 1:
	/* File exists, keep checking */
	break;
	
      case -1:
	/* Error */
	return -1;

      default:
	/* Should not be here */
	verror_put_string("file_exists(%s) return unknown value (%d)",
			  data_path, rc);
	return -1;
    }
    
    /* Everything seems to exist */
    
    /* XXX Should check for expiration? */

    return 1;
}

int
myproxy_creds_is_owner(const char		*username,
			const char		*client_name)
{
    char creds_path[MAXPATHLEN];
    char data_path[MAXPATHLEN];
    struct myproxy_creds retrieved_creds;
    int return_code = -1;

    memset(&retrieved_creds, 0, sizeof(retrieved_creds));

    assert(username != NULL);
    assert(client_name != NULL);
    
    if (get_storage_locations(username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
        goto error;
    }

    if (read_data_file(&retrieved_creds, data_path) == -1)
    {
        goto error;
    }

    if (strcmp(retrieved_creds.owner_name, client_name) == 0)
    {
	/* Is owner */
	return_code = 1;
    }
    else
    {
	/* Is not owner */
	return_code = 0;
    }
    
  error:
    myproxy_creds_free_contents(&retrieved_creds);
    
    return return_code;
}

int my_delete (SQLHDBC hdbc, SQLHSTMT hstmt)
{
  SQLRETURN   rc;

  rc = SQLPrepare(hstmt,"DELETE FROM main where cred_name = ?", SQL_NTS);
  mystmt(hstmt,rc);

  rc = SQLBindParameter (hstmt,1, SQL_PARAM_INPUT, SQL_VARCHAR, SQL_C_CHAR, 255, 0, mydbase.credname, \
  			 strlen (mydbase.credname), NULL);
  mystmt (hstmt, rc);

  rc = SQLExecute (hstmt);
  mystmt (hstmt, rc);

  /* Free statement param resorces */
  rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
  mystmt(hstmt,rc);

  /* Free statement cursor resorces */
  rc = SQLFreeStmt(hstmt, SQL_CLOSE);
  mystmt(hstmt,rc);

  /* commit the transaction */
  rc = SQLEndTran(SQL_HANDLE_DBC, hdbc, SQL_COMMIT);
  mycon(hdbc,rc);

  return 0;
}
   
/*
	Delete credential
*/
int
myproxy_creds_delete(const struct myproxy_creds *creds)
{
  SQLHENV    henv;
  SQLHDBC    hdbc;
  SQLHSTMT   hstmt;
  SQLINTEGER narg;
  int retcode = -1;

  mydsn = strdup (dbase_name); // connect to default database
  myuid = strdup ("root");
  mypwd = strdup ("");

  mydbase.credname = strdup (creds->credname); 
   /*
    * connect to MySQL server
    */ 
    if (myconnect(&henv,&hdbc,&hstmt) < 0)
	return retcode;

    /*
     * delete credential
    */
    if (my_delete(hdbc, hstmt) < 0)
	goto error;

	retcode = 0;
    /*
     * disconnect from the server, by freeing all resources
    */
    error:
    mydisconnect(&henv,&hdbc,&hstmt);

   return retcode;

}

#ifdef TO_BE_REMOVED
    char creds_path[MAXPATHLEN];
    char data_path[MAXPATHLEN];
    struct myproxy_creds tmp_creds;
    int return_code = -1;
        char *tmp1=NULL;
    
    if ((creds == NULL) ||
        (creds->username == NULL))
    {
        verror_put_errno(EINVAL);
        return -1;
    }
    
    if (get_storage_locations(creds->username,
                              creds_path, sizeof(creds_path),
                              data_path, sizeof(data_path)) == -1)
    {
        goto error;
    }

    if (read_data_file(&tmp_creds, data_path) == -1)
    {
        goto error;
    }
    
    if (unlink(creds_path) == -1)
    {
        verror_put_errno(errno);
        verror_put_string("deleting credentials file %s", creds_path);
        goto error;
    }
    
    if (unlink(data_path) == -1)
    {
        verror_put_errno(errno);
        verror_put_string("deleting credentials data file %s", creds_path);
        goto error;
    }

    /* Success */
    return_code = 0;
    
  error:
    return return_code;
}
#endif

int
myproxy_creds_info(struct myproxy_creds *creds, char **records)
{
    char creds_path[MAXPATHLEN];
    char data_path[MAXPATHLEN];
    struct myproxy_creds tmp_creds;
    int return_code = -1;
    time_t end_time;

    if ((creds == NULL) || (creds->username == NULL)) {
       verror_put_errno(EINVAL);
       return -1;
    }

    if (get_storage_locations(creds->username,
	                      creds_path, sizeof(creds_path),
			      data_path, sizeof(data_path)) == -1) {
       goto error;
    }

    if (ssl_get_times(creds_path, &creds->start_time, &creds->end_time) != 0)
       goto error;

    *records = read_from_database_for_info();
    return_code = 0;

error:
    return return_code;
}

void myproxy_creds_free_contents(struct myproxy_creds *creds)
{
    if (creds == NULL)
    {
        return;
    }
    
    if (creds->username != NULL)
    {
        free(creds->username);
        creds->username = NULL;
    }

    if (creds->passphrase != NULL)
    {
        free(creds->passphrase);
        creds->passphrase = NULL;
    }

    if (creds->owner_name != NULL)
    {
        free(creds->owner_name);
        creds->owner_name = NULL;
    }
    
    if (creds->location != NULL)
    {
        free(creds->location);
        creds->location = NULL;
    }
}

void myproxy_set_storage_dir(char *dir)
{ 
    storage_dir=dir;
}

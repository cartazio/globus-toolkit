/*
 * secure_ext.h
 *
 * RFC 2228 FTP Security extensions
 *
 */

#ifndef __SECURE_EXT_H
#define __SECURE_EXT_H	1

/* Succeed authentication mechanism */
extern char *auth_type;

/* Authentication mechanism client is in process of attempting */
extern char *attempting_auth_type;

/* Our current protection level */
extern int protection_level;

/* Names of protection levels */
extern char *protection_levelnames[];

/* Size of protected buffer */
extern unsigned int max_pbuf_size;


int set_prot_level();
int clear_cmd_channel();
int auth();
int auth_data();
int pbsz();
int secure_parse_message();

int send_adat_reply();


/*
 * Protection levels
 */
#define PROT_C          1       /* clear */
#define PROT_S          2       /* safe */
#define PROT_P          3       /* private */
#define PROT_E          4       /* confidential */

/*
 * Does a protection level imply integrity checking?
 */
#define PROT_INTEGRITY(p)	(((p) == PROT_S) || ((p) == PROT_P))

/*
 * Does a protection level imply encryption?
 */
#define PROT_ENCRYPTION(p)	(((p) == PROT_P) || ((p) == PROT_E))

/* Need larger buffers for authentication data */
#define LARGE_BUFSIZE		10240

#endif /* __SECURE_EXT_H */


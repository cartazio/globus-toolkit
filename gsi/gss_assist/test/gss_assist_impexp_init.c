#include <gssapi.h>
#include <globus_gss_assist.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#define INIT_MESSAGE "INITIATOR WRAP MESSAGE"
#define INIT_CONTEXT_FILE               "exported_init_context"

int main(int argc, char * argv[])
{
    gss_cred_id_t                       init_cred = GSS_C_NO_CREDENTIAL;
    OM_uint32                           major_status;
    OM_uint32                           minor_status;
    int                                 token_status;
    gss_ctx_id_t                        init_context = GSS_C_NO_CONTEXT;
    OM_uint32                           ret_flags;
    int                                 sock;
    FILE *                              stre;
    char *                              print_buffer = NULL;
    char *                              recv_buffer = NULL;
    char *                              verbose_env = NULL;
    int                                 buffer_length;
    struct sockaddr_in                  sockaddr;
    struct hostent *                    hostname;
    gss_buffer_desc                     export_token;
    FILE *                              context_outfile = NULL;
    FILE *                              context_infile = NULL;
    unsigned char                       int_buf[4];

    verbose_env = getenv("GSS_ASSIST_VERBOSE_TEST");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("opening stream socket");
        exit(1);
    }
    
    sockaddr.sin_family = AF_INET;

    hostname = gethostbyname(argv[1]);
    if(hostname == 0)
    {
        fprintf(stdout, "%s: uknown host", argv[1]);
        exit(2);
    }

    bcopy(hostname->h_addr, &sockaddr.sin_addr, hostname->h_length);
    sockaddr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)
    {
        perror("connecting stream socket");
        exit(1);
    }

    stre = fdopen(sock, "r+");
    setbuf(stre, NULL);

    /* INITIATOR PROCESS */
    
    major_status = globus_gss_assist_acquire_cred(&minor_status,
                                                  GSS_C_INITIATE,
                                                  &init_cred);
    if(GSS_ERROR(major_status))
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't acquire initiator's credentials",
            major_status,
            minor_status,
            0);
        exit(1);
    }

    major_status = globus_gss_assist_init_sec_context(
        &minor_status,
        init_cred,
        &init_context,
        NULL,
        GSS_C_MUTUAL_FLAG,
        &ret_flags,
        &token_status,
        globus_gss_assist_token_get_fd,
        (void *) (stre),
        globus_gss_assist_token_send_fd,
        (void *) (stre));
    if(GSS_ERROR(major_status))
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't authenticate as initiator\n",
            major_status,
            minor_status,
            token_status);
        exit(1);
    }
    
    if(verbose_env)
    {
        fprintf(stdout, 
                "INITIATOR: "__FILE__":%d"
                ": Initiator successfully created context\n", __LINE__);
    }
    /* export sec context doesn't work for init */
    /*
    context_outfile = fopen(INIT_CONTEXT_FILE, "w");
    if(!context_outfile)
    {
        perror("Can't open "INIT_CONTEXT_FILE" for writing");
        exit(1);
    }

    major_status = gss_export_sec_context(
        &minor_status,
        &init_context,
        (gss_buffer_t) &export_token);

    if(major_status != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't authenticate as initiator\n",
            major_status,
            minor_status,
            0);
        exit(1);
    }

    int_buf[0] = (unsigned char)(((export_token.length)>>24)&0xff);
    int_buf[1] = (unsigned char)(((export_token.length)>>16)&0xff);
    int_buf[2] = (unsigned char)(((export_token.length)>> 8)&0xff);
    int_buf[3] = (unsigned char)(((export_token.length)    )&0xff);
        
    if (fwrite(int_buf, 4, 1, context_outfile) != 1)
    {
        perror("Couldn't write security context length "
               "to open file stream");
        exit(1);
    }
    if (fwrite(export_token.value,
               export_token.length,
               1,
               context_outfile) != 1)
    {
        perror("Couldn't write security context export token "
               "to open file stream");
        exit(1);
    }
        
    major_status = gss_release_buffer(&minor_status, &export_token);
    if(major_status != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status(
            stdout,
            "ACCEPTOR: Couldn't delete export "
            "token for security context\n",
            major_status,
            minor_status,
            0);
        exit(1);
    }

    if(fclose(context_outfile) == EOF)
    {
        perror("Couldn't close export token file stream");
        exit(1);
    }

    context_infile = fopen(INIT_CONTEXT_FILE, "r");
    if(!context_outfile)
    {
        perror("Couldn't open "INIT_CONTEXT_FILE" file for reading");
        exit(1);
    }

    major_status = globus_gss_assist_import_sec_context(
        &minor_status,
        &init_context,
        &token_status,
        fileno(context_infile),
        stdout);
    if(major_status != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't import security context from file\n",
            major_status,
            minor_status,
            token_status);
        exit(1);
    }

    if(fclose(context_infile) == EOF)
    {
        perror("Couldn't close security context input file descriptor");
        exit(1);
    }

    if(verbose_env)
    {
        fprintf(stdout,
                "INITIATOR: "__FILE__":%d: Initiator successfully "
                "exported/imported context\n");
    }

    */
    
    major_status = globus_gss_assist_wrap_send(
        &minor_status,
        init_context,
        INIT_MESSAGE,
        sizeof(INIT_MESSAGE),
        &token_status,
        globus_gss_assist_token_send_fd,
        (void *) (stre),
        stdout);
    if(GSS_ERROR(major_status))
    {
        globus_gss_assist_display_status(
            stdout,
            "INITATOR: Couldn't wrap and send message\n",
            major_status,
            minor_status,
            token_status);
        exit(1);
    }

    major_status = globus_gss_assist_get_unwrap(
        &minor_status,
        init_context,
        &recv_buffer,
        &buffer_length,
        &token_status,
        globus_gss_assist_token_get_fd,
        (void *) (stre),
        stdout);
    if(GSS_ERROR(major_status))
    {
        fprintf(stdout, "INITIATOR ERROR\n");
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't get encrypted message from initiator\n",
            major_status,
            minor_status,
            token_status);
        fprintf(stdout, "INITIATOR ERROR FINISHED\n");
        exit(1);
    }
    
    print_buffer = malloc(buffer_length + 1);
    globus_libc_snprintf(print_buffer, buffer_length + 1, "%s", recv_buffer);

    if(verbose_env)
    {
        fprintf(stdout,
                "INITIATOR: "__FILE__":%d"
                ": received: %s\n", __LINE__, print_buffer);
    }

    free(print_buffer);
    free(recv_buffer);

    major_status = globus_gss_assist_wrap_send(
        &minor_status,
        init_context,
        INIT_MESSAGE,
        sizeof(INIT_MESSAGE),
        &token_status,
        globus_gss_assist_token_send_fd,
        (void *) (stre),
        stdout);
    if(GSS_ERROR(major_status))
    {
        globus_gss_assist_display_status(
            stdout,
            "INITATOR: Couldn't wrap and send message\n",
            major_status,
            minor_status,
            token_status);
        exit(1);
    }

    major_status = globus_gss_assist_get_unwrap(
        &minor_status,
        init_context,
        &recv_buffer,
        &buffer_length,
        &token_status,
        globus_gss_assist_token_get_fd,
        (void *) (stre),
        stdout);
    if(GSS_ERROR(major_status))
    {
        fprintf(stdout, "INITIATOR ERROR\n");
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't get encrypted message from initiator\n",
            major_status,
            minor_status,
            token_status);
        fprintf(stdout, "INITIATOR ERROR FINISHED\n");
        exit(1);
    }
    
    print_buffer = malloc(buffer_length + 1);
    globus_libc_snprintf(print_buffer, buffer_length + 1, "%s", recv_buffer);
    
    if(verbose_env)
    {
        fprintf(stdout,
                "INITIATOR: "__FILE__":%d"
                ": received: %s\n", __LINE__, print_buffer);
    }

    free(print_buffer);
    free(recv_buffer);
    
    major_status = gss_delete_sec_context(&minor_status,
                                          &init_context,
                                          GSS_C_NO_BUFFER);
    if(major_status != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't delete security context\n",
            major_status,
            minor_status,
            0);
        exit(1);
    }
            
    gss_release_cred(&minor_status,
                     &init_cred);
    if(major_status != GSS_S_COMPLETE)
    {
        globus_gss_assist_display_status(
            stdout,
            "INITIATOR: Couldn't delete security context\n",
            major_status,
            minor_status,
            0);
        exit(1);
    }

    if(fclose(stre) == EOF)
    {
        perror("closing stream socket");
        exit(1);
    }

    exit(0);
}

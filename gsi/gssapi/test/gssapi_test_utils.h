
#include "gssapi.h"
#include "globus_gss_assist.h"
#include "globus_common.h"

gss_cred_id_t 
globus_gsi_gssapi_test_acquire_credential();

void 
globus_gsi_gssapi_test_release_credential(
    gss_cred_id_t *                     credential);

globus_bool_t
globus_gsi_gssapi_test_authenticate(
    int                                 fd,
    globus_bool_t                       server, 
    gss_cred_id_t                       credential, 
    gss_ctx_id_t *                      context_handle, 
    char **                             user_id, 
    gss_cred_id_t *                     delegated_cred);

void 
globus_gsi_gssapi_test_cleanup(
    gss_ctx_id_t *                      context_handle,
    char *                              userid,
    gss_cred_id_t *                     delegated_cred);

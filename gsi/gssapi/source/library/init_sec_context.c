#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL
/**
 * @file init_sec_context.c
 * @author Sam Lang, Sam Meder
 * 
 * $RCSfile$
 * $Revision$
 * $Date$
 */
#endif

static char *rcsid = "$Id$";

#include "gssapi_openssl.h"
#include "globus_i_gsi_gss_utils.h"
#include <string.h>
#include "openssl/evp.h"

#include "globus_gsi_cert_utils.h"

OM_uint32 
GSS_CALLCONV gss_init_sec_context(
    OM_uint32 *                         minor_status,
    const gss_cred_id_t                 initiator_cred_handle,
    gss_ctx_id_t *                      context_handle_P,
    const gss_name_t                    target_name,
    const gss_OID                       mech_type,
    OM_uint32                           req_flags,
    OM_uint32                           time_req,
    const gss_channel_bindings_t        input_chan_bindings,
    const gss_buffer_t                  input_token,
    gss_OID *                           actual_mech_type,
    gss_buffer_t                        output_token,
    OM_uint32 *                         ret_flags,
    OM_uint32 *                         time_rec) 
{

    gss_ctx_id_desc *                   context = NULL;
    OM_uint32                           major_status = GSS_S_COMPLETE;
    OM_uint32                           local_minor_status;
    OM_uint32                           local_major_status;
    globus_result_t                     local_result;
    globus_result_t                     callback_error;
    int                                 rc;
    char                                cbuf[1];
    globus_gsi_cert_utils_proxy_type_t  proxy_type = GLOBUS_FULL_PROXY;

    static char *                       _function_name_ = 
        "gss_init_sec_context";

    GLOBUS_I_GSI_GSSAPI_DEBUG_ENTER;

    *minor_status = (OM_uint32) GLOBUS_SUCCESS;
    output_token->length = 0;

    context = *context_handle_P;

    /* module activation if not already done by calling
     * globus_module_activate
     */
    
    globus_thread_once(
        &once_control,
        (void (*)(void))globus_i_gsi_gssapi_module.activation_func);

    if(req_flags & GSS_C_ANON_FLAG & GSS_C_DELEG_FLAG)
    {
        major_status = GSS_S_FAILURE;
        GLOBUS_GSI_GSSAPI_ERROR_RESULT(
            minor_status,
            GLOBUS_GSI_GSSAPI_ERROR_BAD_ARGUMENT,
            ("Can't initialize a context to be both anonymous and "
             "provide delegation"));
        goto error_exit;
    }
    
    if ((context == (gss_ctx_id_t) GSS_C_NO_CONTEXT) ||
        !(context->ctx_flags & GSS_I_CTX_INITIALIZED))
    {
        GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF(
            2, (globus_i_gsi_gssapi_debug_fstream, 
                "Creating context w/ %s.\n",
                (initiator_cred_handle == GSS_C_NO_CREDENTIAL) ?
                "GSS_C_NO_CREDENTIAL" :
                "Credentials provided"));

        major_status = 
            globus_i_gsi_gss_create_and_fill_context(&local_minor_status,
                                                     &context,
                                                     initiator_cred_handle,
                                                     GSS_C_INITIATE,
                                                     req_flags);
        if (GSS_ERROR(major_status))
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_WITH_GSS_CONTEXT);
            goto error_exit;
        }

        *context_handle_P = context;

        if (actual_mech_type != NULL)
        {
            *actual_mech_type = (gss_OID) gss_mech_globus_gssapi_openssl;
        }

        if (ret_flags != NULL)
        {
            *ret_flags = 0 ;
        }

        if (time_rec != NULL)
        {
            *time_rec = GSS_C_INDEFINITE;
        }
    }
    else
    {
        /* first time there is no input token, but after that
         * there will always be one
         */
    	major_status = globus_i_gsi_gss_put_token(&local_minor_status,
                                                  context, 
                                                  NULL, 
                                                  input_token);
    	if (GSS_ERROR(major_status))
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_TOKEN_FAIL);
            goto error_exit;
        }
    }

    switch (context->gss_state)
    {
    case(GSS_CON_ST_HANDSHAKE):
        
        /* do the handshake work */
        
        major_status = globus_i_gsi_gss_handshake(&local_minor_status,
                                                  context);
        
        if (major_status == GSS_S_CONTINUE_NEEDED)
        {
            break;
        }

        /* need to check callback data for error first */
        local_result = globus_gsi_callback_get_error(
            context->callback_data,
            &callback_error);
        if(callback_error != GLOBUS_SUCCESS)
        {
            local_result = callback_error;
        }

        if(local_result != GLOBUS_SUCCESS)
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_result,
                GLOBUS_GSI_GSSAPI_ERROR_REMOTE_CERT_VERIFY_FAILED);
            major_status = GSS_S_FAILURE;
            break;
        }

        /* if failed, may have SSL alert message too */
        if (GSS_ERROR(major_status))
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_HANDSHAKE);
            context->gss_state = GSS_CON_ST_DONE;
            break;
        } 

        /* make sure we are talking to the correct server */
        major_status = globus_i_gsi_gss_retrieve_peer(&local_minor_status,
                                                      context,
                                                      GSS_C_INITIATE);
        if (GSS_ERROR(major_status))
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_WITH_GSS_CONTEXT);
            context->gss_state = GSS_CON_ST_DONE;
            break;
        }

        local_result = globus_gsi_callback_get_proxy_type(
            context->callback_data,
            &proxy_type);
        if(local_result != GLOBUS_SUCCESS)
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_WITH_CALLBACK_DATA);
            major_status = GSS_S_FAILURE;
            goto error_exit;
        }

        /* 
         * Need to check if the server is using a limited proxy. 
         * And if that is acceptable here. 
         * Caller tells us if it is not acceptable to 
         * use a limited proxy. 
         */
        if ((context->req_flags & 
             GSS_C_GLOBUS_DONT_ACCEPT_LIMITED_PROXY_FLAG)
            && (proxy_type == GLOBUS_LIMITED_PROXY))
        {
            major_status = GSS_S_UNAUTHORIZED;
            GLOBUS_GSI_GSSAPI_ERROR_RESULT(
                minor_status,
                GLOBUS_GSI_GSSAPI_ERROR_PROXY_VIOLATION,
                ("Expected limited proxy"));
            context->gss_state = GSS_CON_ST_DONE;
            break;
        }

        /* this is the mutual authentication test */
        if (target_name != NULL)
        {
            major_status = 
                gss_compare_name(&local_minor_status,
                                 context->peer_cred_handle->globusid,
                                 target_name,
                                 &rc);
            if (GSS_ERROR(major_status))
            {
                GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                    minor_status, local_minor_status,
                    GLOBUS_GSI_GSSAPI_ERROR_BAD_NAME);
                context->gss_state = GSS_CON_ST_DONE;
                break;
            }
            else if(rc == GSS_NAMES_NOT_EQUAL)
            {
                GLOBUS_GSI_GSSAPI_ERROR_RESULT(
                    minor_status,
                    GLOBUS_GSI_GSSAPI_ERROR_BAD_NAME,
                    ("The target name in the context, and the target "
                     "name passed to the function do not match"));
                major_status = GSS_S_UNAUTHORIZED;
                context->gss_state = GSS_CON_ST_DONE;
                break;
            }
        }
    
        context->ret_flags |= GSS_C_MUTUAL_FLAG;
        context->ret_flags |= GSS_C_PROT_READY_FLAG; 
        context->ret_flags |= GSS_C_INTEG_FLAG
            | GSS_C_REPLAY_FLAG
            | GSS_C_SEQUENCE_FLAG;
        if (proxy_type == GLOBUS_LIMITED_PROXY)
        {
            context->ret_flags |= GSS_C_GLOBUS_RECEIVED_LIMITED_PROXY_FLAG;
        }

        /* 
         * IF we are talking to a real SSL server,
         * we dont want to do delegation, so we are done
         */

        if (context->req_flags & GSS_C_GLOBUS_SSL_COMPATIBLE)
        {
            context->gss_state = GSS_CON_ST_DONE;
            break;
        }
            
        /*
         * If we have completed the handshake, but dont
         * have any more data to send, we can send the flag
         * now. i.e. fall through without break,
         * Otherwise, we will wait for the null byte
         * to get back in sync which we will ignore
         */

        if (output_token->length != 0)
        {
            context->gss_state=GSS_CON_ST_FLAGS;
            break;
        }

    case(GSS_CON_ST_FLAGS):

        if (input_token->length > 0)
        {   
            BIO_read(context->gss_sslbio, cbuf, 1);
        }

        /* send D if we want delegation, 0 otherwise */
        
        if (context->req_flags & GSS_C_DELEG_FLAG)
        {
            BIO_write(context->gss_sslbio, "D", 1); 
            context->gss_state = GSS_CON_ST_REQ;
        }
        else
        {
            BIO_write(context->gss_sslbio, "0", 1);
            context->gss_state = GSS_CON_ST_DONE;
        } 
        break;
            
    case(GSS_CON_ST_REQ):

        local_result = globus_gsi_proxy_inquire_req(
            context->proxy_handle,
            context->gss_sslbio);
        if(local_result != GLOBUS_SUCCESS)
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_result,
                GLOBUS_GSI_GSSAPI_ERROR_PROXY_NOT_RECEIVED);
            major_status = GSS_S_FAILURE;
            context->gss_state = GSS_CON_ST_DONE;
            goto error_exit;
        }
        
        local_result = globus_gsi_cred_check_proxy(
            context->cred_handle->cred_handle,
            &proxy_type);
        if(local_result != GLOBUS_SUCCESS)
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_result,
                GLOBUS_GSI_GSSAPI_ERROR_WITH_GSI_CREDENTIAL);
            major_status = GSS_S_FAILURE;
            context->gss_state = GSS_CON_ST_DONE;
            goto error_exit;
        }

        if(proxy_type != GLOBUS_RESTRICTED_PROXY &&
           context->req_flags & GSS_C_GLOBUS_DELEGATE_LIMITED_PROXY_FLAG)
        {
            local_result =
                globus_gsi_proxy_handle_set_is_limited(context->proxy_handle,
                                                       GLOBUS_TRUE);
            if(local_result != GLOBUS_SUCCESS)
            {
                GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                    minor_status, local_result,
                    GLOBUS_GSI_GSSAPI_ERROR_WITH_GSI_PROXY);
                major_status = GSS_S_FAILURE;
                context->gss_state = GSS_CON_ST_DONE;
                goto exit;
            }
        }

        local_result = globus_gsi_proxy_sign_req(
            context->proxy_handle,
            context->cred_handle->cred_handle,
            context->gss_sslbio);
        if(local_result != GLOBUS_SUCCESS)
        {
            GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
                minor_status, local_result,
                GLOBUS_GSI_GSSAPI_ERROR_WITH_GSI_PROXY);
            major_status = GSS_S_FAILURE;
            context->gss_state = GSS_CON_ST_DONE;
            goto error_exit;
        }

        context->gss_state = GSS_CON_ST_DONE;
        break;
            
    case(GSS_CON_ST_CERT): ;
    case(GSS_CON_ST_DONE): ;
    } /* end of switch for gss_con_st */

    local_major_status = globus_i_gsi_gss_get_token(&local_minor_status,
                                                    context, 
                                                    NULL, 
                                                    output_token);

    if(GSS_ERROR(local_major_status))
    {
        GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(
            minor_status, local_minor_status,
            GLOBUS_GSI_GSSAPI_ERROR_TOKEN_FAIL);
        major_status = GSS_S_FAILURE;
        context->gss_state = GSS_CON_ST_DONE;
        goto error_exit;
    }

    /* some error occurred during switch */
    if(GSS_ERROR(major_status))
    {
        goto error_exit;
    }

    if (context->gss_state != GSS_CON_ST_DONE)
    {
        major_status |= GSS_S_CONTINUE_NEEDED;
    }
       
    if (ret_flags != NULL)
    {
        *ret_flags = context->ret_flags;
    }

    GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF(
        2, (globus_i_gsi_gssapi_debug_fstream,
            "init_sec_context:major_status:%08x"
            ":gss_state:%d req_flags=%08x:ret_flags=%08x\n",
            (unsigned int) major_status, 
            context->gss_state,
            (unsigned int) req_flags, 
            (unsigned int) context->ret_flags));

    goto exit;

 error_exit:

    gss_delete_sec_context(&local_minor_status, 
                           (gss_ctx_id_t *) &context,
                           output_token);
    *context_handle_P = (gss_ctx_id_t) context;
 
 exit:

    GLOBUS_I_GSI_GSSAPI_DEBUG_EXIT;
    return major_status;
}
/* @} */

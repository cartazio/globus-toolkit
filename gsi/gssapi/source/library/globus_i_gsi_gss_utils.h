#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL
/**
 * @file globus_i_gsi_gss_utils.c
 * @author Sam Lang, Sam Meder
 * 
 * $RCSfile$
 * $Revision$
 * $Date$
 */
#endif

#ifndef GLOBUS_I_GSI_GSS_UTILS_H
#define GLOBUS_I_GSI_GSS_UTILS_H

#include "gssapi.h"
#include "gssapi_openssl.h"

/* ERROR MACROS */

#define GLOBUS_GSI_GSSAPI_ERROR_RESULT(_MIN_RESULT_, _MIN_,             \
                                       _ERRSTR_)                        \
    {                                                                   \
         char *                         tmpstr =                        \
             globus_gsi_cert_utils_create_string _ERRSTR_;              \
         *_MIN_RESULT_ = (OM_uint32) globus_i_gsi_gssapi_error_result(  \
             _MIN_, __FILE__, _function_name_,                          \
             __LINE__, tmpstr);                                         \
         globus_libc_free(tmpstr);                                      \
    }

#define GLOBUS_GSI_GSSAPI_OPENSSL_ERROR_RESULT(_MIN_RESULT_,            \
                                               _ERRORTYPE_, _ERRORSTR_) \
    {                                                                   \
         char *                         tmpstr =                        \
             globus_gsi_cert_utils_create_string _ERRORSTR_;            \
         *_MIN_RESULT_ =                                                \
             (OM_uint32) globus_i_gsi_gssapi_openssl_error_result(      \
             _ERRORTYPE_, __FILE__, _function_name_, __LINE__, tmpstr); \
         globus_libc_free(tmpstr);                                      \
    }

#define GLOBUS_GSI_GSSAPI_ERROR_CHAIN_RESULT(_MIN_RESULT_, _TOP_RESULT_, \
                                             _ERRORTYPE_)                \
    *_MIN_RESULT_ = (OM_uint32) globus_i_gsi_gssapi_error_chain_result(  \
                                 (globus_result_t)_TOP_RESULT_,          \
                                 _ERRORTYPE_, __FILE__,                  \
                                 _function_name_, __LINE__, NULL)

#define GLOBUS_GSI_GSSAPI_MALLOC_ERROR(_MIN_RESULT_)       \
    {                                                      \
        char *                          _tmp_str_ =        \
        globus_l_gsi_gssapi_error_strings[                 \
            GLOBUS_GSI_GSSAPI_ERROR_OUT_OF_MEMORY];        \
        *_MIN_RESULT_ = (OM_uint32) globus_error_put(      \
            globus_error_wrap_errno_error(                 \
                GLOBUS_GSI_GSSAPI_MODULE,                  \
                errno,                                     \
                GLOBUS_GSI_GSSAPI_ERROR_OUT_OF_MEMORY,     \
                "%s:%d: %s: %s",                           \
                __FILE__, __LINE__, _function_name_,       \
                _tmp_str_));                               \
        globus_libc_free(_tmp_str_);                       \
    }

/* DEBUG MACROS */

#ifdef BUILD_DEBUG

extern int                              globus_i_gsi_gssapi_debug_level;
extern FILE *                           globus_i_gsi_gssapi_debug_fstream;

#define GLOBUS_I_GSI_GSSAPI_DEBUG(_LEVEL_) \
    (globus_i_gsi_gssapi_debug_level >= (_LEVEL_))

#define GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF(_LEVEL_, _MESSAGE_) \
{                                                             \
    if (GLOBUS_I_GSI_GSSAPI_DEBUG(_LEVEL_))                   \
    {                                                         \
        globus_libc_fprintf _MESSAGE_;                        \
    }                                                         \
} 

#define GLOBUS_I_GSI_GSSAPI_DEBUG_FNPRINTF(_LEVEL_, _MESSAGE_) \
{ \
        if (GLOBUS_I_GSI_GSSAPI_DEBUG(_LEVEL_)) \
        { \
           char *                       _tmp_str_ = \
               globus_gsi_cert_utils_create_nstring _MESSAGE_; \
           globus_libc_fprintf(globus_i_gsi_gssapi_debug_fstream, \
                               _tmp_str_); \
           globus_libc_free(_tmp_str_); \
        } \
}

#define GLOBUS_I_GSI_GSSAPI_DEBUG_PRINT(_LEVEL_, _MESSAGE_) \
{                                                           \
    if (GLOBUS_I_GSI_GSSAPI_DEBUG(_LEVEL_))                 \
    {                                                       \
        globus_libc_fprintf(                                \
            globus_i_gsi_gssapi_debug_fstream,              \
            _MESSAGE_);                                     \
    }                                                       \
}
 
#define GLOBUS_I_GSI_GSSAPI_DEBUG_PRINT_OBJECT(_LEVEL_, _TYPE_, _OBJ_) \
{                                                                      \
    if (GLOBUS_I_GSI_GSSAPI_DEBUG(_LEVEL_))                            \
    {                                                                  \
        _TYPE_##_print_fp(                                             \
            globus_i_gsi_gssapi_debug_fstream,                         \
            _OBJ_);                                                    \
    }                                                                  \
}

#else

#define GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF(_LEVEL_, _MESSAGE_)
#define GLOBUS_I_GSI_GSSAPI_DEBUG_FNPRINTF(_LEVEL_, _MESSAGE_)
#define GLOBUS_I_GSI_GSSAPI_DEBUG_PRINT(_LEVEL_, _MESSAGE_)
#define GLOBUS_I_GSI_GSSAPI_DEBUG_PRINT_OBJECT(_TYPE_, _OBJ_)

#endif

#define GLOBUS_I_GSI_GSSAPI_DEBUG_ENTER \
            GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF( \
                1, (globus_i_gsi_gssapi_debug_fstream, \
                    "%s entering\n", _function_name_))

#define GLOBUS_I_GSI_GSSAPI_DEBUG_EXIT \
            GLOBUS_I_GSI_GSSAPI_DEBUG_FPRINTF( \
                1, (globus_i_gsi_gssapi_debug_fstream, \
                    "%s exiting\n", _function_name_))

typedef enum {
    
    GLOBUS_I_GSI_GSS_DEFAULT_CONTEXT,
    GLOBUS_I_GSI_GSS_ANON_CONTEXT
} globus_i_gsi_gss_context_type_t;

OM_uint32
globus_i_gsi_gss_copy_name_to_name(
    OM_uint32 *                         minor_status,
    gss_name_desc **                    output,
    const gss_name_desc *               input);

OM_uint32
globus_i_gsi_gss_create_and_fill_context(
    OM_uint32 *                         minor_status,
    gss_ctx_id_desc **                  context_handle,
    gss_cred_id_desc *                  cred_handle,
    const gss_cred_usage_t              cred_usage,
    OM_uint32                           req_flags);

OM_uint32
globus_i_gsi_gss_create_anonymous_cred(
    OM_uint32 *                         minor_status,
    gss_cred_id_t *                     output_cred_handle,
    const gss_cred_usage_t              cred_usage);

OM_uint32
globus_i_gsi_gss_cred_read_bio(
    OM_uint32 *                         minor_status,
    const gss_cred_usage_t              cred_usage,
    gss_cred_id_t *                     cred_id_handle,
    BIO *                               bp);

OM_uint32
globus_i_gsi_gss_cred_read(
    OM_uint32 *                         minor_status,
    const gss_cred_usage_t              cred_usage,
    gss_cred_id_t *                     cred_handle,
    const char *                        desired_subject);

OM_uint32
globus_i_gsi_gss_cred_set(
    OM_uint32 *                         minor_status,
    const gss_cred_usage_t              cred_usage,
    gss_cred_id_t *                     cred_handle,
    X509 *                              ucert,
    EVP_PKEY *                          upkey,
    STACK_OF(X509) *                    cert_chain);

OM_uint32
globus_i_gsi_gss_create_cred(
    OM_uint32 *                         minor_status,
    const gss_cred_usage_t              cred_usage,
    gss_cred_id_t *                     output_cred_handle_P,
    globus_gsi_cred_handle_t            cred_handle);

int globus_i_gsi_gss_verify_extensions_callback(
    globus_gsi_callback_data_t          callback_data,
    X509_EXTENSION *                    extension);

OM_uint32
globus_i_gsi_gss_handshake(
    OM_uint32 *                         minor_status,
    gss_ctx_id_desc *                   context_handle);

OM_uint32
globus_i_gsi_gss_get_token(
    OM_uint32 *                         minor_status,
    const gss_ctx_id_desc *             context_handle,
    BIO *                               bio,
    const gss_buffer_t                  output_token);

OM_uint32
globus_i_gsi_gss_put_token(
    OM_uint32 *                         minor_status,
    const gss_ctx_id_desc *             context_handle,
    BIO *                               bio,
    const gss_buffer_t                  input_token);

OM_uint32
globus_i_gsi_gss_retrieve_peer(
    OM_uint32 *                         minor_status,
    gss_ctx_id_desc *                   context_handle,
    const gss_cred_usage_t              cred_usage);

OM_uint32
globus_i_gsi_gss_SSL_write_bio(
    OM_uint32 *                         minor_status,
    gss_ctx_id_desc *                   context,
    BIO *                               bp);

OM_uint32
globus_i_gsi_gss_SSL_read_bio(
    OM_uint32 *                         minor_status,
    gss_ctx_id_desc *                   context,
    BIO *                               bp);

OM_uint32
globus_i_gsi_gss_get_context_goodtill(
    OM_uint32 *                         minor_status,
    gss_ctx_id_t                        context,
    time_t *                            goodtill);

OM_uint32
globus_i_gsi_gssapi_init_ssl_context(
    OM_uint32 *                         minor_status,
    gss_cred_id_t                       credential,
    globus_i_gsi_gss_context_type_t     anon_ctx);

globus_result_t
globus_i_gsi_gssapi_openssl_error_result(
    int                                 error_type,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        long_desc);

globus_result_t
globus_i_gsi_gssapi_error_result(
    const OM_uint32                     minor_status,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        long_desc);

globus_result_t
globus_i_gsi_gssapi_error_chain_result(
    globus_result_t                     chain_result,
    int                                 error_type,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        long_desc);

#endif /* GLOBUS_I_GSI_GSS_UTILS_H */

#include "config.h"
#include "globus_common.h"
#include "globus_object_cache.h"
#include "version.h"
#include <string.h>

/**********************************************************************
 * Error Types
 *   globus_error_type_t          --   used in error API
 *   globus_error_type_object_t   --   used to implement new error types
 *   globus_error_t               --   used in most APIs
 *   globus_error_object_t        --   used internally
 **********************************************************************/

static char *
s_string_copy (char * string)
{
  char * ns;
  int i, l;

  if ( string == NULL ) return NULL;

  l = strlen (string);

  ns = globus_malloc (sizeof(char *) * (l + 1));
  if ( ns == NULL ) return NULL;

  for (i=0; i<l; i++) {
    ns[i] = string[i];
  }
  ns[l] = '\00';

  return ns;
}

/* default error strings for all types in the error hierarchy,
 * which should be overridden w/ special code over time
 */
char * 
globus_error_generic_string_func (globus_object_t * error)
{
  char * string;
  const globus_object_type_t * type;

  type = globus_object_get_type (error);

  if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NO_CREDENTIALS) 
       == GLOBUS_TRUE ) {
    string = "no credentials were available";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NO_TRUST) 
       == GLOBUS_TRUE ) {
    string = "no trust relationship exists";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_INVALID_CREDENTIALS) 
       == GLOBUS_TRUE ) {
    string = "the credentials were invalid";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_NO_AUTHENTICATION) 
       == GLOBUS_TRUE ) {
    string = "authentication failed";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NO_AUTHORIZATION) 
       == GLOBUS_TRUE ) {
    string = "the operation was not authorized";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_OFFLINE) 
       == GLOBUS_TRUE ) {
    string = "the resource was offline";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_QUOTA_DEPLETED) 
       == GLOBUS_TRUE ) {
    string = "the resource quota was depleted";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_DEPLETED) 
       == GLOBUS_TRUE ) {
    string = "the resource was depleted";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NOT_AVAILABLE) 
       == GLOBUS_TRUE ) {
    string = "the resource was not available";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_ACCESS_FAILED) 
       == GLOBUS_TRUE ) {
    string = "access failed";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_USER_CANCELLED) 
	    == GLOBUS_TRUE ) {
    string = "the operation was cancelled by the user";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_INTERNAL_ERROR) 
	    == GLOBUS_TRUE ) {
    string = "the operation was aborted due to an internal error";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_SYSTEM_ABORTED) 
	    == GLOBUS_TRUE ) {
    string = "the operation was aborted by the system";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_ABORTED) 
	    == GLOBUS_TRUE ) {
    string = "the operation was aborted";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NULL_REFERENCE)
	    == GLOBUS_TRUE ) {
    string = "a NULL reference was encountered";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_TYPE_MISMATCH)
	    == GLOBUS_TRUE ) {
    string = "the data was not of the required type";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NAME_UNKNOWN) 
       == GLOBUS_TRUE ) {
    string = "an unknown resource was encountered";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_BAD_FORMAT)
	    == GLOBUS_TRUE ) {
    string = "badly formatted data was encountered";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_TOO_LARGE)
	    == GLOBUS_TRUE ) {
    string = "the data was too large";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_TOO_SMALL)
	    == GLOBUS_TRUE ) {
    string = "the data was too small";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_OUT_OF_RANGE)
	    == GLOBUS_TRUE ) {
    string = "out-of-range data was encountered";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_BAD_DATA)
	    == GLOBUS_TRUE ) {
    string = "bad data was encountered";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_UNREACHABLE)
	    == GLOBUS_TRUE ) {
    string = "the destination was unreachable";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_PROTOCOL_MISMATCH)
	    == GLOBUS_TRUE ) {
    string = "no common protocol could be negotiated";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_PROTOCOL_VIOLATED)
	    == GLOBUS_TRUE ) {
    string = "the protocol was violated";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_COMMUNICATION_FAILED)
	    == GLOBUS_TRUE ) {
    string = "communication failed";
  }
  else if ( globus_object_type_match (type, 
				      GLOBUS_ERROR_TYPE_ALREADY_REGISTERED)
	    == GLOBUS_TRUE ) {
    string = "the resource is already registered";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_ALREADY_CANCELLED)
	    == GLOBUS_TRUE ) {
    string = "a cancel was already issued";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_ALREADY_DONE)
	    == GLOBUS_TRUE ) {
    string = "the operation was already performed";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_NOT_INITIALIZED)
	    == GLOBUS_TRUE ) {
    string = "the mechanism was not initialized";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_INVALID_USE)
	    == GLOBUS_TRUE ) {
    string = "the use was invalid";
  }
  else if ( globus_object_type_match (type, GLOBUS_ERROR_TYPE_BASE)
	    == GLOBUS_TRUE ) {
    string = "an unknown error occurred";
  }
  else {
    string = NULL;
  }

  return s_string_copy (string);
}

/**********************************************************************
 * Error Creation API
 **********************************************************************/

globus_object_t *
globus_error_initialize_base (globus_object_t *            error,
			      globus_module_descriptor_t * source_module,
			      globus_object_t *            causal_error)
{
  if ( (error == NULL) ||
       (globus_object_is_static (error) 
	== GLOBUS_TRUE) ||
       (globus_object_type_match (globus_object_get_type(error),
				  GLOBUS_ERROR_TYPE_BASE)
       != GLOBUS_TRUE) ) {
    return NULL;
  }

  globus_error_base_set_source (error, source_module);
  globus_error_base_set_cause (error, causal_error);

  return error;
}

globus_object_t *
globus_error_construct_base (globus_module_descriptor_t * source_module,
			     globus_object_t *            causal_error)
{
  globus_object_t * newerror;
  globus_object_t * initerror;

  newerror = globus_object_construct (GLOBUS_ERROR_TYPE_BASE);
  initerror = globus_error_initialize_base (newerror,
					    source_module, causal_error);

  if ( initerror == NULL ) {
    if ( newerror != NULL ) {
      globus_object_free (newerror);
    }
  }

  return initerror;
}


/**********************************************************************
 * Error Management API
 **********************************************************************/

static globus_object_cache_t s_result_to_object_mapper;
static uint32_t              s_next_available_result_count;
static globus_mutex_t        s_result_to_object_mutex;

static int  s_error_cache_initialized = 0;

static int s_error_cache_init ()
{
  globus_object_cache_init (&s_result_to_object_mapper);
  globus_mutex_init (&s_result_to_object_mutex, NULL);
  s_next_available_result_count = 1;
  s_error_cache_initialized = 1;
  return GLOBUS_SUCCESS;
}

static int s_error_cache_destroy ()
{
  globus_object_cache_destroy (&s_result_to_object_mapper);
  globus_mutex_destroy (&s_result_to_object_mutex);
  s_error_cache_initialized = 0;
  return GLOBUS_SUCCESS;
}


globus_object_t *
globus_error_get (globus_result_t result)
{
  globus_object_t * error;
  int err;

  if (! s_error_cache_initialized ) return NULL;

  if ( result == GLOBUS_SUCCESS ) return NULL;

  err = globus_mutex_lock (&s_result_to_object_mutex);
  if (err) return NULL;

  error = globus_object_cache_remove (&s_result_to_object_mapper,
				      result);

  globus_mutex_unlock (&s_result_to_object_mutex);

  if (error!=NULL) 
    return error;
  else
    return GLOBUS_ERROR_NO_INFO;
}

globus_result_t
globus_error_put (globus_object_t * error)
{
  void * new_result;
  int err;

  if (! s_error_cache_initialized ) return GLOBUS_SUCCESS;
  err = globus_mutex_lock (&s_result_to_object_mutex);
  if (err) return NULL;

  if ( globus_object_type_match (globus_object_get_type(error),
				 GLOBUS_ERROR_TYPE_BASE)
       != GLOBUS_TRUE ) {
    error = GLOBUS_ERROR_NO_INFO;
  }

  new_result = (void *) s_next_available_result_count;
  s_next_available_result_count += 1;

  globus_object_cache_insert (&s_result_to_object_mapper,
			      new_result, error);

  globus_mutex_unlock (&s_result_to_object_mutex);

  return new_result;
}

globus_module_descriptor_t globus_i_error_module =
{
  "globus_error",
  s_error_cache_init,
  s_error_cache_destroy,
  GLOBUS_NULL,
  GLOBUS_NULL,
  &local_version
};


/**********************************************************************
 * Error Manipulation API
 **********************************************************************/

typedef struct globus_error_base_instance_s {
  globus_module_descriptor_t * source_module;
  globus_object_t *            causal_error;
} globus_error_base_instance_t;

static globus_error_base_instance_t *
s_base_instance_data (globus_object_t * error)
{
  void * instance_data_vp;
  globus_object_t * base;
  globus_error_base_instance_t * instance_data;

  base = globus_object_upcast (error, GLOBUS_ERROR_TYPE_BASE);

  instance_data_vp 
    = globus_object_get_local_instance_data (base);
  
  instance_data = (globus_error_base_instance_t *) instance_data_vp;

  if ( instance_data != NULL ) {
    return instance_data;
  }
  else {
    instance_data = globus_malloc (sizeof(globus_error_base_instance_t));
    if (instance_data != NULL) {
      instance_data->source_module = NULL;
      instance_data->causal_error = NULL;
      
      globus_object_set_local_instance_data (base,
					     (void *) instance_data);
    }
    
    return instance_data;
  }
}

static void
s_base_instance_copy (void *  instance_datavp,
		      void ** copyvp)
{
  globus_error_base_instance_t * instance_data;
  globus_error_base_instance_t * copy;

  instance_data = ((globus_error_base_instance_t *) instance_datavp);

  if (copyvp!=NULL) {
    if (instance_datavp==NULL) {
      (*copyvp) = (void *) NULL;
      return;
    }

    copy = globus_malloc (sizeof(globus_error_base_instance_t));
    if (copy!=NULL) {
      if (instance_data!=NULL) {
	copy->source_module = instance_data->source_module;
	copy->causal_error = globus_object_copy(instance_data->causal_error);
      }
    }
    (*copyvp) = (void *) copy;
  }
}

static void
s_base_instance_destructor (void *instance_datavp)
{
  globus_error_base_instance_t * instance_data;

  instance_data = ((globus_error_base_instance_t *) instance_datavp);

  if ( instance_data!=NULL ) {
    globus_object_free (instance_data->causal_error);
    globus_free (instance_data);
  }
}

globus_module_descriptor_t *
globus_error_base_get_source (globus_object_t * error)
{
  globus_error_base_instance_t * instance_data;

  instance_data = s_base_instance_data (error);

  if ( instance_data != NULL ) {
    return instance_data->source_module;
  }
  else return NULL;
}

void
globus_error_base_set_source (globus_object_t *            error,
			 globus_module_descriptor_t * source_module)
{
  globus_error_base_instance_t * instance_data;

  instance_data = s_base_instance_data (error);

  if ( instance_data != NULL ) {
    instance_data->source_module = source_module;
  }
}

extern globus_object_t *
globus_error_base_get_cause (globus_object_t *error)
{
  globus_error_base_instance_t * instance_data;

  instance_data = s_base_instance_data (error);

  if ( instance_data != NULL ) {
    return instance_data->causal_error;
  }
  else return NULL;
}

extern void
globus_error_base_set_cause (globus_object_t * error,
			globus_object_t * causal_error)
{
  globus_error_base_instance_t * instance_data;

  instance_data = s_base_instance_data (error);

  if ( instance_data != NULL ) {
    instance_data->causal_error = causal_error;
  }
}


/**********************************************************************
 * Standard Error Type
 * the entire error hierarchy lives under ERROR_TYPE_BASE, a
 * direct child of OBJECT_TYPE_PRINTABLE.
 **********************************************************************/

const globus_object_type_t GLOBUS_ERROR_TYPE_BASE_DEFINITION
= globus_error_type_static_initializer (GLOBUS_OBJECT_TYPE_PRINTABLE,
					s_base_instance_copy,
					s_base_instance_destructor,
					globus_error_generic_string_func);

/**********************************************************************
 * Standard Error Prototype
 **********************************************************************/

globus_object_t GLOBUS_ERROR_BASE_STATIC_PROTOTYPE
= globus_object_static_initializer ((&GLOBUS_ERROR_TYPE_BASE_DEFINITION),
			    (&GLOBUS_OBJECT_PRINTABLE_STATIC_PROTOTYPE));


#if 0
/**********************************************************************
 * Error Callback API
 **********************************************************************/

extern globus_error_t
globus_result_callback_register (globus_module_descriptor_t * source,
				 globu_result_callback_func_t callback,
				 void *                       user_data,
				 long *                       registered_id);

extern globus_error_t 
globus_result_callback_unregister (long registered_id);

extern void
globus_result_signal_fault (globus_module_descriptor_t * source,
			    globus_result_t              fault);

#endif /* 0 */




#include "globus_xio_driver.h"
#include "globus_xio_load.h"
#include "globus_i_xio.h"
#include "globus_common.h"
#include "globus_xio_queue.h"

#define GLOBUS_XIO_QUEUE_DRIVER_MODULE &globus_i_xio_queue

static int
globus_l_xio_queue_activate();

static int
globus_l_xio_queue_deactivate();

#include "version.h"

static globus_module_descriptor_t  globus_i_xio_queue_module =
{
    "globus_xio_queue",
    globus_l_xio_queue_activate,
    globus_l_xio_queue_deactivate,
    GLOBUS_NULL,
    GLOBUS_NULL,
    &local_version
};

typedef struct globus_xio_driver_queue_entry_s
{
    globus_xio_iovec_t *                iovec;
    int                                 iovec_count;
    globus_xio_operation_t              op;
    globus_size_t                       wait_for;
} globus_xio_driver_queue_entry_t;

typedef struct globus_xio_driver_queue_handle_s
{
    globus_bool_t                       outstanding_read;
    globus_bool_t                       outstanding_write;
    globus_fifo_t                       read_q;
    globus_fifo_t                       write_q;
    globus_xio_context_t                context;
    globus_mutex_t                      mutex;
} globus_xio_driver_queue_handle_t;


globus_xio_driver_queue_handle_t *
globus_l_xiod_q_handle_create()
{
    globus_xio_driver_queue_handle_t *  handle;

    handle = (globus_xio_driver_queue_handle_t *) globus_malloc(
        sizeof(globus_xio_driver_queue_handle_t));
    globus_fifo_init(&handle->read_q);
    globus_fifo_init(&handle->write_q);
    globus_mutex_init(&handle->mutex, NULL);

    handle->outstanding_read = GLOBUS_FALSE;
    handle->outstanding_write = GLOBUS_FALSE;

    return handle;
}

void
globus_l_xiod_q_handle_destroy(
    globus_xio_driver_queue_handle_t *  handle)
{
    globus_fifo_destroy(&handle->read_q);
    globus_fifo_destroy(&handle->write_q);
    globus_mutex_destroy(&handle->mutex);
}

/*
 *  open
 */
void
globus_l_xio_queue_open_cb(
    globus_xio_operation_t              op,
    globus_result_t                     result,
    void *                              user_arg)
{
    globus_xio_driver_queue_handle_t *  handle;
    globus_result_t                     res;

    handle = (globus_xio_driver_queue_handle_t *) user_arg;

    if(res != GLOBUS_SUCCESS)
    {
        globus_l_xiod_q_handle_destroy(handle);
        handle = NULL;
    }

    GlobusXIODriverFinishedOpen(handle->context, handle, op, result);
}   

static
globus_result_t
globus_l_xio_queue_open(
    void *                              driver_target,
    void *                              driver_attr,
    globus_xio_operation_t              op)
{
    globus_result_t                     res;
    globus_xio_driver_queue_handle_t *  handle;

    handle = globus_l_xiod_q_handle_create();

    GlobusXIODriverPassOpen(res, handle->context, op, \
        globus_l_xio_queue_open_cb, handle);

    return res;
}

static
globus_result_t
globus_l_xio_queue_close(
    void *                              driver_handle,
    void *                              attr,
    globus_xio_context_t                context,
    globus_xio_operation_t              op)
{
    globus_result_t                     res;
    globus_xio_driver_queue_handle_t *  handle;

    handle = (globus_xio_driver_queue_handle_t *) driver_handle;

    globus_l_xiod_q_handle_destroy(handle);

    GlobusXIODriverPassClose(res, op, NULL, NULL);

    return res;
}

/*
 *  read
 */
void
globus_l_xio_queue_read_cb(
    globus_xio_operation_t              op,
    globus_result_t                     result,
    globus_size_t                       nbytes,
    void *                              user_arg)
{
    globus_xio_driver_queue_handle_t *  handle;
    globus_bool_t                       done = GLOBUS_FALSE;
    globus_xio_driver_queue_entry_t *   entry;
    globus_result_t                     res;
    
    handle = (globus_xio_driver_queue_handle_t *) user_arg;

    globus_mutex_lock(&handle->mutex);
    {
        res = result;
        while(!done)
        {
            GlobusXIODriverFinishedRead(op, res, nbytes);
            if(globus_fifo_empty(&handle->read_q))
            {
                handle->outstanding_read = GLOBUS_FALSE;
                done = GLOBUS_TRUE;
            }
            else
            {
                entry = (globus_xio_driver_queue_entry_t *)
                    globus_fifo_dequeue(&handle->read_q);
                globus_assert(entry != NULL);

                GlobusXIODriverPassRead(
                    res, 
                    entry->op, 
                    entry->iovec,
                    entry->iovec_count, 
                    entry->wait_for,
                    globus_l_xio_queue_read_cb, 
                    handle);
                if(res == GLOBUS_SUCCESS)
                {
                    done = GLOBUS_TRUE;
                }
                else
                {
                    nbytes = 0;
                    op = entry->op;
                    globus_free(entry);
                }
            }
        }
    }
    globus_mutex_unlock(&handle->mutex);
}

static globus_result_t
globus_l_xio_queue_read(
    void *                              driver_handle,
    const globus_xio_iovec_t *          iovec,
    int                                 iovec_count,
    globus_xio_operation_t              op)
{
    globus_result_t                     res;
    globus_xio_driver_queue_entry_t *   entry;
    globus_size_t                       wait_for;
    globus_xio_driver_queue_handle_t *  handle;
    GlobusXIOName(globus_l_xio_queue_read);

    handle = (globus_xio_driver_queue_handle_t *) driver_handle;

    wait_for = GlobusXIOOperationGetWaitFor(op);

    globus_mutex_lock(&handle->mutex);
    {
        if(handle->outstanding_read)
        {
            entry = globus_malloc(sizeof(globus_xio_driver_queue_entry_t));
            if(entry == NULL)
            {
                res = GlobusXIOErrorMemory("entry");
            }
            else
            {
                entry->wait_for = wait_for;
                entry->iovec = (globus_xio_iovec_t *)iovec;
                entry->iovec_count = iovec_count;
                entry->op = op;
                globus_fifo_enqueue(&handle->read_q, entry);
            }
        }
        else
        {
            handle->outstanding_read = GLOBUS_TRUE;
            GlobusXIODriverPassRead(
                res, 
                op, 
                (globus_xio_iovec_t *)iovec, 
                iovec_count, 
                wait_for,
                globus_l_xio_queue_read_cb, 
                handle);
        }
    }
    globus_mutex_unlock(&handle->mutex);

    return res;
}

/*
 *  write
 */
void
globus_l_xio_queue_write_cb(
    globus_xio_operation_t              op,
    globus_result_t                     result,
    globus_size_t                       nbytes,
    void *                              user_arg)
{
    globus_xio_driver_queue_handle_t *  handle;
    globus_bool_t                       done = GLOBUS_FALSE;
    globus_xio_driver_queue_entry_t *   entry;
    globus_result_t                     res;
    
    handle = (globus_xio_driver_queue_handle_t *) user_arg;

    globus_mutex_lock(&handle->mutex);
    {
        res = result;
        while(!done)
        {
            GlobusXIODriverFinishedWrite(op, res, nbytes);
            if(globus_fifo_empty(&handle->write_q))
            {
                handle->outstanding_write = GLOBUS_FALSE;
                done = GLOBUS_TRUE;
            }
            else
            {
                entry = (globus_xio_driver_queue_entry_t *)
                    globus_fifo_dequeue(&handle->write_q);
                globus_assert(entry != NULL);

                GlobusXIODriverPassWrite(
                    res, 
                    entry->op, 
                    entry->iovec, 
                    entry->iovec_count, 
                    entry->wait_for,
                    globus_l_xio_queue_read_cb, 
                    handle);
                if(res == GLOBUS_SUCCESS)
                {
                    done = GLOBUS_TRUE;
                }
                else
                {
                    nbytes = 0;
                    op = entry->op;
                    globus_free(entry);
                }
            }
        }
    }
    globus_mutex_unlock(&handle->mutex);
}

static globus_result_t
globus_l_xio_queue_write(
    void *                              driver_handle,
    const globus_xio_iovec_t *          iovec,
    int                                 iovec_count,
    globus_xio_operation_t              op)
{
    globus_result_t                     res;
    globus_xio_driver_queue_entry_t *   entry;
    globus_size_t                       wait_for;
    globus_xio_driver_queue_handle_t *  handle;
    GlobusXIOName(globus_l_xio_queue_write);

    handle = (globus_xio_driver_queue_handle_t *) driver_handle;
    wait_for = GlobusXIOOperationGetWaitFor(op);

    globus_mutex_lock(&handle->mutex);
    {
        if(handle->outstanding_write)
        {
            entry = globus_malloc(sizeof(globus_xio_driver_queue_entry_t));
            if(entry == NULL)
            {
                res = GlobusXIOErrorMemory("entry");
            }
            else
            {
                entry->wait_for = wait_for;
                entry->iovec = (globus_xio_iovec_t *)iovec;
                entry->iovec_count = iovec_count;
                entry->op = op;
                globus_fifo_enqueue(&handle->write_q, entry);
            }
        }
        else
        {
            handle->outstanding_write = GLOBUS_TRUE;
            GlobusXIODriverPassWrite(
                res, 
                op, 
                (globus_xio_iovec_t *)iovec, 
                iovec_count, 
                wait_for,
                globus_l_xio_queue_write_cb, handle);
        }
    }
    globus_mutex_unlock(&handle->mutex);

    return res;
}

static globus_result_t
globus_l_xio_queue_load(
    globus_xio_driver_t *               out_driver,
    va_list                             ap)
{
    globus_xio_driver_t                 driver;
    globus_result_t                     res;

    res = globus_xio_driver_init(&driver, "queue", NULL);
    if(res != GLOBUS_SUCCESS)
    {
        return res;
    }

    globus_xio_driver_set_transform(
        driver,
        globus_l_xio_queue_open,
        globus_l_xio_queue_close,
        globus_l_xio_queue_read,
        globus_l_xio_queue_write,
        NULL);

    *out_driver = driver;

    return GLOBUS_SUCCESS;
}

static void
globus_l_xio_queue_unload(
    globus_xio_driver_t                 driver)
{
    globus_xio_driver_destroy(driver);
}


static
int
globus_l_xio_queue_activate(void)
{
    int                                 rc;

    rc = globus_module_activate(GLOBUS_COMMON_MODULE);

    return rc;
}

static
int
globus_l_xio_queue_deactivate(void)
{
    return globus_module_deactivate(GLOBUS_COMMON_MODULE);
}

GlobusXIODefineDriver(
    queue,
    &globus_i_xio_queue_module,
    globus_l_xio_queue_load,
    globus_l_xio_queue_unload);

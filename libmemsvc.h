#ifndef __MEMSVC_H
#define __MEMSVC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct memsvc_handle;
typedef struct memsvc_handle *memsvc_handle_t;

/* These functions return -1 on error, and 0 on success.
 *
 * === WARNING: ===
 * A handle will be returned either way!
 * If memsvc_open() returns -1, it will be valid only for memsvc_get_last_error().
 * You are responsible for freeing it with memsvc_close()
 *
 * === SIGNALS: ===
 * memsvc_open() must not be called concurrently with sigaction() or signal()
 * operating on SIGBUS.  memsvc_open() will install a SIGBUS handler if there
 * is none present in order to detect and gracefully handle failures on the AXI
 * bus.  If you install a SIGBUS handler, you are responsible for recovering
 * from such failures on your own.  Your read or write will be incomplete, but
 * the internal library datastructures will be consistent.
 *
 * === THREADS: ===
 * A given memsvc_handle_t should be used only from one thread concurrently,
 * but there are no other restrictions on thread safety.
 */
int memsvc_open(memsvc_handle_t *handle);
int memsvc_close(memsvc_handle_t *handle);

/* Returns a string describing the last error encountered by the library.
 *
 * This string is stored in the memsvc_handle_t structure, and is invalid after
 * memsvc_close().  Do not expect it to remain constant if you call another
 * memsvc_*() function.  You do not need to free() this yourself.
 */
const char *memsvc_get_last_error(memsvc_handle_t handle);


/* These functions return -1 on error and 0 on success.
 *
 * On error, the error message will be available via memsvc_get_last_error()
 */
#define MEMSVC_MAX_WORDS	0x3FFFFFFF
int memsvc_read(memsvc_handle_t handle, uint32_t addr, uint32_t words, uint32_t *data);
int memsvc_read_noaddrinc(memsvc_handle_t handle, uint32_t addr, uint32_t words, uint32_t *data);

int memsvc_write(memsvc_handle_t handle, uint32_t addr, uint32_t words, const uint32_t *data);
int memsvc_write_noaddrinc(memsvc_handle_t handle, uint32_t addr, uint32_t words, const uint32_t *data);

#ifdef __cplusplus
}
#endif

#endif

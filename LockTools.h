#ifndef __LOCKTOOLS_H
#define __LOCKTOOLS_H

#include <string>

/* These functions provide named mutex-style locks.
 * You can use these to provide synchronization between clients.
 * Standard mutexes will not serve this purpose, as clients are forked, not threaded.
 */

/* Return:
 *   -1 on error
 *   lockid>=0 on success
 */
int namedlock_init(std::string module, std::string name);

/* Return:
 *   0 on success
 */
int namedlock_destroy(int lockid);

/* Return:
 *  On success: Returns 0
 *  On error: Returns -1 and sets errno.
 *  On already-locked: Returns -1 and sets errno = EWOULDBLOCK  (relevant only to trylock)
 */
int namedlock_lock(int lockid);
int namedlock_trylock(int lockid);
int namedlock_unlock(int lockid);



/* These functions act as flags for long term lockouts.
 *
 * They are NOT mutexes!  They do NOT provide mutual exclusion!
 *
 * You don't hold ownership of these, and if you crash, nothing will clear them for you.
 */

/* Return:
 *  On success: 0
 *  On error: -1
 */
int namedflag_set(std::string module, std::string name);
int namedflag_clear(std::string module, std::string name);

/* On flag-not-set: 0
 * On flag-set: 1
 * On error: -1
 */
int namedflag_test(std::string module, std::string name);

#endif

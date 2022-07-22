#pragma once

#include <launchdarkly/client.h>


/* ChangeListener represents user-provided callbacks that will be invoked when flag add/upsert operations
 * take place.
 *
 * The ChangeListener struct should be stored as a pointer, and initialized with LDi_initListeners.
 *
 * Only one callback can be registered for a given (flag, function pointer) pair; this is enforced at insertion time.
 * */
struct ChangeListener;

/* Initialize a list of ChangeListeners.
 * Must be called before any other operation. */
void
LDi_initListeners(struct ChangeListener** listeners);

/* Free a list of ChangeListeners. */
void
LDi_freeListeners(struct ChangeListener** listeners);

/* Insert a new listener.
 * If the combination of (flag, function pointer) already exists, no new listener is created.
 * If allocation fails, returns false. */
LDBoolean
LDi_listenerAdd(struct ChangeListener** listeners, const char* flag, LDlistenerfn callback);

/* Deletes a listener from the list. */
void
LDi_listenerRemove(struct ChangeListener** listeners, const char* flag, LDlistenerfn callback);

/* Insert a new listener with user data.
 * If the combination of (flag, function pointer) already exists, no new listener is created.
 * If allocation fails, returns false. */
LDBoolean
LDi_listenerUserDataAdd(struct ChangeListener** listeners, const char* flag, LDlistenerUserDatafn callback, void* const userData);

/* Deletes a listener with user data from the list. */
void
LDi_listenerUserDataRemove(struct ChangeListener** listeners, const char* flag, LDlistenerUserDatafn callback);

/* Dispatches an event for a given flag to all registered listeners. */
void
LDi_listenersDispatch(struct ChangeListener* listeners, const char *flag, LDBoolean status);

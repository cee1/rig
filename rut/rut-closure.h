#ifndef _RUT_CLOSURE_LIST_H_
#define _RUT_CLOSURE_LIST_H_

#include "rut-list.h"

/*
 * This implements a list of callbacks that can be used like signals
 * in GObject, but that don't have any marshalling overhead. The idea
 * is that any object that wants to provide a callback point will
 * provide a function to add a callback for that particular point. The
 * function can take a function pointer with the correct signature.
 * Internally the function will just call rut_closure_list_add. The
 * function should directly return a RutClosure pointer. The caller
 * can use this to disconnect the callback later without the object
 * having to provide a separate disconnect function.
 */

typedef void (* RutClosureDestroyCallback) (void *user_data);

typedef struct
{
  RutList list_node;

  void *function;
  void *user_data;
  RutClosureDestroyCallback destroy_cb;
} RutClosure;

/**
 * rut_closure_disconnect:
 * @closure: A closure connected to a Rut closure list
 *
 * Removes the given closure from the callback list it is connected to
 * and destroys it. If the closure was created with a destroy function
 * then it will be invoked. */
void
rut_closure_disconnect (RutClosure *closure);

void
rut_closure_list_disconnect_all (RutList *list);

RutClosure *
rut_closure_list_add (RutList *list,
                      void *function,
                      void *user_data,
                      RutClosureDestroyCallback destroy_cb);

/**
 * rut_closure_list_invoke:
 * @list: A pointer to a RutList containing RutClosures
 * @cb_type: The name of a typedef for the closure callback function signature
 * @...: The the arguments to pass to the callback
 *
 * A convenience macro to invoke a closure list.
 *
 * Note that the arguments will be evaluated multiple times so it is
 * not safe to pass expressions that have side-effects.
 *
 * Note also that this function ignores the return value from the
 * callbacks. If you want to handle the return value you should
 * manually iterate the list and invoke the callbacks yourself.
 */
#define rut_closure_list_invoke(list, cb_type, ...)             \
  G_STMT_START {                                                \
    RutClosure *_c, *_tmp;                                      \
                                                                \
    rut_list_for_each_safe (_c, _tmp, (list), list_node)        \
      {                                                         \
        cb_type _cb = _c->function;                             \
        _cb (__VA_ARGS__, _c->user_data);                       \
      }                                                         \
  } G_STMT_END

#endif /* _RUT_CLOSURE_LIST_H_ */

/*
 * Rut.
 *
 * Copyright (C) 2013  Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <config.h>

#include "rut-shell.h"
#include "rut-interfaces.h"
#include "rut-transform.h"
#include "rut-box-layout.h"
#include "rut-icon-toggle.h"
#include "rut-icon-toggle-set.h"
#include "rut-composite-sizable.h"

typedef struct _RutIconToggleSetState
{
  RutList list_node;

  RutIconToggle *toggle;
  RutClosure *on_toggle_closure;

  int value;
} RutIconToggleSetState;

enum {
  RUT_ICON_TOGGLE_SET_PROP_SELECTION,
  RUT_ICON_TOGGLE_SET_N_PROPS
};

struct _RutIconToggleSet
{
  RutObjectProps _parent;
  int ref_count;

  RutContext *ctx;

  RutBoxLayout *layout;

  RutList toggles_list;
  RutIconToggleSetState *current_toggle_state;

  RutList on_change_cb_list;

  RutGraphableProps graphable;

  RutSimpleIntrospectableProps introspectable;
  RutProperty properties[RUT_ICON_TOGGLE_SET_N_PROPS];
};

static void
remove_toggle_state (RutIconToggleSetState *toggle_state)
{
  rut_list_remove (&toggle_state->list_node);
  rut_refable_unref (toggle_state->toggle);
  g_slice_free (RutIconToggleSetState, toggle_state);
}

static void
_rut_icon_toggle_set_free (void *object)
{
  RutIconToggleSet *toggle_set = object;
  RutIconToggleSetState *toggle_state, *tmp;

  rut_closure_list_disconnect_all (&toggle_set->on_change_cb_list);

  rut_graphable_destroy (toggle_set);

  rut_list_for_each_safe (toggle_state, tmp,
                          &toggle_set->toggles_list, list_node)
    {
      remove_toggle_state (toggle_state);
    }

  rut_simple_introspectable_destroy (toggle_set);

  g_slice_free (RutIconToggleSet, object);
}

static RutPropertySpec
_rut_icon_toggle_set_prop_specs[] =
{
  {
    .name = "selection",
    .flags = RUT_PROPERTY_FLAG_READWRITE,
    .type = RUT_PROPERTY_TYPE_INTEGER,
    .getter.integer_type = rut_icon_toggle_set_get_selection,
    .setter.integer_type = rut_icon_toggle_set_set_selection
  },
  { 0 } /* XXX: Needed for runtime counting of the number of properties */
};

RutType rut_icon_toggle_set_type;

static void
_rut_icon_toggle_set_init_type (void)
{
  static RutGraphableVTable graphable_vtable = {
      NULL, /* child remove */
      NULL, /* child add */
      NULL /* parent changed */
  };

  static RutSizableVTable sizable_vtable = {
      rut_composite_sizable_set_size,
      rut_composite_sizable_get_size,
      rut_composite_sizable_get_preferred_width,
      rut_composite_sizable_get_preferred_height,
      rut_composite_sizable_add_preferred_size_callback
  };

  static RutIntrospectableVTable introspectable_vtable = {
      rut_simple_introspectable_lookup_property,
      rut_simple_introspectable_foreach_property
  };

  RutType *type = &rut_icon_toggle_set_type;
#define TYPE RutIconToggleSet

  rut_type_init (type, G_STRINGIFY (TYPE));
  rut_type_add_refable (type, ref_count, _rut_icon_toggle_set_free);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_GRAPHABLE,
                          offsetof (TYPE, graphable),
                          &graphable_vtable);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_SIZABLE,
                          0, /* no implied properties */
                          &sizable_vtable);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_COMPOSITE_SIZABLE,
                          offsetof (TYPE, layout),
                          NULL); /* no vtable */
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_INTROSPECTABLE,
                          0, /* no implied properties */
                          &introspectable_vtable);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_SIMPLE_INTROSPECTABLE,
                          offsetof (TYPE, introspectable),
                          NULL); /* no implied vtable */

#undef TYPE
}

RutIconToggleSet *
rut_icon_toggle_set_new (RutContext *ctx,
                         RutIconToggleSetPacking packing)
{
  RutIconToggleSet *toggle_set =
    rut_object_alloc0 (RutIconToggleSet,
                       &rut_icon_toggle_set_type,
                       _rut_icon_toggle_set_init_type);
  RutBoxLayoutPacking box_packing;

  toggle_set->ref_count = 1;

  rut_list_init (&toggle_set->on_change_cb_list);
  rut_list_init (&toggle_set->toggles_list);

  rut_graphable_init (toggle_set);

  rut_simple_introspectable_init (toggle_set, _rut_icon_toggle_set_prop_specs,
                                  toggle_set->properties);

  toggle_set->ctx = ctx;

  switch (packing)
    {
    case RUT_ICON_TOGGLE_SET_PACKING_LEFT_TO_RIGHT:
      box_packing = RUT_BOX_LAYOUT_PACKING_LEFT_TO_RIGHT;
      break;
    case RUT_ICON_TOGGLE_SET_PACKING_RIGHT_TO_LEFT:
      box_packing = RUT_BOX_LAYOUT_PACKING_RIGHT_TO_LEFT;
      break;
    case RUT_ICON_TOGGLE_SET_PACKING_TOP_TO_BOTTOM:
      box_packing = RUT_BOX_LAYOUT_PACKING_TOP_TO_BOTTOM;
      break;
    case RUT_ICON_TOGGLE_SET_PACKING_BOTTOM_TO_TOP:
      box_packing = RUT_BOX_LAYOUT_PACKING_BOTTOM_TO_TOP;
      break;
    }

  toggle_set->layout = rut_box_layout_new (ctx, box_packing);
  rut_graphable_add_child (toggle_set, toggle_set->layout);
  rut_refable_unref (toggle_set->layout);

  toggle_set->current_toggle_state = NULL;

  return toggle_set;
}

RutClosure *
rut_icon_toggle_set_add_on_change_callback (RutIconToggleSet *toggle_set,
                                  RutIconToggleSetChangedCallback callback,
                                  void *user_data,
                                  RutClosureDestroyCallback destroy_cb)
{
  g_return_val_if_fail (callback != NULL, NULL);

  return rut_closure_list_add (&toggle_set->on_change_cb_list,
                               callback,
                               user_data,
                               destroy_cb);
}

static RutIconToggleSetState *
find_state_for_value (RutIconToggleSet *toggle_set,
                      int value)
{
  RutIconToggleSetState *toggle_state;

  rut_list_for_each (toggle_state, &toggle_set->toggles_list, list_node)
    {
      if (toggle_state->value == value)
        return toggle_state;
    }
  return NULL;
}

static RutIconToggleSetState *
find_state_for_toggle (RutIconToggleSet *toggle_set,
                       RutIconToggle *toggle)
{
  RutIconToggleSetState *toggle_state;

  rut_list_for_each (toggle_state, &toggle_set->toggles_list, list_node)
    {
      if (toggle_state->toggle == toggle)
        return toggle_state;
    }

  return NULL;
}

static void
on_toggle_cb (RutIconToggle *toggle,
              bool value,
              void *user_data)
{
  RutIconToggleSet *toggle_set = user_data;
  RutIconToggleSetState *toggle_state;

  if (value == false)
    return;

  toggle_state = find_state_for_toggle (toggle_set, toggle);
  rut_icon_toggle_set_set_selection (toggle_set, toggle_state->value);
}

void
rut_icon_toggle_set_add (RutIconToggleSet *toggle_set,
                         RutIconToggle *toggle,
                         int value)
{
  RutIconToggleSetState *toggle_state;

  g_return_if_fail (rut_object_get_type (toggle_set) ==
                    &rut_icon_toggle_set_type);

  g_return_if_fail (find_state_for_toggle (toggle_set, toggle) == NULL);
  g_return_if_fail (find_state_for_value (toggle_set, value) == NULL);

  toggle_state = g_slice_new0 (RutIconToggleSetState);
  toggle_state->toggle = rut_refable_ref (toggle);
  toggle_state->on_toggle_closure =
    rut_icon_toggle_add_on_toggle_callback (toggle,
                                            on_toggle_cb,
                                            toggle_set,
                                            NULL); /* destroy notify */
  toggle_state->value = value;
  rut_list_insert (&toggle_set->toggles_list, &toggle_state->list_node);

  rut_box_layout_add (toggle_set->layout, FALSE, toggle);
}

void
rut_icon_toggle_set_remove (RutIconToggleSet *toggle_set,
                            RutIconToggle *toggle)
{
  RutIconToggleSetState *toggle_state;

  g_return_if_fail (rut_object_get_type (toggle_set) ==
                    &rut_icon_toggle_set_type);

  toggle_state = find_state_for_toggle (toggle_set, toggle);

  g_return_if_fail (toggle_state);

  if (toggle_set->current_toggle_state == toggle_state)
    toggle_set->current_toggle_state = NULL;

  remove_toggle_state (toggle_state);

  rut_box_layout_remove (toggle_set->layout, toggle);
}

int
rut_icon_toggle_set_get_selection (RutObject *object)
{
  RutIconToggleSet *toggle_set = object;

  return toggle_set->current_toggle_state ?
    toggle_set->current_toggle_state->value : -1;
}

void
rut_icon_toggle_set_set_selection (RutObject *object,
                                   int selection_value)
{
  RutIconToggleSet *toggle_set = object;
  RutIconToggleSetState *toggle_state;

  if (toggle_set->current_toggle_state &&
      toggle_set->current_toggle_state->value == selection_value)
    return;

  if (selection_value > 0)
    {
      toggle_state = find_state_for_value (toggle_set, selection_value);
      g_return_if_fail (toggle_state);
    }
  else
    {
      toggle_state = NULL;
      selection_value = -1;
    }

  if (toggle_set->current_toggle_state)
    {
      rut_icon_toggle_set_state (toggle_set->current_toggle_state->toggle,
                                 false);
    }

  toggle_set->current_toggle_state = toggle_state;
  rut_icon_toggle_set_state (toggle_set->current_toggle_state->toggle,
                             true);

  rut_property_dirty (&toggle_set->ctx->property_ctx,
                      &toggle_set->properties[RUT_ICON_TOGGLE_SET_PROP_SELECTION]);

  rut_closure_list_invoke (&toggle_set->on_change_cb_list,
                           RutIconToggleSetChangedCallback,
                           toggle_set,
                           selection_value);
}

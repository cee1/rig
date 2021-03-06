/*
 * Rig
 *
 * Copyright (C) 2013 Intel Corporation.
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

#include <math.h>

#include "rut-bin.h"
#include "rut-interfaces.h"
#include "rut-transform.h"

struct _RutBin
{
  RutObjectProps _parent;

  RutContext *context;

  RutList preferred_size_cb_list;

  RutObject *child_transform;

  RutObject *child;
  RutClosure *child_preferred_size_closure;

  float left_padding, right_padding;
  float top_padding, bottom_padding;

  RutBinPosition x_position;
  RutBinPosition y_position;

  float width, height;

  RutGraphableProps graphable;

  int ref_count;
};

static void
_rut_bin_free (void *object)
{
  RutBin *bin = object;

  rut_closure_list_disconnect_all (&bin->preferred_size_cb_list);

  rut_bin_set_child (bin, NULL);

  rut_shell_remove_pre_paint_callback_by_graphable (bin->context->shell, bin);

  rut_graphable_destroy (bin);

  g_slice_free (RutBin, bin);
}

static void
allocate_cb (RutObject *graphable,
             void *user_data)
{
  RutBin *bin = graphable;

  if (bin->child)
    {
      float child_width, child_height;
      float child_x = bin->left_padding;
      float child_y = bin->top_padding;
      float available_width =
        bin->width - bin->left_padding - bin->right_padding;
      float available_height =
        bin->height - bin->top_padding - bin->bottom_padding;

      rut_sizable_get_preferred_width (bin->child,
                                       -1, /* for_height */
                                       NULL, /* min_width_p */
                                       &child_width);

      if (child_width > available_width)
        child_width = available_width;

      switch (bin->x_position)
        {
        case RUT_BIN_POSITION_BEGIN:
          break;

        case RUT_BIN_POSITION_CENTER:
          if (child_width < available_width)
            child_x = nearbyintf (bin->width / 2.0f - child_width / 2.0f);
          break;

        case RUT_BIN_POSITION_END:
          if (child_width < available_width)
            child_x = bin->width - bin->right_padding - child_width;
          break;

        case RUT_BIN_POSITION_EXPAND:
          child_width = available_width;
          break;
        }

      rut_sizable_get_preferred_height (bin->child,
                                        child_width, /* for_width */
                                        NULL, /* min_height_p */
                                        &child_height);

      if (child_height > available_height)
        child_height = available_height;

      switch (bin->y_position)
        {
        case RUT_BIN_POSITION_BEGIN:
          break;

        case RUT_BIN_POSITION_CENTER:
          if (child_height < available_height)
            child_y = nearbyintf (bin->height / 2.0f - child_height / 2.0f);
          break;

        case RUT_BIN_POSITION_END:
          if (child_height < available_height)
            child_y = bin->height - bin->bottom_padding - child_height;
          break;

        case RUT_BIN_POSITION_EXPAND:
          child_height = available_height;
          break;
        }

      rut_transform_init_identity (bin->child_transform);
      rut_transform_translate (bin->child_transform, child_x, child_y, 0.0f);
      rut_sizable_set_size (bin->child, child_width, child_height);
    }
}

static void
queue_allocation (RutBin *bin)
{
  rut_shell_add_pre_paint_callback (bin->context->shell,
                                    bin,
                                    allocate_cb,
                                    NULL /* user_data */);
}

static void
preferred_size_changed (RutBin *bin)
{
  rut_closure_list_invoke (&bin->preferred_size_cb_list,
                           RutSizablePreferredSizeCallback,
                           bin);
}

static void
rut_bin_set_size (void *object,
                  float width,
                  float height)
{
  RutBin *bin = object;

  if (width == bin->width && height == bin->height)
    return;

  bin->width = width;
  bin->height = height;

  queue_allocation (bin);
}

static void
rut_bin_get_preferred_width (void *sizable,
                             float for_height,
                             float *min_width_p,
                             float *natural_width_p)
{
  RutBin *bin = sizable;
  float min_width = bin->left_padding + bin->right_padding;
  float natural_width = min_width;

  if (bin->child)
    {
      float child_min_width = 0.0f, child_natural_width = 0.0f;

      if (for_height != -1.0f)
        {
          for_height -= bin->top_padding + bin->bottom_padding;
          if (for_height < 0.0f)
            for_height = 0.0f;
        }

      rut_sizable_get_preferred_width (bin->child,
                                       for_height,
                                       min_width_p ?
                                       &child_min_width :
                                       NULL,
                                       natural_width_p ?
                                       &child_natural_width :
                                       NULL);

      min_width += child_min_width;
      natural_width += child_natural_width;
    }

  if (min_width_p)
    *min_width_p = min_width;
  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
rut_bin_get_preferred_height (void *sizable,
                              float for_width,
                              float *min_height_p,
                              float *natural_height_p)
{
  RutBin *bin = sizable;
  float min_height = bin->top_padding + bin->bottom_padding;
  float natural_height = min_height;

  if (bin->child)
    {
      float child_min_height = 0.0f, child_natural_height = 0.0f;

      if (for_width != -1.0f)
        {
          for_width -= bin->left_padding + bin->right_padding;
          if (for_width < 0.0f)
            for_width = 0.0f;
        }

      rut_sizable_get_preferred_height (bin->child,
                                        for_width,
                                        min_height_p ?
                                        &child_min_height :
                                        NULL,
                                        natural_height_p ?
                                        &child_natural_height :
                                        NULL);

      min_height += child_min_height;
      natural_height += child_natural_height;
    }

  if (min_height_p)
    *min_height_p = min_height;
  if (natural_height_p)
    *natural_height_p = natural_height;
}

static RutClosure *
rut_bin_add_preferred_size_callback (void *object,
                                     RutSizablePreferredSizeCallback cb,
                                     void *user_data,
                                     RutClosureDestroyCallback destroy)
{
  RutBin *bin = object;

  return rut_closure_list_add (&bin->preferred_size_cb_list,
                               cb,
                               user_data,
                               destroy);
}

static void
rut_bin_get_size (void *object,
                         float *width,
                         float *height)
{
  RutBin *bin = object;

  *width = bin->width;
  *height = bin->height;
}

RutType rut_bin_type;

static void
_rut_bin_init_type (void)
{
  static RutRefableVTable refable_vtable = {
      rut_refable_simple_ref,
      rut_refable_simple_unref,
      _rut_bin_free
  };

  static RutGraphableVTable graphable_vtable = {
      NULL, /* child removed */
      NULL, /* child addded */
      NULL /* parent changed */
  };

  static RutSizableVTable sizable_vtable = {
      rut_bin_set_size,
      rut_bin_get_size,
      rut_bin_get_preferred_width,
      rut_bin_get_preferred_height,
      rut_bin_add_preferred_size_callback
  };

  RutType *type = &rut_bin_type;
#define TYPE RutBin

  rut_type_init (type, G_STRINGIFY (TYPE));
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_REF_COUNTABLE,
                          offsetof (TYPE, ref_count),
                          &refable_vtable);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_GRAPHABLE,
                          offsetof (TYPE, graphable),
                          &graphable_vtable);
  rut_type_add_interface (type,
                          RUT_INTERFACE_ID_SIZABLE,
                          0, /* no implied properties */
                          &sizable_vtable);

#undef TYPE
}

RutBin *
rut_bin_new (RutContext *ctx)
{
  RutBin *bin = rut_object_alloc0 (RutBin,
                                   &rut_bin_type,
                                   _rut_bin_init_type);

  bin->ref_count = 1;
  bin->context = ctx;

  bin->x_position = RUT_BIN_POSITION_EXPAND;
  bin->y_position = RUT_BIN_POSITION_EXPAND;

  rut_list_init (&bin->preferred_size_cb_list);

  rut_graphable_init (RUT_OBJECT (bin));

  bin->child_transform = rut_transform_new (ctx);
  rut_graphable_add_child (bin, bin->child_transform);
  rut_refable_unref (bin->child_transform);

  return bin;
}

static void
child_preferred_size_cb (RutObject *sizable,
                         void *user_data)
{
  RutBin *bin = user_data;

  preferred_size_changed (bin);
  queue_allocation (bin);
}

void
rut_bin_set_child (RutBin *bin,
                   RutObject *child_widget)
{
  g_return_if_fail (rut_object_get_type (bin) == &rut_bin_type);

  if (bin->child == child_widget)
    return;

  if (child_widget)
    rut_refable_claim (child_widget, bin);

  if (bin->child)
    {
      rut_graphable_remove_child (bin->child);
      rut_closure_disconnect (bin->child_preferred_size_closure);
      bin->child_preferred_size_closure = NULL;
      rut_refable_release (bin->child, bin);
    }

  bin->child = child_widget;

  if (child_widget)
    {
      rut_graphable_add_child (bin->child_transform, child_widget);
      bin->child_preferred_size_closure =
        rut_sizable_add_preferred_size_callback (child_widget,
                                                 child_preferred_size_cb,
                                                 bin,
                                                 NULL /* destroy */);
      queue_allocation (bin);
    }

  preferred_size_changed (bin);
  rut_shell_queue_redraw (bin->context->shell);
}

RutObject *
rut_bin_get_child (RutBin *bin)
{
  return bin->child;
}

void
rut_bin_set_x_position (RutBin *bin,
                        RutBinPosition position)
{
  bin->x_position = position;
  queue_allocation (bin);
}

void
rut_bin_set_y_position (RutBin *bin,
                        RutBinPosition position)
{
  bin->y_position = position;
  queue_allocation (bin);
}

void
rut_bin_set_top_padding (RutObject *obj,
                         float top_padding)
{
  RutBin *bin = obj;

  bin->top_padding = top_padding;
  preferred_size_changed (bin);
  queue_allocation (bin);
}

void
rut_bin_set_bottom_padding (RutObject *obj,
                            float bottom_padding)
{
  RutBin *bin = obj;

  bin->bottom_padding = bottom_padding;
  preferred_size_changed (bin);
  queue_allocation (bin);
}

void
rut_bin_set_left_padding (RutObject *obj,
                          float left_padding)
{
  RutBin *bin = obj;

  bin->left_padding = left_padding;
  preferred_size_changed (bin);
  queue_allocation (bin);
}

void
rut_bin_set_right_padding (RutObject *obj,
                           float right_padding)
{
  RutBin *bin = obj;

  bin->right_padding = right_padding;
  preferred_size_changed (bin);
  queue_allocation (bin);
}

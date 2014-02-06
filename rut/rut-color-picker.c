/*
 * Rut
 *
 * Copyright (C) 2012 Intel Corporation.
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

#include <cogl/cogl.h>
#include <math.h>

#include "rut-context.h"
#include "rut-interfaces.h"
#include "rut-paintable.h"
#include "rut-property.h"
#include "rut-input-region.h"
#include "rut-color-picker.h"

#include "components/rut-camera.h"

enum {
  RUT_COLOR_PICKER_PROP_COLOR,
  RUT_COLOR_PICKER_N_PROPS
};

typedef enum
{
  RUT_COLOR_PICKER_GRAB_NONE,
  RUT_COLOR_PICKER_GRAB_HS,
  RUT_COLOR_PICKER_GRAB_V
} RutColorPickerGrab;

struct _RutColorPicker
{
  RutObjectProps _parent;

  RutContext *context;

  RutGraphableProps graphable;
  RutPaintableProps paintable;

  RutSimpleIntrospectableProps introspectable;
  RutProperty properties[RUT_COLOR_PICKER_N_PROPS];

  CoglBool hs_pipeline_dirty;
  CoglPipeline *hs_pipeline;

  CoglBool v_pipeline_dirty;
  CoglPipeline *v_pipeline;

  int width, height;

  int dot_width, dot_height;
  CoglPipeline *dot_pipeline;

  CoglPipeline *bg_pipeline;

  RutColorPickerGrab grab;
  RutInputRegion *input_region;

  CoglColor color;
  /* The current component values of the HSV colour */
  float hue, saturation, value;

  int ref_count;
};

RutType rut_color_picker_type;

#define RUT_COLOR_PICKER_HS_SIZE 128
#define RUT_COLOR_PICKER_V_WIDTH 16
#define RUT_COLOR_PICKER_V_HEIGHT 128
#define RUT_COLOR_PICKER_PADDING 8

#define RUT_COLOR_PICKER_HS_X RUT_COLOR_PICKER_PADDING
#define RUT_COLOR_PICKER_HS_Y RUT_COLOR_PICKER_PADDING
#define RUT_COLOR_PICKER_HS_CENTER_X (RUT_COLOR_PICKER_HS_X + \
                                      RUT_COLOR_PICKER_HS_SIZE / 2.0f)
#define RUT_COLOR_PICKER_HS_CENTER_Y (RUT_COLOR_PICKER_HS_Y + \
                                      RUT_COLOR_PICKER_HS_SIZE / 2.0f)

#define RUT_COLOR_PICKER_V_X (RUT_COLOR_PICKER_HS_SIZE + \
                              RUT_COLOR_PICKER_PADDING * 2.0f)
#define RUT_COLOR_PICKER_V_Y RUT_COLOR_PICKER_PADDING

#define RUT_COLOR_PICKER_TOTAL_WIDTH (RUT_COLOR_PICKER_HS_SIZE +        \
                                      RUT_COLOR_PICKER_V_WIDTH +        \
                                      RUT_COLOR_PICKER_PADDING * 3.0f)


#define RUT_COLOR_PICKER_TOTAL_HEIGHT (MAX (RUT_COLOR_PICKER_HS_SIZE,   \
                                            RUT_COLOR_PICKER_V_HEIGHT) + \
                                       RUT_COLOR_PICKER_PADDING * 2.0f)


/* The portion of the edge of the HS circle to blend so that it is
 * nicely anti-aliased */
#define RUT_COLOR_PICKER_HS_BLEND_EDGE 0.98f

static RutPropertySpec
_rut_color_picker_prop_specs[] =
  {
    {
      .name = "color",
      .flags = RUT_PROPERTY_FLAG_READWRITE,
      .type = RUT_PROPERTY_TYPE_COLOR,
      .data_offset = offsetof (RutColorPicker, color),
      .setter.color_type = rut_color_picker_set_color
    },
    { 0 } /* XXX: Needed for runtime counting of the number of properties */
  };

static void
ungrab (RutColorPicker *picker);

static void
_rut_color_picker_free (void *object)
{
  RutColorPicker *picker = object;

  ungrab (picker);

  cogl_object_unref (picker->hs_pipeline);
  cogl_object_unref (picker->v_pipeline);
  cogl_object_unref (picker->dot_pipeline);
  cogl_object_unref (picker->bg_pipeline);

  rut_graphable_remove_child (picker->input_region);
  rut_refable_unref (picker->input_region);

  rut_refable_unref (picker->context);

  rut_simple_introspectable_destroy (picker);
  rut_graphable_destroy (picker);

  g_slice_free (RutColorPicker, picker);
}

RutRefableVTable _rut_color_picker_refable_vtable = {
  rut_refable_simple_ref,
  rut_refable_simple_unref,
  _rut_color_picker_free
};

static void
hsv_to_rgb (const float hsv[3],
            float rgb[3])
{
  /* Based on Wikipedia:
     http://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV */
  float h = hsv[0];
  float s = hsv[1];
  float v = hsv[2];
  float hh = h * 6.0f / (2.0f * G_PI);
  float c = v * s;
  float x = c * (1.0f - fabsf (fmodf (hh, 2.0f) - 1.0f));
  float r, g, b;
  float m;

  if (hh < 1.0f)
    {
      r = c;
      g = x;
      b = 0;
    }
  else if (hh < 2.0f)
    {
      r = x;
      g = c;
      b = 0;
    }
  else if (hh < 3.0f)
    {
      r = 0;
      g = c;
      b = x;
    }
  else if (hh < 4.0f)
    {
      r = 0;
      g = x;
      b = c;
    }
  else if (hh < 5.0f)
    {
      r = x;
      g = 0;
      b = c;
    }
  else
    {
      r = c;
      g = 0;
      b = x;
    }

  m = v - c;
  rgb[0] = r + m;
  rgb[1] = g + m;
  rgb[2] = b + m;
}

static void
rgb_to_hsv (const float rgb[3],
            float hsv[3])
{
  /* Based on this:
     http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29 */
  float r = rgb[0];
  float g = rgb[1];
  float b = rgb[2];
  float rgb_max, rgb_min;
  float h, s, v;

  v = MAX (g, b);
  v = MAX (r, v);

  if (v <= 0.0f)
    {
      memset (hsv, 0, sizeof (float) * 3);
      return;
    }

  r /= v;
  g /= v;
  b /= v;

  rgb_min = MIN (g, b);
  rgb_min = MIN (r, rgb_min);

  s = 1.0f - rgb_min;

  if (s <= 0.0f)
    h = 0.0f;
  else
    {
      /* Normalize saturation to 1 */
      r = (r - rgb_min) / s;
      g = (g - rgb_min) / s;
      b = (b - rgb_min) / s;

      rgb_max = MAX (g, b);
      rgb_max = MAX (r, rgb_max);
      rgb_min = MIN (g, b);
      rgb_min = MIN (r, rgb_min);

      if (rgb_max == r)
        {
          h = G_PI / 3.0f * (g - b);
          if (h < 0.0f)
            h += G_PI * 2.0f;
        }
      else if (rgb_max == g)
        h = G_PI / 3.0f * (2.0f + b - r);
      else /* rgb_max == b */
        h = G_PI / 3.0f * (4.0f + r - g);
    }

  hsv[0] = h;
  hsv[1] = s;
  hsv[2] = v;
}

static void
ensure_hs_pipeline (RutColorPicker *picker)
{
  CoglBitmap *bitmap;
  CoglPixelBuffer *buffer;
  CoglPipeline *pipeline;
  CoglTexture2D *texture;
  int rowstride;
  uint8_t *data, *p;
  int x, y;

  if (!picker->hs_pipeline_dirty)
    return;

  bitmap = cogl_bitmap_new_with_size (picker->context->cogl_context,
                                      RUT_COLOR_PICKER_HS_SIZE,
                                      RUT_COLOR_PICKER_HS_SIZE,
                                      COGL_PIXEL_FORMAT_RGBA_8888_PRE);
  rowstride = cogl_bitmap_get_rowstride (bitmap);
  buffer = cogl_bitmap_get_buffer (bitmap);

  data = cogl_buffer_map (buffer,
                          COGL_BUFFER_ACCESS_WRITE,
                          COGL_BUFFER_MAP_HINT_DISCARD);

  p = data;

  for (y = 0; y < RUT_COLOR_PICKER_HS_SIZE; y++)
    {
      for (x = 0; x < RUT_COLOR_PICKER_HS_SIZE; x++)
        {
          float dx = x * 2.0f / RUT_COLOR_PICKER_HS_SIZE - 1.0f;
          float dy = y * 2.0f / RUT_COLOR_PICKER_HS_SIZE - 1.0f;
          float hsv[3];

          hsv[1] = sqrtf (dx * dx + dy * dy);

          if (hsv[1] >= 1.0f)
            {
              /* Outside of the circle the texture will be fully
               * transparent */
              p[0] = 0;
              p[1] = 0;
              p[2] = 0;
              p[3] = 0;
            }
          else
            {
              float rgb[3];
              float alpha;

              hsv[2] = picker->value;
              hsv[0] = atan2f (dy, dx) + G_PI;

              hsv_to_rgb (hsv, rgb);

              /* Blend the edges of the circle a bit so that it
               * looks anti-aliased */
              if (hsv[1] >= RUT_COLOR_PICKER_HS_BLEND_EDGE)
                alpha = (((RUT_COLOR_PICKER_HS_BLEND_EDGE - hsv[1]) /
                          (1.0f - RUT_COLOR_PICKER_HS_BLEND_EDGE) +
                          1.0f) *
                         255.0f);
              else
                alpha = 255.0f;

              p[0] = nearbyintf (rgb[0] * alpha);
              p[1] = nearbyintf (rgb[1] * alpha);
              p[2] = nearbyintf (rgb[2] * alpha);
              p[3] = alpha;
            }

          p += 4;
        }

      p += rowstride - RUT_COLOR_PICKER_HS_SIZE * 4;
    }

  cogl_buffer_unmap (buffer);

  texture = cogl_texture_2d_new_from_bitmap (bitmap);

  pipeline = cogl_pipeline_copy (picker->hs_pipeline);
  cogl_pipeline_set_layer_texture (pipeline,
                                   0, /* layer */
                                   texture);
  cogl_object_unref (picker->hs_pipeline);
  picker->hs_pipeline = pipeline;

  cogl_object_unref (texture);
  cogl_object_unref (bitmap);

  picker->hs_pipeline_dirty = FALSE;
}

static void
ensure_v_pipeline (RutColorPicker *picker)
{
  CoglBitmap *bitmap;
  CoglPixelBuffer *buffer;
  CoglPipeline *pipeline;
  CoglTexture2D *texture;
  int rowstride;
  uint8_t *data, *p;
  float hsv[3];
  int y;

  if (!picker->v_pipeline_dirty)
    return;

  bitmap = cogl_bitmap_new_with_size (picker->context->cogl_context,
                                      1, /* width */
                                      RUT_COLOR_PICKER_V_HEIGHT,
                                      COGL_PIXEL_FORMAT_RGB_888);
  rowstride = cogl_bitmap_get_rowstride (bitmap);
  buffer = cogl_bitmap_get_buffer (bitmap);

  data = cogl_buffer_map (buffer,
                          COGL_BUFFER_ACCESS_WRITE,
                          COGL_BUFFER_MAP_HINT_DISCARD);

  p = data;

  hsv[0] = picker->hue;
  hsv[1] = picker->saturation;

  for (y = 0; y < RUT_COLOR_PICKER_HS_SIZE; y++)
    {
      float rgb[3];

      hsv[2] = 1.0f - y / (RUT_COLOR_PICKER_HS_SIZE - 1.0f);

      hsv_to_rgb (hsv, rgb);

      p[0] = nearbyintf (rgb[0] * 255.0f);
      p[1] = nearbyintf (rgb[1] * 255.0f);
      p[2] = nearbyintf (rgb[2] * 255.0f);

      p += rowstride;
    }

  cogl_buffer_unmap (buffer);

  texture = cogl_texture_2d_new_from_bitmap (bitmap);

  pipeline = cogl_pipeline_copy (picker->v_pipeline);
  cogl_pipeline_set_layer_texture (pipeline,
                                   0, /* layer */
                                   texture);
  cogl_object_unref (picker->v_pipeline);
  picker->v_pipeline = pipeline;

  cogl_object_unref (texture);
  cogl_object_unref (bitmap);

  picker->v_pipeline_dirty = FALSE;
}

static void
draw_dot (RutColorPicker *picker,
          CoglFramebuffer *fb,
          float x,
          float y)
{
  cogl_framebuffer_draw_rectangle (fb,
                                   picker->dot_pipeline,
                                   x - picker->dot_width / 2.0f,
                                   y - picker->dot_height / 2.0f,
                                   x + picker->dot_width / 2.0f,
                                   y + picker->dot_height / 2.0f);
}

static void
_rut_color_picker_paint (RutObject *object,
                         RutPaintContext *paint_ctx)
{
  RutColorPicker *picker = (RutColorPicker *) object;
  RutCamera *camera = paint_ctx->camera;
  CoglFramebuffer *fb = rut_camera_get_framebuffer (camera);

  cogl_framebuffer_draw_rectangle (fb,
                                   picker->bg_pipeline,
                                   0.0f, 0.0f,
                                   picker->width,
                                   picker->height);

  ensure_hs_pipeline (picker);
  ensure_v_pipeline (picker);

  cogl_framebuffer_draw_rectangle (fb,
                                   picker->hs_pipeline,
                                   RUT_COLOR_PICKER_HS_X,
                                   RUT_COLOR_PICKER_HS_Y,
                                   RUT_COLOR_PICKER_HS_X +
                                   RUT_COLOR_PICKER_HS_SIZE,
                                   RUT_COLOR_PICKER_HS_Y +
                                   RUT_COLOR_PICKER_HS_SIZE);
  cogl_framebuffer_draw_rectangle (fb,
                                   picker->v_pipeline,
                                   RUT_COLOR_PICKER_V_X,
                                   RUT_COLOR_PICKER_V_Y,
                                   RUT_COLOR_PICKER_V_X +
                                   RUT_COLOR_PICKER_V_WIDTH,
                                   RUT_COLOR_PICKER_V_Y +
                                   RUT_COLOR_PICKER_V_HEIGHT);

  draw_dot (picker,
            fb,
            RUT_COLOR_PICKER_HS_CENTER_X -
            cos (picker->hue) * RUT_COLOR_PICKER_HS_SIZE / 2.0f *
            picker->saturation,
            RUT_COLOR_PICKER_HS_CENTER_Y -
            sin (picker->hue) * RUT_COLOR_PICKER_HS_SIZE / 2.0f *
            picker->saturation);
  draw_dot (picker,
            fb,
            RUT_COLOR_PICKER_V_X + RUT_COLOR_PICKER_V_WIDTH / 2.0f,
            RUT_COLOR_PICKER_V_Y +
            RUT_COLOR_PICKER_V_HEIGHT * (1.0f - picker->value));
}

static void
rut_color_picker_set_size (RutObject *object,
                            float width,
                            float height)
{
  RutColorPicker *picker = object;

  rut_shell_queue_redraw (picker->context->shell);

  picker->width = width;
  picker->height = height;
}

static void
rut_color_picker_get_size (RutObject *object,
                            float *width,
                            float *height)
{
  RutColorPicker *picker = object;

  *width = picker->width;
  *height = picker->height;
}

static void
rut_color_picker_get_preferred_width (RutObject *object,
                                       float for_height,
                                       float *min_width_p,
                                       float *natural_width_p)
{
  if (min_width_p)
    *min_width_p = RUT_COLOR_PICKER_TOTAL_WIDTH;
  if (natural_width_p)
    *natural_width_p = RUT_COLOR_PICKER_TOTAL_WIDTH;
}

static void
rut_color_picker_get_preferred_height (RutObject *object,
                                        float for_width,
                                        float *min_height_p,
                                        float *natural_height_p)
{
  if (min_height_p)
    *min_height_p = RUT_COLOR_PICKER_TOTAL_HEIGHT;
  if (natural_height_p)
    *natural_height_p = RUT_COLOR_PICKER_TOTAL_HEIGHT;
}

static RutGraphableVTable _rut_color_picker_graphable_vtable = {
  NULL, /* child removed */
  NULL, /* child addded */
  NULL /* parent changed */
};

static RutPaintableVTable _rut_color_picker_paintable_vtable = {
  _rut_color_picker_paint
};

static RutIntrospectableVTable _rut_color_picker_introspectable_vtable = {
  rut_simple_introspectable_lookup_property,
  rut_simple_introspectable_foreach_property
};

static RutSizableVTable _rut_color_picker_sizable_vtable = {
  rut_color_picker_set_size,
  rut_color_picker_get_size,
  rut_color_picker_get_preferred_width,
  rut_color_picker_get_preferred_height,
  NULL /* add_preferred_size_callback */
};

static void
_rut_color_picker_init_type (void)
{
  rut_type_init (&rut_color_picker_type, "RigColorPicker");
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_REF_COUNTABLE,
                          offsetof (RutColorPicker, ref_count),
                          &_rut_color_picker_refable_vtable);
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_GRAPHABLE,
                          offsetof (RutColorPicker, graphable),
                          &_rut_color_picker_graphable_vtable);
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_PAINTABLE,
                          offsetof (RutColorPicker, paintable),
                          &_rut_color_picker_paintable_vtable);
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_INTROSPECTABLE,
                          0, /* no implied properties */
                          &_rut_color_picker_introspectable_vtable);
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_SIMPLE_INTROSPECTABLE,
                          offsetof (RutColorPicker, introspectable),
                          NULL); /* no implied vtable */
  rut_type_add_interface (&rut_color_picker_type,
                          RUT_INTERFACE_ID_SIZABLE,
                          0, /* no implied properties */
                          &_rut_color_picker_sizable_vtable);
}

static CoglPipeline *
create_hs_pipeline (CoglContext *context)
{
  CoglPipeline *pipeline;

  pipeline = cogl_pipeline_new (context);

  cogl_pipeline_set_layer_null_texture (pipeline, 0, COGL_TEXTURE_TYPE_2D);
  cogl_pipeline_set_layer_filters (pipeline,
                                   0, /* layer */
                                   COGL_PIPELINE_FILTER_NEAREST,
                                   COGL_PIPELINE_FILTER_NEAREST);
  cogl_pipeline_set_layer_wrap_mode (pipeline,
                                     0, /* layer */
                                     COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE);

  return pipeline;
}

static void
create_dot_pipeline (RutColorPicker *picker)
{
  CoglTexture *texture;
  GError *error = NULL;

  picker->dot_pipeline = cogl_pipeline_new (picker->context->cogl_context);

  texture = rut_load_texture_from_data_file (picker->context,
                                             "color-picker-dot.png",
                                             &error);

  if (texture == NULL)
    {
      picker->dot_width = 8;
      picker->dot_height = 8;
    }
  else
    {
      picker->dot_width = cogl_texture_get_width (texture);
      picker->dot_height = cogl_texture_get_height (texture);

      cogl_pipeline_set_layer_texture (picker->dot_pipeline,
                                       0, /* layer */
                                       texture);

      cogl_object_unref (texture);
    }
}

static CoglPipeline *
create_bg_pipeline (CoglContext *context)
{
  CoglPipeline *pipeline;

  pipeline = cogl_pipeline_new (context);

  cogl_pipeline_set_color4ub (pipeline, 0, 0, 0, 200);

  return pipeline;
}

static void
set_value (RutColorPicker *picker,
           float value)
{
  if (picker->value != value)
    {
      picker->hs_pipeline_dirty = TRUE;
      picker->value = value;
    }
}

static void
set_hue_saturation (RutColorPicker *picker,
                    float hue,
                    float saturation)
{
  if (picker->hue != hue ||
      picker->saturation != saturation)
    {
      picker->v_pipeline_dirty = TRUE;
      picker->hue = hue;
      picker->saturation = saturation;
    }
}

static void
set_color_hsv (RutColorPicker *picker,
               const float hsv[3])
{
  float rgb[3];

  hsv_to_rgb (hsv, rgb);
  cogl_color_set_red (&picker->color, rgb[0]);
  cogl_color_set_green (&picker->color, rgb[1]);
  cogl_color_set_blue (&picker->color, rgb[2]);

  rut_property_dirty (&picker->context->property_ctx,
                      &picker->properties[RUT_COLOR_PICKER_PROP_COLOR]);

  rut_shell_queue_redraw (picker->context->shell);
}

static void
update_hs_from_event (RutColorPicker *picker,
                      RutInputEvent *event)
{
  float x, y, dx, dy;
  float hsv[3];

  rut_motion_event_unproject (event, picker, &x, &y);

  dx = x - RUT_COLOR_PICKER_HS_CENTER_X;
  dy = y - RUT_COLOR_PICKER_HS_CENTER_Y;

  hsv[0] = atan2f (dy, dx) + G_PI;
  hsv[1] = sqrtf (dx * dx + dy * dy) / RUT_COLOR_PICKER_HS_SIZE * 2.0f;
  hsv[2] = picker->value;

  if (hsv[1] > 1.0f)
    hsv[1] = 1.0f;

  set_hue_saturation (picker, hsv[0], hsv[1]);

  set_color_hsv (picker, hsv);
}

static void
update_v_from_event (RutColorPicker *picker,
                     RutInputEvent *event)
{
  float x, y;
  float hsv[3];

  rut_motion_event_unproject (event, picker, &x, &y);

  hsv[0] = picker->hue;
  hsv[1] = picker->saturation;
  hsv[2] = 1.0f - (y - RUT_COLOR_PICKER_V_Y) / RUT_COLOR_PICKER_V_HEIGHT;

  if (hsv[2] > 1.0f)
    hsv[2] = 1.0f;
  else if (hsv[2] < 0.0f)
    hsv[2] = 0.0f;

  set_value (picker, hsv[2]);

  set_color_hsv (picker, hsv);
}

static RutInputEventStatus
grab_input_cb (RutInputEvent *event,
               void *user_data)
{
  RutColorPicker *picker = user_data;

  if (rut_input_event_get_type (event) != RUT_INPUT_EVENT_TYPE_MOTION)
    return RUT_INPUT_EVENT_STATUS_UNHANDLED;

  if (rut_motion_event_get_action (event) == RUT_MOTION_EVENT_ACTION_MOVE)
    {
      switch (picker->grab)
        {
        case RUT_COLOR_PICKER_GRAB_V:
          update_v_from_event (picker, event);
          break;

        case RUT_COLOR_PICKER_GRAB_HS:
          update_hs_from_event (picker, event);
          break;

        default:
          break;
        }
    }

  if ((rut_motion_event_get_button_state (event) & RUT_BUTTON_STATE_1) == 0)
    ungrab (picker);

  return RUT_INPUT_EVENT_STATUS_HANDLED;
}

static void
ungrab (RutColorPicker *picker)
{
  if (picker->grab != RUT_COLOR_PICKER_GRAB_NONE)
    {
      rut_shell_ungrab_input (picker->context->shell,
                              grab_input_cb,
                              picker);

      picker->grab = RUT_COLOR_PICKER_GRAB_NONE;
    }
}

static RutInputEventStatus
input_region_cb (RutInputRegion *region,
                 RutInputEvent *event,
                 void *user_data)
{
  RutColorPicker *picker = user_data;
  RutCamera *camera;
  float x, y;

  if (picker->grab == RUT_COLOR_PICKER_GRAB_NONE &&
      rut_input_event_get_type (event) == RUT_INPUT_EVENT_TYPE_MOTION &&
      rut_motion_event_get_action (event) == RUT_MOTION_EVENT_ACTION_DOWN &&
      (rut_motion_event_get_button_state (event) & RUT_BUTTON_STATE_1) &&
      (camera = rut_input_event_get_camera (event)) &&
      rut_motion_event_unproject (event, picker, &x, &y))
    {
      if (x >= RUT_COLOR_PICKER_V_X &&
          x < RUT_COLOR_PICKER_V_X + RUT_COLOR_PICKER_V_WIDTH &&
          y >= RUT_COLOR_PICKER_V_Y &&
          y < RUT_COLOR_PICKER_V_Y + RUT_COLOR_PICKER_V_HEIGHT)
        {
          picker->grab = RUT_COLOR_PICKER_GRAB_V;
          rut_shell_grab_input (picker->context->shell,
                                camera,
                                grab_input_cb,
                                picker);

          update_v_from_event (picker, event);

          return RUT_INPUT_EVENT_STATUS_HANDLED;
        }
      else
        {
          float dx = x - RUT_COLOR_PICKER_HS_CENTER_X;
          float dy = y - RUT_COLOR_PICKER_HS_CENTER_Y;
          float distance = sqrtf (dx * dx + dy * dy);

          if (distance < RUT_COLOR_PICKER_HS_SIZE / 2.0f)
            {
              picker->grab = RUT_COLOR_PICKER_GRAB_HS;

              rut_shell_grab_input (picker->context->shell,
                                    camera,
                                    grab_input_cb,
                                    picker);

              update_hs_from_event (picker, event);

              return RUT_INPUT_EVENT_STATUS_HANDLED;
            }
        }
    }

  return RUT_INPUT_EVENT_STATUS_UNHANDLED;
}

RutColorPicker *
rut_color_picker_new (RutContext *context)
{
  RutColorPicker *picker = g_slice_new0 (RutColorPicker);
  static CoglBool initialized = FALSE;

  if (initialized == FALSE)
    {
      _rut_init ();
      _rut_color_picker_init_type ();

      initialized = TRUE;
    }

  picker->ref_count = 1;
  picker->context = rut_refable_ref (context);

  cogl_color_init_from_4ub (&picker->color, 0, 0, 0, 255);

  rut_object_init (&picker->_parent, &rut_color_picker_type);

  picker->hs_pipeline = create_hs_pipeline (context->cogl_context);
  picker->hs_pipeline_dirty = TRUE;

  picker->v_pipeline = cogl_pipeline_copy (picker->hs_pipeline);
  picker->v_pipeline_dirty = TRUE;

  create_dot_pipeline (picker);

  picker->bg_pipeline = create_bg_pipeline (context->cogl_context);

  rut_paintable_init (RUT_OBJECT (picker));
  rut_graphable_init (RUT_OBJECT (picker));

  rut_simple_introspectable_init (picker,
                                  _rut_color_picker_prop_specs,
                                  picker->properties);

  picker->input_region =
    rut_input_region_new_rectangle (/* x1 */
                                    RUT_COLOR_PICKER_HS_X,
                                    /* y1 */
                                    RUT_COLOR_PICKER_HS_Y,
                                    /* x2 */
                                    RUT_COLOR_PICKER_V_X +
                                    RUT_COLOR_PICKER_V_WIDTH,
                                    /* y2 */
                                    RUT_COLOR_PICKER_V_Y +
                                    RUT_COLOR_PICKER_V_HEIGHT,
                                    input_region_cb,
                                    picker);
  rut_graphable_add_child (picker, picker->input_region);

  rut_sizable_set_size (picker,
                        RUT_COLOR_PICKER_TOTAL_WIDTH,
                        RUT_COLOR_PICKER_TOTAL_HEIGHT);

  return picker;
}

void
rut_color_picker_set_color (RutObject *obj,
                            const CoglColor *color)
{
  RutColorPicker *picker = obj;

  if (memcmp (&picker->color, color, sizeof (CoglColor)))
    {
      float hsv[3];
      float rgb[3];

      picker->color = *color;
      rgb[0] = cogl_color_get_red (color);
      rgb[1] = cogl_color_get_green (color);
      rgb[2] = cogl_color_get_blue (color);

      rgb_to_hsv (rgb, hsv);

      set_hue_saturation (picker, hsv[0], hsv[1]);
      set_value (picker, hsv[2]);

      rut_property_dirty (&picker->context->property_ctx,
                          &picker->properties[RUT_COLOR_PICKER_PROP_COLOR]);

      rut_shell_queue_redraw (picker->context->shell);
    }
}

const CoglColor *
rut_color_picker_get_color (RutColorPicker *picker)
{
  return &picker->color;
}

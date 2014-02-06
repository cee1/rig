/*
 * Rut
 *
 * Copyright (C) 2012  Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <math.h>

#include "rig-node.h"

void
rig_node_integer_lerp (RigNode *a,
                       RigNode *b,
                       float t,
                       int *value)
{
  float range = b->t - a->t;
  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      *value = nearbyint (a->boxed.d.integer_val +
                          (b->boxed.d.integer_val - a->boxed.d.integer_val) *
                          factor);
    }
  else
    *value = a->boxed.d.integer_val;
}

void
rig_node_uint32_lerp (RigNode *a,
                      RigNode *b,
                      float t,
                      uint32_t *value)
{
  float range = b->t - a->t;
  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      *value = nearbyint (a->boxed.d.uint32_val +
                          (b->boxed.d.uint32_val - a->boxed.d.uint32_val) *
                          factor);
    }
  else
    *value = a->boxed.d.uint32_val;
}

void
rig_node_float_lerp (RigNode *a,
                     RigNode *b,
                     float t,
                     float *value)
{
  float range = b->t - a->t;
  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      *value = a->boxed.d.float_val +
        (b->boxed.d.float_val - a->boxed.d.float_val) * factor;
    }
  else
    *value = a->boxed.d.float_val;
}

void
rig_node_double_lerp (RigNode *a,
                      RigNode *b,
                      float t,
                      double *value)
{
  float range = b->t - a->t;
  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      *value = a->boxed.d.double_val +
        (b->boxed.d.double_val - a->boxed.d.double_val) * factor;
    }
  else
    *value = a->boxed.d.double_val;
}

void
rig_node_vec3_lerp (RigNode *a,
                    RigNode *b,
                    float t,
                    float value[3])
{
  float range = b->t - a->t;

  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      value[0] = a->boxed.d.vec3_val[0] +
        (b->boxed.d.vec3_val[0] - a->boxed.d.vec3_val[0]) * factor;
      value[1] = a->boxed.d.vec3_val[1] +
        (b->boxed.d.vec3_val[1] - a->boxed.d.vec3_val[1]) * factor;
      value[2] = a->boxed.d.vec3_val[2] +
        (b->boxed.d.vec3_val[2] - a->boxed.d.vec3_val[2]) * factor;
    }
  else
    memcpy (value, a->boxed.d.vec3_val, sizeof (float) * 3);
}

void
rig_node_vec4_lerp (RigNode *a,
                    RigNode *b,
                    float t,
                    float value[4])
{
  float range = b->t - a->t;

  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      value[0] = a->boxed.d.vec4_val[0] +
        (b->boxed.d.vec4_val[0] - a->boxed.d.vec4_val[0]) * factor;
      value[1] = a->boxed.d.vec4_val[1] +
        (b->boxed.d.vec4_val[1] - a->boxed.d.vec4_val[1]) * factor;
      value[2] = a->boxed.d.vec4_val[2] +
        (b->boxed.d.vec4_val[2] - a->boxed.d.vec4_val[2]) * factor;
      value[3] = a->boxed.d.vec4_val[3] +
        (b->boxed.d.vec4_val[3] - a->boxed.d.vec4_val[3]) * factor;
    }
  else
    memcpy (value, a->boxed.d.vec4_val, sizeof (float) * 4);
}

void
rig_node_color_lerp (RigNode *a,
                     RigNode *b,
                     float t,
                     CoglColor *value)
{
  float range = b->t - a->t;

  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;
      float reda, greena, bluea, alphaa;
      float redb, greenb, blueb, alphab;

      reda = cogl_color_get_red (&a->boxed.d.color_val);
      greena = cogl_color_get_green (&a->boxed.d.color_val);
      bluea = cogl_color_get_blue (&a->boxed.d.color_val);
      alphaa = cogl_color_get_alpha (&a->boxed.d.color_val);

      redb = cogl_color_get_red (&b->boxed.d.color_val);
      greenb = cogl_color_get_green (&b->boxed.d.color_val);
      blueb = cogl_color_get_blue (&b->boxed.d.color_val);
      alphab = cogl_color_get_alpha (&b->boxed.d.color_val);

      cogl_color_set_red (value, reda + (redb - reda) * factor);
      cogl_color_set_green (value, greena + (greenb - greena) * factor);
      cogl_color_set_blue (value, bluea + (blueb - bluea) * factor);
      cogl_color_set_alpha (value, alphaa + (alphab - alphaa) * factor);
    }
  else
    memcpy (value, &a->boxed.d.color_val, sizeof (CoglColor));
}

void
rig_node_quaternion_lerp (RigNode *a,
                          RigNode *b,
                          float t,
                          CoglQuaternion *value)
{
  float range = b->t - a->t;
  if (range)
    {
      float offset = t - a->t;
      float factor = offset / range;

      cogl_quaternion_nlerp (value,
                             &a->boxed.d.quaternion_val,
                             &b->boxed.d.quaternion_val,
                             factor);
    }
  else
    *value = a->boxed.d.quaternion_val;
}

void
rig_node_enum_lerp (RigNode *a,
                    RigNode *b,
                    float t,
                    int *value)
{
  if (a->t >= b->t)
    *value = a->boxed.d.enum_val;
  else
    *value = b->boxed.d.enum_val;
}

void
rig_node_boolean_lerp (RigNode *a,
                       RigNode *b,
                       float t,
                       bool *value)
{
  if (a->t >= b->t)
    *value = a->boxed.d.boolean_val;
  else
    *value = b->boxed.d.boolean_val;
}

void
rig_node_text_lerp (RigNode *a,
                    RigNode *b,
                    float t,
                    const char **value)
{
  if (a->t >= b->t)
    *value = a->boxed.d.text_val;
  else
    *value = b->boxed.d.text_val;
}

void
rig_node_asset_lerp (RigNode *a,
                     RigNode *b,
                     float t,
                     RutAsset **value)
{
  if (a->t >= b->t)
    *value = a->boxed.d.asset_val;
  else
    *value = b->boxed.d.asset_val;
}

void
rig_node_object_lerp (RigNode *a,
                      RigNode *b,
                      float t,
                      RutObject **value)
{
  if (a->t >= b->t)
    *value = a->boxed.d.object_val;
  else
    *value = b->boxed.d.object_val;
}

CoglBool
rig_node_box (RutPropertyType type,
              RigNode *node,
              RutBoxed *value)
{
  switch (type)
    {
    case RUT_PROPERTY_TYPE_FLOAT:
      value->type = RUT_PROPERTY_TYPE_FLOAT;
      value->d.float_val = node->boxed.d.float_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_DOUBLE:
      value->type = RUT_PROPERTY_TYPE_DOUBLE;
      value->d.double_val = node->boxed.d.double_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_INTEGER:
      value->type = RUT_PROPERTY_TYPE_INTEGER;
      value->d.integer_val = node->boxed.d.integer_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_UINT32:
      value->type = RUT_PROPERTY_TYPE_UINT32;
      value->d.uint32_val = node->boxed.d.uint32_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_VEC3:
      value->type = RUT_PROPERTY_TYPE_VEC3;
      memcpy (value->d.vec3_val, node->boxed.d.vec3_val, sizeof (float) * 3);
      return TRUE;

    case RUT_PROPERTY_TYPE_VEC4:
      value->type = RUT_PROPERTY_TYPE_VEC4;
      memcpy (value->d.vec4_val, node->boxed.d.vec4_val, sizeof (float) * 4);
      return TRUE;

    case RUT_PROPERTY_TYPE_COLOR:
      value->type = RUT_PROPERTY_TYPE_COLOR;
      value->d.color_val = node->boxed.d.color_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_QUATERNION:
      value->type = RUT_PROPERTY_TYPE_QUATERNION;
      value->d.quaternion_val = node->boxed.d.quaternion_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_ENUM:
      value->type = RUT_PROPERTY_TYPE_ENUM;
      value->d.enum_val = node->boxed.d.enum_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_BOOLEAN:
      value->type = RUT_PROPERTY_TYPE_BOOLEAN;
      value->d.boolean_val = node->boxed.d.boolean_val;
      return TRUE;

    case RUT_PROPERTY_TYPE_TEXT:
      value->type = RUT_PROPERTY_TYPE_TEXT;
      value->d.text_val = g_strdup (node->boxed.d.text_val);
      return TRUE;

    case RUT_PROPERTY_TYPE_ASSET:
      value->type = RUT_PROPERTY_TYPE_ASSET;
      value->d.asset_val = rut_refable_ref (node->boxed.d.asset_val);
      return TRUE;

    case RUT_PROPERTY_TYPE_OBJECT:
      value->type = RUT_PROPERTY_TYPE_OBJECT;
      value->d.object_val = rut_refable_ref (node->boxed.d.object_val);
      return TRUE;

    case RUT_PROPERTY_TYPE_POINTER:
      value->type = RUT_PROPERTY_TYPE_OBJECT;
      value->d.pointer_val = node->boxed.d.pointer_val;
      return TRUE;
    }

  g_warn_if_reached ();

  return FALSE;
}

void
rig_node_free (RigNode *node)
{
  g_slice_free (RigNode, node);
}

RigNode *
rig_node_new_for_integer (float t, int value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_INTEGER;
  node->boxed.d.integer_val = value;
  return node;
}

RigNode *
rig_node_new_for_uint32 (float t, uint32_t value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_UINT32;
  node->boxed.d.uint32_val = value;
  return node;
}

RigNode *
rig_node_new_for_float (float t, float value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_FLOAT;
  node->boxed.d.float_val = value;
  return node;
}

RigNode *
rig_node_new_for_double (float t, double value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_DOUBLE;
  node->boxed.d.double_val = value;
  return node;
}

RigNode *
rig_node_new_for_vec3 (float t, const float value[3])
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_VEC3;
  memcpy (node->boxed.d.vec3_val, value, sizeof (float) * 3);
  return node;
}

RigNode *
rig_node_new_for_vec4 (float t, const float value[4])
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_VEC4;
  memcpy (node->boxed.d.vec4_val, value, sizeof (float) * 4);
  return node;
}

RigNode *
rig_node_new_for_quaternion (float t, const CoglQuaternion *value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_QUATERNION;
  node->boxed.d.quaternion_val = *value;

  return node;
}

RigNode *
rig_node_new_for_color (float t, const CoglColor *value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_COLOR;
  node->boxed.d.color_val = *value;

  return node;
}

RigNode *
rig_node_new_for_boolean (float t, bool value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_BOOLEAN;
  node->boxed.d.boolean_val = value;

  return node;
}

RigNode *
rig_node_new_for_enum (float t, int value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_ENUM;
  node->boxed.d.enum_val = value;

  return node;
}

RigNode *
rig_node_new_for_text (float t, const char *value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_TEXT;
  node->boxed.d.text_val = g_strdup (value);

  return node;
}

RigNode *
rig_node_new_for_asset (float t, RutAsset *value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_ASSET;
  node->boxed.d.asset_val = rut_refable_ref (value);

  return node;
}

RigNode *
rig_node_new_for_object (float t, RutObject *value)
{
  RigNode *node = g_slice_new (RigNode);
  node->t = t;
  node->boxed.type = RUT_PROPERTY_TYPE_OBJECT;
  node->boxed.d.object_val = rut_refable_ref (value);

  return node;
}

RigNode *
rig_node_copy (RigNode *node)
{
  return g_slice_dup (RigNode, node);
}

RigNode *
rig_nodes_find_less_than (RigNode *start, RutList *end, float t)
{
  RigNode *node;

  for (node = start;
       node != rut_container_of (end, node, list_node);
       node = rut_container_of (node->list_node.prev, node, list_node))
    if (node->t < t)
      return node;

  return NULL;
}

RigNode *
rig_nodes_find_less_than_equal (RigNode *start, RutList *end, float t)
{
  RigNode *node;

  for (node = start;
       node != rut_container_of (end, node, list_node);
       node = rut_container_of (node->list_node.prev, node, list_node))
    if (node->t <= t)
      return node;

  return NULL;
}

RigNode *
rig_nodes_find_greater_than (RigNode *start, RutList *end, float t)
{
  RigNode *node;

  for (node = start;
       node != rut_container_of (end, node, list_node);
       node = rut_container_of (node->list_node.next, node, list_node))
    if (node->t > t)
      return node;

  return NULL;
}

RigNode *
rig_nodes_find_greater_than_equal (RigNode *start, RutList *end, float t)
{
  RigNode *node;

  for (node = start;
       node != rut_container_of (end, node, list_node);
       node = rut_container_of (node->list_node.prev, node, list_node))
    if (node->t >= t)
      return node;

  return NULL;
}

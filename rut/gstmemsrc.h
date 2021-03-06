/* GStreamer
 * Copyright (C) 2013 Intel Corporation.
 *
 * gstmemsrc.h:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_MEM_SRC_H__
#define __GST_MEM_SRC_H__

#include <sys/types.h>

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_MEM_SRC \
  (gst_mem_src_get_type())
#define GST_MEM_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MEM_SRC,GstMemSrc))
#define GST_MEM_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MEM_SRC,GstMemSrcClass))
#define GST_IS_MEM_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MEM_SRC))
#define GST_IS_MEM_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MEM_SRC))
#define GST_MEM_SRC_CAST(obj) ((GstMemSrc*) obj)

typedef struct _GstMemSrc GstMemSrc;
typedef struct _GstMemSrcClass GstMemSrcClass;

/**
 * GstMemSrc:
 *
 * Opaque #GstMemSrc structure.
 */
struct _GstMemSrc {
  GstPushSrc element;

  /*< private >*/
  gchar *uri;

  void *memory;
  guint64 size;
  guint64 offset;
};

struct _GstMemSrcClass {
  GstPushSrcClass parent_class;
};

G_GNUC_INTERNAL GType gst_mem_src_get_type (void);

G_END_DECLS

#endif /* __GST_MEM_SRC_H__ */

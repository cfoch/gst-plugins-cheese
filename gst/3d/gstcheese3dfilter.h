/* 
 * GStreamer
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2018 Fabian Orccon <cfoch.fabian@gmail.com>
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

#ifndef _GST_CHEESE_3DFILTER_H_
#define _GST_CHEESE_3DFILTER_H_

#include <gst/gl/gstglfilter.h>
#include <gst/gl/gstglfuncs.h>

G_BEGIN_DECLS

#define GST_TYPE_CHEESE_3D_FILTER            (gst_cheese_3d_filter_get_type())
#define GST_CHEESE_3D_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CHEESE_3D_FILTER,GstCheese3DFilter))
#define GST_IS_CHEESE_3D_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CHEESE_3D_FILTER))
#define GST_CHEESE_3D_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_CHEESE_3D_FILTER,GstCheese3DFilterClass))
#define GST_IS_CHEESE_3D_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_CHEESE_3D_FILTER))
#define GST_CHEESE_3D_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_CHEESE_3D_FILTER,GstCheese3DFilterClass))

typedef struct _GstCheese3DFilter GstCheese3DFilter;
typedef struct _GstCheese3DFilterClass GstCheese3DFilterClass;

struct _GstCheese3DFilter
{
    GstGLFilter filter;

    GstGLShader *shader;
    GstGLMemory *in_tex;

    /* background color */
    gfloat red;
    gfloat green;
    gfloat blue;

    /* perspective */
    gdouble fovy;
    gdouble aspect;
    gdouble znear;
    gdouble zfar;

    GLuint             vao;
    GLuint             vbo_indices;
    GLuint             vertex_buffer;
    GLint              attr_position;
    GLint              attr_texture;
};

struct _GstCheese3DFilterClass
{
    GstGLFilterClass filter_class;
};

GType gst_cheese_3d_filter_get_type (void);

G_END_DECLS

#endif /* _GST_CHEESE_3DFILTER_H_ */

/*
 * GStreamer Plugins Cheese
 * Copyright (C) 2018 Fabian Orccon <cfoch.fabian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "cheesefacespriteframe.h"

struct _CheeseFaceSpriteFrame {
  GObject parent_instance;
  guint duration;
  gchar * location;
  gdouble base_scale_factor;
  GdkPixbuf *pixbuf;
};

static void cheese_face_sprite_frame_class_init (CheeseFaceSpriteFrameClass *
    klass);
static void cheese_face_sprite_frame_init (CheeseFaceSpriteFrame * self);
static gboolean cheese_face_sprite_frame_set_pixbuf (
    CheeseFaceSpriteFrame *self, GdkPixbuf * pixbuf, gboolean inc_ref);
static gboolean cheese_face_sprite_frame_set_location (
    CheeseFaceSpriteFrame *self, const gchar * location);
static void cheese_face_sprite_frame_get_property (GObject *object,
    guint prop_id, GValue *value, GParamSpec *pspec);
static void cheese_face_sprite_frame_set_property (GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec);
static void cheese_face_sprite_frame_finalize (GObject * object);
CheeseFaceSpriteFrame * cheese_face_sprite_frame_new_from_location (
    const gchar * location, GError ** error);
CheeseFaceSpriteFrame * cheese_face_sprite_frame_new_from_pixbuf (GdkPixbuf *
    pixbuf);

enum {
  PROP_0,
  PROP_DURATION,
  PROP_LOCATION,
  PROP_PIXBUF,
  PROP_BASE_SCALE_FACTOR,
  LAST_PROP
};

#define DEFAULT_DURATION            1
#define DEFAULT_BASE_SCALE_FACTOR   1.0

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (CheeseFaceSpriteFrame, cheese_face_sprite_frame, G_TYPE_OBJECT)
static void
cheese_face_sprite_frame_class_init (CheeseFaceSpriteFrameClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_face_sprite_frame_finalize;
  object_class->get_property = cheese_face_sprite_frame_get_property;
  object_class->set_property = cheese_face_sprite_frame_set_property;

  properties[PROP_DURATION] = g_param_spec_uint ("duration", "Duration",
          "Sets the number of frames the image will be displayed.",
          0, G_MAXUINT, DEFAULT_DURATION,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  properties[PROP_LOCATION] = g_param_spec_string ("location", "Location",
          "The file path to the image.", NULL,
          (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  properties[PROP_PIXBUF] = g_param_spec_object ("pixbuf", "Pixbuf",
      "A #GdkPixbuf object representing the image.", GDK_TYPE_PIXBUF,
       G_PARAM_READABLE);
  properties[PROP_BASE_SCALE_FACTOR] = g_param_spec_double ("base-scale-factor",
          "Base scale factor",
          "Sets the base scale factor that will be used to create the pixbuf.",
          0.0, G_MAXDOUBLE, DEFAULT_BASE_SCALE_FACTOR,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
cheese_face_sprite_frame_init (CheeseFaceSpriteFrame * self)
{
  self->location = NULL;
  self->pixbuf = NULL;
  self->duration = DEFAULT_DURATION;
  self->base_scale_factor = DEFAULT_BASE_SCALE_FACTOR;
}

static gboolean
cheese_face_sprite_frame_set_pixbuf (CheeseFaceSpriteFrame *self,
    GdkPixbuf * pixbuf, gboolean inc_ref)
{
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);
  self->pixbuf = pixbuf;
  if (inc_ref)
    g_object_ref (self->pixbuf);
  return TRUE;
}

static gboolean
cheese_face_sprite_frame_set_location (CheeseFaceSpriteFrame *self,
    const gchar * location)
{
  g_return_val_if_fail (location != NULL && *location != '\0', FALSE);
  self->location = g_strdup (location);
  return TRUE;
}

static void
cheese_face_sprite_frame_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec * pspec)
{
  CheeseFaceSpriteFrame *self = (CheeseFaceSpriteFrame *) object;
  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, self->location);
      break;
    case PROP_PIXBUF:
      g_value_set_object (value, self->pixbuf);
      break;
    case PROP_DURATION:
      g_value_set_uint (value, self->duration);
      break;
    case PROP_BASE_SCALE_FACTOR:
      g_value_set_double (value, self->base_scale_factor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_face_sprite_frame_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
  GdkPixbuf *pixbuf;

  CheeseFaceSpriteFrame *self = (CheeseFaceSpriteFrame *) object;

  switch (prop_id) {
    case PROP_DURATION:
      self->duration = g_value_get_uint (value);
      break;
    case PROP_BASE_SCALE_FACTOR:
      self->base_scale_factor = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
cheese_face_sprite_frame_finalize (GObject * object)
{
  CheeseFaceSpriteFrame *self = (CheeseFaceSpriteFrame *) object;
  if (self->pixbuf)
    g_object_unref (self->pixbuf);
  if (self->location)
    g_free (self->location);
  G_OBJECT_CLASS (cheese_face_sprite_frame_parent_class)->finalize (object);
}

CheeseFaceSpriteFrame *
cheese_face_sprite_frame_new_from_location (const gchar * location,
    GError ** error)
{
  CheeseFaceSpriteFrame *ret;
  GdkPixbuf *pixbuf;
  g_return_val_if_fail (location != NULL && *location != '\0', NULL);

  pixbuf = gdk_pixbuf_new_from_file (location, error);
  if (!pixbuf)
    return NULL;

  ret = g_object_new (CHEESE_TYPE_FACE_SPRITE_FRAME, NULL);
  cheese_face_sprite_frame_set_pixbuf (ret, pixbuf, FALSE);
  cheese_face_sprite_frame_set_location (ret, location);
  return ret;
}

CheeseFaceSpriteFrame *
cheese_face_sprite_frame_new_from_pixbuf (GdkPixbuf * pixbuf)
{
  CheeseFaceSpriteFrame *ret;
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  ret = g_object_new (CHEESE_TYPE_FACE_SPRITE_FRAME, NULL);
  cheese_face_sprite_frame_set_pixbuf (ret, pixbuf, TRUE);
  return ret;
}

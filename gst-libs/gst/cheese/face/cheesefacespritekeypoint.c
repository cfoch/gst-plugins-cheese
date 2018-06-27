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

#include "cheesefacespritekeypoint.h"

GType
cheese_face_keypoint_get_type (void)
{
  static GType cheese_face_keypoiny_type = 0;
  if (!cheese_face_keypoiny_type) {
    static GEnumValue keypoint_types[] = {
      { CHEESE_FACE_KEYPOINT_PHILTRUM, "Philtrum", "philtrum" },
      { CHEESE_FACE_KEYPOINT_MOUTH, "Mouth", "mouth" },
      { CHEESE_FACE_KEYPOINT_LEFT_EYE, "Left Eye", "left-eye" },
      { CHEESE_FACE_KEYPOINT_RIGHT_EYE, "Right Eye", "right-eye" },
      { CHEESE_FACE_KEYPOINT_NOSE, "Nose", "nose" },
      { CHEESE_FACE_KEYPOINT_LEFT_EAR, "Left Ear", "left-ear" },
      { CHEESE_FACE_KEYPOINT_RIGHT_EAR, "Right Ear", "right-ear" },
      { CHEESE_FACE_KEYPOINT_FACE, "Face", "face" },
      { CHEESE_FACE_KEYPOINT_HEAD, "Head", "head" },
      { NULL, NULL, NULL}
    };
    cheese_face_keypoiny_type = g_enum_register_static ("CheeseFaceKeypoint",
        keypoint_types);
  }
  return cheese_face_keypoiny_type;
}

struct _CheeseFaceSpriteKeypoint {
  GObject parent_instance;
  GPtrArray *frames;
  CheeseFaceKeypoint keypoint;
  gboolean rotate;
  gboolean loop;
  gdouble base_scale_factor;
};

static void cheese_face_sprite_keypoint_class_init (
    CheeseFaceSpriteKeypointClass * klass);
static void cheese_face_sprite_keypoint_init (CheeseFaceSpriteKeypoint * self);
static void cheese_face_sprite_keypoint_get_property (GObject *object,
    guint prop_id, GValue *value, GParamSpec * pspec);
static void cheese_face_sprite_keypoint_set_property (GObject *object,
    guint prop_id, const GValue *value, GParamSpec * pspec);
static void cheese_face_sprite_keypoint_finalize (GObject * object);
CheeseFaceSpriteFrame * cheese_face_sprite_keypoint_get_frame (
    CheeseFaceSpriteKeypoint * self, const guint index);
guint cheese_face_sprite_keypoint_count_frames (
    CheeseFaceSpriteKeypoint * self);
void cheese_face_sprite_keypoint_add_frame (CheeseFaceSpriteKeypoint * self,
    CheeseFaceSpriteFrame * frame);


enum {
  PROP_0,
  PROP_KEYPOINT,
  PROP_ROTATE,
  PROP_LOOP,
  PROP_BASE_SCALE_FACTOR,
  LAST_PROP
};

#define DEFAULT_ROTATE              FALSE
#define DEFAULT_LOOP                FALSE
#define DEFAULT_BASE_SCALE_FACTOR   1.0

static GParamSpec *properties[LAST_PROP];

G_DEFINE_TYPE (CheeseFaceSpriteKeypoint, cheese_face_sprite_keypoint,
    G_TYPE_OBJECT)
static void
cheese_face_sprite_keypoint_class_init (CheeseFaceSpriteKeypointClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cheese_face_sprite_keypoint_finalize;
  object_class->get_property = cheese_face_sprite_keypoint_get_property;
  object_class->set_property = cheese_face_sprite_keypoint_set_property;

  properties[PROP_KEYPOINT] = g_param_spec_enum ("keypoint", "Facial keypoint",
      "Sets the type of the facial keypoint.",
      CHEESE_TYPE_FACE_KEYPOINT, CHEESE_FACE_KEYPOINT_FACE,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
  properties[PROP_ROTATE] = g_param_spec_boolean ("rotate", "Rotate",
      "Sets whether frames should rotate against face orientation.",
      DEFAULT_ROTATE,
      (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  properties[PROP_LOOP] = g_param_spec_boolean ("loop", "Loop",
      "Sets whether loop or not the display of frames.", DEFAULT_LOOP,
      (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  properties[PROP_BASE_SCALE_FACTOR] = g_param_spec_double ("base-scale-factor",
      "Base scale factor",
      "Sets the base scale factor that will be applied to the read frames.",
      0.0, G_MAXDOUBLE, DEFAULT_BASE_SCALE_FACTOR,
      (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
cheese_face_sprite_keypoint_init (CheeseFaceSpriteKeypoint * self)
{
  self->rotate = DEFAULT_ROTATE;
  self->loop = DEFAULT_LOOP;
  self->base_scale_factor = DEFAULT_BASE_SCALE_FACTOR;
  self->frames =
      g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
}

static void
cheese_face_sprite_keypoint_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec * pspec)
{
  CheeseFaceSpriteKeypoint *self = (CheeseFaceSpriteKeypoint *) object;
  switch (prop_id) {
    case PROP_KEYPOINT:
      g_value_set_enum (value, self->keypoint);
      break;
    case PROP_ROTATE:
      g_value_set_boolean (value, self->rotate);
      break;
    case PROP_LOOP:
      g_value_set_boolean (value, self->loop);
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
cheese_face_sprite_keypoint_set_property (GObject *object, guint prop_id,
    const GValue *value, GParamSpec * pspec)
{
  CheeseFaceSpriteKeypoint *self = (CheeseFaceSpriteKeypoint *) object;
  switch (prop_id) {
    case PROP_KEYPOINT:
      self->keypoint = g_value_get_enum (value);
      break;
    case PROP_ROTATE:
      self->rotate = g_value_get_boolean (value);
      break;
    case PROP_LOOP:
      self->loop = g_value_get_boolean (value);
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
cheese_face_sprite_keypoint_finalize (GObject * object)
{
  CheeseFaceSpriteKeypoint *self = (CheeseFaceSpriteKeypoint *) object;
  g_ptr_array_free (self->frames, TRUE);
  G_OBJECT_CLASS (cheese_face_sprite_keypoint_parent_class)->finalize (object);
}

/**
 * cheese_face_sprite_keypoint_get_frame:
 * @self: a #CheeseFaceSpriteKeypoint
 * @index: the index of the #CheeseFaceSpriteFrame object to return
 *
 * Gets the #CheeseFaceSpriteFrame at the given index.
 *
 * Returns: (transfer full) (nullable): The newly created
 * #CheeseFaceSpriteFrame, or %NULL if the index is out of range.
 */
CheeseFaceSpriteFrame *
cheese_face_sprite_keypoint_get_frame (CheeseFaceSpriteKeypoint * self,
    const guint index)
{
  if (index >= self->frames->len)
    return NULL;
  return (CheeseFaceSpriteFrame *) g_ptr_array_index (self->frames, index);
}


guint
cheese_face_sprite_keypoint_get_duration (CheeseFaceSpriteKeypoint * self)
{
  /* This may be improved by having a 'duration' attribute that increments as */
  /* soon a new sprite_frame is added to the keypoint. */
  guint i, frame_duration, duration = 0;
  for (i = 0; i < cheese_face_sprite_keypoint_count_frames (self); i++) {
    CheeseFaceSpriteFrame *sprite_frame;
    sprite_frame = cheese_face_sprite_keypoint_get_frame (self, i);
    g_object_get (G_OBJECT (sprite_frame), "duration", &frame_duration, NULL);
    duration += frame_duration;
  }
  return duration;
}

/**
 * cheese_face_sprite_keypoint_get_frame_at:
 * @self: a #CheeseFaceSpriteKeypoint
 * @frame_number: the index of the #CheeseFaceSpriteFrame object to return
 *
 * Gets the #CheeseFaceSpriteFrame in which the given frame number is between
 * the start and end (accumulated duration of previous frames). If loop property
 * is set, then if the frame number is greater than the duration the accumulated
 * duration is reset to zero.
 *
 * Returns: (transfer full) (nullable): The newly created
 * #CheeseFaceSpriteFrame, or %NULL if the total duration is zero.
 */
CheeseFaceSpriteFrame *
cheese_face_sprite_keypoint_get_frame_at (CheeseFaceSpriteKeypoint * self,
    guint64 frame_number)
{
/* TODO gtk doc
 * Gets sprite frame at certain frame number.
 * Frames starting from 1 are expected.
 */
  CheeseFaceSpriteFrame *sprite_frame = NULL;
  guint frame_duration, acc_duration = 0, duration, relative_frame_number, i;
  duration = cheese_face_sprite_keypoint_get_duration (self);

  /* Loop is not supported yet */

  if (duration == 0)
    return NULL;

  /* FIXME: There are innecessary loops which may cause inneficiency */
  if (self->loop)
    relative_frame_number = (frame_number - 1) % duration;
  else
    relative_frame_number = frame_number - 1;

  for (i = 0; i < cheese_face_sprite_keypoint_count_frames (self); i++) {
    sprite_frame = cheese_face_sprite_keypoint_get_frame (self, i);
    g_object_get (G_OBJECT (sprite_frame), "duration", &frame_duration, NULL);
    acc_duration += frame_duration;
    if (relative_frame_number < acc_duration)
      break;
  }
  return sprite_frame;
}

guint
cheese_face_sprite_keypoint_count_frames (CheeseFaceSpriteKeypoint * self)
{
  return self->frames->len;
}

void
cheese_face_sprite_keypoint_add_frame (CheeseFaceSpriteKeypoint * self,
    CheeseFaceSpriteFrame * frame)
{
  g_ptr_array_add (self->frames, frame);
}

CheeseFaceSpriteKeypoint *
cheese_face_sprite_keypoint_new (CheeseFaceKeypoint type)
{
  return g_object_new (CHEESE_TYPE_FACE_SPRITE_KEYPOINT, "keypoint", type,
      NULL);
}

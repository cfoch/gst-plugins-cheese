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

#include "cheesefacesprite.h"

struct _CheeseFaceSprite {
  GObject parent_instance;
  GPtrArray *keypoints;
};

G_DEFINE_QUARK (cheese-face-sprite-error-quark, cheese_face_sprite_error)

static void cheese_face_sprite_class_init (CheeseFaceSpriteClass * klass);
static void cheese_face_sprite_init (CheeseFaceSprite * self);
static void cheese_face_sprite_finalize (GObject * object);

CheeseFaceSpriteKeypoint * cheese_face_sprite_get_sprite_keypoint (
    CheeseFaceSprite *self, const guint index);
guint cheese_face_sprite_count_sprite_keypoint (CheeseFaceSprite *self);
void cheese_face_sprite_add_sprite_keypoint (CheeseFaceSprite *self,
    CheeseFaceSpriteKeypoint * sprite_keypoint);
CheeseFaceSprite * cheese_face_sprite_new_from_json_object (
    JsonObject *face_object, GError ** error);
CheeseFaceSprite * cheese_face_sprite_new_from_string (const gchar * string,
    GError ** error);
CheeseFaceSprite * cheese_face_sprite_new_from_location (
    const gchar * location, GError ** error);
CheeseFaceSprite * cheese_face_sprite_new ();


G_DEFINE_TYPE (CheeseFaceSprite, cheese_face_sprite, G_TYPE_OBJECT)
static void
cheese_face_sprite_class_init (CheeseFaceSpriteClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_face_sprite_finalize;
}

static void
cheese_face_sprite_init (CheeseFaceSprite * self)
{
  self->keypoints = g_ptr_array_new_with_free_func (
      (GDestroyNotify) g_object_unref);
}

static void
cheese_face_sprite_finalize (GObject * object)
{
  CheeseFaceSprite *self = (CheeseFaceSprite *) object;
  g_ptr_array_free (self->keypoints, TRUE);
  G_OBJECT_CLASS (cheese_face_sprite_parent_class)->finalize (object);
}

/**
 * cheese_face_sprite_get_sprite_keypoint:
 * @self: a #CheeseFaceSprite
 * @index: the index of the #CheeseFaceSpriteKeypoint object to return
 *
 * Gets the #CheeseFaceSpriteKeypoint at the given index.
 *
 * Returns: (transfer full) (nullable): The newly created
 * #CheeseFaceSpriteKeypoint, or %NULL if the index is out of range.
 */
CheeseFaceSpriteKeypoint *
cheese_face_sprite_get_sprite_keypoint (CheeseFaceSprite *self,
    const guint index)
{
  if (index >= self->keypoints->len)
    return NULL;
  return (CheeseFaceSpriteKeypoint *) g_ptr_array_index (self->keypoints,
      index);
}

guint
cheese_face_sprite_count_sprite_keypoint (CheeseFaceSprite *self)
{
  return self->keypoints->len;
}

void
cheese_face_sprite_add_sprite_keypoint (CheeseFaceSprite *self,
    CheeseFaceSpriteKeypoint * sprite_keypoint)
{
  g_ptr_array_add (self->keypoints, (gpointer) sprite_keypoint);
}

CheeseFaceSprite *
cheese_face_sprite_new_from_json_object (JsonObject *face_object,
    GError ** error)
{
  guint f, k;
  JsonNode *frame_node;
  JsonObject *keypoint_object, *frame_object;
  CheeseFaceSprite *face_sprite;
  CheeseFaceSpriteKeypoint *sprite_keypoint;
  CheeseFaceSpriteFrame *sprite_frame;
  GEnumClass *keypoint_info =
      G_ENUM_CLASS (g_type_class_ref (CHEESE_TYPE_FACE_KEYPOINT));

  face_sprite = cheese_face_sprite_new ();
  /* Iterate over each enum of CheeseFaceKeypoint. */
  for (k = 0; k < keypoint_info->n_values; k++) {
    gboolean rotate, loop;
    const gchar *keypoint_nick = keypoint_info->values[k].value_nick;
    gdouble base_scale_factor;

    if (!json_object_has_member (face_object, keypoint_nick))
      continue;

    keypoint_object = json_object_get_object_member (face_object,
        keypoint_nick);
    if (keypoint_object) {
      JsonArray *frames_array;

      /* Get the array of frames */
      if (!json_object_has_member (keypoint_object, "frames"))
        continue;

      frames_array = json_object_get_array_member (keypoint_object, "frames");
      if (frames_array) {
        sprite_keypoint =
            cheese_face_sprite_keypoint_new (keypoint_info->values[k].value);
        for (f = 0; f < json_array_get_length (frames_array); f++) {
          guint duration;
          gdouble base_scale_factor;
          const gchar *location = NULL;
          frame_node = json_array_get_element (frames_array, f);
          if (!JSON_NODE_HOLDS_OBJECT (frame_node))
            goto format_error;

          frame_object = json_node_get_object (frame_node);
          if (!json_object_has_member (frame_object, "location"))
            goto format_error;

          location = json_object_get_string_member (frame_object, "location");
          if (!location)
            goto format_error;

          sprite_frame = cheese_face_sprite_frame_new_from_location (location,
              error);
          if (!sprite_frame)
            goto format_error;

          if (json_object_has_member (frame_object, "duration")) {
            duration = json_object_get_int_member (frame_object,
                "duration");
            g_object_set (G_OBJECT (sprite_frame), "duration", duration,
                NULL);
          }
          if (json_object_has_member (frame_object, "base-scale-factor")) {
            base_scale_factor = json_object_get_double_member (frame_object,
                "base-scale-factor");
            g_object_set (G_OBJECT (sprite_frame), "base-scale-factor",
                base_scale_factor, NULL);
          }

          cheese_face_sprite_keypoint_add_frame (sprite_keypoint,
              sprite_frame);
        }

        if (cheese_face_sprite_keypoint_count_frames (sprite_keypoint) == 0) {
          g_object_unref (sprite_keypoint);
          goto format_error;
        }

        if (json_object_has_member (keypoint_object, "rotate")) {
          rotate = json_object_get_boolean_member (keypoint_object, "rotate");
          g_object_set (G_OBJECT (sprite_keypoint), "rotate", rotate, NULL);
        }
        if (json_object_has_member (keypoint_object, "loop")) {
          loop = json_object_get_boolean_member (keypoint_object, "loop");
          g_object_set (G_OBJECT (sprite_keypoint), "loop", loop, NULL);
        }
        if (json_object_has_member (keypoint_object, "base-scale-factor")) {
          base_scale_factor = json_object_get_double_member (keypoint_object,
              "base-scale-factor");
          g_object_set (G_OBJECT (sprite_keypoint), "base-scale-factor",
              base_scale_factor, NULL);
        }
        cheese_face_sprite_add_sprite_keypoint (face_sprite, sprite_keypoint);
      }
    }
  }

  if (cheese_face_sprite_count_sprite_keypoint (face_sprite) == 0) {
    g_object_unref (face_sprite);
    goto format_error;
  }
  return face_sprite;

format_error:
  return NULL;
}

static CheeseFaceSprite *
cheese_face_sprite_new_from_parser (JsonParser * parser, GError ** error)
{
  guint i;
  JsonNode *root;
  JsonObject *face_object;
  CheeseFaceSprite *face_sprite;

  root = json_parser_get_root (parser);
  if (!JSON_NODE_HOLDS_OBJECT (root))
    goto format_error;

  face_object = json_node_get_object (root);
  face_sprite = cheese_face_sprite_new_from_json_object (face_object, error);
  if (!face_sprite)
    goto format_error;
  return face_sprite;

format_error:
  if (*error == NULL) {
    g_set_error (error, CHEESE_FACE_SPRITE_ERROR,
        CHEESE_FACE_SPRITE_ERROR_DESERIALIZE,
        /* TODO: gettext? */
        "Not a valid CheeseFaceSprite JSON format.");
  }
  return NULL;
}

CheeseFaceSprite *
cheese_face_sprite_new_from_string (const gchar * string, GError ** error)
{
  JsonParser *parser;
  parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, string, -1, error))
    return NULL;
  return cheese_face_sprite_new_from_parser (parser, error);
}

CheeseFaceSprite *
cheese_face_sprite_new_from_location (const gchar * location, GError ** error)
{
  JsonParser *parser;
  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, location, error))
    return NULL;
  return cheese_face_sprite_new_from_parser (parser, error);
}

CheeseFaceSprite *
cheese_face_sprite_new ()
{
  return g_object_new (CHEESE_TYPE_FACE_SPRITE, NULL);
}

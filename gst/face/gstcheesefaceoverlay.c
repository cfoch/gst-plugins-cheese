/* GStreamer
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-faceoverlay
 *
 * Overlays a SVG image over a detected face in a video stream.
 * x, y, w, and h properties are optional, and change the image position and
 * size relative to the detected face position and size.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 autovideosrc ! videoconvert ! faceoverlay location=/path/image1.svg,/path/image2.svg x=0.5 y=0.5 w=0.7 h=0.7 ! videoconvert ! autovideosink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <string.h>

#include "gstcheesefaceoverlay.h"

GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_overlay_debug);
#define GST_CAT_DEFAULT gst_cheese_face_overlay_debug

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_X,
  PROP_Y,
  PROP_W,
  PROP_H
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{RGB}")));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{BGRA}")));

#define gst_cheese_face_overlay_parent_class parent_class
G_DEFINE_TYPE (GstCheeseFaceOverlay, gst_cheese_face_overlay, GST_TYPE_BIN);

static void gst_cheese_face_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_cheese_face_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_cheese_face_overlay_message_handler (GstBin * bin,
    GstMessage * message);
static void gst_cheese_face_overlay_draw_overlay (GstElement * overlay,
    cairo_t * cr, guint64 timestamp, guint64 duration, gpointer user_data);
static GstStateChangeReturn gst_cheese_face_overlay_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_cheese_face_overlay_create_children (GstCheeseFaceOverlay * filter);
static void gst_cheese_face_overlay_reset_locations (GstCheeseFaceOverlay * filter);
static void gst_cheese_face_overlay_dispose (GObject * object);
static void gst_cheese_face_overlay_set_location (GstCheeseFaceOverlay * filter,
    const char *str);
static gboolean gst_cheese_face_overlay_add_sprite_from_file (
    GstCheeseFaceOverlay * filter, const gchar * location);

#define _ROTATION_VECTOR_IN_RANGE(v, vl, vr) ((v.x >= vl.x && v.y >= vl.y && v.z >= vl.z &&\
                                               v.x < vr.x && v.y < vr.y && v.z < vr.z))

static CheeseSpriteInfo *
_lookup_sprite_info_index (CheeseSprite * sprite, graphene_point3d_t
    face_rotation_vector) {
  CheeseSpriteInfo *ret = NULL;
  guint i;
  g_print ("overlay_rot_face: [%lf, %lf, %lf]\n", face_rotation_vector.x,
      face_rotation_vector.y, face_rotation_vector.z);
  for (i = 0; i < sprite->infos->len; i++) {
    CheeseSpriteInfo *info = (CheeseSpriteInfo *)
        g_ptr_array_index (sprite->infos, i);
    if (_ROTATION_VECTOR_IN_RANGE (face_rotation_vector, info->min_rotation,
        info->max_rotation)) {
      ret = info;
      break;
    }
  }
  return ret;
}

static gboolean
gst_cheese_face_overlay_add_sprite_from_file (GstCheeseFaceOverlay * filter,
    const gchar * location)
{
  struct CheeseSprite *sprite;
  GIOChannel *file;
  gsize length;
  gsize terminator_pos;
  GIOStatus status;
  GError *error = NULL;
  gchar *line;
  guint line_number = 0;

  file = g_io_channel_new_file (location, "r", &error);
  if (error != NULL)
    goto handle_error;

  sprite = g_new (struct CheeseSprite, 1);

  g_print ("gst_cheese_face_overlay_add_sprite_from_file\n");
  while (TRUE) {
    gchar **tokens;
    status = g_io_channel_read_line (file, &line, &length, &terminator_pos,
        &error);

    if (status == G_IO_STATUS_ERROR)
      goto handle_error;
    else if (status == G_IO_STATUS_EOF)
      break;
    /*
    if (error != NULL) {
      g_print ("Blah\n");
      goto handle_error;
    }*/
    tokens = g_strsplit (line, ",", -1);
    g_print (line);
    if (line_number == 0) {
      guint n_rotation_lines;
      if (g_strv_length (tokens) != 2)
        goto format_error;
      n_rotation_lines = g_ascii_strtoull (tokens[1], NULL, 10);
      sprite->facial_keypoint_index = g_ascii_strtoull (tokens[0], NULL, 10);
      sprite->infos = g_ptr_array_sized_new (n_rotation_lines);
      // g_print ("%d,%d\n", sprite->facial_keypoint_index, n_rotation_lines);
    } else {
      struct CheeseSpriteInfo *info;
      if (g_strv_length (tokens) != 7)
        goto format_error;

      info = g_new (struct CheeseSpriteInfo, 1);
      info->location = g_strdup (tokens[0]);
      info->min_rotation.x = g_strtod (tokens[1], NULL);
      info->min_rotation.y = g_strtod (tokens[2], NULL);
      info->min_rotation.z = g_strtod (tokens[3], NULL);
      info->max_rotation.x = g_strtod (tokens[4], NULL);
      info->max_rotation.y = g_strtod (tokens[5], NULL);
      info->max_rotation.z = g_strtod (tokens[6], NULL);
      g_ptr_array_add (sprite->infos, info);
    }
    line_number++;
  }
  g_ptr_array_add (filter->sprites, sprite);

  return TRUE;
format_error:
  {
    g_print ("FORMAT ERROR\n");
    return FALSE;
  }
handle_error:
  {
    g_print ("HANDLE ERROR\n");
    /* TODO */
    /* Handle error properly */
    return FALSE;
  }
}

static gboolean
gst_cheese_face_overlay_create_children (GstCheeseFaceOverlay * filter)
{
  GstElement *converter, *face_detect, *overlay;
  GstPad *pad;

  converter = gst_element_factory_make ("videoconvert", NULL);
  face_detect = gst_element_factory_make ("cheesefacedetect", NULL);
  overlay = gst_element_factory_make ("cairooverlay", NULL);

  g_signal_connect (overlay, "draw",
      G_CALLBACK (gst_cheese_face_overlay_draw_overlay), filter);

  /* FIXME: post missing-plugin messages on NULL->READY if needed */
  if (converter == NULL || face_detect == NULL || overlay == NULL)
    goto element_not_found;

  /* g_object_set (face_detect, "display", FALSE, NULL); */
  g_object_set (face_detect, "scale-factor", 0.5,
      "landmark", "/home/cfoch/Documents/git/gst-plugins-cheese/shape_predictor_68_face_landmarks.dat",
      NULL);

  gst_bin_add_many (GST_BIN (filter), face_detect, converter, overlay, NULL);
  filter->overlay = overlay;

  if (!gst_element_link_many (face_detect, converter, overlay, NULL))
    GST_ERROR_OBJECT (filter, "couldn't link elements");

  pad = gst_element_get_static_pad (face_detect, "sink");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (filter->sinkpad), pad))
    GST_ERROR_OBJECT (filter->sinkpad, "couldn't set sinkpad target");
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (overlay, "src");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (filter->srcpad), pad))
    GST_ERROR_OBJECT (filter->srcpad, "couldn't set srcpad target");
  gst_object_unref (pad);

  return TRUE;

element_not_found:
  {
    /* clean up */
    if (converter == NULL)
      GST_ERROR_OBJECT (filter, "videoconvert element not found");
    else
      gst_object_unref (converter);

    if (face_detect == NULL)
      GST_ERROR_OBJECT (filter, "cheesefacedetect element not found");
    else
      gst_object_unref (face_detect);

    if (overlay == NULL)
      GST_ERROR_OBJECT (filter, "cairooverlay element not found");
    else
      gst_object_unref (overlay);

    return FALSE;
  }
}

static GstStateChangeReturn
gst_cheese_face_overlay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (filter->overlay == NULL) {
        GST_ELEMENT_ERROR (filter, CORE, MISSING_PLUGIN, (NULL),
            ("Some required plugins are missing, probably either "
                "cheesefacedetect element or cairooverlay"));
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    default:
      break;
  }

  return ret;
}

static void
gst_cheese_face_overlay_draw_overlay (GstElement * overlay, cairo_t * cr,
    guint64 timestamp, guint64 duration, gpointer user_data)
{
  GstCheeseFaceOverlay *filter;
  const GstStructure *face;
  const GValue *faces_list, *face_val;
  gint face_count, i;
  filter = GST_CHEESEFACEOVERLAY (user_data);
  g_print ("DRAW CALLED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  if (!filter->draw) {
    GST_LOG_OBJECT (filter, "Not drawing.");
    return;
  }

  GST_LOG ("structure is %" GST_PTR_FORMAT, filter->st);
  faces_list = gst_structure_get_value (filter->st, "faces");
  if (!faces_list)
    return;
  face_count = gst_value_list_get_size (faces_list);
  GST_LOG_OBJECT (filter, "face count: %d", face_count);
  g_print ("face count: %d\n", face_count);

  filter->draw = filter->draw || face_count > 0 || filter->location == 0;
 
  if (!filter->draw) {
    GST_DEBUG_OBJECT (filter, "No face detected. Clearing.");
    return;
  }
  for (i = 0; i < face_count; i++) {
    CheeseSprite *sprite;
    CheeseSpriteInfo *sprite_info;
    const GValue *id_value, *bounding_box_value, *face_rotation_vector_value;
    graphene_rect_t *bounding_box_ptr;
    graphene_point3d_t *face_rotation_vector_ptr;
    guint face_id;
    guint x, y, width, height, img_width, img_height;
    gfloat img_scale_factor;
    GdkPixbuf *pixbuf;
    GError *pixbuf_err = NULL;


    g_print ("Face (%d)\n", i);
    face_val = gst_value_list_get_value (faces_list, i);
    face = gst_value_get_structure (face_val);
    /* face_id = g_value_get_uint (id_value); */

    id_value = gst_structure_get_value (face, "id");
    face_id = g_value_get_uint (id_value);
    bounding_box_value = gst_structure_get_value (face, "bounding-box");
    bounding_box_ptr =
        (graphene_rect_t *) g_value_get_boxed (bounding_box_value);
    face_rotation_vector_value = gst_structure_get_value (face,
        "pose-rotation-vector");
    g_print ("pose value: %p\n", face_rotation_vector_value);
    face_rotation_vector_ptr =
        (graphene_point3d_t *) g_value_get_boxed (face_rotation_vector_value);

    x = graphene_rect_get_x (bounding_box_ptr);
    y = graphene_rect_get_y (bounding_box_ptr);
    width = graphene_rect_get_width (bounding_box_ptr);
    height = graphene_rect_get_height (bounding_box_ptr);

    sprite =
        g_hash_table_lookup (filter->sprites_map, GINT_TO_POINTER (face_id));
    if (!sprite) {
      sprite = g_ptr_array_index (filter->sprites, filter->next_sprite);
      g_print ("sprite found at %p\n", sprite);
      g_print ("Insert sprite at %d\n", face_id);
      g_hash_table_insert (filter->sprites_map, GINT_TO_POINTER (face_id),
          sprite);
      g_print ("Inserted sprite at %d\n", face_id);
      filter->next_sprite = (filter->next_sprite + 1) % filter->sprites->len;
      g_print ("next sprite at pos %d\n", filter->next_sprite);
    }

    sprite_info = _lookup_sprite_info_index (sprite, *face_rotation_vector_ptr);
    if (!sprite_info) {
      /* TODO */
      /* Free stuff */
      g_print ("Error: not found sprite for the given rotation vector"
          "(%lf, %lf, %lf)\n", face_rotation_vector_ptr->x,
          face_rotation_vector_ptr->y, face_rotation_vector_ptr->z);
      continue;
    } else {
      g_print ("Read successfuly sprite for the given rotation vector"
          "(%lf, %lf, %lf)\n", face_rotation_vector_ptr->x,
          face_rotation_vector_ptr->y, face_rotation_vector_ptr->z);
    }

    g_print ("There sprite info: Location: %s\n", sprite_info->location);

    pixbuf = gdk_pixbuf_new_from_file (sprite_info->location, &pixbuf_err);
    g_print ("just after reading pixbuf\n");
    if (pixbuf_err) {
      g_print ("ergrrrrr\n");
      GST_DEBUG_OBJECT (filter, "Error: %s", pixbuf_err->message);
      /* TODO */
      /* Free stuff */
      g_print ("Error: %s\n", pixbuf_err->message);
      continue;
    } else {
      GST_LOG ("File \"%s\" added", sprite_info->location);
      g_print ("Sprite file read successfully: %s\n", sprite_info->location);
    }
    g_print ("after reading pixbuf\n");
    GST_LOG_OBJECT (filter, "Drawing image \"%d\" at (%d, %d)", i, x, y);

    /*
    x += filter->x * width;
    y += filter->y * height;
    width *= filter->w;
    height *= filter->h;
    */
    img_scale_factor = height / (gfloat) gdk_pixbuf_get_height (pixbuf);
    img_width = gdk_pixbuf_get_width (pixbuf) * img_scale_factor * 1.5;
    img_height = height * 1.5;

    g_print ("scale factor: %lf\n", width * img_scale_factor);
    pixbuf = gdk_pixbuf_scale_simple (pixbuf, img_width, img_height,
        GDK_INTERP_NEAREST);
    gdk_cairo_set_source_pixbuf (cr, pixbuf, x - img_width / 2 + width / 2, y);
    cairo_paint (cr);
    g_object_unref (pixbuf);
  }
}

static void
gst_cheese_face_overlay_message_handler (GstBin * bin, GstMessage * message)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (bin);
  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT) {
    if (filter->st)
      gst_structure_free (filter->st);
    filter->st = gst_structure_copy (gst_message_get_structure (message));
    filter->draw = gst_structure_has_name (filter->st, "cheesefacedetect");
  } else
    filter->draw = FALSE;

  GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}

static void
gst_cheese_face_overlay_class_init (GstCheeseFaceOverlayClass * klass)
{
  GObjectClass *gobject_class;
  GstBinClass *gstbin_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbin_class = GST_BIN_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_cheese_face_overlay_set_property;
  gobject_class->get_property = gst_cheese_face_overlay_get_property;
  gobject_class->dispose = gst_cheese_face_overlay_dispose;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location of SVG file to use for face overlay",
          "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_X,
      g_param_spec_float ("x", "face x offset",
          "Specify image x relative to detected face x.", -G_MAXFLOAT,
          G_MAXFLOAT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_Y,
      g_param_spec_float ("y", "face y offset",
          "Specify image y relative to detected face y.", -G_MAXFLOAT,
          G_MAXFLOAT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_W,
      g_param_spec_float ("w", "face width percent",
          "Specify image width relative to face width.", 0, G_MAXFLOAT, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_H,
      g_param_spec_float ("h", "face height percent",
          "Specify image height relative to face height.", 0, G_MAXFLOAT, 1,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "cheesefaceoverlay",
      "Filter/Editor/Video",
      "Overlays SVG graphics over a detected face in a video stream",
      "Fabian Orccon <cfoch.fabian@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  gstbin_class->handle_message =
      GST_DEBUG_FUNCPTR (gst_cheese_face_overlay_message_handler);
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_cheese_face_overlay_change_state);
}

static void
gst_cheese_face_overlay_init (GstCheeseFaceOverlay * filter)
{
  GstPadTemplate *tmpl;

  filter->x = 0;
  filter->y = 0;
  filter->w = 1;
  filter->h = 1;
  filter->overlay = NULL;
  filter->location = NULL;
  filter->draw = FALSE;
  filter->locations = g_ptr_array_new ();
  filter->sprites = g_ptr_array_new ();
  filter->sprites_map = g_hash_table_new (g_direct_hash, g_direct_equal);
  filter->next_sprite = 0;
  filter->location = NULL;

  tmpl = gst_static_pad_template_get (&sink_factory);
  filter->sinkpad = gst_ghost_pad_new_no_target_from_template ("sink", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  tmpl = gst_static_pad_template_get (&src_factory);
  filter->srcpad = gst_ghost_pad_new_no_target_from_template ("src", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  gst_cheese_face_overlay_create_children (filter);
}

static void
gst_cheese_face_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (filter);
      g_free (filter->location);
      filter->location = g_value_dup_string (value);
      gst_cheese_face_overlay_set_location (filter, g_value_dup_string (value));
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_X:
      GST_OBJECT_LOCK (filter);
      filter->x = g_value_get_float (value);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_Y:
      GST_OBJECT_LOCK (filter);
      filter->y = g_value_get_float (value);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_W:
      GST_OBJECT_LOCK (filter);
      filter->w = g_value_get_float (value);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_H:
      GST_OBJECT_LOCK (filter);
      filter->h = g_value_get_float (value);
      GST_OBJECT_UNLOCK (filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (filter);
      g_value_set_string (value, filter->location);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_X:
      GST_OBJECT_LOCK (filter);
      g_value_set_float (value, filter->x);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_Y:
      GST_OBJECT_LOCK (filter);
      g_value_set_float (value, filter->y);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_W:
      GST_OBJECT_LOCK (filter);
      g_value_set_float (value, filter->w);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_H:
      GST_OBJECT_LOCK (filter);
      g_value_set_float (value, filter->h);
      GST_OBJECT_UNLOCK (filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_overlay_dispose (GObject * object)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);

  g_free (filter->location);

  gst_cheese_face_overlay_reset_locations (filter);
  g_ptr_array_free (filter->locations, TRUE);

  if (filter->st)
    gst_structure_free (filter->st);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
gst_cheese_face_overlay_reset_locations (GstCheeseFaceOverlay * filter)
{
  gint i;
  for (i = 0; i < filter->locations->len; i++)
    g_object_unref (g_ptr_array_index (filter->locations, i));
  g_ptr_array_remove_range (filter->locations, 0, filter->locations->len);
}

static void
gst_cheese_face_overlay_set_location (GstCheeseFaceOverlay * filter,
    const char *str)
{
  gint i;
  gchar **locations;

  gst_cheese_face_overlay_reset_locations (filter);
  locations = g_strsplit (str, ",", -1);
  for (i = 0; locations && locations[i]; i++) {
    /*
    GdkPixbuf *pixbuf;
    GError *pixbuf_err = NULL;
    pixbuf = gdk_pixbuf_new_from_file (locations[i], &pixbuf_err);
    if (pixbuf_err) {
      GST_DEBUG_OBJECT (filter, "Error: %s", pixbuf_err->message);
      continue;
    } else
      GST_LOG ("File \"%s\" added", locations[i]);
    g_ptr_array_add (filter->locations, (gpointer) pixbuf);
    */
    gst_cheese_face_overlay_add_sprite_from_file (filter, locations[i]);
  }
  g_strfreev (locations);
}

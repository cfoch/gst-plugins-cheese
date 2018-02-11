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
  g_object_set (face_detect, "scale-factor", 0.5, NULL);

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
    const GValue *bounding_box_value;
    graphene_rect_t *bounding_box_ptr;
    guint x, y, width, height, img_width, img_height;
    gfloat img_scale_factor;
    GdkPixbuf *pixbuf;

    face_val = gst_value_list_get_value (faces_list, i);
    face = gst_value_get_structure (face_val);
    bounding_box_value = gst_structure_get_value (face, "bounding-box");
    bounding_box_ptr =
      (graphene_rect_t *) g_value_get_boxed (bounding_box_value);

    x = graphene_rect_get_x (bounding_box_ptr);
    y = graphene_rect_get_y (bounding_box_ptr);
    width = graphene_rect_get_width (bounding_box_ptr);
    height = graphene_rect_get_height (bounding_box_ptr);

    pixbuf = g_ptr_array_index (filter->locations, i % filter->locations->len);
    GST_LOG_OBJECT (filter, "Drawing image \"%d\" at (%d, %d)", i, x, y);

    /*
    x += filter->x * width;
    y += filter->y * height;
    width *= filter->w;
    height *= filter->h;
    */
    img_scale_factor = height / (gfloat) gdk_pixbuf_get_height (pixbuf);
    img_width = gdk_pixbuf_get_width (pixbuf) * img_scale_factor;
    img_height = height;

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
   GdkPixbuf *pixbuf;
    GError *pixbuf_err = NULL;
    pixbuf = gdk_pixbuf_new_from_file (locations[i], &pixbuf_err);
    if (pixbuf_err) {
      GST_DEBUG_OBJECT (filter, "Error: %s", pixbuf_err->message);
      continue;
    } else
      GST_LOG ("File \"%s\" added", locations[i]);
    g_ptr_array_add (filter->locations, (gpointer) pixbuf);
  }
  g_strfreev (locations);
}

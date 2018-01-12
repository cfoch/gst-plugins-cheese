/*
 * GStreamer
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

/**
 * SECTION:element-cheesefacedetect
 *
 * FIXME:Describe cheesefacedetect here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! cheesefacedetect ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <cstdio>
#include <iostream>
#include <vector>

#include "gstcheesefacedetect.h"

using namespace std;
using namespace dlib;

GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_detect_debug);
#define GST_CAT_DEFAULT gst_cheese_face_detect_debug

#define DEFAULT_LANDMARK NULL

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_DISPLAY,
  PROP_LANDMARK
};

// static dlib::frontal_face_detector mydetector = get_frontal_face_detector();

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB"))
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB"))
    );

#define gst_cheese_face_detect_parent_class parent_class
G_DEFINE_TYPE (GstCheeseFaceDetect, gst_cheese_face_detect,
               GST_TYPE_OPENCV_VIDEO_FILTER);

static void gst_cheese_face_detect_finalize (GObject * obj);
static void gst_cheese_face_detect_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_cheese_face_detect_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_cheese_face_detect_transform_ip (
    GstOpencvVideoFilter * filter, GstBuffer * buf, IplImage * img);

/* GObject vmethod implementations */

/* initialize the cheesefacedetect's class */
static void
gst_cheese_face_detect_class_init (GstCheeseFaceDetectClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstOpencvVideoFilterClass *gstopencvbasefilter_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstopencvbasefilter_class = (GstOpencvVideoFilterClass *) klass;

  gstopencvbasefilter_class->cv_trans_ip_func =
      gst_cheese_face_detect_transform_ip;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_cheese_face_detect_finalize);
  gobject_class->set_property = gst_cheese_face_detect_set_property;
  gobject_class->get_property = gst_cheese_face_detect_get_property;

  g_object_class_install_property (gobject_class, PROP_DISPLAY,
      g_param_spec_boolean ("display", "Display",
          "Sets whether the detected faces should be highlighted in the output",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_LANDMARK,
      g_param_spec_string ("landmark", "Landmark shape model",
          "Location of the shape model. You can get one at "
          "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2",
          DEFAULT_LANDMARK,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  gst_element_class_set_details_simple(gstelement_class,
    "CheeseFaceDetect",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabian Orccon <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_cheese_face_detect_init (GstCheeseFaceDetect * filter)
{
  filter->display = TRUE;
  filter->landmark = NULL;
  filter->face_detector =
      new frontal_face_detector (get_frontal_face_detector());
  filter->shape_predictor = NULL;


  gst_opencv_video_filter_set_in_place (GST_OPENCV_VIDEO_FILTER_CAST (filter),
      TRUE);
}

static void
gst_cheese_face_detect_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (object);

  switch (prop_id) {
    case PROP_DISPLAY:
      filter->display = g_value_get_boolean (value);
      break;
    case PROP_LANDMARK:
      filter->landmark = g_value_dup_string (value);
      filter->shape_predictor = new shape_predictor;
      /* TODO: Handle error. For example, file does not exist. */
      dlib::deserialize (filter->landmark) >> *filter->shape_predictor;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_detect_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (object);

  switch (prop_id) {
    /*case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;*/
    case PROP_DISPLAY:
      g_value_set_boolean (value, filter->display);
      break;
    case PROP_LANDMARK:
      g_value_set_string (value, filter->landmark);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstMessage *
gst_cheese_face_detect_message_new (GstCheeseFaceDetect * filter,
    GstBuffer * buf)
{
  GstBaseTransform *trans = GST_BASE_TRANSFORM_CAST (filter);
  GstStructure *s;
  GstClockTime running_time, stream_time;

  running_time = gst_segment_to_running_time (&trans->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (buf));
  stream_time = gst_segment_to_stream_time (&trans->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (buf));

  s = gst_structure_new ("facedetect",
      "timestamp", G_TYPE_UINT64, GST_BUFFER_TIMESTAMP (buf),
      "stream-time", G_TYPE_UINT64, stream_time,
      "running-time", G_TYPE_UINT64, running_time,
      "duration", G_TYPE_UINT64, GST_BUFFER_DURATION (buf), NULL);

  return gst_message_new_element (GST_OBJECT (filter), s);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_cheese_face_detect_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  int i;
  GstMessage *msg;
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (base);
  cv::Mat cvImg (cv::cvarrToMat (img));
  cv::Mat resizedImg;


  GValue facelist = G_VALUE_INIT;
  GValue facedata = G_VALUE_INIT;

  cout << "height: " << img->height << endl;
  cout << "width: " << img->width << endl;


  //cv::resize(cvImg, resizedImg, cv::Size(200, 200));

  auto start = chrono::steady_clock::now();
  cv_image<bgr_pixel> dlib_img (img);
  std::vector<rectangle> dets = (*filter->face_detector) (dlib_img);

  msg = gst_cheese_face_detect_message_new (filter, buf);
  g_value_init (&facelist, GST_TYPE_LIST);

  for (i = 0; i < dets.size(); i++) {
    GstStructure *s;
    s = gst_structure_new ("face",
        "l", G_TYPE_UINT, dets[i].left (),
        "t", G_TYPE_UINT, dets[i].top (),
        "r", G_TYPE_UINT, dets[i].right (),
        "b", G_TYPE_UINT, dets[i].bottom (), NULL);

    g_value_init (&facedata, GST_TYPE_STRUCTURE);
    g_value_take_boxed (&facedata, s);
    gst_value_list_append_value (&facelist, &facedata);
    g_value_unset (&facedata);

    cout << "face number " << i + 1 << ": " << dets[i] << endl;
    if (filter->display) {
      cv::Point tl, br;
      tl = cv::Point(dets[i].left(), dets[i].top());
      br = cv::Point(dets[i].right(), dets[i].bottom());
      cv::rectangle (cvImg, tl, br, cv::Scalar (0, 255, 0));
    }

    if (filter->shape_predictor) {
      dlib::full_object_detection shape =
          (*filter->shape_predictor) (dlib_img, dets[i]);

      if (filter->display) {
        guint j;
        for (j = 0; j < shape.num_parts (); j++) {
          cv::Point pt (shape.part (j).x (), shape.part (j).y ());
          cv::circle(cvImg, pt, 3, cv::Scalar (255, 0, 0), CV_FILLED);
        }
      }
    }
  }
  cvImg.release ();

  gst_structure_set_value ((GstStructure *) gst_message_get_structure (msg),
      "faces", &facelist);
  auto end = chrono::steady_clock::now();

  cout << "Elapsed time in miliseconds: " <<
      chrono::duration_cast<chrono::milliseconds>(end - start).count() <<
      "ms" << endl;
  return GST_FLOW_OK;
}

static void
gst_cheese_face_detect_finalize (GObject * obj)
{
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (obj);

  cout << "bye :) " << endl;
  if (filter->face_detector)
    delete filter->face_detector;
  if (filter->shape_predictor)
    delete filter->shape_predictor;

  G_OBJECT_CLASS (gst_cheese_face_detect_parent_class)->finalize (obj);
}

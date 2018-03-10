/*
 * GStreamer Plugins Cheese
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2008 Michael Sheldon <mike@mikeasoft.com>
 * Copyright (C) 2011 Stefan Sauer <ensonic@users.sf.net>
 * Copyright (C) 2014 Robert Jobbagy <jobbagy.robert@gmail.com>
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


#define DEFAULT_HUNGARIAN_DELETE_THRESHOLD                72
#define DEFAULT_SCALE_FACTOR                              1.0

GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_detect_debug);
#define GST_CAT_DEFAULT gst_cheese_face_detect_debug

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
  PROP_DISPLAY_BOUNDING_BOX,
  PROP_DISPLAY_ID,
  PROP_DISPLAY_LANDMARK,
  PROP_DISPLAY_POSE_ESTIMATION,
  PROP_LANDMARK,
  PROP_USE_HUNGARIAN,
  PROP_HUNGARIAN_DELETE_THRESHOLD,
  PROP_USE_POSE_ESTIMATION,
  PROP_SCALE_FACTOR
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

  GST_DEBUG_CATEGORY_INIT (gst_cheese_face_detect_debug, "gstcheesefacedetect",
      0, "Cheese Face Detect");

  klass->cheese_face_free_user_data_func = NULL;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstopencvbasefilter_class = (GstOpencvVideoFilterClass *) klass;

  gstopencvbasefilter_class->cv_trans_ip_func =
      gst_cheese_face_detect_transform_ip;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_cheese_face_detect_finalize);
  gobject_class->set_property = gst_cheese_face_detect_set_property;
  gobject_class->get_property = gst_cheese_face_detect_get_property;

  g_object_class_install_property (gobject_class, PROP_DISPLAY_BOUNDING_BOX,
      g_param_spec_boolean ("display-bounding-box", "Display",
          "Sets whether the detected faces should be highlighted in the output",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_ID,
      g_param_spec_boolean ("display-id", "Display the ID of each face",
          "Sets whether to display the ID of each face",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_LANDMARK,
      g_param_spec_boolean ("display-landmark", "Display the landmark",
          "Sets whether display the landmark for each face",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_POSE_ESTIMATION,
      g_param_spec_boolean ("display-pose-estimation", "Display the landmark",
          "Sets whether display the axis of the pose estimation for each face",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_LANDMARK,
      g_param_spec_string ("landmark", "Landmark shape model",
          "Location of the shape model. You can get one from "
          "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2",
          NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_USE_HUNGARIAN,
      g_param_spec_boolean ("use-hungarian", "Display",
          "Sets whether to use the Hungarian algorithm to matching faces in "
          "the next frames.",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_HUNGARIAN_DELETE_THRESHOLD,
      g_param_spec_uint ("hungarian-delete-threshold",
          "Hungarian delete threshold",
          "Sets the number of frames that need to pass to delete a face if it "
          "has not been detected in that period",
          0, G_MAXUINT,
          DEFAULT_HUNGARIAN_DELETE_THRESHOLD,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_USE_POSE_ESTIMATION,
      g_param_spec_boolean ("use-pose-estimation", "Pose estimation",
          "Sets whether to use estimate the pose of each face.",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_SCALE_FACTOR,
      g_param_spec_float ("scale-factor", "Scale Factor",
          "Sets the scale factor the frame which be scaled with before face "
          "detection",
          0, G_MAXFLOAT,
          DEFAULT_SCALE_FACTOR,
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

static void
gst_cheese_face_detect_init (GstCheeseFaceDetect * filter)
{
  filter->last_face_id = 0;
  filter->frame_number = 1;

  filter->use_hungarian = TRUE;
  filter->use_pose_estimation = TRUE;
  filter->hungarian_delete_threshold = DEFAULT_HUNGARIAN_DELETE_THRESHOLD;
  filter->display_bounding_box = TRUE;
  filter->display_id = TRUE;
  filter->display_pose_estimation = TRUE;
  filter->landmark = NULL;
  filter->face_detector =
      new frontal_face_detector (get_frontal_face_detector());
  filter->shape_predictor = NULL;
  filter->scale_factor = DEFAULT_SCALE_FACTOR;

  filter->faces = new std::map<guint, CheeseFace>;

  filter->pose_model_points = new std::vector<cv::Point3d>;

  filter->camera_matrix = NULL;
  filter->dist_coeffs = NULL;
  /* Pose estimation 3D model */
  /* Nose tip */
  filter->pose_model_points->push_back (cv::Point3d (0.f, 0.f, 0.f));
  /* Chin */
  filter->pose_model_points->push_back (cv::Point3d (0.f, -330.f, -65.f));
  /* Left eye left corner */
  filter->pose_model_points->push_back (cv::Point3d (-225.f, 170.f, -135.f));
  /* Right eye right corner */
  filter->pose_model_points->push_back (cv::Point3d (225.f, 170.f, -135.f));
  /* Left Mouth corner */
  filter->pose_model_points->push_back (cv::Point3d (-150.f, -150.f, -125.f));
  /* Right mouth corner */
  filter->pose_model_points->push_back (cv::Point3d (150.f, -150.f, -125.f));

  gst_opencv_video_filter_set_in_place (GST_OPENCV_VIDEO_FILTER_CAST (filter),
      TRUE);
}

static void
gst_cheese_face_detect_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (object);

  switch (prop_id) {
    case PROP_DISPLAY_BOUNDING_BOX:
      filter->display_bounding_box = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_ID:
      filter->display_id = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_LANDMARK:
      filter->display_landmark = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_POSE_ESTIMATION:
      filter->display_pose_estimation = g_value_get_boolean (value);
      break;
    case PROP_LANDMARK:
      filter->landmark = g_value_dup_string (value);
      filter->shape_predictor = new dlib::shape_predictor;
      /* TODO: Handle error. For example, file does not exist. */
      dlib::deserialize (filter->landmark) >> *filter->shape_predictor;
      break;
    case PROP_USE_HUNGARIAN:
      filter->use_hungarian = g_value_get_boolean (value);
      break;
    case PROP_HUNGARIAN_DELETE_THRESHOLD:
      filter->hungarian_delete_threshold = g_value_get_uint (value);
      break;
    case PROP_USE_POSE_ESTIMATION:
      filter->use_pose_estimation = g_value_get_boolean (value);
      break;
    case PROP_SCALE_FACTOR:
      filter->scale_factor = g_value_get_float (value);
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
    case PROP_DISPLAY_BOUNDING_BOX:
      g_value_set_boolean (value, filter->display_bounding_box);
      break;
    case PROP_DISPLAY_ID:
      g_value_set_boolean (value, filter->display_id);
      break;
    case PROP_DISPLAY_LANDMARK:
      g_value_set_boolean (value, filter->display_landmark);
      break;
    case PROP_LANDMARK:
      g_value_set_string (value, filter->landmark);
      break;
    case PROP_DISPLAY_POSE_ESTIMATION:
      g_value_set_boolean (value, filter->display_pose_estimation);
      break;
    case PROP_USE_HUNGARIAN:
      g_value_set_boolean (value, filter->use_hungarian);
      break;
    case PROP_HUNGARIAN_DELETE_THRESHOLD:
      g_value_set_uint (value, filter->hungarian_delete_threshold);
      break;
    case PROP_USE_POSE_ESTIMATION:
      g_value_set_boolean (value, filter->use_pose_estimation);
      break;
    case PROP_SCALE_FACTOR:
      g_value_set_float (value, filter->scale_factor);
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

  s = gst_structure_new ("cheesefacedetect",
      "timestamp", G_TYPE_UINT64, GST_BUFFER_TIMESTAMP (buf),
      "stream-time", G_TYPE_UINT64, stream_time,
      "running-time", G_TYPE_UINT64, running_time,
      "duration", G_TYPE_UINT64, GST_BUFFER_DURATION (buf), NULL);

  return gst_message_new_element (GST_OBJECT (filter), s);
}

static cv::Point
calculate_centroid(cv::Point & p1, cv::Point & p2)
{
  return (p1 + p2) / 2;
}

static cv::Point
calculate_centroid(dlib::rectangle & rectangle)
{
  cv::Point tl, br;
  tl = cv::Point(rectangle.left(), rectangle.top());
  br = cv::Point(rectangle.right(), rectangle.bottom());
  return (tl + br) / 2;
}


/* Took from examples OpenCV source code */
// Converts a given Rotation Matrix to Euler angles
cv::Mat rot2euler(const cv::Mat & rotationMatrix)
{
  cv::Mat euler(3,1,CV_64F);

  double m00 = rotationMatrix.at<double>(0,0);
  double m02 = rotationMatrix.at<double>(0,2);
  double m10 = rotationMatrix.at<double>(1,0);
  double m11 = rotationMatrix.at<double>(1,1);
  double m12 = rotationMatrix.at<double>(1,2);
  double m20 = rotationMatrix.at<double>(2,0);
  double m22 = rotationMatrix.at<double>(2,2);

  double x, y, z;

  // Assuming the angles are in radians.
  if (m10 > 0.998) { // singularity at north pole
    x = 0;
    y = CV_PI/2;
    z = atan2(m02,m22);
  }
  else if (m10 < -0.998) { // singularity at south pole
    x = 0;
    y = -CV_PI/2;
    z = atan2(m02,m22);
  }
  else
  {
    x = atan2(-m12,m11);
    y = asin(m10);
    z = atan2(-m20,m00);
  }

  euler.at<double>(0) = x;
  euler.at<double>(1) = y;
  euler.at<double>(2) = z;

  return euler;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_cheese_face_detect_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  guint i, j;
  GValue faces_values = G_VALUE_INIT;
  GstMessage *msg;
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (base);
  GstCheeseFaceDetectClass *klass = GST_CHEESEFACEDETECT_GET_CLASS (filter);
  /* TODO */
  /* Handle more cases for posting messages like gstfacedetect. */
  gboolean post_msg = TRUE;
  cv::Mat cvImg (cv::cvarrToMat (img));
  cv::Mat resizedImg;
  cv_image<bgr_pixel> dlib_img;
  gboolean debug = gst_debug_is_active ();
  gint64 start, end;
  gint64 time_scale_down, time_hungarian, time_face_detection, time_scale_up;
  gint64 time_pose_estimation, time_landmark, time_others, time_total;
  gint64 time_post;

  time_scale_down = time_hungarian = time_face_detection = time_scale_up =
      time_pose_estimation = time_landmark = time_post = -1;

  if (debug)
    time_total = cv::getTickCount ();
  GST_LOG ("Frame size: %d (height) x %d (width).", img->height, img->width);

  if (post_msg)
    time_post = 0;

  if (post_msg) {
    if (debug)
      start = cv::getTickCount ();
    msg = gst_cheese_face_detect_message_new (filter, buf);
    g_value_init (&faces_values, GST_TYPE_LIST);
    if (debug) {
      end = cv::getTickCount ();
      time_post += end - start;
    }
  }

  /* Scale the frame */
  if (filter->scale_factor != 1.0) {
    if (debug)
      start = cv::getTickCount ();
    cv::resize(cvImg, resizedImg, cv::Size(img->width * filter->scale_factor,
        img->height * filter->scale_factor));
    if (debug) {
      end = cv::getTickCount ();
      time_scale_down = end - start;
    }
    GST_LOG ("Image scaled by the factor %.2f. New image processing size: "
        "%d (height) x %d (width).", filter->scale_factor,
        resizedImg.rows, resizedImg.cols);
    dlib_img = cv_image<bgr_pixel> (resizedImg);
  } else
    dlib_img = cv_image<bgr_pixel> (cvImg);

  /* start = chrono::steady_clock::now(); */
  if (debug)
    start = cv::getTickCount ();
  std::vector<rectangle> dets = (*filter->face_detector) (dlib_img);
  if (debug) {
    end = cv::getTickCount ();
    time_face_detection = end - start;
  }

  /* Get the original coordinates */
  if (filter->scale_factor != 1.0) {
    if (debug)
      start = cv::getTickCount ();
    for (i = 0; i < dets.size (); i++) {
      dlib::rectangle new_det (
          dets[i].left () / filter->scale_factor,
          dets[i].top () / filter->scale_factor,
          dets[i].right () / filter->scale_factor,
          dets[i].bottom () / filter->scale_factor);
      dets[i] = new_det;
    }
    if (debug) {
      end = cv::getTickCount ();
      time_scale_up = end - start;
    }
  }

  if (!filter->use_hungarian) {
    /* If we are not remapping faces by using the Hungarian Algorithm
     * so reset all the info as defaults. */
    filter->last_face_id = 0;
    filter->faces->clear ();
  }

  /* Init faces */
  if (filter->faces->empty ()) {
    for (i = 0; i < dets.size(); i++) {
      /**
       * FIXME (Ineficient code)
       * When we declare a new CheeseFace in this way we actually create a
       * copy and this copy is obviously destructed after the end of this
       * block/scope. A solution may be to use a constructor but I am not
       * sure yet if CheeseFace should be a struct, a C++ class, a GObject...
       **/
      CheeseFace face_info;
      face_info.last_detected_frame = filter->frame_number;
      face_info.bounding_box = dets[i];
      face_info.centroid = calculate_centroid(dets[i]);
      face_info.free_user_data_func = klass->cheese_face_free_user_data_func;
      (*filter->faces)[++filter->last_face_id] = face_info;
    };
  }

  if (filter->use_hungarian) {
    std::vector<guint> to_remove;
    for (auto &kv : *filter->faces) {
      guint delta_since_detected;
      guint id = kv.first;
      CheeseFace &face = kv.second;

      delta_since_detected = filter->frame_number - face.last_detected_frame;
      GST_LOG ("Face %d: number of frames passed since last detection of "
          "face is delta=%d.", id, delta_since_detected);

      if (delta_since_detected > filter->hungarian_delete_threshold) {
        GST_LOG ("Face %d will be deleted: "
            "delta=%d > hungarian-delete-threshold=%d.", id,
            delta_since_detected, filter->hungarian_delete_threshold);
        to_remove.push_back (id);
      }
    }
    for (i = 0; i < to_remove.size (); i++) {
      GST_LOG ("Face %d was deleted.", i);
      filter->faces->erase (to_remove[i]);
    }
  }

  if (!filter->faces->empty() && filter->use_hungarian) {
    guint r, c;
    HungarianAlgorithm HungAlgo;
    std::vector<cv::Point> cur_centroids;
    std::vector<std::vector<double>> cost_matrix;
    std::vector<guint> faces_keys;
    std::vector<CheeseFace *> faces_vals;
    std::vector<int> assignment;

    if (debug)
      start = cv::getTickCount ();

    // Calculate current centroids.
    for (i = 0; i < dets.size(); i++) {
      cv::Point centroid = calculate_centroid(dets[i]);
      cur_centroids.push_back(centroid);
    }

    /* Put the info of the previous detected faces in vectors */
    for (auto &kv : (*filter->faces)) {
      guint id = kv.first;
      CheeseFace *face = &kv.second;
      faces_keys.push_back(id);
      faces_vals.push_back(face);
    }

    GST_LOG ("Hungarian method: initialize cost matrix of "
        "previous detected faces x current detected faces: %d(rows) x %d(cols)",
        (gint) faces_vals.size (), (gint) cur_centroids.size ());
    /* Initialize cost matrix. */
    for (r = 0; r < faces_vals.size (); r++) {
      std::vector<double> row;
      for (c = 0; c < cur_centroids.size (); c++) {
        float dist;
        dist = cv::norm (cv::Mat (cur_centroids[c]),
            cv::Mat (faces_vals[r]->centroid));
        row.push_back(dist);
      }
      cost_matrix.push_back(row);
    }

    /* Solve the Hungarian problem */
    GST_LOG ("Hungarian method: solve the Hungarian problem.");
    HungAlgo.Solve(cost_matrix, assignment);

    /* Reorder faces */
    GST_LOG ("Hungarian method: reorder faces according the solution of the"
        "Hungarian problem.");
    for (i = 0; i < faces_vals.size(); i++) {
      if (assignment[i] == -1) {
        GST_LOG ("Hungarian method: current detected face at position %d "
            "will be ignored.", i);
      } else {
        faces_vals[i]->bounding_box = dets[assignment[i]];
        faces_vals[i]->centroid = calculate_centroid (dets[assignment[i]]);
        faces_vals[i]->last_detected_frame = filter->frame_number;
        GST_LOG ("Hungarian method: previous detected face %d mapped to "
            "current detected face at position %d.", i, assignment[i]);
      }
    }

    /* Create new faces */
    GST_LOG ("Create new faces for restant faces not assigned by the "
        "Hungarian method.");
    for (i = 0; i < dets.size (); i++) {
      if (!(std::find (assignment.begin(), assignment.end (), i) !=
          assignment.end ())) {
        /**
         * FIXME
         * When we declare a new CheeseFace in this way we actually create a
         * copy and this copy is obviously destructed after the end of this
         * block/scope. Avoid doing copies!
         **/
        CheeseFace face_info;
        face_info.last_detected_frame = filter->frame_number;
        face_info.bounding_box = dets[i];
        face_info.centroid = calculate_centroid(dets[i]);
        (*filter->faces)[++filter->last_face_id] = face_info;
        face_info.free_user_data_func = klass->cheese_face_free_user_data_func;
        GST_LOG ("Face %d has been created.", filter->last_face_id);
      }
    }
    if (debug) {
      end = cv::getTickCount ();
      time_hungarian = end - start;
    }
  }

  /* Initialize camera matrix */
  if (!filter->camera_matrix) {
    cv::Point2d center (cvImg.cols / 2, cvImg.rows / 2);
    double focal_length = cvImg.cols;
    filter->camera_matrix = new cv::Mat;
    filter->dist_coeffs = new cv::Mat;
    *filter->camera_matrix =
        (cv::Mat_<double>(3, 3) << focal_length, 0, center.x,
                                   0, focal_length, center.y,
                                   0, 0, 1);
    *filter->dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type);
  }

  if (debug) {
    time_pose_estimation = 0;
    time_landmark = 0;
  }
  guint text_i = 0;
  for (auto &kv : *filter->faces) {
    GValue facedata_value = G_VALUE_INIT;
    GstStructure *facedata_st;
    guint id = kv.first;
    CheeseFace &face = kv.second;

    cv::Mat rotation_vector;
    cv::Mat translation_vector;
    std::vector<cv::Point2d> image_points;
    guint pose_pts[6] = {30, 8, 36, 45, 48, 54};

    if (post_msg && face.last_detected_frame == filter->frame_number) {
      if (debug)
        start = cv::getTickCount ();
      facedata_st = gst_structure_new_empty ("face");
      g_value_init (&facedata_value, GST_TYPE_STRUCTURE);
      if (debug) {
        end = cv::getTickCount ();
        time_post += end - start;
      }
    }

    /* Post the bounding box info of the face */
    if (post_msg && face.last_detected_frame == filter->frame_number) {
      if (debug)
        start = cv::getTickCount ();
      GValue box_value = G_VALUE_INIT;
      graphene_rect_t graphene_bounding_box;

      g_value_init (&box_value, GRAPHENE_TYPE_RECT);

      graphene_bounding_box = GRAPHENE_RECT_INIT (face.bounding_box.left (),
          face.bounding_box.top (), face.bounding_box.width (),
          face.bounding_box.height ());
      g_value_set_boxed (&box_value, &graphene_bounding_box);
      gst_structure_set_value (facedata_st, "bounding-box", &box_value);
      if (debug) {
        end = cv::getTickCount ();
        time_post += end - start;
      }
      GST_LOG ("Face %d: add bounding box information to the message.", id);
    }
    /* Draw bounding box of the face */
    if (filter->display_bounding_box &&
        face.last_detected_frame == filter->frame_number) {
      cv::Point tl, br;
      tl = cv::Point(face.bounding_box.left(), face.bounding_box.top());
      br = cv::Point(face.bounding_box.right(), face.bounding_box.bottom());
      cv::rectangle (cvImg, tl, br, cv::Scalar (0, 255, 0));
      GST_LOG ("Face %d: drawing bounding.", id);
    }

    /* Post the ID assigned to the face */
    if (post_msg && face.last_detected_frame == filter->frame_number) {
      GValue id_value = G_VALUE_INIT;
      g_value_init (&id_value, G_TYPE_UINT);
      g_value_set_uint (&id_value, id);
      gst_structure_set_value (facedata_st, "id", &id_value);
      GST_LOG ("Face %d: add id information to the message.", id);
    }
    /* Draw ID assigned to the face */
    if (filter->display_id &&
        face.last_detected_frame == filter->frame_number) {
      cv::putText (cvImg, std::to_string (id), face.centroid,
          cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar (255, 0, 0));
      GST_LOG ("Face %d: drawing id.", id);
    }

    if (filter->shape_predictor) {
      dlib::rectangle scaled_det (
          face.bounding_box.left () * filter->scale_factor,
          face.bounding_box.top () * filter->scale_factor,
          face.bounding_box.right () * filter->scale_factor,
          face.bounding_box.bottom () * filter->scale_factor);

      GST_LOG ("Face %d: detect landmark.", id);
      if (debug)
        start = cv::getTickCount ();
      dlib::full_object_detection shape =
          (*filter->shape_predictor) (dlib_img, scaled_det);
      if (debug) {
        end = cv::getTickCount ();
        time_landmark += end - start;
      }

      /* Draw the landmark of the face or/and post it as a message */
      if ((filter->display_landmark || post_msg) &&
          face.last_detected_frame == filter->frame_number) {
        GValue landmark_values = G_VALUE_INIT;

        if (post_msg)
          g_value_init (&landmark_values, GST_TYPE_ARRAY);

        /* Pose estimation */
        if (filter->use_pose_estimation) {
          cv::Mat rotation_matrix;
          GST_LOG ("Face %d: calculate pose estimation.", id);
          for (i = 0; i < 6; i++) {
            const guint index = pose_pts[i];
            cv::Point2d pt (shape.part (index).x () / filter->scale_factor,
                            shape.part (index).y () / filter->scale_factor);
            image_points.push_back(pt);
          }
          if (debug)
            start = cv::getTickCount ();
          cv::solvePnP (*filter->pose_model_points, image_points,
              *filter->camera_matrix, *filter->dist_coeffs,
              rotation_vector, translation_vector);
          if (debug) {
            end = cv::getTickCount ();
            time_pose_estimation += end - start;
          }

          std::vector<cv::Point3d> nose_end_point3D;
          std::vector<cv::Point2d> nose_end_point2D;
          nose_end_point3D.push_back(cv::Point3d (0, 0, 1000.0));
          nose_end_point3D.push_back(cv::Point3d (0, 1000.0, 0));
          nose_end_point3D.push_back(cv::Point3d (-1000.0, 0, 0));

          projectPoints(nose_end_point3D, rotation_vector, translation_vector,
              *filter->camera_matrix, *filter->dist_coeffs, nose_end_point2D);

          GST_LOG ("Face %d: rotation vector is (%.4f, %.4f, %.4f).", id,
              rotation_vector.at<double> (0, 0),
              rotation_vector.at<double> (1, 0),
              rotation_vector.at<double> (2, 0));
          /* TODO: Log translation matrix */

          if (filter->display_pose_estimation) {
            cv::line(cvImg, image_points[0], nose_end_point2D[0],
                cv::Scalar(255, 0, 0), 2);
            cv::line(cvImg, image_points[0], nose_end_point2D[1],
                cv::Scalar(0, 255, 0), 2);
            cv::line(cvImg, image_points[0], nose_end_point2D[2],
                cv::Scalar(0, 0, 255), 2);
            GST_LOG ("Face %d: drawing pose estimation axis.", id);
          }

          if (post_msg) {
            cv::Vec3d axis {0, 0, -1};
            graphene_point3d_t rotation_graphene_vector;
            GValue rotation_value = G_VALUE_INIT;
            double dir_x, dir_y, dir_z, len;
            cv::Mat direction_matrix;
            if (debug)
              start = cv::getTickCount ();
            g_value_init (&rotation_value, GRAPHENE_TYPE_POINT3D);

            cv::Rodrigues(rotation_vector, rotation_matrix);
            cv::Mat measured_eulers(3, 1, CV_64F);
            measured_eulers = rot2euler(rotation_matrix);

            rotation_graphene_vector = GRAPHENE_POINT3D_INIT (
                (float) measured_eulers.at<double> (0),
                (float) measured_eulers.at<double> (1),
                (float) measured_eulers.at<double> (2));

            /* TODO */
            /* DEBUG Rotation matrix */
            GST_LOG ("Face %d: euler angles are (%.4f, %.4f, %.4f).", id,
                rotation_graphene_vector.x, rotation_graphene_vector.y,
                rotation_graphene_vector.z);

            cv::putText (cvImg,
                g_strdup_printf ("x: %lf", rotation_graphene_vector.x),
                cv::Point (10, (text_i + 1) * 20), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar (0, 255, 0));
            cv::putText (cvImg,
                g_strdup_printf ("y: %lf", rotation_graphene_vector.y),
                cv::Point (10, (text_i + 2) * 20), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar (0, 255, 0));
            cv::putText (cvImg,
                g_strdup_printf ("z: %lf", rotation_graphene_vector.z),
                cv::Point (10, (text_i + 3) * 20), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar (0, 255, 0));

            text_i += 3;
            g_value_set_boxed (&rotation_value, &rotation_graphene_vector);
            gst_structure_set_value (facedata_st, "pose-rotation-vector",
                &rotation_value);
            if (debug) {
              end = cv::getTickCount ();
              time_post += end - start;
            }
            GST_LOG ("Face %d: add pose euler angles to the message.", id);
          }
        }
        /* end */
        face.landmark.clear ();
        for (j = 0; j < shape.num_parts (); j++) {
          cv::Point pt (shape.part (j).x () / filter->scale_factor,
              shape.part (j).y () / filter->scale_factor);
          face.landmark.push_back (pt);
          if (filter->display_landmark) {
            cv::circle(cvImg, pt, 2, cv::Scalar (0, 0, 255), CV_FILLED);
          }
          if (post_msg) {
            GValue point_value = G_VALUE_INIT;
            graphene_point_t graphene_point =
                GRAPHENE_POINT_INIT (shape.part (j).x () / filter->scale_factor,
                    shape.part (j).y () / filter->scale_factor);
            if (debug)
              start = cv::getTickCount ();
            g_value_init (&point_value, GRAPHENE_TYPE_POINT);
            g_value_set_boxed (&point_value, &graphene_point);
            gst_value_array_append_value (&landmark_values, &point_value);
            if (debug) {
              end = cv::getTickCount ();
              time_post += end - start;
            }
          }
        }

        if (post_msg && face.last_detected_frame == filter->frame_number) {
          if (debug)
            start = cv::getTickCount ();
          gst_structure_set_value (facedata_st, "landmark", &landmark_values);
          if (debug) {
            end = cv::getTickCount ();
            time_post += end - start;
          }
          GST_LOG ("Face %d: add landmark information to the message.", id);
        }
      }
    }
    if (post_msg && face.last_detected_frame == filter->frame_number) {
      if (debug)
        start = cv::getTickCount ();
      g_value_take_boxed (&facedata_value, facedata_st);
      gst_value_list_append_value (&faces_values, &facedata_value);
      g_value_unset (&facedata_value);
      if (debug) {
        end = cv::getTickCount ();
        time_post += end - start;
      }
      GST_LOG ("Faces: append message with information about face %d.", id);
    }
  }
  cvImg.release ();
  /* end = chrono::steady_clock::now(); */

  if (post_msg) {
    if (debug)
      start = cv::getTickCount ();
    gst_structure_set_value ((GstStructure *) gst_message_get_structure (msg),
        "faces", &faces_values);
    g_value_unset (&faces_values);
    gst_element_post_message (GST_ELEMENT (filter), msg);
    if (debug) {
      end = cv::getTickCount ();
      time_post += end - start;
    }
    GST_LOG ("Faces: post the message to the bus.");
  }
  if (debug)
    time_total = cv::getTickCount () - time_total;

  if (time_scale_down != -1)
    GST_DEBUG ("Time to scale down frame: %.2f ms.",
        ((double) time_scale_down * 1000) / cv::getTickFrequency());
  if (time_hungarian != -1)
    GST_DEBUG ("Time to solve hungarian problem: %.2f ms.",
        ((double) time_hungarian * 1000) / cv::getTickFrequency());
  if (time_face_detection != -1)
    GST_DEBUG ("Time to detect a face: %.2f ms.",
        ((double) time_face_detection * 1000) / cv::getTickFrequency());
  if (time_scale_up != -1)
    GST_DEBUG ("Time to scale up frame: %.2f ms.",
        ((double) time_scale_up * 1000) / cv::getTickFrequency());
  if (time_pose_estimation != -1)
    GST_DEBUG ("Time to estimate the pose: %.2f ms.",
        ((double) time_pose_estimation * 1000) / cv::getTickFrequency());
  if (time_landmark != -1)
    GST_DEBUG ("Time to calculate landmark: %.2f ms.",
        ((double) time_landmark * 1000) / cv::getTickFrequency());
  if (time_post != -1)
    GST_DEBUG ("Time post: %.2f ms.",
        ((double) time_post * 1000) / cv::getTickFrequency ());
  if (time_total != -1)
    GST_DEBUG ("Time total: %.2f ms.",
        ((double) time_total * 1000) / cv::getTickFrequency ());
  filter->frame_number++;

  return GST_FLOW_OK;
}

static void
gst_cheese_face_detect_finalize (GObject * obj)
{
  GstCheeseFaceDetect *filter = GST_CHEESEFACEDETECT (obj);

  if (filter->face_detector)
    delete filter->face_detector;
  if (filter->shape_predictor)
    delete filter->shape_predictor;
  if (filter->camera_matrix)
    delete filter->camera_matrix;
  if (filter->dist_coeffs)
    delete filter->dist_coeffs;

  G_OBJECT_CLASS (gst_cheese_face_detect_parent_class)->finalize (obj);
}

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

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
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

  filter->pose_model_points->push_back (cv::Point3d (0.0f, 0.0f, 0.0f));               // Nose tip
  filter->pose_model_points->push_back (cv::Point3d (0.0f, -330.0f, -65.0f));          // Chin
  filter->pose_model_points->push_back (cv::Point3d (-225.0f, 170.0f, -135.0f));       // Left eye left corner
  filter->pose_model_points->push_back (cv::Point3d (225.0f, 170.0f, -135.0f));        // Right eye right corner
  filter->pose_model_points->push_back (cv::Point3d (-150.0f, -150.0f, -125.0f));      // Left Mouth corner
  filter->pose_model_points->push_back (cv::Point3d (150.0f, -150.0f, -125.0f));       // Right mouth corner

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
  /* TODO */
  /* Handle more cases for posting messages like gstfacedetect. */
  gboolean post_msg = TRUE;
  cv::Mat cvImg (cv::cvarrToMat (img));
  cv::Mat resizedImg;
  cv_image<bgr_pixel> dlib_img;

  cout << "height: " << img->height << endl;
  cout << "width: " << img->width << endl;

  if (post_msg) {
    msg = gst_cheese_face_detect_message_new (filter, buf);
    g_value_init (&faces_values, GST_TYPE_LIST);
  }

  /* Scale the frame */
  if (filter->scale_factor != 1.0) {
    cv::resize(cvImg, resizedImg, cv::Size(img->width * filter->scale_factor,
        img->height * filter->scale_factor));

    cout << "new height: " << resizedImg.rows << endl;
    cout << "new width: " << resizedImg.cols << endl;

    dlib_img = cv_image<bgr_pixel> (resizedImg);
  } else
    dlib_img = cv_image<bgr_pixel> (cvImg);

  auto start = chrono::steady_clock::now();
  std::vector<rectangle> dets = (*filter->face_detector) (dlib_img);

  /* Get the original coordinates */
  if (filter->scale_factor != 1.0) {
    for (i = 0; i < dets.size (); i++) {
      dlib::rectangle new_det (
          dets[i].left () / filter->scale_factor,
          dets[i].top () / filter->scale_factor,
          dets[i].right () / filter->scale_factor,
          dets[i].bottom () / filter->scale_factor);
      dets[i] = new_det;
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
      CheeseFace face_info;
      face_info.last_detected_frame = filter->frame_number;
      face_info.bounding_box = dets[i];
      face_info.centroid = calculate_centroid(dets[i]);
      (*filter->faces)[++filter->last_face_id] = face_info;
    };
  }

  if (filter->use_hungarian) {
    std::vector<guint> to_remove;
    for (auto &kv : *filter->faces) {
      guint delta_since_detected;
      guint id = kv.first;
      CheeseFace face = kv.second;

      delta_since_detected = filter->frame_number - face.last_detected_frame;
      cout << "delta[" << id << "]=" << delta_since_detected << endl;
      if (delta_since_detected > filter->hungarian_delete_threshold) {
        cout << "TO DELETE ID " << id << endl;
        to_remove.push_back (id);
      }
    }
    for (i = 0; i < to_remove.size (); i++) {
      /*const guint id = to_remove[i];
      std::map<guint,CheeseFace>::iterator it;
      it = mymap.find(id);
      */
      cout << "DELETED ID " << to_remove[i] << endl;
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

    cout << "Calculate current centroids" << endl;

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

    cout << "Initialize cost matrix" << endl;
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
    cout << "Solve the Hungarian problem" << endl;
    HungAlgo.Solve(cost_matrix, assignment);

    /* Reorder faces */
    cout << "Reordering faces" << endl;
    for (i = 0; i < faces_vals.size(); i++) {
      if (assignment[i] == -1) {
        cout << "A face was not detected" << endl;
      } else {
        faces_vals[i]->bounding_box = dets[assignment[i]];
        faces_vals[i]->centroid = calculate_centroid (dets[assignment[i]]);
        faces_vals[i]->last_detected_frame = filter->frame_number;
      }
    }

    /* Create new faces */
    for (i = 0; i < dets.size (); i++) {
      if (!(std::find (assignment.begin(), assignment.end (), i) !=
          assignment.end ())) {
        CheeseFace face_info;
        face_info.last_detected_frame = filter->frame_number;
        face_info.bounding_box = dets[i];
        face_info.centroid = calculate_centroid(dets[i]);
        (*filter->faces)[++filter->last_face_id] = face_info;
      }
    }
  }

  cv::Point2d center (cvImg.cols / 2, cvImg.rows / 2);
  double focal_length = cvImg.cols;
  cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0, center.x,
                                                    0, focal_length, center.y,
                                                    0, 0, 1);
  cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type);

  for (auto &kv : *filter->faces) {
    GValue facedata_value = G_VALUE_INIT;
    GstStructure *facedata_st;
    guint id = kv.first;
    CheeseFace face = kv.second;

    cv::Mat rotation_vector;
    cv::Mat translation_vector;
    std::vector<cv::Point2d> image_points;
    guint pose_pts[6] = {30, 8, 36, 45, 48, 54};

    if (post_msg && face.last_detected_frame == filter->frame_number) {
      facedata_st = gst_structure_new ("face", NULL);
      g_value_init (&facedata_value, GST_TYPE_STRUCTURE);
    }

    /* Post the bounding box info of the face */
    if (post_msg && face.last_detected_frame == filter->frame_number) {
      GValue tl_value = G_VALUE_INIT;
      GValue br_value = G_VALUE_INIT;
      graphene_point_t tl_graphene_point;
      graphene_point_t br_graphene_point;

      g_value_init (&tl_value, GRAPHENE_TYPE_POINT);
      g_value_init (&br_value, GRAPHENE_TYPE_POINT);

      tl_graphene_point = GRAPHENE_POINT_INIT ((float) face.bounding_box.left(),
          (float) face.bounding_box.top());
      br_graphene_point = GRAPHENE_POINT_INIT (
          (float) face.bounding_box.right(),
          (float) face.bounding_box.bottom());
      g_value_set_boxed (&tl_value, &tl_graphene_point);
      g_value_set_boxed (&br_value, &br_graphene_point);

      gst_structure_set_value (facedata_st, "bounding-box-tl", &tl_value);
      gst_structure_set_value (facedata_st, "bounding-box-br", &br_value);
    }
    /* Draw bounding box of the face */
    if (filter->display_bounding_box &&
        face.last_detected_frame == filter->frame_number) {
      cv::Point tl, br;
      tl = cv::Point(face.bounding_box.left(), face.bounding_box.top());
      br = cv::Point(face.bounding_box.right(), face.bounding_box.bottom());
      cv::rectangle (cvImg, tl, br, cv::Scalar (0, 255, 0));
    }

    /* Post the ID assigned to the face */
    if (post_msg && face.last_detected_frame == filter->frame_number) {
      GValue id_value = G_VALUE_INIT;
      g_value_init (&id_value, G_TYPE_UINT);
      g_value_set_uint (&id_value, id);
      gst_structure_set_value (facedata_st, "id", &id_value);
    }
    /* Draw ID assigned to the face */
    if (filter->display_id && face.last_detected_frame == filter->frame_number)
      cv::putText (cvImg, std::to_string (id), face.centroid,
          cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar (255, 0, 0));

    if (filter->shape_predictor) {
      dlib::rectangle scaled_det (
          face.bounding_box.left () * filter->scale_factor,
          face.bounding_box.top () * filter->scale_factor,
          face.bounding_box.right () * filter->scale_factor,
          face.bounding_box.bottom () * filter->scale_factor);

      dlib::full_object_detection shape =
          (*filter->shape_predictor) (dlib_img, scaled_det);

      /* Draw the landmark of the face or/and post it as a message */
      if ((filter->display_landmark || post_msg) &&
          face.last_detected_frame == filter->frame_number) {
        GValue landmark_values = G_VALUE_INIT;

        if (post_msg)
          g_value_init (&landmark_values, GST_TYPE_ARRAY);

        /* Pose estimation */
        if (filter->use_pose_estimation) {
          cv::Mat rotation_matrix;
          cout << "Calculate Pose estimation" << endl;
          for (i = 0; i < 6; i++) {
            const guint index = pose_pts[i];
            cv::Point2d pt (shape.part (index).x () / filter->scale_factor,
                            shape.part (index).y () / filter->scale_factor);
            image_points.push_back(pt);
          }
          cv::solvePnP (*filter->pose_model_points, image_points, camera_matrix,
              dist_coeffs, rotation_vector, translation_vector);

          std::vector<cv::Point3d> nose_end_point3D;
          std::vector<cv::Point2d> nose_end_point2D;
          nose_end_point3D.push_back(cv::Point3d (0, 0, 1000.0));
          nose_end_point3D.push_back(cv::Point3d (0, 1000.0, 0));
          nose_end_point3D.push_back(cv::Point3d (-1000.0, 0, 0));

          projectPoints(nose_end_point3D, rotation_vector, translation_vector,
              camera_matrix, dist_coeffs, nose_end_point2D);

          if (filter->display_pose_estimation) {
            cv::line(cvImg, image_points[0], nose_end_point2D[0],
                cv::Scalar(255, 0, 0), 2);
            cv::line(cvImg, image_points[0], nose_end_point2D[1],
                cv::Scalar(0, 255, 0), 2);
            cv::line(cvImg, image_points[0], nose_end_point2D[2],
                cv::Scalar(0, 0, 255), 2);
          }

          if (false) {
            graphene_point3d_t rotation_graphene_vector;
            GValue rotation_value = G_VALUE_INIT;

            g_value_init (&rotation_value, GRAPHENE_TYPE_POINT);

            cv::Rodrigues(rotation_vector, rotation_matrix);
            rotation_graphene_vector = GRAPHENE_POINT3D_INIT (
                rotation_matrix.at<float> (0,0),
                rotation_matrix.at<float> (0,1),
                rotation_matrix.at<float> (0,2));
            g_value_set_boxed (&rotation_value, &rotation_graphene_vector);
            gst_structure_set_value (facedata_st, "pose-rotation-vector",
                &rotation_value);
          }

          cout << "n points" << nose_end_point2D.size () << endl;

          cout << "Rotation Vector " << endl << rotation_vector << endl;
          cout << "Translation Vector" << endl << translation_vector << endl;
          cout <<  nose_end_point2D << endl;
        }
        /* end */
        for (j = 0; j < shape.num_parts (); j++) {
          if (filter->display_landmark) {
            cv::Point pt (shape.part (j).x () / filter->scale_factor,
                shape.part (j).y () / filter->scale_factor);
            cv::circle(cvImg, pt, 2, cv::Scalar (0, 0, 255), CV_FILLED);
          }
          if (post_msg) {
            GValue point_value;
            graphene_point_t graphene_point =
                GRAPHENE_POINT_INIT (shape.part (j).x () / filter->scale_factor,
                    shape.part (j).y () / filter->scale_factor);
            g_value_init (&point_value, GRAPHENE_TYPE_POINT);
            g_value_set_boxed (&point_value, &graphene_point);
            gst_value_array_append_value (&landmark_values, &point_value);
          }
        }

        if (post_msg && face.last_detected_frame == filter->frame_number)
          gst_structure_set_value (facedata_st, "landmark", &landmark_values);
      }
    }
    if (post_msg) {
      cout << "Append face values" << endl;
      g_value_take_boxed (&facedata_value, facedata_st);
      gst_value_list_append_value (&faces_values, &facedata_value);
    }
  }
  cvImg.release ();
  auto end = chrono::steady_clock::now();

  if (post_msg) {
    gst_structure_set_value ((GstStructure *) gst_message_get_structure (msg),
        "faces", &faces_values);
    g_value_unset (&faces_values);
    gst_element_post_message (GST_ELEMENT (filter), msg);
  }

  cout << "Elapsed time in miliseconds: " <<
      chrono::duration_cast<chrono::milliseconds>(end - start).count() <<
      "ms" << endl;

  filter->frame_number++;

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

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

#ifndef __GST_CHEESEFACEDETECT_H__
#define __GST_CHEESEFACEDETECT_H__

#include <gst/gst.h>
#include <graphene.h>
#include <graphene-gobject.h>
#include <gst/opencv/gstopencvvideofilter.h>

#include "opencv2/opencv.hpp"
#include <opencv2/core/core_c.h>
#if (CV_MAJOR_VERSION >= 3)
#include <opencv2/imgproc/imgproc_c.h>
#endif

#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>

#include <chrono>
#include <string>
#include <map>

#include "Hungarian.h"



using namespace std;
using namespace dlib;


G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CHEESEFACEDETECT \
  (gst_cheese_face_detect_get_type())
#define GST_CHEESEFACEDETECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CHEESEFACEDETECT,GstCheeseFaceDetect))
#define GST_CHEESEFACEDETECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CHEESEFACEDETECT,GstCheeseFaceDetectClass))
#define GST_IS_CHEESEFACEDETECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CHEESEFACEDETECT))
#define GST_IS_CHEESEFACEDETECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CHEESEFACEDETECT))

typedef struct _GstCheeseFaceDetect      GstCheeseFaceDetect;
typedef struct _GstCheeseFaceDetectClass GstCheeseFaceDetectClass;

struct CheeseFace {
  public:
    cv::Point centroid;
    dlib::rectangle bounding_box;
    dlib::rectangle scaled_bounding_box;
    guint last_detected_frame;

    CheeseFace ()
    {
    }

    ~CheeseFace ()
    {
    }
};

struct _GstCheeseFaceDetect
{
  GstOpencvVideoFilter element;

  /* props */
  gboolean display_bounding_box;
  gboolean display_id;
  gboolean display_landmark;
  gboolean display_pose_estimation;
  gchar *landmark;
  gboolean use_hungarian;
  gboolean use_pose_estimation;
  guint hungarian_delete_threshold;
  gfloat scale_factor;

  /* private props */
  dlib::frontal_face_detector *face_detector;
  dlib::shape_predictor *shape_predictor;
  std::vector<cv::Point3d> *pose_model_points;

  guint last_face_id;
  guint frame_number;
  std::map<guint, CheeseFace> *faces;
};

struct _GstCheeseFaceDetectClass 
{
  GstOpencvVideoFilterClass parent_class;
};

GType gst_cheese_face_detect_get_type (void);

G_END_DECLS

#endif /* __GST_CHEESEFACEDETECT_H__ */

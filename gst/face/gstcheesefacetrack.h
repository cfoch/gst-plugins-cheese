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

#ifndef __GST_CHEESEFACETRACK_H__
#define __GST_CHEESEFACETRACK_H__

#include <gst/gst.h>
#include <graphene.h>
#include <graphene-gobject.h>
#include <gst/opencv/gstopencvvideofilter.h>

#include "opencv2/opencv.hpp"
#include <opencv2/core/core_c.h>
#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#if (CV_MAJOR_VERSION >= 3)
#include <opencv2/imgproc/imgproc_c.h>
#endif

#include <dlib/string.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>

#include <string>
#include <map>
#include <math.h>

#include "Hungarian.h"

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CHEESEFACETRACK \
  (gst_cheese_face_track_get_type())
#define GST_CHEESEFACETRACK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CHEESEFACETRACK,GstCheeseFaceTrack))
#define GST_CHEESEFACETRACK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CHEESEFACETRACK,GstCheeseFaceTrackClass))
#define GST_IS_CHEESEFACETRACKER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CHEESEFACETRACK))
#define GST_IS_CHEESEFACETRACK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CHEESEFACETRACK))
#define GST_CHEESEFACETRACK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_CHEESEFACETRACK,GstCheeseFaceTrackClass))

#define MAX_FACIAL_KEYPOINTS      68

typedef enum {
  GST_CHEESEFACETRACK_TRACKER_BOOSTING,
  GST_CHEESEFACETRACK_TRACKER_GOTURN,
  GST_CHEESEFACETRACK_TRACKER_KCF,
  GST_CHEESEFACETRACK_TRACKER_MEDIANFLOW,
  GST_CHEESEFACETRACK_TRACKER_MIL,
  GST_CHEESEFACETRACK_TRACKER_TLD
} GstCheeseFaceTrackTrackerType;


typedef struct _GstCheeseFaceTrack      GstCheeseFaceTrack;
typedef struct _GstCheeseFaceTrackClass GstCheeseFaceTrackClass;

/**
 * I still doubt if this struct should contain objects or just C structs like
 * graphene points and rects
 **/

GType gst_cheese_face_track_get_type (void);

G_END_DECLS

#endif /* __GST_CHEESEFACETRACK_H__ */

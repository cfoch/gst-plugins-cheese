#include <cstdio>
#include <vector>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_io.h>
#include <dlib/gui_widgets.h>
#include <dlib/opencv.h>
#include "opencv2/opencv.hpp"

#include <gst/gst.h>
#include <gst/opencv/gstopencvvideofilter.h>

#include <opencv2/core/core_c.h>
#if (CV_MAJOR_VERSION >= 3)
#include <opencv2/imgproc/imgproc_c.h>
#endif
#include <chrono>

using namespace cv;
using namespace std;
using namespace dlib;

static inline Scalar
BOX_COLOR(int i)
{
    return Scalar(i == 0 ? 255 : 0, 0, i == 1 ? 255 : 0);
}

int
main(int argc, const char ** argv)
{
  VideoCapture cap;
  const char * filename;
  namedWindow("capture", WINDOW_AUTOSIZE);
  frontal_face_detector detector;

  cout << "naargs: " << argc << endl;

  if (argc < 2) {
    cerr << "This program receives a filename as an argument" << endl;
    exit (1);
  }

  filename = argv[1];
  cap.open(0);

  if (!cap.isOpened ()) {
    cerr << "There was a problem opening '" << filename << "'" << endl;
    exit (1);
  }

  detector = get_frontal_face_detector();

  for (;;) {
    Mat frame;
    int i;

    cap >> frame;
    if (!cap.read (frame))
      break;

	auto start = chrono::steady_clock::now();
    cv_image<bgr_pixel> dlib_image(frame);
    std::vector<dlib::rectangle> dets = detector(dlib_image);
	auto end = chrono::steady_clock::now();
    cout << "Elapsed time in miliseconds: " <<
        chrono::duration_cast<chrono::milliseconds>(end - start).count() <<
        "ms" << endl;

    for (i = 0; i < dets.size(); i++) {
        cv::Point tl, br;
        tl = cv::Point(dets[i].left(), dets[i].top());
        br = cv::Point(dets[i].right(), dets[i].bottom());

        cv::rectangle(frame, tl, br, BOX_COLOR(i));
    }

    cout << "DETECTED FACES: " << dets.size() << endl;


    imshow ("capture", frame);
    if (waitKey (30) >= 0)
      break;
  }

  return 0;
}

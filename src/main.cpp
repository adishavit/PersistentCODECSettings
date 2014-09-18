#include <iostream>
#include "cap_vfw.h"

int main(int argc, char* argv[])
{
   using namespace cv;
   using namespace std;

   if (argc < 3)
   {
     cout << "Usage: " << argv[0] << "<AVI filename> < fourcc | codec-settings-filename >" << endl;
     return 0;
   }

   // create camera capture
   VideoCapture cap; 
   cap.open(0);
   Mat frame;
 
   cap >> frame;

   // create video writer
   CvVideoWriter_VFW vid;
   
   if (!vid.open(argv[1], argv[2], 15, cvSize(frame.cols, frame.rows), 1))
   {
      cout << "Failed opening AVI file."<< endl;
      return 0;
   }

   std::string winName = "Press ESC to close. ";
   winName += "  :: Video=";
   winName += argv[1];
   winName += " :: CODEC=";
   winName += argv[2];
   winName += " ::";

   namedWindow(winName);
   while (true)
   {
      cap >> frame;
      if (!frame.data)
         break;

      imshow(winName, frame);
      if (waitKey(1) >= 0)
         break;

      vid.addFrame(&IplImage(frame));
   }
   return 0;
}
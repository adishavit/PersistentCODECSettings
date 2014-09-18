#ifndef CAP_VFW_H__
#define CAP_VFW_H__

#include <cv.h>
#include <highgui.h>

#if defined WIN32 || defined _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// #undef min
// #undef max
#endif

#include <vfw.h>
#include <vector>

class CvVideoWriter_VFW
{
public:
   CvVideoWriter_VFW();
   ~CvVideoWriter_VFW();

   bool open( const char* filename, int fourcc, double fps, CvSize const& frameSize, bool isColor );
   bool open( const char* filename, std::string fourcc_or_codecConfigFileName, double fps, CvSize const& frameSize, bool isColor );

   void close();

   bool addFrame( const IplImage* );

private:
   bool fileExists( std::string &configFileName ); // utility function
   void resetParams();
   bool createStreams( CvSize frameSize, bool isColor );
   bool writeCodecParams(const char* configFileName) const;
   bool readCodecParams(const char* configFileName);

private:
   PAVIFILE      avifile_;
   PAVISTREAM    compressed_;
   PAVISTREAM    uncompressed_;
   AVICOMPRESSOPTIONS copts_;
   double      fps_;
   IplImage*   tempFrame_;
   long        pos_;
   int         fourcc_;
   std::string codecConfigFileName_;
   bool shouldCallAVISaveOptionsFree_;

   // vectors to hold the internal codec settings.
   std::vector<char> copts_Parms_, copts_Format_;
};

#endif // CAP_VFW_H__

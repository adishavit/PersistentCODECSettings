#include <io.h>
#include <fstream>
#include <fstream>
#include <iostream>

#include "cap_vfw.h"


/********************* Capturing video from AVI via VFW ************************/

void CvVideoWriter_VFW::resetParams()
{
  avifile_                    = NULL;
  compressed_ = uncompressed_ = NULL;
  tempFrame_                  = NULL;
  fps_    = 0;
  pos_    = 0;
  fourcc_ = 0;
}

void CvVideoWriter_VFW::close()
{
  if( compressed_ )
  {
    codecConfigFileName_.clear();

    size_t refCount = AVIStreamRelease( compressed_ );
    assert(0 == refCount);
    compressed_ = NULL;
  }
  if( uncompressed_ )
  { 
    size_t refCount = AVIStreamRelease( uncompressed_ );
    assert(0 == refCount);
    uncompressed_ = NULL;
  }
  if (shouldCallAVISaveOptionsFree_)
  { // Free the resources allocated by the AVISaveOptions function.
    AVICOMPRESSOPTIONS* pcopts = &copts_;
    AVISaveOptionsFree(1, &pcopts); 
    shouldCallAVISaveOptionsFree_ = false;
  }
  if( avifile_ )
  {
    size_t refCount = AVIFileRelease( avifile_ );
    assert(0 == refCount);
    avifile_ = NULL;
  }

  cvReleaseImage( &tempFrame_ );
  assert(NULL == tempFrame_);
  resetParams();
}


// philipg.  Made this code capable of writing 8bpp gray scale bitmaps
struct BITMAPINFO_8Bit
{  
  // empty default ctor
  BITMAPINFO_8Bit(){}

  // value filling ctor.
  BITMAPINFO_8Bit(int width, int height, int bpp, int compression = BI_RGB)
  {
    // Initialize the BITMAPINFOHEADER
    memset( &bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(bmiHeader);
    bmiHeader.biWidth = width;
    bmiHeader.biHeight = height;
    bmiHeader.biBitCount = (WORD)bpp;
    bmiHeader.biCompression = compression;
    bmiHeader.biPlanes = 1;

    // For the case of 8 bit images, init the RGBQUAD color table.
    for( int i = 0; i < 256; i++ )
    {
      bmiColors[i].rgbBlue = (BYTE)i;
      bmiColors[i].rgbGreen = (BYTE)i;
      bmiColors[i].rgbRed = (BYTE)i;
      bmiColors[i].rgbReserved = 0;
    }
  }

  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[256];
};

//////////////////////////////////////////////////////////////////////////

bool CvVideoWriter_VFW::open( const char* filename, int fourcc, double fps, CvSize const& frameSize, bool isColor )
{
  close();

  if( AVIFileOpen( &avifile_, filename, OF_CREATE | OF_WRITE, 0 ) == AVIERR_OK )
  {
    fourcc_ = fourcc;
    fps_    = fps;

    if( frameSize.width > 0 && frameSize.height > 0 &&
      !createStreams( frameSize, isColor ) )
    {
      close();
      return false;
    }
  }
  return true;
}

bool CvVideoWriter_VFW::open( const char* filename, std::string fourcc_or_codecConfigFileName, double fps, CvSize const& frameSize, bool isColor )
{
  // if the fourcc_or_codecConfigFileName string is 4 characters long, then we assume it is a fourcc
  // otherwise it is assumed to be codeConfigFileName
  if (4 != fourcc_or_codecConfigFileName.size())
  {
    codecConfigFileName_ = fourcc_or_codecConfigFileName;
    fourcc_ = -1;
  }
  else
  {
    codecConfigFileName_.clear();
    fourcc_ = mmioFOURCC(fourcc_or_codecConfigFileName[0], fourcc_or_codecConfigFileName[1], fourcc_or_codecConfigFileName[2], fourcc_or_codecConfigFileName[3]);
  }

  return open( filename, fourcc_, fps, frameSize, isColor );
}

bool CvVideoWriter_VFW::createStreams( CvSize frameSize, bool isColor )
{
  if( !avifile_ )
    return false;

  AVISTREAMINFO aviinfo;

  // Set basic file format and info
  memset( &aviinfo, 0, sizeof(aviinfo));
  aviinfo.fccType = streamtypeVIDEO;
  aviinfo.fccHandler = 0;
  // use highest possible accuracy for dwRate/dwScale
  aviinfo.dwScale = (DWORD)((double)0x7FFFFFFF / fps_);
  aviinfo.dwRate = cvRound(fps_ * aviinfo.dwScale);
  aviinfo.rcFrame.top = aviinfo.rcFrame.left = 0;
  aviinfo.rcFrame.right = frameSize.width;
  aviinfo.rcFrame.bottom = frameSize.height;

  if( AVIERR_OK != AVIFileCreateStream( avifile_, &uncompressed_, &aviinfo ) )
    return false; // "Unable to Create Video Stream in the Movie File"

  // Proceed to create compressed stream...
  // if a codec config file was not given, then assume a FOURCC was given
  if (codecConfigFileName_.empty() && fourcc_ != -1) 
  {
    // initialize AVICOMPRESSOPTIONS struct
    ZeroMemory(&copts_,sizeof(AVICOMPRESSOPTIONS)); // reset to 0s
    copts_.fccType = streamtypeVIDEO;
    copts_.fccHandler = fourcc_;
    copts_.dwKeyFrameEvery = 1;
    copts_.dwQuality = 10000;
    copts_.dwFlags = AVICOMPRESSF_VALID;
  }
  else
  {
    // A codec config file name was given.
    if (!fileExists(codecConfigFileName_)) 
    {
      // No such codec config file found. 
      // Open the AVISaveOptions() dlg and when it returns store the setting 
      // to this file name and use these settings.
      ZeroMemory(&copts_,sizeof(AVICOMPRESSOPTIONS)); // reset to 0s 
      AVICOMPRESSOPTIONS* pcopts = &copts_;
      if (!AVISaveOptions(NULL, 0, 1, &uncompressed_, &pcopts)) // Open Video Compression dlg.
        return false;

      shouldCallAVISaveOptionsFree_ = true;

      if (!codecConfigFileName_.empty())
        if (!writeCodecParams(codecConfigFileName_.c_str())) // write codec setting to file.
          return false; // write failed
    }
    else // codec file exists, load it and use it
      if (!readCodecParams(codecConfigFileName_.c_str()))
        return false; // read failed
  }

  if(AVIERR_OK != AVIMakeCompressedStream( &compressed_, uncompressed_, &copts_, 0 ))
  {
    // "Unable to Create Compressed Stream: Check your CODEC options"
    // One reason this error might occur is if you are using a Codec that is not
    // available on your system. Check the mmioFOURCC() code you are using and make
    // sure you have that codec installed properly on your machine.      
    return false;
  }   

  // Set frame format struct
  BITMAPINFO_8Bit bmih(frameSize.width, frameSize.height, isColor ? 24 : 8);

  if (AVIERR_OK != AVIStreamSetFormat( compressed_, 0, &bmih, sizeof(bmih)) )
  {
    // One reason this error might occur is if your bitmap does not meet the CODEC requirements.
    // For example,
    //   your bitmap is 32bpp while the Codec supports only 16 or 24 bpp; Or
    //   your bitmap is 274 * 258 size, while the Codec supports only sizes that are powers of 2; etc...
    // Possible solution to avoid this is: make your bitmap suit the codec requirements,
    // or Choose a codec that is suitable for your bitmap.
    // "Unable to Set Video Stream Format"
    return false;
  }

  fourcc_ = (int)copts_.fccHandler;
  tempFrame_ = cvCreateImage( frameSize, 8, (isColor ? 3 : 1) );
  return true;
}


bool CvVideoWriter_VFW::addFrame( const IplImage* image )
{
  bool result = false;
  CV_FUNCNAME( "CvVideoWriter_VFW::writeFrame" );

  __BEGIN__;

  if( !image )
    EXIT;

  if( !compressed_ && !createStreams( cvGetSize(image), image->nChannels > 1 ))
    EXIT;

  if( image->width != tempFrame_->width || image->height != tempFrame_->height )
    CV_ERROR( CV_StsUnmatchedSizes,
    "image size is different from the currently set frame size" );

  if( image->nChannels != tempFrame_->nChannels ||
    image->depth != tempFrame_->depth ||
    image->origin == 0 ||
    image->widthStep != cvAlign(image->width*image->nChannels*((image->depth & 255)/8), 4))
  {
    cvConvertImage( image, tempFrame_, image->origin == 0 ? CV_CVTIMG_FLIP : 0 );
    image = (const IplImage*)tempFrame_;
  }

  result = AVIStreamWrite( compressed_, pos_++, 1, image->imageData,
    image->imageSize, AVIIF_KEYFRAME, 0, 0 ) == AVIERR_OK;

  __END__;

  return result;
}

CvVideoWriter_VFW::CvVideoWriter_VFW():
  avifile_(NULL), 
  compressed_(NULL), 
  uncompressed_(NULL),
  fps_(0), tempFrame_(NULL),
  pos_(0), fourcc_(0),
  shouldCallAVISaveOptionsFree_(false)
{
  AVIFileInit();  // Initializes the AVIFile library and increments the reference count for the library
  resetParams();
}

CvVideoWriter_VFW::~CvVideoWriter_VFW()
{
  close();
  AVIFileExit();  // Exit the AVIFile library and decrements the reference count for the library
}

bool CvVideoWriter_VFW::fileExists( std::string &fileName )
{
  return _access( fileName.c_str(), 04 ) == 0;
}

bool CvVideoWriter_VFW::writeCodecParams( const char* configFileName ) const
{ 
  using namespace std;    
  try
  { // Open config file
    ofstream cfgFile(configFileName, ios::out | ios::binary);           
    cfgFile.exceptions ( fstream::failbit | fstream::badbit );
    // Write AVICOMPRESSOPTIONS struct data 
    cfgFile.write(reinterpret_cast<char const*>(&copts_), sizeof(copts_));  

    if (copts_.cbParms != 0)// Write codec param buffer
      cfgFile.write(reinterpret_cast<char const*>(copts_.lpParms), copts_.cbParms);    

    if (copts_.cbFormat != 0)// Write codec format buffer
      cfgFile.write(reinterpret_cast<char const*>(copts_.lpFormat), copts_.cbFormat); 
  }
  catch (fstream::failure const&) 
  { return false; } // write failed
  return true;
}

bool CvVideoWriter_VFW::readCodecParams( char const* configFileName )
{  
  using namespace std;
  try
  { // Open config file
    ifstream cfgFile(configFileName, ios::in | ios::binary); 
    cfgFile.exceptions ( fstream::failbit | fstream::badbit | fstream::eofbit );
    // Read AVICOMPRESSOPTIONS struct data 
    cfgFile.read(reinterpret_cast<char*>(&copts_), sizeof(copts_));         
  
    if (copts_.cbParms != 0)
    { copts_Parms_.resize(copts_.cbParms,0);                // Allocate buffer 
      cfgFile.read(&copts_Parms_[0], copts_Parms_.size());  // Read param buffer 
      copts_.lpParms = &copts_Parms_[0];                    // Set lpParms to buffer
    }
    else
    { copts_Parms_.clear();
      copts_.lpParms = NULL;
    }
  
    if (copts_.cbFormat != 0)
    { copts_Format_.resize(copts_.cbFormat,0);              // Allocate buffer 
      cfgFile.read(&copts_Format_[0], copts_Format_.size());// Read format buffer 
      copts_.lpFormat = &copts_Format_[0];                  // Set lpFormat to buffer
    }
    else
    { copts_Format_.clear();
      copts_.lpFormat = NULL;
    }
  }
  catch (fstream::failure const&) 
  { // A read failed, clean up.
    ZeroMemory(&copts_,sizeof(AVICOMPRESSOPTIONS)); 
    copts_Parms_.clear();
    copts_Format_.clear();    
    return false;  
  }
  return true;
}

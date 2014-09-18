
bool CvVideoWriter_VFW::writeCodecParams( char const* cfgName )
{ 
  using namespace std;    
  try
  { // Open config file
    ofstream cfgFile(cfgName, ios::out | ios::binary);           
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

bool CvVideoWriter_VFW::readCodecParams( char const* cfgName )
{  
  using namespace std;
  try
  { // Open config file
    ifstream cfgFile(cfgName, ios::in | ios::binary); 
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

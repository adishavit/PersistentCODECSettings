Persistent CODEC Settings
=========================

This is a mirror of the code accompanying my Dec. 2011 MSDN Article - ["Saving and Reusing Video Encoding Settings"](http://msdn.microsoft.com/en-us/magazine/hh580739.aspx).

In the article, I presented a simple yet general way to allow video processing
applications to save video with consistent compression settings, thus avoiding having to manually specify the codec settings
each time the application or the machine is started.  
I show how to access the internal setting buffers of a VfW codec so that they can be easily
saved, reloaded and reused. This works for any video codec installed
on the machine, without requiring the use of any codec-specific APIs.

The original MSDN article can be read [here](http://msdn.microsoft.com/en-us/magazine/hh580739.aspx).  
The original [code sample download link](http://msdn.microsoft.com/en-us/magazine/msdnmag1211.aspx).  
According to the magazine [Terms of Use](http://msdn.microsoft.com/cc300389.aspx) (see bottom of [article page](http://msdn.microsoft.com/en-us/magazine/hh580739.aspx)) the license is apparently the Microsoft Limited Public License (Ms-LPL).

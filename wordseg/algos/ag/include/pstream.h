// pstream.h
//
// Mark Johnson, (c) 14th March 2008
//
// This is a revised version of popen.h (renamed to avoid name clash with
// popen (3))
//
//! An ipstream is an istream that reads from a popen command.
//! A izstream is an istream that reads from a (possibly) compressed file
//!
//! Note that these all use popen, which works by invoking the Unix system
//! routing fork (2), which temporarily DOUBLES your virtual memory usage.
//! So open these streams when your program before your program starts using
//! lots of memory!

#ifndef PSTREAM_H
#define PSTREAM_H

#include <cstdio>
#include <ext/stdio_filebuf.h>
#include <iostream>
#include <string>
#include <cstring>

namespace pstream {

  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                 Input                                  //
  //                                                                        //
  ////////////////////////////////////////////////////////////////////////////

  //! istream_helper{} exists so that the various file buffers get created before the istream
  //! gets created.
  //
  struct istream_helper {
    FILE* stdio_fp;
    __gnu_cxx::stdio_filebuf<char> stdio_fb;
    
    istream_helper(const char* command) 
      : stdio_fp(popen(command, "r")), stdio_fb(stdio_fp, std::ios_base::in) { }
    
    ~istream_helper() { pclose(stdio_fp); }  // close the popen'd stream
  }; // pstream::istream_helper{}


  //! An istream inherits from an istream.  
  //! I really can't believe that this works, but it does.
  //
  struct istream : public istream_helper, public std::istream {
    istream(const char* command)       //!< shell command whose output is sent to the istream
      : istream_helper(command), std::istream(&stdio_fb) { }
    
    istream(const std::string& command) //!< shell command whose output is sent to the istream
      : istream_helper(command.c_str()), std::istream(&stdio_fb) { }
  }; // pstream::istream{}


  //! izstream_helper{} exists to compute the command that istream has to execute
  //! while izstream is being constructed.
  //
  struct izstream_helper {
    std::string popen_command;
    
    izstream_helper(const char* filename) 
    {
      const char* filesuffix = strrchr(filename, '.');
      popen_command = (strcasecmp(filesuffix, ".bz2")
		       ? (strcasecmp(filesuffix, ".gz") ? "cat " : "zcat ")
		       : "bzcat ");
      popen_command += filename;
    }
  };  // pstream::izstream_helper{}

  //! An izstream popen's a set of files, running them through zcat, bzcat or cat
  //! depending on the suffix of the last file.
  //
  struct izstream : public izstream_helper, public istream {
    izstream(const char* filename) 
      : izstream_helper(filename), istream(popen_command.c_str()) { }
  }; // pstream::izstream{}

  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                Output                                  //
  //                                                                        //
  ////////////////////////////////////////////////////////////////////////////

  //! ostream_helper{} exists so that the various file buffers get created before the ostream
  //! gets created.
  //
  struct ostream_helper {
    FILE* stdio_fp;
    __gnu_cxx::stdio_filebuf<char> stdio_fb;
    
    ostream_helper(const char* command) 
      : stdio_fp(popen(command, "w")), stdio_fb(stdio_fp, std::ios_base::out) { }
    
    ~ostream_helper() { pclose(stdio_fp); }  // close the popen'd stream
  }; // pstream::ostream_helper{}

  //! Instantiating ostream_flushcout{} flushes std::cout, which should be done
  //! before a popen or fork command (the output buffer is shared between processes)
  //
  struct ostream_flushcout {
    ostream_flushcout() { std::cout << std::flush; }
  };  // pstream::ostream_flushcout{}

  //! An ostream inherits from an ostream.  
  //
  struct ostream : public ostream_flushcout, public ostream_helper, public std::ostream {
    ostream(const char* command)       //!< shell command whose output is sent to the istream
      : ostream_helper(command), std::ostream(&stdio_fb) { }
    
    ostream(const std::string& command) //!< shell command whose output is sent to the istream
      : ostream_helper(command.c_str()), std::ostream(&stdio_fb) { }
  }; // pstream::ostream{}

}  // namespace pstream

#endif // PSTREAM_H

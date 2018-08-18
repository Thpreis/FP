////////////////////////////////////////////////////////////////////////
// logstream.cc		implementation of a class for logging output
//
// a logstream can be used in much the same way as standard C++ output
// streams; however, one can define different logging facilities and
// severities, and one can select how severe an event being logged
// must be to actually show up in the output. this way, one can separate
// debugging output from more severe warnings and turn off debugging
// output during normal program runs. see below for examples of how to
// use/define a logstream.
//
// Dec 2007 - Jan 2008	M. Schiller
// 			initial development, debugging
// Feb 10 2008		M. Schiller
// 			first publically available revision
////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

#include "logstream.h"

namespace logstreams {
    ////////////////////////////////////////////////////////////////////////
    // get
    ////////////////////////////////////////////////////////////////////////
    logstream& logstream::get(std::ostream& s, int mylevel,
	    const std::string& name)
    {
	static bool cleanupRegistered = false;

	if (!cleanupRegistered) {
	    // register a function to be called at program exit which takes
	    // care of flushing the buffers, the underlying streams and
	    // freeing resources
	    atexit(flushStreams);
	    cleanupRegistered = true;
	}

	// if the logstream is already registered, return a reference to it
	// create a new one otherwise
	if (logstream::isRegistered(s, mylevel, name)) {
	    logstream* retVal = streams[StreamMapKey(&s,mylevel,name)];
	    // increment the number of users (unless the stream is permanent)
	    if (retVal->isPermanent())
		++retVal->nUsers;
	    return *retVal;
	} else
	    return * new logstream(&s, mylevel, name);
    }

    ////////////////////////////////////////////////////////////////////////
    // release
    ////////////////////////////////////////////////////////////////////////
    void logstream::release()
    {
	// if nobody is using this logstream any longer, self-destruct
	// we need to call the static unregister function with our data
	// because we will cease to exist while cleaning out the streams map
	if (0 != nUsers && 0 == --nUsers)
	    return logstream::unregisterStream(this, true);
    }

    ////////////////////////////////////////////////////////////////////////
    // class functions managing the streams array
    ////////////////////////////////////////////////////////////////////////
    bool logstream::isRegistered(std::ostream& s, int l, const std::string& n)
    {
	StreamMap::iterator it = streams.find(StreamMapKey(&s, l, n));
	if (streams.end() == it) return false;
	else return true;
    }

    void logstream::registerStream(logstream* log)
    {
	// register log in the streams map
	streams[StreamMapKey(log->stream,log->myLogLevel,log->myname)] = log;
    }

    void logstream::unregisterStream(logstream* log, bool doDelete)
    {
	using std::swap;
	// copy the relevant bits of information from log so we can delete it
	std::ostream* s = log->stream;
	int l = log->myLogLevel;
	std::string n = log->myname;
	// delete log, if desired
	if (doDelete)
	    delete log;
	// unregister with streams map
	streams.erase(StreamMapKey(s, l, n));
	if (!streams.empty()) return;
	// streams is empty, so make sure it releases its resources
	StreamMap tmp;
	swap(streams, tmp);
    }

    ////////////////////////////////////////////////////////////////////////
    // member functions for permanent streams
    ////////////////////////////////////////////////////////////////////////
    logstream& logstream::makePermanent() { nUsers = 0; return *this; }
    bool logstream::isPermanent() const { return 0 == nUsers; }

    ////////////////////////////////////////////////////////////////////////
    // constructors
    ////////////////////////////////////////////////////////////////////////
    logstream::logstream(std::ostream* s, int mylevel, const std::string& name):
		stream(s), myLogLevel(mylevel), myname(name), bufdirty(true),
		buffer(), nUsers(1)
    {
	// initialize buffers to consistent state
	flush();
	// and register
	logstream::registerStream(this);
    }
    logstream::logstream() : stream(0) {}
    logstream::logstream(const logstream&) : stream(0) {}
    logstream& logstream::operator=(const logstream&) { return *this; }

    ////////////////////////////////////////////////////////////////////////
    // destructor
    ////////////////////////////////////////////////////////////////////////
    logstream::~logstream()
    {
	// check if initialization went well
	if (0 != stream) {
	    // yes, so force to flush buffers and the underlying stream
	    flush(true);
	}
    }

    ////////////////////////////////////////////////////////////////////////
    // stream reattach
    ////////////////////////////////////////////////////////////////////////
    logstream& logstream::reattach(std::ostream& newstream, int& newlevel,
	    const std::string& newname)
    {
	// if the target stream does already exist, throw
	if (logstream::isRegistered(newstream, newlevel, newname))
	    throw;
	// flush to the old stream
	flush(true);
	// remove from streams, but do not delete
	logstream::unregisterStream(this, false);
	// set members to perform the reattachment
	stream = &newstream;
	myLogLevel = newlevel;
	myname = newname;
	// and put back into streams
	logstream::registerStream(this);
	// clear the buffer and mark dirty
	buffer.str("");
	bufdirty = true;
	// re-initialize buffer to consistent state
	flush();
	return *this;
    }

    ////////////////////////////////////////////////////////////////////////
    // flush
    ////////////////////////////////////////////////////////////////////////
    void logstream::flush(bool flushStream)
    {
	if (bufdirty) {
	    // a dirty buffer needs to be written to the underlying stream
	    if (!buffer.str().empty()) {
		// need to append my name after each newline
		std::string str = buffer.str();
		std::string::size_type start = 0;
		std::string::size_type end = str.find("\n", start);
		while (str.size() > start) {
		    if (std::string::npos != end) ++end;
		    *stream << str.substr(start, end - start);
		    if (str.size() <= end) break;
		    *stream << myname.substr(0,
			    std::min(namewidth, unsigned(myname.size())));
		    if (myname.size() < namewidth)
			*stream << std::string(namewidth - myname.size(),
				stream->fill());
		    start = end;
		    end = str.find("\n", start);
		}
	    }
	    // flush the stream if neccessary
	    if (flushStream) stream->flush();
	    if (namewidth > 0) {
		buffer.str("");
		// refill the buffer with the start of a line for this
		// logstream
		buffer << myname.substr(0,
			std::min(namewidth, unsigned(myname.size())));
		// if neccessary, pad with the underlying stream's default
		// fill character to the right so our name field is namewidth
		// characters long
		if (myname.size() < namewidth)
		    buffer << std::string(namewidth - myname.size(),
			    stream->fill());
	    }
	    // buffer is clean again
	    bufdirty = false;
	}
    }

    ////////////////////////////////////////////////////////////////////////
    // getters for logstream properties
    ////////////////////////////////////////////////////////////////////////
    std::ostream& logstream::getStream() const { return *stream; }
    const std::string& logstream::getName() const { return myname; }
    int logstream::getLogLevel() const { return myLogLevel; }

    ////////////////////////////////////////////////////////////////////////
    // class functions
    ////////////////////////////////////////////////////////////////////////
    int logstream::logLevel() { return globalLogLevel; }
    int logstream::setLogLevel(int level)
    { using std::swap; swap(level, globalLogLevel); return level; }

    unsigned logstream::nameWidth() { return namewidth; }
    unsigned logstream::setMaxNameWidth(unsigned maxWidth)
    { using std::swap; swap(namewidth, maxWidth); return maxWidth; }

    void logstream::flushStreams()
    {
	using std::swap;
	using std::for_each;
	// free all pointers in streams recursively
	StreamMap tmp;
	swap(tmp, streams);
	for_each(tmp.begin(), tmp.end(), deleter<StreamMapKey,logstream>());
    }

    ////////////////////////////////////////////////////////////////////////
    // initialize class variables
    ////////////////////////////////////////////////////////////////////////
    unsigned logstream::namewidth = 8;
    int logstream::globalLogLevel = 1;
    logstream::StreamMap logstream::streams = logstream::StreamMap();

    ////////////////////////////////////////////////////////////////////////
    // initialize standard streams and make them permanent
    ////////////////////////////////////////////////////////////////////////
    logstream& debug = logstream::get(std::cerr, 0, "DEBUG").makePermanent();
    logstream& warn = logstream::get(std::cerr, 1, "WARNING").makePermanent();
    logstream& info = logstream::get(std::cerr, 2, "INFO").makePermanent();
    logstream& error = logstream::get(std::cerr, 3, "ERROR").makePermanent();
    logstream& fatal = logstream::get(std::cerr, 4, "FATAL").makePermanent();
    logstream& always = logstream::get(std::cerr, 5, "ALWAYS").makePermanent();
}
// vim:sw=4:tw=78:ft=cpp

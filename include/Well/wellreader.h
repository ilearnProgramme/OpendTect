#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "wellio.h"
#include "sets.h"
#include "ranges.h"
#include "od_iosfwd.h"
class BufferStringSet;


namespace Well
{
class Data;
class Log;

/*!
\brief Reads Well::Data.
*/

mExpClass(Well) Reader : public IO
{
public:

			Reader(const char* fnm,Data&);

    bool		get() const;		//!< Just read all

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model only
    bool		getCSMdl() const;	//!< Read Checkshot model only
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read lognm only

    bool		getInfo(od_istream&) const;
    bool		addLog(od_istream&) const;
    bool		getMarkers(od_istream&) const;
    bool		getD2T(od_istream&) const;
    bool		getCSMdl(od_istream&) const;
    bool		getDispProps(od_istream&) const;

    void		getLogInfo(BufferStringSet& lognms) const;

    // These should not be here. Move to Well::Log class
    Interval<float>	getLogDahRange(const char* lognm) const;
    			//!< If no log with this name, returns [undef,undef]
    Interval<float>	getAllLogsDahRange() const;
    			//!< If no log returns [undef,undef]

protected:

    Data&		wd;

    bool		getOldTimeWell(od_istream&) const;
    void		getLogInfo(BufferStringSet&,TypeSet<int>&) const;
    void		readLogData(Log&,od_istream&,int) const;
    bool		getTrack(od_istream&) const;
    bool		doGetD2T(od_istream&,bool csmdl) const;
    bool		doGetD2T(bool) const;

    static Log*		rdLogHdr(od_istream&,int&,int);

};

}; // namespace Well

#endif


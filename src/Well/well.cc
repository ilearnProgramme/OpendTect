/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: well.cc,v 1.22 2004-05-08 17:47:32 bert Exp $";

#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "finding.h"
#include "iopar.h"

const char* Well::Info::sKeyuwid	= "Unique Well ID";
const char* Well::Info::sKeyoper	= "Operator";
const char* Well::Info::sKeystate	= "State";
const char* Well::Info::sKeycounty	= "County";
const char* Well::Info::sKeycoord	= "Surface coordinate";
const char* Well::Info::sKeyelev	= "Surface elevation";
const char* Well::D2TModel::sKeyTimeWell = "=Time";
const char* Well::D2TModel::sKeyDataSrc	= "Data source";
const char* Well::Marker::sKeyDah	= "Depth along hole";
const char* Well::Log::sKeyUnitLbl	= "Unit of Measure";


Well::Data::Data( const char* nm )
    : info_(nm)
    , track_(*new Well::Track)
    , logs_(*new Well::LogSet)
    , d2tmodel_(0)
    , markerschanged(this)
{
}


Well::Data::~Data()
{
    delete &track_;
    delete &logs_;
    delete d2tmodel_;
}


void Well::Data::setD2TModel( D2TModel* d )
{
    delete d2tmodel_;
    d2tmodel_ = d;
}


Well::LogSet::~LogSet()
{
    deepErase( logs );
}


void Well::LogSet::add( Well::Log* l )
{
    if ( !l ) return;

    logs += l;
    updateDahIntv( *l );;
}


void Well::LogSet::updateDahIntv( const Well::Log& wl )
{
    if ( !wl.size() ) return;

    if ( mIsUndefined(dahintv.start) )
	{ dahintv.start = wl.dah(0); dahintv.stop = wl.dah(wl.size()-1); }
    else
    {
	if ( dahintv.start > wl.dah(0) )
	    dahintv.start = wl.dah(0);
	if ( dahintv.stop < wl.dah(wl.size()-1) )
	    dahintv.stop = wl.dah(wl.size()-1);
    }
}


void Well::LogSet::updateDahIntvs()
{
    for ( int idx=0; idx<logs.size(); idx++ )
	updateDahIntv( *logs[idx] );
}


Well::Log* Well::LogSet::remove( int idx )
{
    Log* l = logs[idx]; logs -= l;
    ObjectSet<Well::Log> tmp( logs );
    logs.erase(); init();
    for ( int idx=0; idx<tmp.size(); idx++ )
	add( tmp[idx] );
    return l;
}


float Well::Log::getValue( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return val_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return mUndefValue;

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    return ( d1*val_[idx2] + d2*val_[idx1] ) / (d1 + d2);
}


void Well::Log::addValue( float z, float val )
{
    if ( !mIsUndefined(val) ) 
    {
	if ( val < range_.start ) range_.start = val;
	if ( val > range_.stop ) range_.stop = val;
	assign( selrange_, range_ );
    }

    dah_ += z; 
    val_ += val;
}


void Well::Log::setSelValueRange( const Interval<float>& newrg )
{
    assign( selrange_, newrg );
}


Coord3 Well::Track::getPos( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return pos_[idx1];
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
	return Coord3(0,0,0);

    return coordAfterIdx( dh, idx1 );
}


Coord3 Well::Track::coordAfterIdx( float dh, int idx1 ) const
{
    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    const Coord3& c1 = pos_[idx1];
    const Coord3& c2 = pos_[idx2];
    const float f = 1. / (d1 + d2);
    return Coord3( f * (d1 * c2.x + d2 * c1.x), f * (d1 * c2.y + d2 * c1.y),
		   f * (d1 * c2.z + d2 * c1.z) );
}


float Well::Track::getDahForTVD( float z, float prevdah ) const
{
    bool haveprevdah = !mIsUndefined(prevdah);
    int foundidx = -1;
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const Coord3& c = pos_[idx];
	float cz = pos_[idx].z;
	if ( haveprevdah && prevdah-1e-4 > dah_[idx] )
	    continue;
	if ( pos_[idx].z + 1e-4 > z )
	    { foundidx = idx; break; }
    }
    if ( foundidx < 1 )
	return foundidx ? mUndefValue : dah_[0];

    const int idx1 = foundidx - 1;
    const int idx2 = foundidx;
    float z1 = pos_[idx1].z;
    float z2 = pos_[idx2].z;
    float dah1 = dah_[idx1];
    float dah2 = dah_[idx2];
    return ((z-z1) * dah2 + (z2-z) * dah1) / (z2-z1);
}


void Well::Track::toTime( const D2TModel& d2t )
{
    for ( int idx=0; idx<dah_.size(); idx++ )
    {
	Coord3& pt = pos_[idx];
	pt.z = d2t.getTime( dah_[idx] );
    }
}


float Well::D2TModel::getTime( float dh ) const
{
    int idx1;
    if ( findFPPos(dah_,dah_.size(),dh,-1,idx1) )
	return t_[idx1];
    else if ( dah_.size() < 2 )
	return mUndefValue;
    else if ( idx1 < 0 || idx1 == dah_.size()-1 )
    {
	// Extrapolate. Not very correct I guess.
	int idx0 = idx1 < 0 ? 1 : idx1;
	const float v = (dah_[idx0] - dah_[idx0-1]) / (t_[idx0] - t_[idx0-1]);
	idx0 = idx1 < 0 ? 0 : idx1;
	return t_[idx0] + ( dh - dah_[idx0] ) / v;
    }

    const int idx2 = idx1 + 1;
    const float d1 = dh - dah_[idx1];
    const float d2 = dah_[idx2] - dh;
    //TODO not a time-correct average.
    return (d1 * t_[idx2] + d2 * t_[idx1]) / (d1 + d2);
}


#define mName "Well name"

void Well::Info::fillPar(IOPar& par) const
{
    par.set( mName, name() );
    par.set( sKeyuwid, uwid );
    par.set( sKeyoper, oper );
    par.set( sKeystate, state );
    par.set( sKeycounty, county );

    BufferString coord;
    surfacecoord.fill( coord.buf() );
    par.set( sKeycoord, coord );

    par.set( sKeyelev, surfaceelev );
}

void Well::Info::usePar(const IOPar& par)
{
    setName( par[mName] );
    par.get( sKeyuwid, uwid );
    par.get( sKeyoper, oper );
    par.get( sKeystate, state );
    par.get( sKeycounty, county );

    BufferString coord;
    par.get( sKeycoord, coord );
    surfacecoord.use( coord );

    par.get( sKeyelev, surfaceelev );
}


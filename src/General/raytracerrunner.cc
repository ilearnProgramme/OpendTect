/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: raytracerrunner.cc,v 1.9 2011-12-12 14:45:50 cvsbruno Exp $";


#include "raytracerrunner.h"

RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims, 
				    const IOPar& raypars ) 
    : aimodels_(aims)
    , raypar_(raypars)		
{}


RayTracerRunner::~RayTracerRunner() 
{ deepErase( raytracers_ );}


od_int64 RayTracerRunner::nrIterations() const
{ return aimodels_.size(); }


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool RayTracerRunner::doPrepare( int nrthreads )
{
    deepErase( raytracers_ );

    if ( aimodels_.isEmpty() )
	mErrRet( "No AI model set" );

    if ( RayTracer1D::factory().getNames(false).isEmpty() )
	return false;

    BufferString errmsg;
    for ( int idx=0; idx<aimodels_.size(); idx++ )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, errmsg );
	if ( !rt1d )
	{
	    rt1d = RayTracer1D::factory().create( 
		    *RayTracer1D::factory().getNames(false)[0] );
	    rt1d->usePar( raypar_ );
	}
	raytracers_ += rt1d;
    }

    if ( raytracers_.isEmpty() && !errmsg.isEmpty() )
	mErrRet( errmsg.buf() );

    return true;
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int thread )
{
    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const ElasticModel& aim = aimodels_[idx];
	if ( aim.isEmpty() ) 
	    continue;

	RayTracer1D* rt1d = raytracers_[idx];
	rt1d->setModel( aim );
	if ( !rt1d->execute() )
	    mErrRet( rt1d->errMsg() );
    }
    return true;
}


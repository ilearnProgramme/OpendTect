/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: cubicbeziercurve.cc,v 1.2 2005-01-10 15:27:03 kristofer Exp $";

#include "cubicbeziercurve.h"

#include "errh.h"

#define mRetErr( msg, retval ) { errmsg()=msg; return retval; }

#ifndef mEPS
#define mEPS 1e-10
#endif

namespace Geometry
{

CubicBezierCurve::CubicBezierCurve( const Coord3& c0, const Coord3& c1,
				    int fp, int step_ )
    : firstparam( fp )
    , paramstep( step_ )
    , directioninfluence( step_/3.0 )
    , iscircular(false)
{
    if ( !c0.isDefined() || !c1.isDefined() )
    {
	pErrMsg("Object must be initiated with valid coords" );
	return;
    }

    positions += c0;
    directions += Coord3::udf();
    positions += c1;
    directions += Coord3::udf();
}


CubicBezierCurve* CubicBezierCurve::clone() const
{ return new CubicBezierCurve(*this); }


IntervalND<float> CubicBezierCurve::boundingBox(bool approx) const
{
    if ( approx )
	return ParametricCurve::boundingBox(approx);

    pErrMsg("Not implemnted");
    IntervalND<float> res(3);
    return res;
}


/*! Implementation of deCastaleau's algoritm. For more info, refer to
 *  "The NURBS book", figure 1.17. */

Coord3 CubicBezierCurve::computePosition( float param ) const
{
    const float relparam = param - firstparam;
    const int previdx = (int) relparam;
	if ( previdx<0 || previdx>positions.size()-2 )
	return Coord3::udf();

    const float u = relparam - previdx;
    const float one_minus_u = 1-u;

    const Coord3& prevpos = positions[previdx];
    const Coord3 prevdir =
	getDirection(previdx+firstparam,true).normalize()*directioninfluence;
    const Coord3& nextpos = positions[previdx+1];
    const Coord3 nextdir =
	getDirection(previdx+firstparam+1,true).normalize()*directioninfluence;

    Coord3 interpolpos0 = prevpos + prevdir*u;
    Coord3 interpolpos1 = (prevpos+prevdir)*u+(nextpos-nextdir)*one_minus_u;
    Coord3 interpolpos2 = nextpos-nextdir*one_minus_u;

    interpolpos0 = interpolpos0*u+interpolpos1*one_minus_u;
    interpolpos1 = interpolpos1*u+interpolpos2*one_minus_u;

    return interpolpos0*u+interpolpos1*one_minus_u;
}


Coord3 CubicBezierCurve::computeDirection( float param ) const
{
    pErrMsg("Not implemented");
    return Coord3::udf();
}


StepInterval<int> CubicBezierCurve::parameterRange() const
{
    return StepInterval<int>( firstparam,
			      firstparam+(positions.size()-1)*paramstep,
			      paramstep);
}


Coord3 CubicBezierCurve::getPosition( GeomPosID param ) const
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=positions.size() )
	return Coord3::udf();

    return positions[idx];
}


bool CubicBezierCurve::setPosition( GeomPosID param, const Coord3& np )
{
    if ( !np.isDefined() ) return unSetPosition( param );

    const int idx = getIndex(param);

    if ( idx<-1||idx>positions.size() )
	mRetErr("Cannot add position that is not a neighbor to an existing"
		" position", false );

    if ( idx==-1 )
    {
	positions.insert( 0, np );
	directions.insert( 0, Coord3::udf() );
	firstparam = param;
	triggerNrPosCh( param );
    }
    else if ( idx==positions.size() )
    {
	positions += np;
	directions += Coord3::udf();
	triggerNrPosCh( param );
    }
    else
    {
	positions[idx] = np;
	triggerMovement( param );
    }

    return true;
}


bool CubicBezierCurve::insertPosition( GeomPosID param, const Coord3& np )
{
    if ( !np.isDefined() ) 
	mRetErr("Cannot insert undefined position", false );

    const int idx = getIndex(param);
    if ( idx<0 || idx>=positions.size() )
	return setPosition(param, np );

    positions.insert( idx, np );
    directions.insert( idx, Coord3::udf() );

    TypeSet<GeomPosID> changedpids;
    for ( int idy=idx; idy<positions.size()-1; idy ++ )
	changedpids += firstparam+idy*paramstep;

    triggerMovement( changedpids );
    triggerNrPosCh( firstparam+(positions.size()-1)*paramstep);
    return true;
}


bool CubicBezierCurve::removePosition( GeomPosID param )
{
    const int idx = getIndex(param);
    if ( idx<0 || idx>=positions.size() )
	mRetErr("Cannot remove non-existing position", false );

    if ( idx==0 || idx==positions.size()-1 )
	return unSetPosition(param);

    positions.remove( idx );
    directions.remove( idx );

    TypeSet<GeomPosID> changedpids;
    for ( int idy=idx; idy<positions.size(); idy ++ )
	changedpids += firstparam+idy*paramstep;

    triggerMovement( changedpids );
    triggerNrPosCh( firstparam+positions.size()*paramstep);
    return true;
}


bool CubicBezierCurve::unSetPosition( GeomPosID param )
{
    const int idx = getIndex( param );

    if ( positions.size()<3 )
	mRetErr("Cannot remove positions since too few positions will be left",
		false );

    if ( !idx || idx==positions.size()-1)
    {
	if ( !idx ) firstparam += paramstep;
	positions.remove(idx);
	directions.remove(idx);
	triggerNrPosCh( param );
	return true;
    }

    mRetErr("Cannot remove positions in the middle of a curve, since that "
	    "would split the curve.", false );
}


bool CubicBezierCurve::isDefined( GeomPosID param ) const
{
    const int index = getIndex( param );
    return index>=0 && index<positions.size();
}


Coord3 CubicBezierCurve::getDirection( GeomPosID param, bool computeifudf ) const
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=directions.size() )
	return Coord3::udf();

    if ( !directions[idx].isDefined() && computeifudf )
	return computeDirection( param );

    return directions[idx];
}


bool CubicBezierCurve::setDirection( GeomPosID param, const Coord3& np )
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=positions.size() )
	mRetErr("No corresponding position", false );

    directions[idx] = np;

    triggerMovement( param );
    return true;
}


bool CubicBezierCurve::unSetDirection( GeomPosID param )
{ return setDirection( param, Coord3::udf() ); }


bool CubicBezierCurve::isDirectionDefined( GeomPosID param ) const
{
    const int index = getIndex( param );
    return index>=0 && index<positions.size() && directions[index].isDefined();
}


float CubicBezierCurve::directionInfluence() const { return directioninfluence;}


void CubicBezierCurve::setDirectionInfluence( float ndi )
{
    directioninfluence=ndi;
    triggerMovement();
}


bool CubicBezierCurve::isCircular() const
{ return iscircular && positions.size()>2; }


bool CubicBezierCurve::setCircular(bool yn)
{
    if ( positions.size()<3 ) return false;
    iscircular=yn;

    return true;
}


Coord3 CubicBezierCurve::computeDirection( GeomPosID param ) const
{
    const int idx = getIndex( param );
    if ( idx<0||idx>=positions.size() )
    {
	pErrMsg("Outside range");
	return Coord3::udf();
    }

    int idx0 = idx-1, idx1 = idx+1;
    if ( !idx )
	idx0 = isCircular() ? positions.size()-1 : 0;
    else if ( idx==positions.size()-1 )
	idx1 = isCircular() ? 0 : positions.size()-1;
   
    const Coord3& c0 = positions[idx0];
    const Coord3& c1 = positions[idx1];

    if ( c0.distance(c1)<mEPS )
	return Coord3::udf();

    return (c1-c0)/(idx1-idx0)*directioninfluence;
}


}; //Namespace


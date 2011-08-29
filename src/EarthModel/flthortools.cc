/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: flthortools.cc,v 1.45 2011-08-29 20:40:01 cvsyuancheng Exp $";

#include "flthortools.h"

#include "binidvalset.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emmanager.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "survinfo.h"


int FaultTrace::nextID( int previd ) const
{ return previd >= -1 && previd < coords_.size()-1 ? previd + 1 : -1; }

void FaultTrace::setIndices( const TypeSet<int>& indices )
{ coordindices_ = indices; }

const TypeSet<int>& FaultTrace::getIndices() const
{ return coordindices_; }

int FaultTrace::add( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );
    coords_ += pos;
    return coords_.size() - 1;
}


int FaultTrace::add( const Coord3& pos, float trcnr )
{
    Threads::MutexLocker lock( mutex_ );
    coords_ += pos;
    trcnrs_ += trcnr;
    return coords_.size() - 1;
}


Coord3 FaultTrace::get( int idx ) const
{ return idx >= 0 && idx < coords_.size() ? coords_[idx] : Coord3::udf(); }

float FaultTrace::getTrcNr( int idx ) const
{ return idx >= 0 && idx < trcnrs_.size() ? trcnrs_[idx] : -1; }

void FaultTrace::set( int idx, const Coord3& pos )
{
    if ( coords_.validIdx(idx) )
	coords_[idx] = pos;
}


void FaultTrace::set( int idx, const Coord3& pos, float trcnr )
{
    if ( coords_.validIdx(idx) )
    {
	coords_[idx] = pos;
	if ( trcnrs_.validIdx(idx) )
	    trcnrs_[idx] = trcnr;
    }
}


void FaultTrace::remove( int idx )
{
    Threads::MutexLocker lock( mutex_ );
    coords_.remove( idx );
}


bool FaultTrace::isDefined( int idx ) const
{ return coords_.validIdx(idx) && coords_[idx].isDefined(); }


FaultTrace* FaultTrace::clone()
{
    Threads::MutexLocker lock( mutex_ );
    FaultTrace* newobj = new FaultTrace;
    newobj->coords_ = coords_;
    newobj->trcnrs_ = trcnrs_;
    return newobj;
}


bool FaultTrace::getImage( const BinID& bid, float z,
			   const Interval<float>& ztop,
			   const Interval<float>& zbot,
			   const StepInterval<int>& trcrg,
			   BinID& bidimg, float& zimg, bool posdir ) const
{
    if ( !isinl_ ) return false;

    float z1 = posdir ? ztop.start : ztop.stop;
    float z2 = posdir ? zbot.start : zbot.stop;

    const float frac = mIsEqual(z1,z2,1e-6) ? 0 : ( z - z1 ) / ( z2 - z1 );
    z1 = posdir ? ztop.stop : ztop.start;
    z2 = posdir ? zbot.stop : zbot.start;
    zimg = z1 + frac * ( z2 - z1 );
    BinID start( isinl_ ? nr_ : trcrange_.start,
		 isinl_ ? trcrange_.start : nr_ );
    BinID stop( isinl_ ? nr_ : trcrange_.stop, isinl_ ? trcrange_.stop : nr_ );
    Coord intsectn = getIntersection( start,zimg, stop, zimg );
    if ( intsectn == Coord::udf() )
	return false;

    const float fidx = trcrg.getfIndex( intsectn.x );
    const int index = posdir ? mNINT( ceil(fidx) ) : mNINT( floor(fidx) );
    bidimg.inl = isinl_ ? nr_ : trcrg.atIndex( index );
    bidimg.crl = isinl_ ? trcrg.atIndex( index ) : nr_;
    return true;
}


bool FaultTrace::getHorIntersection( const EM::Horizon& hor, BinID& bid ) const
{
    BinID prevbid;
    float prevz = mUdf(float);
    const EM::SectionID sid = hor.sectionID( 0 );
    StepInterval<int> trcrg = isinl_ ? hor.geometry().colRange()
				     : hor.geometry().rowRange( sid );
    trcrg.limitTo( trcrange_ );
    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
    {
	BinID curbid( isinl_ ? nr_ : trcnr, isinl_ ? trcnr : nr_ );
	const float curz = hor.getPos( sid, curbid.toInt64() ).z;
	if ( mIsUdf(curz) )
	    continue;

	if ( !mIsUdf(prevz) )
	{
	    Coord intsectn = getIntersection( curbid, curz, prevbid, prevz );
	    if ( intsectn.isDefined() )
	    {
		bid.inl = isinl_ ? nr_ : trcrg.snap( mNINT(intsectn.x) );
		bid.crl = isinl_ ? trcrg.snap( mNINT(intsectn.x) ) : nr_;
		return true;
	    }
	}

	prevbid = curbid;
	prevz = curz;
    }

    return false;
}


bool FaultTrace::handleUntrimmed( const BinIDValueSet& bvs,
			Interval<float>& zintv, const BinID& negbid,
			const BinID& posbid, bool istop ) const
{
    BinID bid = negbid;
    float prevz = mUdf(float);
    int& trcvar = isinl_ ? bid.crl : bid.inl;
    BinID origin( isinl_ ? nr_ : trcrange_.stop+10,
	    	  isinl_ ? trcrange_.stop+10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar-- )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.start = prevz; 
    bid = posbid;
    prevz = mUdf(float);
    origin = BinID( isinl_ ? nr_ : trcrange_.start-10,
	    	    isinl_ ? trcrange_.start-10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar++ )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.stop = prevz; 
    return true;
}


bool FaultTrace::getHorCrossings( const BinIDValueSet& bvs,
				  Interval<float>& ztop,
				  Interval<float>& zbot ) const
{
    BinID start( isinl_ ? nr_ : trcrange_.start,
	         isinl_ ? trcrange_.start : nr_ );
    float starttopz=mUdf(float), startbotz=mUdf(float);
    int& startvar = isinl_ ? start.crl : start.inl;
    int step = isinl_ ? SI().crlStep() : SI().inlStep();
    const int bvssz = bvs.totalSize();
    for ( int idx=0; idx<bvssz; idx++,startvar += step )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( start );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	bvs.get( pos, dummy, starttopz, startbotz );
	if ( !mIsUdf(starttopz) && !mIsUdf(startbotz) )
	    break;
    }

    BinID stop( isinl_ ? nr_ : trcrange_.stop,
	    	isinl_ ? trcrange_.stop : nr_ );
    int& stopvar = isinl_ ? stop.crl : stop.inl;
    step = -step;
    float stoptopz, stopbotz;
    float prevstoptopz = mUdf(float), prevstopbotz = mUdf(float);
    BinID stoptopbid, stopbotbid, prevstoptopbid, prevstopbotbid;
    bool foundtop = false, foundbot = false;
    bool tophasisect = false, bothasisect = false;
    for ( int idx=0; idx<bvssz; idx++,stopvar += step )
    {
	if ( foundtop && foundbot )
	    break;

	BinIDValueSet::Pos pos = bvs.findFirst( stop );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float z1=mUdf(float), z2=mUdf(float);
	bvs.get( pos, dummy, z1, z2 );
	if ( !foundtop && !mIsUdf(z1) )
	{
	    stoptopbid = stop;
	    stoptopz = z1;
	    Coord intsectn = getIntersection( start, starttopz,
		    			      stop, stoptopz );
	    if ( !tophasisect )
		tophasisect = intsectn != Coord::udf();

	    if ( tophasisect && intsectn == Coord::udf() )
		foundtop = true;
	    else
	    {
		prevstoptopbid = stop;
		prevstoptopz = stoptopz;
	    }
	}

	if ( !foundbot && !mIsUdf(z2) )
	{
	    stopbotbid = stop;
	    stopbotz = z2;
	    Coord intsectn = getIntersection( start, startbotz,
		    			      stop, stopbotz );

	    if ( !bothasisect )
		bothasisect = intsectn != Coord::udf();

	    if ( bothasisect && intsectn == Coord::udf() )
		foundbot = true;
	    else
	    {
		prevstopbotbid = stop;
		prevstopbotz = stopbotz;
	    }
	}
    }

    if ( !tophasisect && !bothasisect )
	return false;

    if ( !tophasisect && !mIsUdf(zrange_.start) )
	prevstoptopz = stoptopz = zrange_.start;
    if ( !bothasisect && !mIsUdf(zrange_.stop) )
	prevstopbotz = stopbotz = zrange_.stop;

    if ( trcnrs_.size() )
    {
	if ( tophasisect && !handleUntrimmed(bvs,ztop,stoptopbid,
		    			     prevstoptopbid,true) )
	    return false;
	if ( bothasisect && !handleUntrimmed(bvs,zbot,stopbotbid,
		    			     prevstopbotbid,false) )
	    return false;

	return true;
    }

    ztop.set( stoptopz, prevstoptopz );
    zbot.set( stopbotz, prevstopbotz );
    return true;
}


float FaultTrace::getZValFor( const BinID& bid ) const
{
    const StepInterval<float>& zrg = SI().zRange( false );
    Coord intersectn = getIntersection( bid, zrg.start, bid, zrg.stop );
    return intersectn.y;
}


bool FaultTrace::isOnPosSide( const BinID& bid, float z ) const
{
    if ( ( isinl_ && bid.inl != nr_ ) || ( !isinl_ && bid.crl != nr_ ) )
	return false;

    const int trcnr = isinl_ ? bid.crl : bid.inl;
    if ( trcnr >= trcrange_.stop )
	return true;
    if ( trcnr <= trcrange_.start )
	return true;

    const BinID startbid( isinl_ ? nr_ : trcrange_.start,
	    		  isinl_ ? trcrange_.start : nr_ );
    const BinID stopbid( isinl_ ? nr_ : trcrange_.stop,
	    		  isinl_ ? trcrange_.stop : nr_ );
    const float zmid = ( zrange_.start + zrange_.stop ) / 2;
    Coord startintersectn = getIntersection( startbid, zmid, bid, z );
    if ( startintersectn.isDefined() )
	return true;

    Coord stopintersectn = getIntersection( stopbid, zmid, bid, z );
    if ( stopintersectn.isDefined() )
	return false;

    return false;
}


bool FaultTrace::includes( const BinID& bid ) const
{
    return isinl_ ? bid.inl == nr_ && trcrange_.includes( bid.crl )
		  : bid.crl == nr_ && trcrange_.includes( bid.inl );
}


void FaultTrace::computeTraceSegments()
{
    tracesegs_.erase();
    for ( int idx=1; idx<coordindices_.size(); idx++ )
    {
	const int curidx = coordindices_[idx];
	const int previdx = coordindices_[idx-1];
	if ( curidx < 0 || previdx < 0 )
	    continue;

	const Coord3& pos1 = get( previdx );
	const Coord3& pos2 = get( curidx );
	Coord nodepos1, nodepos2;
	const bool is2d = !trcnrs_.isEmpty();
	if ( is2d )
	{
	    nodepos1.setXY( getTrcNr(previdx), pos1.z * SI().zFactor() );
	    nodepos2.setXY( getTrcNr(curidx), pos2.z * SI().zFactor() );
	}
	else
	{
	    Coord posbid1 = SI().binID2Coord().transformBackNoSnap( pos1 );
	    Coord posbid2 = SI().binID2Coord().transformBackNoSnap( pos2 );
	    nodepos1.setXY( isinl_ ? posbid1.y : posbid1.x,
		    	   pos1.z * SI().zFactor() );
	    nodepos2.setXY( isinl_ ? posbid2.y : posbid2.x,
		    	   pos2.z * SI().zFactor() );
	}

	tracesegs_ += Line2( nodepos1, nodepos2 );
    }
}


Coord FaultTrace::getIntersection( const BinID& bid1, float z1,
				   const BinID& bid2, float z2  ) const
{
    if ( !getSize() )
	return Coord::udf();

    const bool is2d = trcnrs_.size();
    if ( is2d && trcnrs_.size() != getSize() )
	return Coord::udf();

    Interval<float> zrg( z1, z2 );
    zrg.sort();
    z1 *= SI().zFactor();
    z2 *= SI().zFactor();
    if ( ( isinl_ && (bid1.inl != nr_ || bid2.inl != nr_) )
	    || ( !isinl_ && (bid1.crl != nr_ || bid2.crl != nr_) ) )
	return Coord::udf();

    Coord pt1( isinl_ ? bid1.crl : bid1.inl, z1 );
    Coord pt2( isinl_ ? bid2.crl : bid2.inl, z2 );
    Line2 line( pt1, pt2 );

    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const Line2& fltseg = tracesegs_[idx];
	Coord interpos = line.intersection( fltseg );
	if ( interpos != Coord::udf() )
	{
	    interpos.y /= SI().zFactor();
	    return interpos;
	}
    }

    return Coord::udf();
}


bool FaultTrace::getIntersection( const BinID& bid1, float z1,
				  const BinID& bid2, float z2,
				  BinID& bid, float& z,
				  const StepInterval<int>* intv ) const
{
    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    if ( !intersection.isDefined() )
	return false;

    int trcnr = mNINT( intersection.x );
    if ( intv )
	trcnr = intv->snap( trcnr );

    bid.inl = isinl_ ? nr_ : trcnr;
    bid.crl = isinl_ ? trcnr : nr_;
    z = intersection.y;
    return !intv || intv->includes( trcnr );
}


bool FaultTrace::isCrossing( const BinID& bid1, float z1,
			     const BinID& bid2, float z2  ) const
{
    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    return intersection.isDefined();
}


void FaultTrace::computeRange()
{
    trcrange_.set( mUdf(int), -mUdf(int) );
    zrange_.set( mUdf(float), -mUdf(float) );
    if ( trcnrs_.size() )
    {
	Interval<float> floattrcrg( mUdf(float), -mUdf(float) );
	for ( int idx=0; idx<trcnrs_.size(); idx++ )
	    floattrcrg.include( trcnrs_[idx], false );

	trcrange_.set( (int) floattrcrg.start, (int) ceil(floattrcrg.stop) );
    }
    else
    {
	for ( int idx=0; idx<coords_.size(); idx++ )
	{
	    const BinID bid = SI().transform( coords_[idx] );
	    trcrange_.include( isinl_ ? bid.crl : bid.inl, false );
	    zrange_.include( coords_[idx].z, false );
	}
    }

    computeTraceSegments();
}


bool FaultTrace::isOK() const
{
    if ( coords_.isEmpty() ) return false;

    float prevz = coords_[0].z;
    for ( int idx=1; idx<coords_.size(); idx++ )
    {
	if ( coords_[idx].z < prevz )
	    return false;

	prevz = coords_[idx].z;
    }

    return true;
}


// FaultTraceExtractor
FaultTraceExtractor::FaultTraceExtractor( EM::Fault* flt,
					  int nr, bool isinl )
  : fault_(flt)
  , nr_(nr), isinl_(isinl)
  , flttrc_(0)
  , is2d_(false)
{
    fault_->ref();
}


FaultTraceExtractor::FaultTraceExtractor( EM::Fault* flt,
					  const PosInfo::GeomID& geomid )
  : fault_(flt)
  , nr_(0),isinl_(true)
  , geomid_(geomid)
  , flttrc_(0)
  , is2d_(true)
{
    fault_->ref();
}



FaultTraceExtractor::~FaultTraceExtractor()
{
    fault_->unRef();
    if ( flttrc_ ) flttrc_->unRef();
}


bool FaultTraceExtractor::execute()
{
    if ( flttrc_ )
    {
	flttrc_->unRef();
	flttrc_ = 0;
    }

    if ( is2d_ )
	return get2DFaultTrace();

    EM::SectionID fltsid = fault_->sectionID( 0 );
    mDynamicCastGet(EM::Fault3D*,fault3d,fault_)
    Geometry::IndexedShape* efss = new Geometry::ExplFaultStickSurface(
		fault3d->geometry().sectionGeometry(fltsid), SI().zFactor() );
    efss->setCoordList( new FaultTrace, new FaultTrace, 0 );
    if ( !efss->update(true,0) )
	return false;

    CubeSampling cs;
    BinID start( isinl_ ? nr_ : cs.hrg.start.inl,
	    	 isinl_ ? cs.hrg.start.crl : nr_ );
    BinID stop( isinl_ ? nr_ : cs.hrg.stop.inl,
	    	isinl_ ? cs.hrg.stop.crl : nr_ );
    Coord3 p0( SI().transform(start), cs.zrg.start );
    Coord3 p1( SI().transform(start), cs.zrg.stop );
    Coord3 p2( SI().transform(stop), cs.zrg.stop );
    Coord3 p3( SI().transform(stop), cs.zrg.start );
    TypeSet<Coord3> pts;
    pts += p0; pts += p1; pts += p2; pts += p3;
    const Coord3 normal = (p1-p0).cross(p3-p0).normalize();

    Geometry::ExplPlaneIntersection* insectn =
					new Geometry::ExplPlaneIntersection;
    insectn->setShape( *efss );
    insectn->addPlane( normal, pts );
    Geometry::IndexedShape* idxdshape = insectn;
    idxdshape->setCoordList( new FaultTrace, new FaultTrace, 0 );
    if ( !idxdshape->update(true,0) )
	return false;

    Coord3List* clist = idxdshape->coordList();
    mDynamicCastGet(FaultTrace*,flttrc,clist);
    if ( !flttrc ) return false;

    const Geometry::IndexedGeometry* idxgeom = idxdshape->getGeometry()[0];
    if ( !idxgeom ) return false;

    flttrc_ = flttrc->clone();
    flttrc_->ref();
    flttrc_->setIndices( idxgeom->coordindices_ );
    flttrc_->setIsInl( isinl_ );
    flttrc_->setLineNr( nr_ );
    flttrc_->computeRange();
    delete efss;
    delete insectn;
    return true;
}
    

static float getFloatTrcNr( const PosInfo::Line2DData& linegeom,
			    const Coord& crd )
{
    const int index = linegeom.nearestIdx( crd );
    if ( index < 0 ) return mUdf(float);

    const TypeSet<PosInfo::Line2DPos>& posns = linegeom.positions();
    const PosInfo::Line2DPos& pos = posns[index];
    float closestdistfromnode = mUdf(float);
    int index2 = -1;
    if ( index > 0 )
    {
	const PosInfo::Line2DPos& prevpos = posns[index-1];
	const float distfromnode = prevpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index - 1;
	}
    }

    if ( posns.validIdx(index+1) )
    {
	const PosInfo::Line2DPos& nextpos = posns[index+1];
	const float distfromnode = nextpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index + 1;
	}
    }

    if ( index2 < 0 )
	return mUdf( float );

    const Coord linepos1 = pos.coord_;
    const Coord linepos2 = posns[index2].coord_;
    Line2 line( linepos1, linepos2 );
    Coord posonline = line.closestPoint( crd );
    if ( posonline.distTo(crd) > 100 )
	return mUdf(float);

    const float frac = linepos1.distTo(posonline) / linepos1.distTo(linepos2);
    return (float)pos.nr_ + frac * ( posns[index2].nr_ - pos.nr_ );
}


bool FaultTraceExtractor::get2DFaultTrace()
{
    mDynamicCastGet(const EM::FaultStickSet*,fss,fault_)
    if ( !fss ) return false;

    EM::SectionID sid = fault_->sectionID( 0 );
    S2DPOS().setCurLineSet( geomid_.lsid_ );
    PosInfo::Line2DData linegeom;
    if ( !S2DPOS().getGeometry(geomid_.lineid_,linegeom) )
	return false;

    const int nrsticks = fss->geometry().nrSticks( sid );
    TypeSet<int> indices;
    for ( int stickidx=0; stickidx<nrsticks; stickidx++ )
    {
	const Geometry::FaultStickSet* fltgeom =
				fss->geometry().sectionGeometry( sid );
	if ( !fltgeom ) continue;

	const int sticknr = fltgeom->rowRange().atIndex( stickidx );
	const MultiID* lsid = fss->geometry().lineSet( sid, sticknr );
	if ( !lsid ) continue;

	PtrMan<IOObj> lsobj = IOM().get( *lsid );
	if ( !lsobj ) continue;

	const char* linenm = fss->geometry().lineName( sid, sticknr );
	PosInfo::GeomID geomid = S2DPOS().getGeomID( lsobj->name(),linenm );
	if ( !(geomid==geomid_) ) continue;

	const int nrknots = fltgeom->nrKnots( sticknr );
	if ( nrknots < 2 ) continue;

	if ( !flttrc_ )
	{
	    flttrc_ = new FaultTrace;
	    flttrc_->ref();
	    flttrc_->setIsInl( true );
	    flttrc_->setLineNr( 0 );
	}

	StepInterval<int> colrg = fltgeom->colRange( sticknr );
	for ( int idx=colrg.start; idx<=colrg.stop; idx+=colrg.step )
	{
	    const Coord3 knot = fltgeom->getKnot( RowCol(sticknr,idx) );
	    const float trcnr = getFloatTrcNr( linegeom, knot );
	    if ( mIsUdf(trcnr) )
		break;

	    indices += flttrc_->add( knot, trcnr );
	}

	indices += -1;
    }

    if ( !flttrc_ )
	return false;

    flttrc_->setIndices( indices );
    flttrc_->computeRange();
    return flttrc_->getSize() > 1;
}


FaultTraceCalc::FaultTraceCalc( EM::Fault* flt, const HorSampling& hs,
		ObjectSet<FaultTrace>& trcs )
    : Executor("Extracting Fault Traces")
    , hs_(*new HorSampling(hs))
    , flt_(flt)
    , flttrcs_(trcs)
    , nrdone_(0)
    , isinl_(true)
{
    if ( flt )
	flt->ref();

    curnr_ = hs_.start.inl;
}

FaultTraceCalc::~FaultTraceCalc()
{ delete &hs_; if ( flt_ ) flt_->unRef(); }

od_int64 FaultTraceCalc::nrDone() const
{ return nrdone_; }

od_int64 FaultTraceCalc::totalNr() const
{ return hs_.nrInl() + hs_.nrCrl(); }

const char* FaultTraceCalc::message() const
{ return "Extracting Fault Traces"; }

int FaultTraceCalc::nextStep()
{
    if ( !isinl_ && (hs_.nrInl() == 1 || curnr_ > hs_.stop.crl) )
	return Finished();

    if ( isinl_ && (hs_.nrCrl() == 1 || curnr_ > hs_.stop.inl) )
    {
	isinl_ = false;
	curnr_ = hs_.start.crl;
	return MoreToDo();
    }

    FaultTraceExtractor ext( flt_, curnr_, isinl_ );
    if ( !ext.execute() )
	return ErrorOccurred();

    FaultTrace* flttrc = ext.getFaultTrace();
    if ( flttrc )
	flttrc->ref();

    flttrcs_ += flttrc;
    curnr_ += isinl_ ? hs_.step.inl : hs_.step.crl;
    nrdone_++;
    return MoreToDo();
}


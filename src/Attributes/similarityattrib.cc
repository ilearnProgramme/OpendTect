/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          June 2005
 RCS:           $Id: similarityattrib.cc,v 1.27 2006-05-12 07:55:37 cvshelene Exp $
________________________________________________________________________

-*/

#include "similarityattrib.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsteering.h"
#include "genericnumer.h"
#include "runstat.h"

#define mExtensionNone		0
#define mExtensionRot90		1
#define mExtensionRot180	2
#define mExtensionCube		3

namespace Attrib
{

void Similarity::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    ZGateParam* gate = new ZGateParam( gateStr() );
    gate->setLimits( Interval<float>(-1000,1000) );
    gate->setDefaultValue( Interval<float>(-28, 28) );
    desc->addParam( gate );

    BinIDParam* pos0 = new BinIDParam( pos0Str() );
    pos0->setDefaultValue( BinID(0,1) );    
    desc->addParam( pos0 );
    
    BinIDParam* pos1 = new BinIDParam( pos1Str() );
    pos1->setDefaultValue( BinID(0,-1) );    
    desc->addParam( pos1 );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    //Note: Ordering must be the same as numbering!
    EnumParam* extension = new EnumParam( extensionStr() );
    extension->addEnum( extensionTypeStr(mExtensionNone) );
    extension->addEnum( extensionTypeStr(mExtensionRot90) );
    extension->addEnum( extensionTypeStr(mExtensionRot180) );
    extension->addEnum( extensionTypeStr(mExtensionCube) );
    extension->setDefaultValue( mExtensionRot90 );
    desc->addParam( extension );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( true );
    desc->addParam( steering );

    BoolParam* normalize = new BoolParam( normalizeStr() );
    normalize->setDefaultValue( false );
    desc->addParam( normalize );

    desc->addInput( InputSpec("Input data",true) );
    desc->setNrOutputs( Seis::UnknowData, 5 );

    InputSpec steeringspec( "Steering data", false );
    steeringspec.issteering = true;
    desc->addInput( steeringspec );

    PF().addDesc( desc, createInstance );
    desc->unRef();
}


Provider* Similarity::createInstance( Desc& desc )
{
    Similarity* res = new Similarity( desc );
    res->ref();

    if ( !res->isOK() )
    {
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


void Similarity::updateDesc( Desc& desc )
{
    BufferString extstr = desc.getValParam(extensionStr())->getStringValue();
    const bool iscube = extstr == extensionTypeStr( mExtensionCube );
    desc.setParamEnabled( pos0Str(), !iscube );
    desc.setParamEnabled( pos1Str(), !iscube );
    desc.setParamEnabled( stepoutStr(), iscube );

    desc.inputSpec(1).enabled = desc.getValParam(steeringStr())->getBoolValue();
}


const char* Similarity::extensionTypeStr( int type )
{
    if ( type==mExtensionNone ) return "None";
    if ( type==mExtensionRot90 ) return "90";
    if ( type==mExtensionRot180 ) return "180";
    return "Cube";
}


Similarity::Similarity( Desc& desc_ )
    : Provider( desc_ )
{
    if ( !isOK() ) return;

    inputdata_.allowNull(true);

    mGetFloatInterval( gate_, gateStr() );
    gate_.scale( 1/zFactor() );

    mGetBool( dosteer_, steeringStr() );
    mGetBool( donormalize_, normalizeStr() );

    mGetEnum( extension_, extensionStr() );
    if ( extension_==mExtensionCube )
	mGetBinID( stepout_, stepoutStr() )
    else
    {
	mGetBinID( pos0_, pos0Str() )
	mGetBinID( pos1_, pos1Str() )

	if ( extension_==mExtensionRot90 )
	{
	    int maxstepout = mMAX( mMAX( abs(pos0_.inl), abs(pos1_.inl) ), 
		    		   mMAX( abs(pos0_.crl), abs(pos1_.crl) ) );
	    stepout_ = BinID( maxstepout, maxstepout );
	}
	else
	    stepout_ = BinID(mMAX(abs(pos0_.inl),abs(pos1_.inl)),
			     mMAX(abs(pos0_.crl),abs(pos1_.crl)) );

	    
    }
    getTrcPos();

    const float extraz = dosteer_ ?
	mMAX( stepout_.inl*inldist(), stepout_.crl*crldist() ) * mMAXDIP : 0;
    desgate_ = Interval<float>( gate_.start-extraz, gate_.stop+extraz );
}


bool Similarity::getTrcPos()
{
    trcpos_.erase();
    if ( extension_==mExtensionCube )
    {
	BinID bid;
	for ( bid.inl=-stepout_.inl; bid.inl<=stepout_.inl; bid.inl++ )
	    for ( bid.crl=-stepout_.crl; bid.crl<=stepout_.crl; bid.crl++ )
		trcpos_ += bid;

	for ( int idx=0; idx<trcpos_.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<trcpos_.size(); idy++)
	    {
		pos0s_ += idx;
		pos1s_ += idy;
	    }
	}
    }
    else
    {
	trcpos_ += pos0_;
	trcpos_ += pos1_;

	if ( extension_==mExtensionRot90 )
	{
	    trcpos_ += BinID(pos0_.crl,-pos0_.inl);
	    trcpos_ += BinID(pos1_.crl,-pos1_.inl);
	}
	else if ( extension_==mExtensionRot180 )
	{
	    trcpos_ += BinID(-pos0_.inl,-pos0_.crl);
	    trcpos_ += BinID(-pos1_.inl,-pos1_.crl);
	}
    }

    if ( dosteer_ )
    {
	steerindexes_.erase();
	for ( int idx=0; idx<trcpos_.size(); idx++ )
	    steerindexes_ += getSteeringIndex( trcpos_[idx] );
    }

    return true;
}


void Similarity::initSteering()
{
    for( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] && inputs[idx]->getDesc().isSteering() )
	    inputs[idx]->initSteering( stepout_ );
    }
}


bool Similarity::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !dosteer_ || !input ) return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<trcpos_.size(); idx++ )
	res += steerindexes_[idx];

    return true;
}


bool Similarity::getInputData( const BinID& relpos, int zintv )
{
    while ( inputdata_.size() < trcpos_.size() )
	inputdata_ += 0;

    const BinID bidstep = inputs[0]->getStepoutStep();
    for ( int idx=0; idx<trcpos_.size(); idx++ )
    {
	const DataHolder* data = 
		    inputs[0]->getData( relpos+trcpos_[idx]*bidstep, zintv );
	if ( !data ) return false;
	inputdata_.replace( idx, data );
    }
    
    dataidx_ = getDataIndex( 0 );

    steeringdata_ = dosteer_ ? inputs[1]->getData( relpos, zintv ) : 0;
    if ( dosteer_ && !steeringdata_ )
	return false;

    return true;
}


bool Similarity::computeData( const DataHolder& output, const BinID& relpos, 
			      int z0, int nrsamples ) const
{
    if ( !inputdata_.size() ) return false;

    const Interval<int> samplegate( mNINT(gate_.start/refstep),
				    mNINT(gate_.stop/refstep) );

    const int gatesz = samplegate.width() + 1;
    const int nrpairs = extension_==mExtensionCube ? pos0s_.size()
						   : inputdata_.size()/2;
    const int firstsample = inputdata_[0] ? z0-inputdata_[0]->z0_ : z0;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	RunningStatistics<float> stats;
	for ( int pair=0; pair<nrpairs; pair++ )
	{
	    const int idx0 = extension_==mExtensionCube ? pos0s_[pair] : pair*2;
	    const int idx1 = extension_==mExtensionCube ? pos1s_[pair]
							: pair*2 +1;
	    float s0 = firstsample + idx + samplegate.start;
	    float s1 = s0;

	    if ( !inputdata_[idx0] || !inputdata_[idx1] )
		continue;
	     
	    if ( dosteer_ )
	    {
		ValueSeries<float>* serie0 = 
			steeringdata_->series( steerindexes_[idx0] );
		if ( serie0 ) s0 += serie0->value( z0+idx-steeringdata_->z0_ );

		ValueSeries<float>* serie1 = 
			steeringdata_->series( steerindexes_[idx1] );
		if ( serie1 ) s1 += serie1->value( z0+idx-steeringdata_->z0_ );
	    }

	    SimiFunc vals0( *(inputdata_[idx0]->series(dataidx_)), 
			    inputdata_[idx0]->nrsamples_-1 );
	    SimiFunc vals1( *(inputdata_[idx1]->series(dataidx_)), 
			    inputdata_[idx1]->nrsamples_-1 );
	    const bool valids0 = s0>=0 && 
				 (s0+gatesz)<=inputdata_[idx0]->nrsamples_;
	    if ( !valids0 ) s0 = firstsample + idx + samplegate.start;

	    const bool valids1 = s1>=0 && 
				 (s1+gatesz)<=inputdata_[idx1]->nrsamples_;
	    if ( !valids1 ) s1 = firstsample + idx + samplegate.start;

	    stats += similarity( vals0, vals1, s0, s1, 1, gatesz, donormalize_);
	}

	const int outidx = z0 - output.z0_ + idx;
	if ( !stats.size() )
	{
	    for ( int sidx=0; sidx<outputinterest.size(); sidx++ )
		if ( outputinterest[sidx] ) 
		    output.series(sidx)->setValue( outidx, 0 );
	}
	else
	{
	    if ( outputinterest[0] ) 
		output.series(0)->setValue( outidx, stats.mean() );
	    if ( outputinterest[1] ) 
		output.series(1)->setValue( outidx, stats.median() );
	    if ( outputinterest[2] ) 
		output.series(2)->setValue( outidx, stats.variance() );
	    if ( outputinterest[3] ) 
		output.series(3)->setValue( outidx, stats.min() );
	    if ( outputinterest[4] ) 
		output.series(4)->setValue( outidx, stats.max() );
	}
    }

    return true;
}


const BinID* Similarity::reqStepout( int inp, int out ) const
{ return inp ? 0 : &stepout_; }


const Interval<float>* Similarity::reqZMargin( int inp, int ) const
{ 
    if ( inp ) return 0;

    bool chgstart = mNINT(desgate_.start*zFactor()) % mNINT(refstep*zFactor());
    bool chgstop = mNINT(desgate_.stop*zFactor()) % mNINT(refstep*zFactor());

    if ( chgstart )
    {
	int minstart = (int)(desgate_.start / refstep);
	const_cast<Similarity*>(this)->desgate_.start = (minstart-1) * refstep;
    }
    if ( chgstop )
    {
	int minstop = (int)(desgate_.stop / refstep);
	const_cast<Similarity*>(this)->desgate_.stop = (minstop+1) * refstep;
    }
    
    return &desgate_;
}

}; //namespace

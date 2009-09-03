/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieunitfactors.cc,v 1.25 2009-09-03 14:04:30 cvsbruno Exp $";

#include "welltieunitfactors.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "unitofmeasure.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllogset.h"
#include "wellman.h"

#include "welltiecshot.h"
#include "welltiesetup.h"

namespace WellTie
{

UnitFactors::UnitFactors( const WellTie::Setup* wtsetup )
	: velfactor_(0)
	, denfactor_(0)
	, veluom_(0)		    
	, denuom_(0)
{
    if ( !wtsetup ) return; 

    Well::Data* wd = Well::MGR().get( wtsetup->wellid_ ); 
    if ( !wd ) return; 

    const Well::Log* vellog =  wd->logs().getLog( wtsetup->vellognm_ );
    const Well::Log* denlog =  wd->logs().getLog( wtsetup->denlognm_ );
    if ( !vellog || !denlog ) 
	{ pErrMsg("Unable to access logs"); return; }

    const char* umlden = denlog->unitMeasLabel();
    denuom_ = UnitOfMeasure::getGuessed( umlden );
    const char* umlval = vellog->unitMeasLabel();
    veluom_ = UnitOfMeasure::getGuessed( umlval );

    if ( !denuom_ || !veluom_ )
	{ pErrMsg("No valid log units specified"); return; }

    velfactor_  = calcVelFactor( veluom_->symbol(), wtsetup->issonic_ );
    denfactor_ = calcDensFactor( denuom_->symbol() );
}


double UnitFactors::calcVelFactor( const char* velunit, bool issonic )
{
    return ( issonic ? calcSonicVelFactor( velunit ):calcVelFactor( velunit ) );
}


double UnitFactors::calcSonicVelFactor( const char* velunit )
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 0.001*mFromFeetFactor;
}


double UnitFactors::calcVelFactor( const char* velunit )
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 1000/mFromFeetFactor;
}


double UnitFactors::calcDensFactor( const char* densunit )
{
    const UnitOfMeasure* um = UoMR().get( densunit );
    return um ? um->userValue(1.0) : 1000;
}



Params::Params( const WellTie::Setup& wts, Well::Data* wd,
		const Attrib::DescSet& ads )
	: wtsetup_(wts)
	, uipms_(wd)				    
	, dpms_(wd,wts)				    
	, wd_(*wd)
	, ads_(ads)	  
{
    dpms_.corrstartdah_ = wd_.track().dah(0);
    dpms_.corrstopdah_  = wd_.track().dah(wd_.track().size()-1);
    dpms_.currvellognm_ = wts.vellognm_;
    dpms_.attrnm_ = getAttrName(ads);
    if ( wd_.checkShotModel() )
	dpms_.currvellognm_ = wtsetup_.corrvellognm_;
    dpms_.createColNames();

    resetParams();
}


BufferString Params::getAttrName( const Attrib::DescSet& ads ) const
{
    const Attrib::Desc* ad = ads.getDesc( wtsetup_.attrid_ );
    if ( !ad ) return 0;

    Attrib::SelInfo attrinf( &ads, 0, ads.is2D() );
    BufferStringSet bss;
    SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(0) ) );
    sii.getDefKeys( bss, true );
    const char* defkey = bss.get(0).buf();
    BufferString attrnm = ad->userRef();
    return SeisIOObjInfo::defKey2DispName(defkey,attrnm);
}

bool Params::resetParams()
{
    if ( !dpms_.resetDataParams() )
	return false;

    resetVellLognm();
    //TODO this should be an easiest way to set the vellognm than using 
    // one name for display and one for data!

    return true;
}


void Params::resetVellLognm()
{
    dpms_.currvellognm_ = uipms_.iscscorr_? wtsetup_.corrvellognm_
					  : wtsetup_.vellognm_;
    dpms_.dispcurrvellognm_ = uipms_.iscscorr_? dpms_.corrvellognm_
					      : dpms_.vellognm_;
}


#define mMaxWorkArraySize (int)1.e5
#define mComputeStepFactor (SI().zStep()/step_)
#define mMinWorkArraySize (int)20

bool Params::DataParams::resetDataParams()
{
    const float startdah = wd_.track().dah(0);
    const float stopdah  = wd_.track().dah(wd_.track().size()-1);

    setTimes( timeintv_, startdah, stopdah );
    setTimes( corrtimeintv_, corrstartdah_, corrstopdah_ );
    setDepths( timeintv_, dptintv_ );

    //TODO: change structure to get time and corrtime ALWAYS start at 0.
    //->no use to update startintv anymore!
    timeintv_.start = 0;
    if ( corrtimeintv_.start<0 ) corrtimeintv_.start = 0; 
    if ( corrtimeintv_.stop>timeintv_.stop ) corrtimeintv_.stop=timeintv_.stop;

    worksize_ = (int) ( (timeintv_.stop-timeintv_.start)/timeintv_.step );
    dispsize_ = (int) ( worksize_/step_ )-1;
    corrsize_ = (int) ( (corrtimeintv_.stop - corrtimeintv_.start )
	    		 	/( step_*timeintv_.step ) );

    if ( corrsize_>dispsize_ ) corrsize_ = dispsize_;
    if ( worksize_ > mMaxWorkArraySize ) return false;
    if ( worksize_ < mMinWorkArraySize || dispsize_ < 2 ) return false;
    
    return true;
}


bool Params::DataParams::setTimes( StepInterval<double>& timeintv, 
			      float startdah, float stopdah )
{
    const Well::D2TModel* d2t = wd_.d2TModel();
    if ( !d2t ) return false;

    timeintv.start = d2t->getTime( startdah );
    timeintv.stop  = d2t->getTime( stopdah );
    timeintv.step  = mComputeStepFactor;
    timeintv.sort();

    if ( timeintv.step < 1e-6 )
	return false;

    return true;
}


bool Params::DataParams::setDepths( const StepInterval<double>& timeintv,					   StepInterval<double>& dptintv )
{
    const Well::D2TModel* d2tm = wd_.d2TModel();
    if ( !d2tm ) return false;

    dptintv.start = d2tm->getDepth( timeintv.start );
    dptintv.stop  = d2tm->getDepth( timeintv.stop );
    return true;
}


void Params::DataParams::createColNames()
{
    colnms_.add ( dptnm_ = "Depth" );	
    colnms_.add( timenm_ = "Time" );
    colnms_.add( corrvellognm_ = wts_.corrvellognm_ );
    colnms_.add( vellognm_ = wts_.vellognm_ );
    colnms_.add( denlognm_ = wts_.denlognm_ );
    colnms_.add( ainm_ = "Computed AI" );     
    colnms_.add( refnm_ ="Computed Reflectivity" );
    colnms_.add( synthnm_ = "Synthetics" );
    colnms_.add( crosscorrnm_ = "Cross Correlation" );
    colnms_.add( attrnm_ );

    BufferString add2name = "'"; 
    vellognm_ += add2name;
    denlognm_ += add2name;		
    corrvellognm_ += add2name;
}

}; //namespace WellTie

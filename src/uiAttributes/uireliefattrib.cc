/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2016
________________________________________________________________________

-*/

#include "uireliefattrib.h"
#include "reliefattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "energyattrib.h"
#include "hilbertattrib.h"
#include "instantattrib.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"

using namespace Attrib;

mInitAttribUI(uiReliefAttrib,Relief,"Pseudo Relief",
	      sKeyBasicGrp())

uiReliefAttrib::uiReliefAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,mODHelpKey(mReliefAttribHelpID))

{
    inpfld_ = createInpFld( is2d );
    setHAlignObj( inpfld_ );
}


bool uiReliefAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Relief::attribName() )
	return false;
    return true;
}


bool uiReliefAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiReliefAttrib::setOutput( const Desc& )
{ return true; }


bool uiReliefAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Relief::attribName() )
	return false;
    return true;
}


static DescID hasDesc( const char* attrnm, const char* usrref,
		       const DescSet& ds, const DescID& inpid,
		       const DescID& inpid2=DescID::undef() )
{
    for ( int idx=0; idx<ds.size(); idx++ )
    {
	const Desc* desc = ds.desc( idx );
	if ( !desc ) continue;

	const FixedString usrrefstr = desc->userRef();
	if ( desc->attribName()!=attrnm || usrrefstr!=usrref )
	    continue;

	const Desc* inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id()!=inpid )
	    continue;

	if ( inpid2.isValid() )
	{
	    const Desc* inputdesc2 = desc->getInput( 1 );
	    if ( !inputdesc2 || inputdesc2->id()!=inpid2 )
		continue;
	}

	return desc->id();
    }

    return DescID::undef();
}


static BufferString createUserRef( const char* inpnm, const char* attrnm )
{
    BufferString usrref = "_";
    usrref.add( inpnm ).add( "_" ).add( attrnm ).add( "_" );
    return usrref;
}


static DescID addEnergyAttrib( DescSet& ds, const DescID& inpid )
{
    const Desc* inpdesc = ds.getDesc( inpid );
    if ( !inpdesc ) return DescID::undef();

    const char* attribnm = Energy::attribName();
    const BufferString usrref = createUserRef( inpdesc->userRef(), attribnm );
    const DescID descid = hasDesc( attribnm, usrref, ds, inpid );
    if ( descid.isValid() )
	return descid;

    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc ) return DescID::undef();

    newdesc->setInput( 0, inpdesc );
    newdesc->selectOutput( 0 );
    newdesc->setHidden( true );

    // The zgate has to be tested. If a zero timegate works best, we can remove
    // the scaling and even replace the Energy attribute by a simple Math
    // attribute
    Interval<float> zgate( 0, 0 );
    zgate.scale( SI().zDomain().userFactor() );
    mDynamicCastGet(ZGateParam*,param,newdesc->getParam(Energy::gateStr()))
    if ( param ) param->setValue( zgate );
    newdesc->setUserRef( usrref );
    return ds.addDesc( newdesc );
}


static DescID addHilbertAttrib( DescSet& ds, const DescID& inpid )
{
    const Desc* inpdesc = ds.getDesc( inpid );
    if ( !inpdesc ) return DescID::undef();

    const char* attribnm = Hilbert::attribName();
    const BufferString usrref = createUserRef( inpdesc->userRef(), attribnm );
    const DescID descid = hasDesc( attribnm, usrref, ds, inpid );
    if ( descid.isValid() )
	return descid;

    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc ) return DescID::undef();

    newdesc->setInput( 0, inpdesc );
    newdesc->selectOutput( 0 );
    newdesc->setHidden( true );
    newdesc->setUserRef( usrref );
    return ds.addDesc( newdesc );
}


static DescID addInstantaneousAttrib( DescSet& ds, const DescID& realid,
				      const DescID& imagid )
{
    const Desc* realdesc = ds.getDesc( realid );
    const Desc* imagdesc = ds.getDesc( imagid );
    if ( !realdesc || !imagdesc ) return DescID::undef();

    const char* attribnm = Instantaneous::attribName();
    const BufferString usrref = createUserRef( realdesc->userRef(), attribnm );
    const DescID descid = hasDesc( attribnm, usrref, ds, realid, imagid );
    if ( descid.isValid() )
	return descid;

    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc ) return DescID::undef();

    newdesc->setInput( 0, realdesc );
    newdesc->setInput( 1, imagdesc );
    newdesc->selectOutput( 13 ); // Phase rotation
    newdesc->setHidden( true );
    mDynamicCastGet(FloatParam*,param,
		    newdesc->getParam(Instantaneous::rotateAngle()))
    if ( param ) param->setValue( 90.f );
    newdesc->setUserRef( usrref );
    return ds.addDesc( newdesc );
}


bool uiReliefAttrib::getInput( Desc& desc )
{
    DescSet* ds = desc.descSet();
    if ( !ds ) return false;

    const DescID inpid = inpfld_->attribID();
    const DescID enid = addEnergyAttrib( *ds, inpid );
    const DescID hilbid = addHilbertAttrib( *ds, enid );
    const DescID instid = addInstantaneousAttrib( *ds, enid, hilbid );

    return desc.setInput( 0, ds->getDesc(instid) );
}


bool uiReliefAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, 0 );
    return true;
}

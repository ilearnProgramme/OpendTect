/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: gmtbasemap.cc,v 1.6 2008-09-02 11:08:30 cvsraman Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "draw.h"
#include "filepath.h"
#include "gmtbasemap.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"

int GMTBaseMap::factoryid_ = -1;

void GMTBaseMap::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Basemap", GMTBaseMap::createInstance );
}

GMTPar* GMTBaseMap::createInstance( const IOPar& iop )
{
    return new GMTBaseMap( iop );
}


bool GMTBaseMap::execute( std::ostream& strm, const char* fnm )
{
    strm << "Creating the Basemap ...  ";
    BufferString maptitle = find( ODGMT::sKeyMapTitle );
    Interval<float> lblintv;
    if ( !get(ODGMT::sKeyLabelIntv,lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps = false, dogrid = false;
    getYN( ODGMT::sKeyClosePS, closeps );
    getYN( ODGMT::sKeyDrawGridLines, dogrid );

    BufferString comm = "psbasemap ";
    BufferString str; mGetRangeProjString( str, "X" );
    comm += str; comm += " -Ba";
    comm += lblintv.start;
    if ( dogrid ) { comm += "g"; comm += lblintv.start; }
    comm += "/a"; comm += lblintv.stop;
    if ( dogrid ) { comm += "g"; comm += lblintv.stop; }
    comm += ":\"."; comm += maptitle; comm += "\":";
    comm += " --Y_AXIS_TYPE=ver_text";
    comm += " --X_ORIGIN="; comm += xmargin;
    comm += "c --Y_ORIGIN="; comm += ymargin;
    comm += "c --PAPER_MEDIA=Custom_";
    const float pagewidth = mapdim.start + ( closeps ? 2 : 4 ) * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    comm += pageheight < 21 ? 21 : pageheight; comm += "cx";
    comm += pagewidth < 21 ? 21 : pagewidth; comm += "c ";
    if ( !closeps ) comm += "-K ";
    comm += "> "; comm += fnm;
    if ( system(comm) )
	mErrStrmRet("Failed to create Basemap")

    strm << "Done" << std::endl;
    return true;
}


int GMTLegend::factoryid_ = -1;

void GMTLegend::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Legend", GMTLegend::createInstance );
}

GMTPar* GMTLegend::createInstance( const IOPar& iop )
{
    return new GMTLegend( iop );
}


bool GMTLegend::execute( std::ostream& strm, const char* fnm )
{
    strm << "Posting legends ...  ";
    Interval<float> mapdim;
    get( ODGMT::sKeyMapDim, mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    bool hascolbar = false;
    ObjectSet<IOPar> parset;
    for ( int idx=0; idx<100; idx++ )
    {
	IOPar* par = subselect( idx );
	if ( !par ) break;

	if ( par->find(ODGMT::sKeyPostColorBar) )
	{
	    hascolbar = true;
	    StepInterval<float> rg;
	    par->get( ODGMT::sKeyDataRange, rg );
	    FilePath fp( fnm );
	    fp.setExtension( "cpt" );
	    BufferString colbarcomm = "psscale --LABEL_FONT_SIZE=14 -D";
	    colbarcomm += mapdim.start + xmargin; colbarcomm += "c/";
	    colbarcomm += ymargin; colbarcomm += "c/";
	    colbarcomm += mapdim.stop / 2; colbarcomm += "c/";
	    colbarcomm += mapdim.start / 10; colbarcomm += "c -O -C";
	    colbarcomm += fp.fullPath(); colbarcomm += " -B";
	    colbarcomm += rg.step * 5; colbarcomm += ":\"";
	    colbarcomm += par->find( sKey::Name ); colbarcomm += "\":/:";
	    colbarcomm += SI().getZUnit(false); colbarcomm += ": -K >> ";
	    colbarcomm += fnm;
	    if ( system(colbarcomm) )
		mErrStrmRet("Failed to post color bar")

	    if ( !par->find(ODGMT::sKeyLineStyle) )
		continue;
	}

	parset += par;
    }

    BufferString comm = "@pslegend -R -J -O -Dx";
    comm += mapdim.start + xmargin; comm += "c/";
    comm += ymargin + ( hascolbar ? mapdim.stop / 2 : 0 ); comm += "c/";
    comm += 10; comm += "c/";
    comm += parset.size(); comm += "c/BL ";
    
    comm += ">> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<parset.size(); idx++ )
    {
	IOPar* par = parset[idx];
	BufferString namestr = par->find( sKey::Name );
	if ( namestr.isEmpty() )
	    continue;

	float size = 1;
	BufferString symbstr, penstr;
	const char* shapestr = par->find( ODGMT::sKeyShape );
	const ODGMT::Shape shape = eEnum( ODGMT::Shape, shapestr );
	if ( shape < 0 ) continue;

	symbstr = ODGMT::sShapeKeys[(int)shape];
	par->get( sKey::Size, size );
	if ( shape == ODGMT::Polygon || shape == ODGMT::Line )
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle );
	    if ( !lsstr ) continue;

	    LineStyle ls;
	    ls.fromString( lsstr );
	    if ( ls.type_ != LineStyle::None )
	    {
		mGetLineStyleString( ls, penstr );
	    }
	    else if ( shape == ODGMT::Line )
		continue;
	}
	else
	{
	    Color pencol;
	    par->get( sKey::Color, pencol );
	    BufferString colstr;
	    mGetColorString( pencol, colstr );
	    penstr = "1p,"; penstr += colstr;
	}

	bool dofill;
	par->getYN( ODGMT::sKeyFill, dofill );
	BufferString legendstring = "S 0.6c ";
	legendstring += symbstr; legendstring += " "; 
	legendstring += size > 1 ? 1 : size;
	legendstring += "c ";
	if ( dofill )
	{
	    BufferString fillcolstr;
	    Color fillcol;
	    par->get( ODGMT::sKeyFillColor, fillcol );
	    mGetColorString( fillcol, fillcolstr );
	    legendstring += fillcolstr;
	}
	else
	    legendstring += "-";

	legendstring += " ";
	if ( penstr.isEmpty() )
	    legendstring += "-";
	else
	    legendstring += penstr;

	legendstring += " "; legendstring += 1.3;
	legendstring += " "; legendstring += namestr;
	*sd.ostrm << legendstring << std::endl;
	*sd.ostrm << "G0.2c" << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}


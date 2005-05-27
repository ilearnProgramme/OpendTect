/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribstorprovider.cc,v 1.5 2005-05-27 07:28:02 cvshelene Exp $";

#include "attribstorprovider.h"

#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisread.h"
#include "seisreq.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "attriblinebuffer.h"
#include "threadwork.h"
#include "basictask.h"

namespace Attrib
{


void StorageProvider::initClass()
{
    Desc* desc = new Desc( attribName(), updateDesc );
    desc->ref();

    desc->addParam( new SeisStorageRefParam(keyStr()) );

    if ( !desc->init() )
    { desc->unRef(); return; }

    desc->addOutputDataType( Seis::UnknowData );

    PF().addDesc( desc, createFunc );
    desc->unRef();
}


Provider* StorageProvider::createFunc( Desc& ds )
{
    StorageProvider* res = new StorageProvider( ds );
    res->ref();

    if ( !res->isOK() ) { res->unRef(); return 0; }

    res->unRefNoDelete();
    return res; 
}


void StorageProvider::updateDesc( Desc& ds )
{
    ds.removeOutputs();

    const LineKey lk( ds.getParam(keyStr())->getStringValue(0) );

    const MultiID key( lk.lineName() );
    const BufferString attrnm = lk.attrName();
    
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.ioObj() || !rdr.prepareWork(true) ) return;

    SeisTrcTranslator* transl = rdr.translator();
    Seis2DLineSet* lset = rdr.lineSet();

    const bool is2d = rdr.is2D();
    if ( (is2d && !lset) || (!is2d && !transl) ) return;

    if ( is2d )
    {
	const bool issteering = attrnm==sKey::Steering;
	if ( issteering )
	{
	    ds.addOutputDataType( Seis::Dip );
	    ds.addOutputDataType( Seis::Dip );
	}
	else
	{
	    ds.addOutputDataType( Seis::UnknowData );
	}
    }
    else
    {
	const int nrattribs = transl->componentInfo().size();

	for ( int idx=0; idx<nrattribs; idx++ )
	    ds.addOutputDataType( (Seis::DataType)
		    transl->componentInfo()[idx]->datatype );
    }
}


StorageProvider::StorageProvider( Desc& desc_ )
    : Provider( desc_ )
    , status( Nada )
    , currentreq( 0 )
{
}


StorageProvider::~StorageProvider()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->unRef();
	    inputs.erase();

    desc.unRef();

    delete threadmanager;
    deepErase( computetasks );

    delete linebuffer;

    deepErase( rg );
}


bool StorageProvider::init()
{
    if ( status!=Nada ) return false;

    const LineKey lk( desc.getParam(keyStr())->getStringValue(0) );
    const MultiID mid( lk.lineName() );

    IOPar iopar;
    iopar.set( IOPar::compKey( SeisRequester::sKeysubsel, "ID" ) , mid );
    rg.usePar( iopar );
    if ( !rg.size() ) return false;

    bool isset = false;
    for ( int req=0; req<rg.size(); req++ )
    {
	if ( !initSeisRequester(req) ) { deepErase( rg ); return false; }

	const SeisTrcReader* reader = rg[req]->reader();
	if ( !reader ) { deepErase(rg); return false; }

	if ( reader->is2D() )
	{
	    const Seis2DLineSet* lset = reader->lineSet();
	    if ( !lset ) { deepErase(rg); return false; }

	    const int lineidx = lset->indexOf( lk.buf() );
	    if ( lineidx==-1 )
	    {
		storedvolume.hrg.start.inl = 0;
		storedvolume.hrg.stop.inl = lset->nrLines()-1;
		storedvolume.hrg.step.inl = 1;

		for ( int idx=0; idx<lset->nrLines(); idx++ )
		{
		    StepInterval<int> trcrg;
		    StepInterval<float> zrg;
		    if ( !lset->getRanges( idx, trcrg, zrg ) )
			continue;

		    if ( !isset )
		    {
			isset = true;
			storedvolume.hrg.start.crl = trcrg.start;
			storedvolume.hrg.stop.crl = trcrg.stop;
			storedvolume.zrg.start = zrg.start;
			storedvolume.zrg.stop = zrg.stop;
		    }
		    else
		    {
			storedvolume.hrg.start.crl =
			    mMIN(trcrg.start, storedvolume.hrg.start.crl );
			storedvolume.hrg.stop.crl =
			    mMAX(trcrg.stop, storedvolume.hrg.stop.crl );

			storedvolume.zrg.include( zrg );
		    }
		}
	    }
	    else
	    {
		storedvolume.hrg.start.inl = lineidx;
		storedvolume.hrg.stop.inl = lineidx;
		StepInterval<int> trcrg;
		StepInterval<float> zrg;
		if ( lset->getRanges( lineidx, trcrg, zrg ) )
		{
		    isset = true;
		    storedvolume.hrg.start.crl = trcrg.start;
		    storedvolume.hrg.stop.crl = trcrg.stop;
		    storedvolume.zrg.start = zrg.start;
		    storedvolume.zrg.stop = zrg.stop;
		}
	    }
	}
	else
	{
	    SeisTrcTranslator::getRanges(mid,storedvolume,lk);
	    //TODO see if we have to add it also in 2D.

	    const SeisTrcTranslator* transl = reader->translator();
	    const SeisSelData* seldata = transl ? transl->selData() : 0;
	    if ( seldata && seldata->type_==SeisSelData::Range )
	    {
		storedvolume.hrg.start.inl = seldata->inlrg_.start;
		storedvolume.hrg.stop.inl = seldata->inlrg_.stop;
		storedvolume.hrg.start.crl = seldata->crlrg_.start;
		storedvolume.hrg.stop.crl = seldata->crlrg_.stop;
		storedvolume.zrg.start = seldata->zrg_.start;
		storedvolume.zrg.stop = seldata->zrg_.stop;
	    }

	    isset = true;
	}
    }

    if ( !isset ) { deepErase( rg ); return false; }

    status = StorageOpened;
    return true;
}


int StorageProvider::moveToNextTrace()
{
    if ( status==Nada )
	return -1;

    if ( status==StorageOpened )
    {
	if ( !setSeisRequesterSelection( currentreq ) )
	    return -1;

	status = Ready;
    }

    while ( true ) 
    {
	const int res = rg[currentreq]->next();
	if ( res==-1 ) return -1;
	if ( !res )
	{
	    currentreq++;
	    if ( currentreq>=rg.size() )
		return 0;

	    if ( !setSeisRequesterSelection(currentreq) )
		return -1;

	    continue;
	}
	if ( res<3 )
	{
	    SeisTrc* trc = rg[currentreq]->get(0,0);
	    if ( trc )
	    {
		currentbid = trc->info().binid;
		if ( strcmp(curlinekey_.lineName(),"") )
		    trcnr_ = trc->info().nr;
		
		break;
	    }
	}
    }

    return 1;
}


bool StorageProvider::getPossibleVolume(int,CubeSampling& res)
{ res = storedvolume; return true; }


SeisRequester* StorageProvider::getSeisRequester()
{ return currentreq!=-1 && currentreq<rg.size() ? rg[currentreq] : 0; }


bool StorageProvider::initSeisRequester(int req)
{
    rg[req]->setStepout( bufferstepout.inl, bufferstepout.crl );
    return rg[req]->prepareWork();
}


void StorageProvider::updateStorageReqs()
{
    for ( int req=0; req<rg.size(); req++ )
	initSeisRequester(req);
}


bool StorageProvider::setSeisRequesterSelection(int req)
{
    if ( !desiredvolume ) return true;
    if ( ! &storedvolume ) return false;
    
    SeisTrcReader* reader = rg[req]->reader();
    if ( !reader ) return false;

    if ( reader->is2D() )
    {
	Seis2DLineSet* lset = reader->lineSet();
	if ( !lset ) return false;
	SeisSelData* sel = new SeisSelData;
	sel->type_ = SeisSelData::Range;
	sel->linekey_.setAttrName( curlinekey_.attrName() );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( (const char*)curlinekey_.lineName() != "" )
	{
	    sel->linekey_.setLineName( curlinekey_.lineName() );
	    int idx = lset->indexOf( curlinekey_ );
	    if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
	    {
		sel->crlrg_.start = desiredvolume->hrg.start.crl < trcrg.start?
		    		trcrg.start : desiredvolume->hrg.start.crl;
		sel->crlrg_.stop = desiredvolume->hrg.stop.crl > trcrg.start ?
		    		trcrg.stop : desiredvolume->hrg.stop.crl;
		sel->zrg_.start = desiredvolume->zrg.start < zrg.start ?
		    		zrg.start : desiredvolume->zrg.start;
		sel->zrg_.stop = desiredvolume->zrg.stop > zrg.stop ?
		    		zrg.stop : desiredvolume->zrg.stop;
	    }
	    reader->setSelData( sel ); //Memleak!
	}
	//TODO?
	return true;
    }
    
    CubeSampling cs;
    cs.hrg.start.inl = desiredvolume->hrg.start.inl<storedvolume.hrg.start.inl ?
		storedvolume.hrg.start.inl : desiredvolume->hrg.start.inl;
    cs.hrg.stop.inl = desiredvolume->hrg.stop.inl > storedvolume.hrg.stop.inl ?
	        storedvolume.hrg.stop.inl : desiredvolume->hrg.stop.inl;
    cs.hrg.stop.crl = desiredvolume->hrg.stop.crl > storedvolume.hrg.stop.crl ?
	        storedvolume.hrg.stop.crl : desiredvolume->hrg.stop.crl;
    cs.hrg.start.crl = desiredvolume->hrg.start.crl<storedvolume.hrg.start.crl ?
		storedvolume.hrg.start.crl : desiredvolume->hrg.start.crl;
    cs.zrg.start = desiredvolume->zrg.start < storedvolume.zrg.start ?
		storedvolume.zrg.start : desiredvolume->zrg.start;
    cs.zrg.stop = desiredvolume->zrg.stop > storedvolume.zrg.stop ?
	                storedvolume.zrg.stop : desiredvolume->zrg.stop;

    SeisSelData* sel = new SeisSelData;
    sel->type_ = SeisSelData::Range;
    sel->inlrg_.start = cs.hrg.start.inl;
    sel->inlrg_.stop = cs.hrg.stop.inl;
    sel->crlrg_.start = cs.hrg.start.crl;
    sel->crlrg_.stop = cs.hrg.stop.crl;
    sel->zrg_.start = cs.zrg.start;
    sel->zrg_.stop = cs.zrg.stop;

    reader->setSelData( sel ); //Memleak!

    SeisTrcTranslator* transl = reader->translator();

    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	if ( !outputinterest[idx] ) transl->componentInfo()[idx]->destidx = -1;
    }

    return true;
}


bool StorageProvider::computeData( const DataHolder& output,
				   const BinID& relpos,
				   int t0, int nrsamples ) const
{
    BinID nullbid(0,0);
    SeisTrc* trc;
    if ( relpos==nullbid )
	trc = rg[currentreq]->get(0,0);
    else
    {
	if (trcnr_ == -1)
	    trc = rg[currentreq]->get(currentbid + relpos);
	else
	{
	    BinID bid(currentbid);
	    bid.crl = trcnr_;
	    trc = rg[currentreq]->get(bid + relpos);
	}
    }
    
    if ( trc )
    {
	fillDataHolderWithTrc( trc, output );
	return true;
    }
    else
	return false;
}


void StorageProvider::fillDataHolderWithTrc( const SeisTrc* trc, 
				    const DataHolder& trcsamplvalues ) const
{
    int t0 = trcsamplvalues.t0_;
    float starttrctime = trc->startPos();
    float trcstep = trc->info().sampling.step;
    float reqstarttime = starttrctime + t0 * trcstep;
    for (int idx=0 ; idx<trcsamplvalues.nrsamples_ ; idx++)
    {
	for ( int idy=0; idy< outputinterest.size(); idy++ )
	    if ( outputinterest[idy] )
	    {
		float val = trc->getValue( reqstarttime + idx * refstep, idy );
		trcsamplvalues.item(idy)->setValue(idx,val);
	    }
    }
}


BinID StorageProvider::getStepoutStep()
{
    SeisRequester* req = getSeisRequester();
    if ( !req ) return (0,0);
    SeisTrcTranslator* transl = req->reader()->translator();
    BinID mystep( transl->packetInfo().inlrg.step,
		  transl->packetInfo().crlrg.step );
    return mystep;
}


void StorageProvider::adjust2DLineStoredVolume()
{
    const SeisTrcReader* reader = rg[currentreq]->reader();
    if ( reader->is2D() )
    {
        const Seis2DLineSet* lset = reader->lineSet();
	int idx = lset->indexOf( curlinekey_ );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( idx >= 0 && lset->getRanges(idx,trcrg,zrg) )
	{
	    storedvolume.hrg.start.crl = trcrg.start;
	    storedvolume.hrg.stop.crl = trcrg.stop;
	    storedvolume.zrg.start = zrg.start;
	    storedvolume.zrg.stop = zrg.stop;
	}
    }
}

}; //namespace

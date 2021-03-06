#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2004
________________________________________________________________________

-*/

#include "seistrctr.h"
#include "executor.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;
namespace PosInfo	{ class Line2DData; }
namespace Seis		{ class LineProvider; class SelData; }


/*!\brief TranslatorGroup for 2D Seismic Data */

mExpClass(Seis) SeisTrc2DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(SeisTrc2D);
    mODTextTranslationClass(SeisTrc2DTranslatorGroup);
public:
			mDefEmptyTranslatorGroupConstructor(SeisTrc2D)

    static const char*	sKeyDefault()	{ return "2D Cube"; }
    const char*		getSurveyDefaultKey(const IOObj*) const;
};


/*!\brief interface for object that writes 2D seismic data */

mExpClass(Seis) Seis2DLinePutter
{ mODTextTranslationClass(Seis2DLinePutter);
public:

    virtual		~Seis2DLinePutter()	{}

    virtual bool	put(const SeisTrc&)	= 0;
				//!< Return false on success, err msg on failure
    virtual bool	close()			= 0;
				//!< Return null on success, err msg on failure
    virtual uiString	errMsg() const		= 0;
				//!< Only when put or close returns false
    virtual int		nrWritten() const	= 0;

};


/*!\brief Provides access to 2D seismic line data. */

mExpClass(Seis) Seis2DTraceGetter
{ mODTextTranslationClass(Seis2DTraceGetter);
public:

    typedef IdxPair::IdxType	TrcNrType;
    typedef IdxPair::IdxType	LineNrType;

    virtual		~Seis2DTraceGetter();

    const IOObj&	ioobj() const		{ return ioobj_; }
    Pos::GeomID		geomID() const		{ return geomid_; }

    uiRetVal		get(TrcNrType,SeisTrc&) const;
    uiRetVal		get(TrcNrType,TraceData&,SeisTrcInfo*) const;
    uiRetVal		getNext(SeisTrc&) const;

    bool		getComponentInfo(BufferStringSet&) const;

protected:

			Seis2DTraceGetter(const IOObj&,Pos::GeomID,
					  const Seis::SelData*);

    virtual void	mkTranslator() const	= 0;

    IOObj&		ioobj_;
    const Pos::GeomID	geomid_;
    Seis::SelData*	seldata_;
    mutable uiString	initmsg_;
    mutable SeisTrcTranslator* tr_;

    bool		ensureTranslator() const;
    void		ensureCorrectTrcKey(SeisTrcInfo&) const;
    LineNrType		lineNr() const		{ return geomid_; }
    void		setErrMsgForNoTrMade() const;

private:

    uiRetVal		doGet(TrcNrType,SeisTrc*,TraceData&,SeisTrcInfo*) const;

    friend class	Seis::LineProvider;

};


/*!\brief Provides read/write to/from 2D seismic lines.
	  Only interesting if you want to add your own 2D data I/O. */

mExpClass(Seis) Seis2DLineIOProvider
{ mODTextTranslationClass(Seis2DLineIOProvider);
public:

    virtual		~Seis2DLineIOProvider()			{}

    virtual bool	isEmpty(const IOObj&,Pos::GeomID) const		= 0;
    virtual uiRetVal	getGeomIDs(const IOObj&,TypeSet<Pos::GeomID>&) const
									= 0;
    virtual uiRetVal	getGeometry(const IOObj&,Pos::GeomID,
				    PosInfo::Line2DData&) const		= 0;

    virtual Seis2DTraceGetter*	getTraceGetter(const IOObj&,Pos::GeomID,
				    const Seis::SelData*,uiRetVal&)	= 0;
    virtual Seis2DLinePutter*	getPutter(const IOObj&,Pos::GeomID,
					  uiRetVal&)			= 0;

    virtual bool	getTxtInfo(const IOObj&,Pos::GeomID,BufferString&,
				   BufferString&) const		{ return false;}
    virtual bool	getRanges(const IOObj&,Pos::GeomID,StepInterval<int>&,
				   StepInterval<float>&) const	{ return false;}

    virtual bool	removeImpl(const IOObj&,Pos::GeomID) const	= 0;
    virtual bool	renameImpl(const IOObj&,const char*) const	= 0;

    const char*		type() const			{ return type_.buf(); }

protected:

			Seis2DLineIOProvider( const char* t )
			: type_(t)				{}

    const BufferString	type_;

};


mGlobal(Seis) ObjectSet<Seis2DLineIOProvider>& S2DLIOPs();
//!< Sort of factory. Add a new type via this function.


//------
//! Translator mechanism is only used for selection etc.

mExpClass(Seis) TwoDSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(TwoDSeisTrcTranslator); isTranslator(TwoD,SeisTrc)
public:
			TwoDSeisTrcTranslator( const char* s1, const char* s2 )
			: SeisTrcTranslator(s1,s2)      {}

    const char*		defExtension() const		{ return "2ds"; }
    bool		initRead_();		//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }

    bool		implRemove(const IOObj*) const;
    bool		implRename( const IOObj*,const char*,
				    const CallBack* cb=0) const;

};


/*!\brief Dummy old translator used during conversion only */
mExpClass(Seis) TwoDDataSeisTrcTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(TwoDDataSeisTrcTranslator);
  isTranslator(TwoDData,SeisTrc)
public:
			TwoDDataSeisTrcTranslator(const char* s1,const char* s2)
			: SeisTrcTranslator(s1,s2)      {}

};


/*!\brief Base translator for 2D Seismics */
mExpClass(Seis) SeisTrc2DTranslator : public SeisTrcTranslator
{ mODTextTranslationClass(SeisTrc2DTranslator);
public:
			SeisTrc2DTranslator(const char* s1,const char* s2)
			: SeisTrcTranslator(s1,s2)	{}

    bool		initRead_();		//!< supporting getRanges()
    bool		initWrite_(const SeisTrc&)	{ return false; }

    bool		isUserSelectable(bool) const	{ return true; }

    bool		implRemove(const IOObj*) const;
    bool		implRename( const IOObj*,const char*,
				    const CallBack* cb=0) const;

};


/*!\brief CBVS translator for 2D Seismics */
mExpClass(Seis) CBVSSeisTrc2DTranslator : public SeisTrc2DTranslator
{ mODTextTranslationClass(CBVSSeisTrc2DTranslator);
  isTranslator(CBVS,SeisTrc2D)
public:
			CBVSSeisTrc2DTranslator(const char* s1,const char* s2)
			: SeisTrc2DTranslator(s1,s2)	{}

    bool		isUserSelectable(bool) const	{ return true; }
};

/*!\brief SEGYDirect translator for 2D Seismics */
mExpClass(Seis) SEGYDirectSeisTrc2DTranslator : public SeisTrc2DTranslator
{ mODTextTranslationClass(SEGYDirectSeisTrc2DTranslator);
  isTranslator(SEGYDirect,SeisTrc2D)
public:
			SEGYDirectSeisTrc2DTranslator(const char* s1,
						      const char* s2)
			: SeisTrc2DTranslator(s1,s2)	{}

    virtual bool	isUserSelectable(bool fr) const { return fr; }
    virtual const char* iconName() const		{ return "segy"; }
};

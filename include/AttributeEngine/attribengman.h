#ifndef attribengman_h
#define attribengman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id: attribengman.h,v 1.15 2005-10-19 11:28:43 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "ranges.h"
#include "bufstring.h"
#include "attribdescid.h"

class BinIDValueSet;
class BufferStringSet;
class CubeSampling;
class ExecutorGroup;
class IOPar;
class LineKey;
class NLAModel;
class SeisTrcBuf;
class SeisTrcInfo;

namespace Attrib
{
class SeisTrcStorOutput;
class Desc;
class DescSet;
class SelSpec;
class SliceSet;
class Processor;
class DataHolder;

/*!\brief The Attribute engine Manager. */

class EngineMan
{
public:
			EngineMan();
    virtual		~EngineMan();

    void		usePar(const IOPar&,const DescSet&,
	    		       const char* linename,
			       ObjectSet<Processor>&); 

    static void		createProcSet(ObjectSet<Processor>& procset,
				      const DescSet& attribset,
				      const char* linename,
				      const TypeSet<DescID>&);
    static void		getPossibleVolume(const DescSet&,CubeSampling&,
	    				  const char* linename,const DescID&);
    static void		addNLADesc(const char* defstr,DescID& nladescid,
				   DescSet&,int outputnr,const NLAModel*,
				   BufferString& errmsg);

    SeisTrcStorOutput* 	createOutput(const IOPar&,const LineKey&);

    const DescSet* 	attribSet() const	{ return inpattrset; }
    const NLAModel*	nlaModel() const	{ return nlamodel; }
    const CubeSampling&	cubeSampling() const	{ return cs_; }
    const BufferString&	lineKey() const		{ return linekey; }
    float		undefValue() const	{ return udfval; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setAttribSpecs(const TypeSet<SelSpec>&);
    void		setCubeSampling(const CubeSampling&);
    void		setLineKey( const char* lk )	{ linekey = lk; }
    void		setUndefValue( float v )	{ udfval = v; }
    DescSet*		createNLAADS(DescID& outid,BufferString& errmsg,
	    			     const DescSet* addtoset=0);
    static DescID	createEvaluateADS(DescSet&, const TypeSet<DescID>&,
	    				  BufferString&);

    ExecutorGroup*	createSliceSetOutput(BufferString& errmsg,
	    			      const SliceSet* cached_data = 0);
    			//!< Give the previous calculated data in cached data
    			//!< and some parts may not be recalculated.
    SliceSet*		getSliceSetOutput();
    			//!< Mem transfer here

    ExecutorGroup* 	createFeatureOutput(const BufferStringSet& inputs,
					    const ObjectSet<BinIDValueSet>&);

    ExecutorGroup*	createScreenOutput2D(BufferString& errmsg,
	    				      ObjectSet<DataHolder>&,
					      ObjectSet<SeisTrcInfo>&);
    ExecutorGroup*	createLocationOutput(BufferString& errmsg,
					     ObjectSet<BinIDValueSet>&);

    ExecutorGroup*	createTrcSelOutput(BufferString& errmsg,
	    				   const BinIDValueSet& bidvalset,
	    				   SeisTrcBuf&, float outval=0,
					   Interval<float>* extraz=0);
    int			getNrOutputsToBeProcessed() const; 

    const char*		getCurUserRef() const;

protected:

    const DescSet* 	inpattrset;
    const NLAModel*	nlamodel;
    CubeSampling&	cs_;
    SliceSet*		cache;
    float		udfval;
    BufferString	linekey;

    const DescSet* 	curattrset;
    DescSet*		procattrset;
    int			curattridx;
    TypeSet<SelSpec>	attrspecs_;

    bool		getProcessors(ObjectSet<Processor>&,BufferString& err);
    BufferString	createExecutorName() const;
    ExecutorGroup*	createExecutorGroup() const;

private:

    friend class		AEMFeatureExtracter;

    ObjectSet<Processor> 	procset;
    void			clearZPtrs();

};

};//namespace


#endif

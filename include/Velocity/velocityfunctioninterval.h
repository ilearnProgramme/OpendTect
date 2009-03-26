#ifndef velocityfunctioninterval_h
#define velocityfunctioninterval_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		December 2008
 RCS:		$Id: velocityfunctioninterval.h,v 1.3 2009-03-26 12:45:00 cvskris Exp $
________________________________________________________________________


-*/

#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"

class BinIDValueSet;
class MultiID;
class SeisTrcReader;
class Gridder2D;

namespace Vel
{

class IntervalSource;

/*!A velocity funcion that computes interval velocity from where from
   another velocity function with RMS velocity */

mClass IntervalFunction : public Function
{
public:
			IntervalFunction(IntervalSource&);

    StepInterval<float>	getAvailableZ() const;
    bool		moveTo(const BinID&);
    void		setInput(Function*);

protected:
    			~IntervalFunction();

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;

    Function*		inputfunc_;
};


mClass IntervalSource : public FunctionSource
{
public:
    			IntervalSource();
    const VelocityDesc&	getDesc() const;
    const char*		type() const { return sType(); }
    static const char*	sType() { return "Interval"; }

    void		setInput(FunctionSource*);
    void		getAvailablePositions(HorSampling&) const;
    void		getSources(TypeSet<MultiID>&) const;
    void		getAvailablePositions(BinIDValueSet&) const;

    NotifierAccess*	changeNotifier();
    BinID		changeBinID() const;

protected:
    void		sourceChangeCB(CallBacker*);
    IntervalFunction*	createFunction(const BinID&);
    			~IntervalSource();

    FunctionSource*	inputsource_;
    VelocityDesc	veldesc_;
};


}; //namespace

#endif

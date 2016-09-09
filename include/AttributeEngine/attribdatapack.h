#ifndef attribdatapack_h
#define attribdatapack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "datapackbase.h"
#include "cubesampling.h"
#include "attribdescid.h"
#include "paralleltask.h"
#include "seisinfo.h"
#include "survgeom.h"

template <class T> class Array2D;
template <class T> class Array2DSlice;
template <class T> class Array2DImpl;
class SeisTrcBuf;
class ZAxisTransform;

namespace Attrib
{
class DataCubes;
class Data2DHolder;
class DataHolderArray;
class Data2DArray;

/*!
\brief Mixin to provide general services to attribute data packs.
*/

mExpClass(AttributeEngine) DataPackCommon
{
public:
    			DataPackCommon( DescID id )
			    : descid_(id)	{}

    virtual const char*	sourceType() const	= 0;
    virtual bool	isVertical() const	{ return false; }

    DescID		descID() const		{ return descid_; }

    void		dumpInfo(IOPar&) const;

    static const char*	categoryStr(bool vertical,bool is2d = false);

protected:

    DescID		descid_;

};


/*!
\brief Base class data pack for 2D.
*/

mExpClass(AttributeEngine) Flat2DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:
    			Flat2DDataPack(DescID);
    virtual const char*	sourceType() const			= 0;
    virtual bool	isVertical() const			{ return true; }

    virtual Array2D<float>&	data()				= 0;

    Coord3		getCoord(int,int) const			= 0;
    void		getAltDim0Keys(BufferStringSet&) const;
    bool		isAltDim0InInt(const char* key) const;
    double		getAltDim0Value(int,int) const		= 0;
    void		getAuxInfo(int,int,IOPar&) const	= 0;

    void		dumpInfo(IOPar&) const;
    const char*		dimName(bool) const;

protected:

    TypeSet<SeisTrcInfo::Fld> tiflds_;
};


/*!
\brief Data pack from 2D attribute data.
*/

mExpClass(AttributeEngine) Flat2DDHDataPack : public Flat2DDataPack
{
public:
    			Flat2DDHDataPack(DescID,const Data2DHolder&,
					 bool usesingtrc=false,int component=0,
					 const Pos::GeomID& geomid=
					 Survey::GM().cUndefGeomID());
			Flat2DDHDataPack(DescID,const Array2D<float>*,
					 const Pos::GeomID& geomid,
					 const SamplingData<float>& zsamp,
					 const StepInterval<int>& trcrg);
			~Flat2DDHDataPack();

    bool		isOK() const		{ return dataholderarr_; }

    const Data2DArray*	dataarray() const	{ return dataholderarr_; }
    void		setDataArray(const Data2DArray*);
    void		setRefNrs(TypeSet<float>);

    virtual const char*	sourceType() const	{ return "2D"; }

    void		getPosDataTable(TypeSet<int>& trcnrs,
	    				TypeSet<float>& dist) const;
    void		getCoordDataTable(const TypeSet<int>& trcnrs,
	    				  TypeSet<Coord>& coords) const;
    Array2D<float>&	data()			{ return *arr2d_; }

    void		setLineName(const char* nm)
			{ geomid_ = Survey::GM().getGeomID(nm); }
    void		getLineName( BufferString& nm ) const
			{ nm = Survey::GM().getName( geomid_ ); }
    Pos::GeomID		getGeomID() const	{ return geomid_; }
    TrcKey		getTrcKey(int index) const
			{ return Survey::GM().traceKey(
				geomid_, tracerange_.atIndex(index) ); }

    Coord3		getCoord(int,int) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;

    const StepInterval<int>&	getTraceRange() const	{ return tracerange_; }
    CubeSampling		getCubeSampling() const;

protected:

    Array2DSlice<float>*	array2dslice_;
    const Data2DArray*		dataholderarr_;
    StepInterval<int>		tracerange_;
    SamplingData<float>		samplingdata_;

    bool			usesingtrc_;
    Pos::GeomID			geomid_;
    BufferString		linenm_;

    void			setPosData();
};


/*!
\brief Flat data pack from 3D attribute extraction.
*/ 

mExpClass(AttributeEngine) Flat3DDataPack : public ::FlatDataPack
		     , public DataPackCommon
{
public:

    			Flat3DDataPack(DescID,const DataCubes&,int cubeidx);
    virtual		~Flat3DDataPack();
    virtual const char*	sourceType() const	{ return "3D"; }
    virtual bool	isVertical() const
    			{ return dir_ != CubeSampling::Z; }

    int			getCubeIdx() const	{ return cubeidx_; }
    const DataCubes&	cube() const		{ return cube_; }
    Array2D<float>&	data();
    bool		setDataDir(CubeSampling::Dir);
    CubeSampling::Dir	dataDir() const		{ return dir_; }
    int			nrSlices() const;
    bool		setDataSlice(int);
    int			getDataSlice() const;
    const char*		dimName(bool) const;

    Coord3		getCoord(int,int) const;
    bool		isAltDim0InInt(const char* key) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    Array2DSlice<float>* arr2dsl_;
    Array2D<float>*	arr2dsource_;
    CubeSampling::Dir	dir_;
    bool		usemultcubes_;
    int			cubeidx_;

    void		setPosData();
    void		createA2DSFromMultCubes();
    void		createA2DSFromSingCube(int);
};


/*!
\brief Volume data pack.
*/ 

mExpClass(AttributeEngine) CubeDataPack : public ::CubeDataPack
		   , public DataPackCommon
{
public:

    			CubeDataPack(DescID,const DataCubes&,int cubeidx);
			~CubeDataPack();

    virtual const char*	sourceType() const	{ return "3D"; }

    const DataCubes&	cube() const		{ return cube_; }
    Array3D<float>&	data();

    void		getAuxInfo(int,int,int,IOPar&) const;
    void		dumpInfo(IOPar&) const;

protected:

    const DataCubes&	cube_;
    int			cubeidx_;

};


/*!
\brief Data pack from random traces extraction.
*/

mExpClass(AttributeEngine) FlatRdmTrcsDataPack : public Flat2DDataPack
{
public:
    			FlatRdmTrcsDataPack(DescID,const SeisTrcBuf&,
					    TypeSet<BinID>* path=0);
			FlatRdmTrcsDataPack(DescID,const Array2DImpl<float>*,
					    const SamplingData<float>& zsamp,
					    TypeSet<BinID>* path=0);
			~FlatRdmTrcsDataPack();
    virtual const char*	sourceType() const	{ return "Random Line"; }

    const SeisTrcBuf&	seisBuf() const		{ return *seisbuf_; }
    Array2D<float>&	data()			{ return *arr2d_; }

    void		setSeisTrcBuf(const SeisTrcBuf*);
    void		setRefNrs(TypeSet<float>);

    Coord3		getCoord(int,int) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;

    const TypeSet<BinID>* pathBIDs() const { return path_; }

protected:

    SeisTrcBuf* 	seisbuf_;

    void		setPosData(TypeSet<BinID>*);
    void		fill2DArray(TypeSet<BinID>*);

    const SamplingData<float>	samplingdata_;
    TypeSet<BinID>*		path_;
};


/*!
\brief Transforms datapacks using ZAxisTransform. Output datapacks will be of
same type as inputdp_. At present, this works only for Flat2DDHDataPack and
FlatRdmTrcsDataPack.
*/

mExpClass(AttributeEngine) FlatDataPackZAxisTransformer : public ParallelTask
{
public:
				FlatDataPackZAxisTransformer(ZAxisTransform&);
				~FlatDataPackZAxisTransformer();

    void			setInput( FlatDataPack* fdp )
				{ inputdp_ = fdp; }
    void			setOutput( TypeSet<DataPack::ID>& dpids )
				{ dpids_ = &dpids; }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

protected:

    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int threadid);
    bool			doFinish(bool success);
    od_int64			nrIterations() const;

    DataPackMgr&		dpm_;
    ZAxisTransform&		transform_;
    StepInterval<float>		zrange_;
    bool			interpolate_;
    FlatDataPack*		inputdp_;

    TypeSet<DataPack::ID>*		dpids_;
    ObjectSet<Array2DImpl<float> >	arr2d_;
};


} // namespace Attrib

#endif

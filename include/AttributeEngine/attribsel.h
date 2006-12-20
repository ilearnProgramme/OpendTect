#ifndef attribsel_h
#define attribsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2001
 RCS:           $Id: attribsel.h,v 1.12 2006-12-20 11:23:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "multiid.h"
#include "bufstringset.h"
#include "attribdescid.h"

class IOPar;
class NLAModel;

/*!\brief specifies an attribute selection (ID or output number of NN).

  When attrib sets and NLAs change, the IDs may be no longer valid. Thus, the
  user reference is stored, so you can try to get a valid ID in that situation.

  Object reference holds the NLA or attribute set name.
  discrSpec() specifies whether (if (0,0) interval not) and how a discrete
  output is to be expected.
 
 */

namespace Attrib 
{

class Desc;
class DescSet;

class SelSpec
{
public:
			SelSpec( const char* r=0, DescID i=cAttribNotSel(),
				 bool n=false, const char* objr=0 )
			: ref_(r), id_(i), isnla_(n)
			, objref_(objr)
			, is2d_(false)		{}

    const DescID&	id() const		{ return id_; }
    bool		isNLA() const		{ return isnla_; }
    bool		is2D() const		{ return is2d_; }
    const char*		userRef() const		{ return ref_; }
    const char*		objectRef() const	{ return objref_; }
    const char*		defString() const	{ return defstring_; }
    const char*		depthDomainKey() const	{ return depthdomainkey_; }

    bool		operator==(const SelSpec&) const;
    bool		operator!=(const SelSpec&) const;

    void		set(const Desc&);
    void		set(const NLAModel&,int);
    void		setObjectRef( const char* objr )
			    { objref_ = objr; }
    void		setDefString( const char* def )
			    { defstring_ = def;}
    void		setDepthDomainKey( const char* key )
			    { depthdomainkey_ = key; }
    void		setDepthDomainKey(const Desc&);

    void		set( const char* r, DescID i, bool isnla, 
	    		     const char* objr )
			{ ref_ = r; id_ = i; isnla_ = isnla; objref_ = objr;
		          defstring_ = ""; depthdomainkey_ = ""; }

    void		setIDFromRef(const NLAModel&);
    void		setIDFromRef(const DescSet&);
    void		setRefFromID(const NLAModel&);
    void		setRefFromID(const DescSet&);
    void		set2DFlag()			{ is2d_ = true; }

    const StepInterval<int>&	discrSpec() const	{ return discrspec_; }
    void		setDiscrSpec( const StepInterval<int>& ds )
			{ discrspec_ = ds; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const DescID& cNoAttrib();
    static const DescID& cAttribNotSel();
    static const DescID& cOtherAttrib();

protected:

    BufferString	ref_;
    BufferString	objref_;
    BufferString	defstring_;
    BufferString	depthdomainkey_;
    DescID		id_;
    bool		isnla_;
    StepInterval<int>	discrspec_;
    bool		is2d_;

    static const char*	sKeyRef();
    static const char*	sKeyObjRef();
    static const char*	sKeyID();
    static const char*	sKeyIsNLA();
    static const char*	sKeyDefStr();
    static const char*	sKeyDepthDomainStr();

    void		setDiscr(const DescSet&);
    void		setDiscr(const NLAModel&);
};


/*!\brief specifies current attribute choices (ID or output nr of NLA model). */

class CurrentSel
{
public:
			CurrentSel()
			: attrid(DescID(-1,true)), outputnr(-1)	{}

    DescID		attrid;
    MultiID		ioobjkey;
    int			outputnr; // For NLA or attribute nr in 2D

};


/*!\brief supplies lists of available attribute input */

class SelInfo
{
public:

			SelInfo(const DescSet*,const NLAModel* n=0,
				bool is2d=false,
				const DescID& ignoreid=DescID::undef());
			SelInfo(const SelInfo&);
    SelInfo&		operator=(const SelInfo&);

    BufferStringSet	ioobjnms;
    BufferStringSet	ioobjids;
    BufferStringSet	attrnms;
    TypeSet<DescID>	attrids;
    BufferStringSet	nlaoutnms;

    static bool		is2D(const char* defstr_or_ioobjid);
    static void		getAttrNames(const char* defstr_or_,BufferStringSet&);
    			//!< 2D only
    void		fillStored(const char* filter=0);
    static void		getSpecialItems(const char* key,BufferStringSet&);
    			//!< Filters on given DepthDomain key

protected:

    bool		is2d_;

};

}; // namespace Attrib

#endif

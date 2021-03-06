#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "convert.h"


/*!\brief single integer ID with comparison but no automatic conversion.
  Note that you will want to make a subclass to make the IDs specific for your
  class. Therefore, use the mDefIntegerID macro.

  typedef IntegerID<int> ID;

  void dummy( ID id )
  {
      id.setI( 666 ); // fine
      id = 666; // nope
      id += 666; // nope (good thing)
      id.setI( id.getI() + 666 ); // fine (weird, but you wanted it)
  }

  void dummy2()
  {
      dummy( ID::get(555) ); // fine
      dummy( 555 ); // nope
  }

  class X { X(); ID id_; };

  X::X() : id_(ID::get(444)) {} // fine
  X::X() : id_(444) {} // nope
  X::X() {} // nope

*/


template <class IntType>
mClass(Basic) IntegerID
{
public:

    typedef IntType	IDType;

    inline		IntegerID() : nr_(-1)	{}
    inline explicit	IntegerID( IntType nr )
			: nr_(nr)		{}
    inline explicit	operator IntType() const{ return nr_; }
    static inline IntegerID get( IntType i )	{ return IntegerID(i); }
			mImplSimpleEqOpers1Memb(IntegerID,nr_)
				// Do not add '>' or similar!

    inline IntType&	getI()			{ return nr_; }
    inline IntType	getI() const		{ return nr_; }
    inline void		setI( IntType i )	{ nr_ = i; }
    inline void		setI( const char* s )	{ nr_ = Conv::to<IntType>(s); }

    inline bool		isInvalid() const	{ return nr_ < 0; }
    inline bool		isValid() const		{ return !isInvalid(); }
    inline void		setInvalid()		{ nr_ = -1; }
    static inline IntegerID getInvalid()	{ return IntegerID(-1); }

protected:

    IntType		nr_;

};


#define mDefIntegerIDType(IntType,classname) \
 \
class classname : public IntegerID<IntType> \
{ \
public: \
 \
    inline		classname()			{} \
    inline explicit	classname( IntType i ) \
			    : IntegerID<IntType>(i)	{} \
    inline explicit	operator IntType() const	{ return this->nr_; } \
 \
    static inline classname get( IntType i ) \
					{ return classname(i); } \
 \
    inline bool		operator ==( const classname& oth ) const \
			{ return IntegerID<IntType>::operator ==(oth); } \
    inline bool		operator !=( const classname& oth ) const \
			{ return IntegerID<IntType>::operator !=(oth); } \
 \
    static inline classname getInvalid() { return classname(-1); } \
 \
}

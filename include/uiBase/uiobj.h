#ifndef uiobj_h
#define uiobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"
#include "uigeom.h"
#include "uilayout.h"
#include "uistring.h"
#include "color.h"

mFDQtclass(QString)
class MouseCursor;
class uiFont;
class uiObjectBody;
class uiParent;
class uiMainWin;
class uiPixmap;
class uiEventFilter;


/*!
\brief The base class for most UI elements.
*/

mExpClass(uiBase) uiObject : public uiBaseObject
{
public:
			uiObject(uiParent*,const char* nm);
			~uiObject();

/*! \brief How should the object's size behave?
    Undef       : use default.
    Small       : 1 base sz.
    Medium      : 2* base sz + 1.
    Wide        : 4* base sz + 3.
    The xxVar options specify that the element may have a bigger internal
    preferred size. In that case, the maximum is taken.
    The xxMax options specify that the element should take all available
    space ( stretch = 2 )
*/
    enum		SzPolicy{ Undef, Small, Medium, Wide,
				  SmallVar, MedVar, WideVar,
				  SmallMax, MedMax, WideMax };

    void		setHSzPol(SzPolicy);
    void		setVSzPol(SzPolicy);
    SzPolicy		szPol( bool hor=true) const;

    virtual int		width() const;	//!< Actual size in pixels
    virtual int		height() const;	//!< Actual size in pixels

    virtual void	setName(const char*);

    void		setToolTip(const uiString&);
    const uiString&	toolTip() const;
    static void		updateToolTips();

    void		translateText();

    void		display(bool yn,bool shrink=false,bool maximized=false);


    virtual void	setCursor(const MouseCursor&);
    bool		isCursorInside() const;

    virtual Color	backgroundColor() const;
    Color		roBackgroundColor() const;
    virtual void	setBackgroundColor(const Color&);
    virtual void	setBackgroundPixmap(const uiPixmap&);
    virtual void	setTextColor(const Color&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;
    bool		visible() const;
    bool		isDisplayed() const;

    int			prefHNrPics() const;
    virtual void	setPrefWidth(int);
    void		setPrefWidthInChar(int);
    void		setPrefWidthInChar(float);
    void		setMinimumWidth(int);
    void		setMaximumWidth(int);
    int			prefVNrPics() const;
    virtual void	setPrefHeight(int);
    void		setPrefHeightInChar(int);
    void		setPrefHeightInChar(float);
    void		setMinimumHeight(int);
    void		setMaximumHeight(int);

/*! \brief Sets stretch factors for object
    If stretch factor is > 1, then object will already grow at pop-up.
*/
    void		setStretch(int hor,int ver);


/*! \brief attaches object to another
    In case the stretched... options are used, margin=-1 (default) stretches
    the object not to cross the border.
    margin=-2 stretches the object to fill the parent's border. This looks nice
    with separators.
*/

    void		attach(constraintType,uiObject*);

    static void		setTabOrder(uiObject* first, uiObject* second);

    void		setFont(const uiFont&);
    const uiFont*	font() const;



    void		shallowRedraw(CallBacker* =0)	{ reDraw( false ); }
    void		deepRedraw(CallBacker* =0)	{ reDraw( true ); }
    void		reDraw(bool);

    uiSize		actualsize(bool include_border=true) const;

    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const
			    { return const_cast<uiObject*>(this)->parent(); }
    void		reParent(uiParent*);

    uiMainWin*		mainwin();

    virtual bool	handleLongTabletPress();

    Notifier<uiObject>	closed;
			//!< Triggered when object closes.
    void		close();


			/*! \brief triggered when getting a new geometry
			    A reference to the new geometry is passed
			    which *can* be manipulated, before the
			    geometry is actually set to the QWidget.
			*/
    CNotifier<uiObject,uiRect&>	setGeometry;

    static int		baseFldSize();
    static int		iconSize();

protected:
    static mQtclass(QWidget*)	getParentWidget(uiParent* p);
    
			//! hook. Accepts/denies closing of window.
    virtual bool	closeOK()	{ closed.trigger(); return true; }

    void		updateToolTip(CallBacker* = 0);

    uiString		tooltip_;

private:

    uiParent*		parent_;
};


mExpClass(uiBase) uiSingleWidgetObject : public uiObject
{
protected:
			uiSingleWidgetObject(uiParent*,const char* nm,
                                             mQtclass(QWidget)* = 0 );
    void		setSingleWidget(mQtclass(QWidget)*);
    
public:
    
    			~uiSingleWidgetObject();
    int			getNrWidgets() const;
    mQtclass(QWidget*)	getWidget(int);
    
    void		setFocus();
    bool		hasFocus() const;
    void		disabFocus();
    
    void		setCaption(const uiString&);
    
    virtual void	setStyleSheet(const char*);
    
    uiEventFilter*	eventFilter();
    
private:
    
    void		longTabletPressCB(CallBacker*);
    void		popupVirtualKeyboardCB(CallBacker*);
    void		enableVirtualKeyboard();
    
    mQtclass(QWidget*)	singlewidget_;
    uiEventFilter*	eventfilter_;

};

#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)


#define mUsrEvGuiThread	   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 0 )
#define mUsrEvPopUpReady   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 1 )
#define mUsrEvLongTabletPress  mQtclass(QEvent)::Type(mQtclass(QEvent)::User+2)

#endif

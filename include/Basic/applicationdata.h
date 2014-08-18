#ifndef applicationdata_h
#define applicationdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Aug 2014
 RCS:		$Id$
________________________________________________________________________

Wrapper class around QCoreApplication

-*/

#include "basicmod.h"

#include "callback.h"
#include "thread.h"

mFDQtclass(QCoreApplication);
class QEventLoopReceiver;

/*!Wrapper class around the QCoreApplicaiton, which provides an event-loop
   for console applications. It can also be used in GUI program as an
   access-point (adding events and telling it to quit) to QApplication (which
   has to be created before creating the ApplicationData.
*/

mExpClass(Basic) ApplicationData : public CallBacker
{
public:
			ApplicationData();
			/*!<Will create a QCoreApplication if no
			    QCoreApplication (or inheriting classes
			    are instantiated. */
			~ApplicationData();

    static int		exec();
			//!<Starts the event loop
			
    void		addToEventLoop(const CallBack& cb);
			/*!<Trigger a callback from inside the event-loop.
			    Callback will only be called once, and has to
			    remain in memory untill it is called. */

    static void		exit(int returncode);
			//!<Tells the eventloop to quit.


    static void		setOrganizationName(const char*);
    static void		setOrganizationDomain(const char*);
    static void		setApplicationName(const char*);

protected:

    mQtclass(QCoreApplication)*		application_;

    CallBackSet				eventloopqueue_;
    Threads::Mutex			eventloopqueuelock_;

    friend class QEventLoopReceiver;
    QEventLoopReceiver*			eventloopreceiver_;
};


#endif


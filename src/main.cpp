/*
 *
 *  Copyright (c) 2013
 *  name : Francis Banyikwa
 *  email: mhogomchungu@gmail.com
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tray_application_type.h"

#include "version.h"

#if USE_KDE_STATUS_NOTIFIER

#include <QApplication>
#include "qtUpdateNotifier.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kuniqueapplication.h>

#include <unistd.h>

int autoStart( KUniqueApplication& a )
{
	if( qtUpdateNotifier::autoStartEnabled() ){
		qtUpdateNotifier w ;
		w.setDebug( KCmdLineArgs::allArguments().contains( "-d" ) ) ;
		w.start() ;
		return a.exec() ;
	}else{
		return 0 ;
	}
}

int start( KUniqueApplication& a )
{
	qtUpdateNotifier w ;
	w.setDebug( KCmdLineArgs::allArguments().contains( "-d" ) ) ;
	w.start() ;
	return a.exec() ;
}

int main( int argc,char * argv[] )
{
	KAboutData aboutData( 	"qt-update-notifier",
				0,
				ki18n( "qt-update-notifier" ),
				VERSION,
				ki18n( "a qt based apt-get package updates checker." ),
				KAboutData::License_GPL_V2,
				ki18n( "(c)2013,ink Francis\nemail:mhogomchungu@gmail.com" ),
				ki18n( "mhogomchungu@gmail.com" ),
				"http://www.pclinuxos.com/forum/index.php/topic,112999.0/topicseen.html",
				"http://www.pclinuxos.com/forum/index.php/topic,112999.0/topicseen.html" ) ;

	KCmdLineArgs::init( argc,argv,&aboutData ) ;

	KCmdLineOptions options;
	options.add( "a",ki18n( "auto start application" ) ) ;
	options.add( "d",ki18n( "debug application" ) ) ;
	KCmdLineArgs::addCmdLineOptions( options ) ;

	KUniqueApplication::addCmdLineOptions() ;

	settings::init() ;

	if( KUniqueApplication::start() ){
		KUniqueApplication a ;
		if( KCmdLineArgs::allArguments().contains( "-a" ) ){
			return autoStart( a ) ;
		}else{
			return start( a ) ;
		}
	}else{
		return qtUpdateNotifier::instanceAlreadyRunning() ;
	}
}

#else
#include <QApplication>
#include "qtUpdateNotifier.h"
#include <iostream>

int main( int argc,char * argv[] )
{
	QApplication a( argc,argv ) ;
	QStringList v = QCoreApplication::arguments() ;
	settings::init() ;
	if( v.contains( "-a" ) ){
		if( qtUpdateNotifier::autoStartEnabled() ){
			qtUpdateNotifier w ;
			w.setDebug( v.contains( "-d" ) ) ;
			w.start() ;
			return a.exec() ;
		}else{
			return 0 ;
		}
	}else if( v.contains( "-h" ) || v.contains( "-v" ) ||
		  v.contains( "--help" ) || v.contains( "--version" ) ||
		  v.contains( "-version" ) || v.contains( "-help" ) ){
		const char * msg = "\
copyright: 2013-2014 Ink Francis,mhogomchungu@gmail.com\n\
license  : GPLv2+" ;
		std::cout << "version  : " << VERSION << "\n" << msg << std::endl ;
		return 0 ;
	}else{
		qtUpdateNotifier w ;
		w.setDebug( v.contains( "-d" ) ) ;
		w.start() ;
		return a.exec() ;
	}
}

#endif

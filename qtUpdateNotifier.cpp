/*
 *
 *  Copyright (c) 2013
 *  name : mhogo mchungu
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

#include "qtUpdateNotifier.h"
#include <cmath>

qtUpdateNotifier::qtUpdateNotifier() :KStatusNotifierItem( 0 )
{
	m_timer = new QTimer() ;
	connect( m_timer,SIGNAL( timeout() ),this,SLOT( automaticCheckForUpdates() ) ) ;
	m_trayMenu = 0 ;
	m_threadIsRunning = false ;
	this->setStatus( KStatusNotifierItem::Passive );
	this->setCategory( KStatusNotifierItem::ApplicationStatus );
	this->changeIcon( QString( "qt-update-notifier" ) );
	this->createEnvironment();
	this->logActivity( QString( "qt-update-notifier started" ) ) ;
	QCoreApplication::setApplicationName( QString( "qt-update-notifier" ) ) ;
	this->setObjectName( "qtUpdateNotifier" );
}

void qtUpdateNotifier::logWindowShow()
{
	logWindow * w = new logWindow( QString( "update output log window" ) ) ;
	connect( this,SIGNAL( updateLogWindow() ),w,SLOT( updateLogWindow() ) );
	w->showLogWindow( m_configLog );
}

void qtUpdateNotifier::aptGetLogWindow()
{
	logWindow * w = new logWindow( QString( "apt-get upgrade output log window" ) )  ;
	connect( this,SIGNAL( updateLogWindow() ),w,SLOT( updateLogWindow_1() ) );
	w->showAptGetWindow( m_aptGetConfigLog );
}

void qtUpdateNotifier::createEnvironment()
{
	KStandardDirs k ;
	m_configPath = k.localxdgconfdir() + QString( "/qt-update-notifier" ) ;

	m_aptGetConfigLog = k.localxdgconfdir() + QString( "/qt-update-notifier/aptGetOutPut.log" ) ;

	QDir d ;
	d.mkpath( m_configPath ) ;

	m_configTime = m_configPath + QString( "/qt-update-notifier.time" ) ;
	m_configLog = m_configPath  + QString( "/qt-update-notifier.log" ) ;

	m_CheckDelayOnStartUp = m_configPath + QString( "/qt-update-notifier-firstCheck.time" ) ;

	QFile w( m_CheckDelayOnStartUp ) ;

	if( !w.exists() ){
		w.open( QIODevice::WriteOnly ) ;
		w.write( "300" ) ; // wait for 5 minutes before check for updates on startup
		w.close();
	}

	w.open( QIODevice::ReadOnly ) ;
	QString wd = w.readAll() ;
	w.close();
	m_waitForFirstCheck = 1000 * wd.toInt() ;

	int rr = 60 * 1000 ;

	char buffer[64] ;

	if( fmod( m_waitForFirstCheck,rr ) == 0 ){
		int ff = m_waitForFirstCheck / rr ;
		snprintf( buffer,64,"%d",ff ) ;
	}else{
		float ff =  static_cast<float>( m_waitForFirstCheck ) / rr ;
		snprintf( buffer,64,"%.2f",ff ) ;
	}

	QString z = QString( "waiting for %1 minutes before checking for updates" ).arg( QString( buffer ) ) ;
	QString a = QString( "qt-update-notifier" ) ;
	QString b = QString( "status" ) ;

	this->showToolTip( a,b,z ) ;

	m_updateCheckInterval = m_configPath + QString( "/qt-update-notifier.interval" ) ;

	QFile f( m_updateCheckInterval ) ;
	if( !f.exists() ){
		f.open( QIODevice::WriteOnly ) ;
		f.write( "86400" ) ; // wait for 24 hours before checking for updates
		f.close();
	}

	f.open( QIODevice::ReadOnly ) ;
	QString x = f.readAll() ;
	f.close();
	m_sleepDuration = 1000 * x.toInt() ;
}

void qtUpdateNotifier::start()
{
	QMetaObject::invokeMethod( this,"run",Qt::QueuedConnection ) ;
}

void qtUpdateNotifier::changeIcon( QString icon )
{
	this->setIconByName( icon );
	this->setAttentionIconByName( icon ) ;
}

void qtUpdateNotifier::startUpdater()
{
	startSynaptic * s = new startSynaptic() ;
	connect( s,SIGNAL( destroyed() ),this,SLOT( updaterClosed() ) ) ;
	s->start();
}

void qtUpdateNotifier::updaterClosed()
{
	this->doneUpdating() ;
}

void qtUpdateNotifier::doneUpdating()
{
	QString y = QString( "qt-update-notifier" ) ;
	QString z = QString( "status" ) ;
	this->showToolTip( y,z ) ;

	this->changeIcon( QString( "qt-update-notifier" ) );
	this->setStatus( KStatusNotifierItem::Passive );
}

bool qtUpdateNotifier::autoStartEnabled()
{
	KStandardDirs k ;
	QString configPath = k.localxdgconfdir() + QString( "/qt-update-notifier/doNotAutoStart" ) ;
	QFile f( configPath ) ;
	return !f.exists() ;
}

void qtUpdateNotifier::enableAutoStart()
{
	KStandardDirs k ;
	QString autoStart = k.localxdgconfdir() + QString( "/qt-update-notifier/doNotAutoStart" ) ;
	QFile f( autoStart ) ;
	f.remove() ;
}

void qtUpdateNotifier::disableAutoStart()
{
	KStandardDirs k ;
	QString configPath = k.localxdgconfdir() + QString( "/qt-update-notifier/doNotAutoStart" ) ;
	QFile f( configPath ) ;
	f.open( QIODevice::WriteOnly ) ;
	f.close();
}

void qtUpdateNotifier::toggleAutoStart( bool b )
{
	if( b ){
		this->enableAutoStart();
	}else{
		this->disableAutoStart();
	}
}

void qtUpdateNotifier::openConfigureDialog()
{
	QStringList l ;
	l.append( m_CheckDelayOnStartUp ) ;
	l.append( m_updateCheckInterval );
	configureDialog * cfg = new configureDialog( l,qtUpdateNotifier::autoStartEnabled() ) ;
	connect( cfg,SIGNAL( toggleAutoStart( bool ) ),this,SLOT( toggleAutoStart( bool ) ) ) ;
	connect( cfg,SIGNAL( setUpdateInterval( int ) ),this,SLOT( setUpdateInterval( int ) ) ) ;
	cfg->showUI();
}

void qtUpdateNotifier::run()
{
	instance * s = new instance( this ) ;

	if( !s->firstInstance() ){
		qDebug() << "another instance is already running,closing this one" ;
		this->closeApp();
	}

	m_trayMenu = new KMenu() ;

	m_trayMenu->addAction( tr( "check for updates" ),this,SLOT( manualCheckForUpdates() ) );
	m_trayMenu->addAction( tr( "done updating" ),this,SLOT( doneUpdating() ) );
	m_trayMenu->addAction( tr( "open synaptic" ),this,SLOT( startUpdater() ) );
	m_trayMenu->addAction( tr( "open update log window" ),this,SLOT( logWindowShow() ) );
	m_trayMenu->addAction( tr( "open apt-get log window" ),this,SLOT( aptGetLogWindow() ) );
	m_trayMenu->addAction( tr( "configuration window" ),this,SLOT( openConfigureDialog() ) );

	this->setContextMenu( m_trayMenu );
	this->contextMenu()->setEnabled( true );

	QTimer * t = new QTimer() ;
	t->setSingleShot( true ) ;
	connect( t,SIGNAL( timeout() ),this,SLOT( checkForUpdatesOnStartUp() ) ) ;
	connect( t,SIGNAL( timeout() ),t,SLOT( deleteLater() ) ) ;
	t->start( m_waitForFirstCheck ) ;
}

void qtUpdateNotifier::checkForUpdatesOnStartUp()
{
	QFile f( m_configTime ) ;
	if( !f.open( QIODevice::ReadOnly ) ){
		/*
		 * config file doesnt seem to be present,ignore it,it will be created later on
		 */
		this->checkForUpdates();
	}else{
		m_currentTime = this->getCurrentTime() ;
		u_int64_t configTime = this->nextScheduledUpdateTime() ;
		u_int64_t interval = m_currentTime - configTime ;
		if( interval >= m_sleepDuration ){
			/*
			 * the wait interval has passed,check for updates now
			 */
			this->checkForUpdates();
		}else{
			/*
			 * the wait interval has not passed,wait for the remainder of the interval before
			 * checking for updates
			 */
			int x = m_sleepDuration - interval ;
			this->scheduleUpdates( x );
			QString y = QString( "qt-update-notifier" ) ;
			QString z = QString( "status" ) ;
			this->showToolTip( y,z,x ) ;
		}
	}
}

u_int64_t qtUpdateNotifier::getCurrentTime()
{
	return static_cast<u_int64_t>( QDateTime::currentDateTime().toMSecsSinceEpoch() );
}

void qtUpdateNotifier::logActivity( QString msg )
{
	QFile f( m_configLog ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Append ) ;
	QDateTime t = QDateTime::currentDateTime() ;
	QString time = QString( "%1:   %2\n").arg( t.toString( Qt::TextDate ) ).arg( msg )  ;
	QByteArray r = time.toAscii() ;
	f.write( r ) ;
	f.close();
}

u_int64_t qtUpdateNotifier::nextScheduledUpdateTime()
{
	QFile f( m_configTime ) ;
	if( f.open( QIODevice::ReadOnly ) ){
		QString x = f.readAll() ;
		f.close();
		return x.toULongLong() ;
	}else{
		return 0 ;
	}
}

void qtUpdateNotifier::writeUpdateTimeToConfigFile()
{
	QFile f( m_configTime ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	QString z = QString::number( this->getCurrentTime() ) ;
	f.write( z.toAscii() ) ;
	f.close();
}

void qtUpdateNotifier::manualCheckForUpdates()
{
	this->logActivity( QString( "manual check for updates initiated" ) ) ;
	this->checkForUpdates();
}

void qtUpdateNotifier::automaticCheckForUpdates()
{
	this->logActivity( QString( "automatic check for updates initiated" ) ) ;
	this->checkForUpdates();
}

void qtUpdateNotifier::checkForUpdates()
{
	if( m_threadIsRunning ){
		this->logActivity( QString( "warning:\tattempt to start a check while another is already in progress" ) ) ;
		return ;
	}

	QString icon = QString( "qt-update-notifier-updating" ) ;

	this->changeIcon( icon );

	this->showToolTip( icon,QString( "status" ),QString( "checking for updates" ) );

	m_threadIsRunning = true ;
	m_updates = new check_updates( m_configPath ) ;

	connect( m_updates,SIGNAL( updateStatus( int,QStringList ) ),this,SLOT( updateStatus( int,QStringList ) ) ) ;
	connect( m_updates,SIGNAL( terminated() ),this,SLOT( threadTerminated() ) ) ;
	connect( m_updates,SIGNAL( finished() ),this,SLOT( threadisFinished() ) ) ;
	connect( m_updates,SIGNAL( finished() ),m_updates,SLOT( deleteLater() ) ) ;

	this->contextMenu()->setEnabled( false );
	m_updates->start();
	emit updateLogWindow() ;
}

void qtUpdateNotifier::saveAptGetLogOutPut( QStringList log )
{
	int j = log.size() ;
	if( j == 0 ){
		/*
		 * log appear to be empty,dont replace a log file with useful info with an empty one
		 */
		return ;
	}

	QFile f( m_aptGetConfigLog ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;

	QString line = QString( "-------------------------------------------------------------------------------\n" ) ;
	QString msg = QString( "log entry was created at: " ) ;
	QString header = line + msg + QDateTime::currentDateTime().toString( Qt::TextDate ) + QString( "\n" ) + line ;

	f.write( header.toAscii() ) ;

	QByteArray nl( "\n" ) ;
	for( int i = 0 ; i < j ; i++ ){
		f.write( log.at( i ).toAscii() + nl ) ;
	}
	f.close();
}

void qtUpdateNotifier::updateStatus( int st,QStringList list )
{
	m_threadIsRunning = false ;
	this->contextMenu()->setEnabled( true );
	QString icon ;

	this->saveAptGetLogOutPut( list ) ;

	if( st == UPDATES_FOUND ){
		icon = QString( "qt-update-notifier-updates-are-available" ) ;
		this->changeIcon( icon ) ;
		this->setStatus( KStatusNotifierItem::NeedsAttention );
		this->logActivity( QString( "update check complete,UPDATES FOUND" ) ) ;
		this->showToolTip( icon,QString( "there are updates in the repository" ),list );
	}else if( st == INCONSISTENT_STATE ){
		icon = QString( "qt-update-notifier" ) ;
		this->changeIcon( icon );
		this->setStatus( KStatusNotifierItem::Passive );
		this->logActivity( QString( "update check complete,repository appear to be in an inconsistent state" ) ) ;
		this->showToolTip( icon,QString( "no updates foung" ) );
	}else if( st == NO_UPDATES_FOUND ){
		icon = QString( "qt-update-notifier" ) ;
		this->changeIcon( icon );
		this->setStatus( KStatusNotifierItem::Passive );
		this->logActivity( QString( "update check complete,no updates found" ) ) ;
		this->showToolTip( icon,QString( "no updates foung" ) );
	}else if( st == NO_NET_CONNECTION ){
		icon = QString( "qt-update-notifier" ) ;
		this->changeIcon( icon );
		this->setStatus( KStatusNotifierItem::Passive );
		this->logActivity( QString( "check skipped,user is not connected to the internet" ) ) ;
		this->showToolTip( icon,QString( "no updates foung" ) );
	}else{
		/*
		 * currently,we dont get here,added for completeness' sake
		 */
		icon = QString( "qt-update-notifier" ) ;
		this->changeIcon( icon );
		this->setStatus( KStatusNotifierItem::Passive );
		this->logActivity( QString( "update check complete,repository is in an unknown state" ) ) ;
		this->showToolTip( icon,QString( "no updates foung" ) );
	}

	emit updateLogWindow();
}

void qtUpdateNotifier::showToolTip( QString x,QString y,QStringList list )
{
	Q_UNUSED( y ) ;
	this->setToolTip( x,QString( "updates found" ),list.at( 0 ) );
}

void qtUpdateNotifier::showToolTip( QString x,QString y,QString z )
{
	this->setToolTip( x,y,z );
}

void qtUpdateNotifier::showToolTip( QString x,QString y,int z )
{
	QString n = QString( "next update check will be at %1" ).arg( this->nextUpdateTime( z ) ) ;
	this->setToolTip( x,y,n );
}

void qtUpdateNotifier::showToolTip( QString x,QString y )
{
	QDateTime d ;
	d.setMSecsSinceEpoch( this->nextScheduledUpdateTime() );
	QString n = QString( "next update check will be at %1" ).arg( d.toString( Qt::TextDate ) ) ;
	this->setToolTip( x,y,n );
}

QString qtUpdateNotifier::nextUpdateTime( void )
{
	return this->nextUpdateTime( m_sleepDuration ) ;
}

QString qtUpdateNotifier::nextUpdateTime( int interval )
{
	QDateTime d ;
	d.setMSecsSinceEpoch( QDateTime::currentMSecsSinceEpoch() + interval ) ;
	return d.toString( Qt::TextDate ) ;
}

QString qtUpdateNotifier::logMsg( int interval )
{
	char num[ 64 ] ;
	float f = static_cast<float>( interval ) ;
	snprintf( num,64,"%.2f",f / ( 1000 * 60 * 60 ) ) ;
	QString n = this->nextUpdateTime( interval ) ;
	return QString( "scheduled next check to be in %1 hours at %2" ).arg( QString( num ) ).arg( n ) ;
}

void qtUpdateNotifier::scheduleUpdates( int interval )
{
	if( interval >= 10 * 60 * 1000 ){
		this->logActivity( this->logMsg( interval ) ) ;
		m_timer->stop();
		m_timer->start( interval );
	}else{
		this->logActivity( QString( "schedules check interval is less that 10 minutes,reseting it to 10 minutes" ) ) ;
		m_timer->stop();
		m_timer->start( 10 * 60 * 1000 );
	}
}

void qtUpdateNotifier::setUpdateInterval( int interval )
{
	this->logActivity( QString( "rescheduling update check" ) ) ;

	m_sleepDuration = interval;

	if( m_sleepDuration < 10 * 60 * 1000 ){
		this->logActivity( QString( "update interval is less than 10 minutes,resetting it to 10 minutes" ) ) ;
		m_sleepDuration = 10 * 60 * 1000 ;
	}

	this->logActivity( this->logMsg( m_sleepDuration ) ) ;

	this->writeUpdateTimeToConfigFile();

	m_timer->stop();
	m_timer->start( m_sleepDuration );

	QString x = this->iconName() ;
	QString y = this->toolTipTitle();

	int d = static_cast<int>( m_sleepDuration ) ;
	this->showToolTip( x,y,d ) ;
}

void qtUpdateNotifier::threadTerminated( void )
{
	QCoreApplication::exit( 0 ) ;
}

void qtUpdateNotifier::threadisFinished()
{
	m_threadIsRunning = false ;
	this->contextMenu()->setEnabled( true );
}

void qtUpdateNotifier::closeApp()
{
	if( m_threadIsRunning ){
		m_updates->terminate();
	}else{
		QCoreApplication::exit( 0 ) ;
	}
}

void qtUpdateNotifier::closeApp( int st )
{
	if( m_threadIsRunning ){
		m_updates->terminate();
	}else{
		QCoreApplication::exit( st ) ;
	}
}

qtUpdateNotifier::~qtUpdateNotifier()
{
	if( m_trayMenu ){
		m_trayMenu->deleteLater();
	}
	if( m_timer ){
		m_timer->stop();
		m_timer->deleteLater();
	}
	this->logActivity( QString( "qt-update-notifier quitting" ) ) ;
}

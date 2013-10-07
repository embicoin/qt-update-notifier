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
	QCoreApplication::setApplicationName( QString( "qt-update-notfier" ) ) ;
	KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
	KStatusNotifierItem::setCategory( KStatusNotifierItem::ApplicationStatus ) ;
	this->changeIcon( QString( "qt-update-notifier" ) ) ;
	this->createEnvironment() ;
}

void qtUpdateNotifier::logWindowShow()
{
	logWindow * w = new logWindow( tr( "Update output log window" ) ) ;
	connect( this,SIGNAL( updateLogWindow() ),w,SLOT( updateLogWindow() ) ) ;
	connect( this,SIGNAL( configOptionsChanged_1() ),w,SLOT( updateLogWindow() ) ) ;
	w->showLogWindow( m_configLog ) ;
}

void qtUpdateNotifier::aptGetLogWindow()
{
	logWindow * w = new logWindow( tr( "Apt-get upgrade output log window" ) )  ;
	connect( this,SIGNAL( updateLogWindow() ),w,SLOT( updateLogWindow_1() ) ) ;
	connect( this,SIGNAL( configOptionsChanged_1() ),w,SLOT( updateLogWindow_1() ) ) ;
	w->showAptGetWindow( m_aptGetConfigLog ) ;
}

void qtUpdateNotifier::createEnvironment()
{
	/*
	 * delete ~/.config/autostart/qt-update-notifier.desktop if present as its no longer necessary
	 */
	QFile::remove( QDir::homePath() + QString( "/.config/autostart/qt-update-notifier.desktop" ) ) ;

	KStandardDirs k ;
	m_configPath = k.localxdgconfdir() + QString( "/qt-update-notifier" ) ;

	m_aptGetConfigLog = k.localxdgconfdir() + QString( "/qt-update-notifier/qt-update-notifier-apt_output.log" ) ;

	QFile e( m_configPath + QString( "/language.option"  ) ) ;

	if( !e.exists() ){
		e.open( QIODevice::WriteOnly ) ;
		e.write( "english_US" ) ;
		e.close() ;
	}

	e.open( QIODevice::ReadOnly ) ;
	m_prefferedLanguage = e.readAll() ;
	e.close() ;

	this->setupTranslationText() ;

	QCoreApplication::setApplicationName( tr( "Qt-update-notifier" ) ) ;
	this->setObjectName( "qtUpdateNotifier" ) ;

	QDir d ;
	d.mkpath( m_configPath ) ;

	m_configTime = m_configPath + QString( "/qt-update-notifier-next_auto_update.time" ) ;
	m_configLog = m_configPath  + QString( "/qt-update-notifier-activity.log" ) ;

	m_CheckDelayOnStartUp = m_configPath + QString( "/qt-update-notifier-startup_check_delay.time" ) ;

	QFile w( m_CheckDelayOnStartUp ) ;

	if( !w.exists() ){
		w.open( QIODevice::WriteOnly ) ;
		w.write( "300" ) ; // wait for 5 minutes before check for updates on startup
		w.close() ;
	}

	w.open( QIODevice::ReadOnly ) ;
	QString wd = w.readAll() ;
	w.close() ;
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

	QString z = tr( "Waiting for %1 minutes before checking for updates" ).arg( QString( buffer ) ) ;
	QString a = QString( "qt-update-notifier" ) ;
	QString b = tr( "Status" ) ;

	this->showToolTip( a,b,z ) ;

	m_updateCheckInterval = m_configPath + QString( "/qt-update-notifier-interval.time" ) ;

	QFile f( m_updateCheckInterval ) ;
	if( !f.exists() ){
		f.open( QIODevice::WriteOnly ) ;
		f.write( "86400" ) ; // wait for 24 hours before checking for updates
		f.close() ;
	}

	f.open( QIODevice::ReadOnly ) ;
	QString x = f.readAll() ;
	f.close() ;
	m_sleepDuration = 1000 * x.toInt() ;
}

void qtUpdateNotifier::start()
{
	QMetaObject::invokeMethod( this,"run",Qt::QueuedConnection ) ;
}

void qtUpdateNotifier::changeIcon( QString icon )
{
	KStatusNotifierItem::setIconByName( icon ) ;
	KStatusNotifierItem::setAttentionIconByName( icon ) ;
}

void qtUpdateNotifier::startUpdater()
{
	startSynaptic * s = new startSynaptic() ;
	connect( s,SIGNAL( destroyed() ),this,SLOT( updaterClosed() ) ) ;

	if( this->autoRefreshSYnaptic() ){
		s->start( QString( "--start-synaptic --update-at-startup" ) ) ;
	}else{
		s->start( QString( "--start-synaptic" ) ) ;
	}
}

void qtUpdateNotifier::updaterClosed()
{
	this->doneUpdating() ;
}

void qtUpdateNotifier::doneUpdating()
{
	QDateTime d ;
	d.setMSecsSinceEpoch( this->nextScheduledUpdateTime() ) ;

	QString n = tr( "Next update check will be at %1" ).arg( d.toString( Qt::TextDate ) ) ;

	QString y = QString( "qt-update-notifier" ) ;
	QString z = tr( "Status" ) ;
	this->showToolTip( y,z,n ) ;
	KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
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
	f.close() ;
}

void qtUpdateNotifier::toggleAutoStart( bool b )
{
	if( b ){
		this->enableAutoStart() ;
	}else{
		this->disableAutoStart() ;
	}
}

void qtUpdateNotifier::openConfigureDialog()
{
	QStringList l ;
	l.append( m_CheckDelayOnStartUp ) ;
	l.append( m_updateCheckInterval ) ;
	configureDialog * cfg = new configureDialog( l,qtUpdateNotifier::autoStartEnabled(),this->autoRefreshSYnaptic() ) ;
	connect( cfg,SIGNAL( toggleAutoStart( bool ) ),this,SLOT( toggleAutoStart( bool ) ) ) ;
	connect( cfg,SIGNAL( setUpdateInterval( int ) ),this,SLOT( setUpdateInterval( int ) ) ) ;
	connect( cfg,SIGNAL( configOptionsChanged() ),this,SLOT( configOptionsChanged() ) ) ;
	connect( cfg,SIGNAL( localizationLanguage( QString ) ),this,SLOT( localizationLanguage( QString ) ) ) ;
	connect( cfg,SIGNAL( autoReshreshSynaptic( bool ) ),this,SLOT( autoRefreshSynaptic( bool ) ) ) ;
	cfg->showUI( m_prefferedLanguage ) ;
}

void qtUpdateNotifier::autoRefreshSynaptic( bool b )
{
	QString x = m_configPath + QString( "/qt-update-notifier-synaptic_autorefresh.option" ) ;
	if( b ){
		QFile f( x ) ;
		f.open( QIODevice::WriteOnly ) ;
		f.close() ;
	}else{
		QFile::remove( x ) ;
	}
}

bool qtUpdateNotifier::autoRefreshSYnaptic()
{
	QString x = m_configPath + QString( "/qt-update-notifier-synaptic_autorefresh.option" ) ;
	QFile f( x ) ;
	return f.exists() ;
}

void qtUpdateNotifier::configOptionsChanged()
{
	emit configOptionsChanged_1() ;
}

void qtUpdateNotifier::setupTranslationText()
{
	QFile f( m_configPath + QString( "/language.option" ) ) ;

	m_translator = new QTranslator( this ) ;

	f.open( QIODevice::ReadOnly ) ;
	QByteArray r = f.readAll() ;
	f.close() ;
	QByteArray e( "english_US" ) ;
	if( e == r ){
		/*
		 *english_US language,its the default and hence dont load anything
		 */
	}else{
		m_translator->load( r.constData(),QString( QT_UPDATE_NOTIFIER_TRANSLATION_PATH ) ) ;
		QCoreApplication::installTranslator( m_translator ) ;
	}
}

void qtUpdateNotifier::localizationLanguage( QString language )
{
	m_prefferedLanguage = language ;
	QFile f( m_configPath + QString( "/language.option" ) ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	f.write( language.toAscii() ) ;
	f.close() ;
	QCoreApplication::removeTranslator( m_translator ) ;
	m_translator->deleteLater() ;
	this->setupTranslationText() ;
}

int qtUpdateNotifier::instanceAlreadyRunning()
{
	KStandardDirs k ;

	QFile f( k.localxdgconfdir() + QString( "/qt-update-notifier/language.option" ) ) ;

	QTranslator * translator = new QTranslator() ;

	f.open( QIODevice::ReadOnly ) ;
	QByteArray r = f.readAll() ;
	f.close() ;
	QByteArray e( "english_US" ) ;
	if( e == r ){
		/*
		 * english_US language,its the default and hence dont load anything
		 */
		qDebug() << tr( "Another instance is already running, closing this one" ) ;
	}else{
		const char * x[ 2 ] ;
		x[ 0 ] = "qt-update-notifier" ;
		x[ 1 ] = 0 ;
		int z = 1 ;
		QCoreApplication app( z,( char ** ) x ) ;

		translator->load( r.constData(),QString( QT_UPDATE_NOTIFIER_TRANSLATION_PATH ) ) ;
		app.installTranslator( translator ) ;
		qDebug() << tr( "Another instance is already running, closing this one" ) ;
		app.removeTranslator( translator ) ;
		translator->deleteLater() ;
	}
	return 1 ;
}

void qtUpdateNotifier::run()
{
	this->logActivity( tr( "Qt-update-notifier started" ) ) ;

	m_trayMenu = new KMenu() ;

	m_trayMenu->addAction( tr( "Open synaptic" ),this,SLOT( startUpdater() ) ) ;
	m_trayMenu->addAction( tr( "Check for updates" ),this,SLOT( manualCheckForUpdates() ) ) ;
	m_trayMenu->addAction( tr( "Done updating" ),this,SLOT( doneUpdating() ) ) ;
	m_trayMenu->addAction( tr( "Open update log window" ),this,SLOT( logWindowShow() ) ) ;
	m_trayMenu->addAction( tr( "Open apt-get log window" ),this,SLOT( aptGetLogWindow() ) ) ;
	m_trayMenu->addAction( tr( "Configuration window" ),this,SLOT( openConfigureDialog() ) ) ;

	KStatusNotifierItem::setContextMenu( m_trayMenu ) ;

	QTimer * t = new QTimer() ;
	t->setSingleShot( true ) ;
	connect( t,SIGNAL( timeout() ),this,SLOT( checkForUpdatesOnStartUp() ) ) ;
	connect( t,SIGNAL( timeout() ),t,SLOT( deleteLater() ) ) ;
	t->start( m_waitForFirstCheck ) ;
}

void qtUpdateNotifier::printTime( const QString& zz,u_int64_t time )
{
	QDateTime d ;
	d.setMSecsSinceEpoch( time ) ;
	qDebug() << zz << d.toString( Qt::TextDate ) ;
}

void qtUpdateNotifier::checkForUpdatesOnStartUp()
{
	QFile f( m_configTime ) ;
	if( !f.open( QIODevice::ReadOnly ) ){

		/*
		 * create a logfile that records time for the next update.
		 */
		//printTime( "kk",this->() + m_sleepDuration ) ;

		m_timer->stop() ;
		m_timer->start( m_sleepDuration ) ;
		this->automaticCheckForUpdates() ;
	}else{
		m_currentTime = this->getCurrentTime() ;
		u_int64_t x = m_currentTime ;
		u_int64_t y = this->nextScheduledUpdateTime() ;
		u_int64_t z = m_sleepDuration ;

		int64_t interval = y - x ;

		if( interval >= 0 ){
			/*
			 * the wait interval has not passed,wait for the remainder of the interval before
			 * checking for updates
			 */
			QTimer * t = new QTimer() ;
			t->setSingleShot( true ) ;
			connect( t,SIGNAL( timeout() ),t,SLOT( deleteLater() ) ) ;
			connect( t,SIGNAL( timeout() ),this,SLOT( startTimer() ) ) ;
			t->start( interval ) ;

			this->showToolTip( QString( "qt-update-notifier" ),tr( "Status" ),interval ) ;

			this->logActivity( this->logMsg() ) ;
		}else{
			u_int64_t e = ( x - y ) / z ;
			e = e + 1 ;
			z = z * e ;

			this->writeUpdateTimeToConfigFile( y + z ) ;

			this->logActivity( tr( "Automatic check for updates initiated" ) ) ;

			this->checkForUpdates() ;

			QTimer * t = new QTimer() ;
			t->setSingleShot( true ) ;
			connect( t,SIGNAL( timeout() ),t,SLOT( deleteLater() ) ) ;
			connect( t,SIGNAL( timeout() ),this,SLOT( startTimer_1() ) ) ;
			t->start( y + z - x ) ;
		}
	}
}

void qtUpdateNotifier::startTimer()
{
	m_timer->stop() ;
	m_timer->start( m_sleepDuration ) ;
	this->automaticCheckForUpdates() ;
}

void qtUpdateNotifier::startTimer_1()
{
	m_timer->stop() ;
	m_timer->start( m_sleepDuration ) ;
	this->automaticCheckForUpdates() ;
}

u_int64_t qtUpdateNotifier::getCurrentTime()
{
	return static_cast<u_int64_t>( QDateTime::currentDateTime().toMSecsSinceEpoch() ) ;
}

QString qtUpdateNotifier::getCurrentTime_1()
{
	return QDateTime::currentDateTime().toString( Qt::TextDate ) ;
}

void qtUpdateNotifier::logActivity( const QString& msg )
{
	QString t = this->getCurrentTime_1() ;
	QString log = QString( "%1:   %2\n").arg( t ).arg( msg ) ;
	utility::writeToFile( m_configLog,log,false ) ;
	emit updateLogWindow() ;
}

void qtUpdateNotifier::logActivity_1( const QString& msg )
{
	QString line = QString( "------------------------------------------------------" ) ;
	line += QString( "----------------------------------------------------------------" ) ;
	QString t = this->getCurrentTime_1() ;
	QString log = QString( "%1\n%2:   %3\n%4\n" ).arg( line ).arg( t ).arg( msg ).arg( line )  ;
	utility::writeToFile( m_configLog,log,false ) ;
	emit updateLogWindow() ;
}

u_int64_t qtUpdateNotifier::nextScheduledUpdateTime()
{
	QFile f( m_configTime ) ;
	if( f.open( QIODevice::ReadOnly ) ){
		QString x = f.readAll() ;
		f.close() ;
		return x.toULongLong() ;
	}else{
		return 0 ;
	}
}

void qtUpdateNotifier::writeUpdateTimeToConfigFile( u_int64_t time )
{
	QFile f( m_configTime ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	QString z = QString::number( time ) ;
	f.write( z.toAscii() ) ;
	f.close() ;
}

void qtUpdateNotifier::manualCheckForUpdates()
{
	this->logActivity( tr( "Manual check for updates initiated" ) ) ;
	this->checkForUpdates() ;
}

void qtUpdateNotifier::automaticCheckForUpdates()
{
	this->logActivity( tr( "Automatic check for updates initiated" ) ) ;
	this->checkForUpdates() ;
	this->writeUpdateTimeToConfigFile( this->getCurrentTime() + m_sleepDuration ) ;
}

void qtUpdateNotifier::checkForUpdates()
{
	if( m_threadIsRunning ){
		this->logActivity( tr( "Warning:\tattempt to start update check while another one is still in progress" ) ) ;
	}else{
		QString icon = QString( "qt-update-notifier-updating" ) ;

		this->showToolTip( icon,tr( "Status" ),tr( "Checking for updates" ) ) ;

		m_threadIsRunning = true ;

		m_updates = new check_updates( m_configPath,m_prefferedLanguage ) ;
		connect( m_updates,SIGNAL( updateStatus( int,QStringList ) ),this,SLOT( updateStatus( int,QStringList ) ) ) ;
		connect( m_updates,SIGNAL( terminated() ),this,SLOT( threadTerminated() ) ) ;
		connect( m_updates,SIGNAL( finished() ),this,SLOT( threadisFinished() ) ) ;
		connect( m_updates,SIGNAL( finished() ),m_updates,SLOT( deleteLater() ) ) ;

		m_updates->start() ;
	}
}

void qtUpdateNotifier::saveAptGetLogOutPut( const QStringList& log )
{
	int j = log.size() ;
	if( j == 0 ){
		/*
		 * log appear to be empty,dont replace a log file with useful info with an empty one
		 */
		return ;
	}

	QString x = log.at( 1 ) ;

	if( x == tr( "No updates found" ) ){
		/*
		 * update log file only when there are new updates
		 */
		return ;
	}

	QString line = QString( "-------------------------------------------------------------------------------\n" ) ;
	QString msg = tr( "Log entry was created at: " ) ;
	QString header = line + msg + QDateTime::currentDateTime().toString( Qt::TextDate ) + QString( "\n" ) + line ;

	utility::writeToFile( m_aptGetConfigLog,header + x,true ) ;
}

void qtUpdateNotifier::autoUpdatePackages()
{
	KStandardDirs k ;
	QString update = k.localxdgconfdir() + QString( "/qt-update-notifier/autoUpdatePackages.option" ) ;
	if( QFile::exists( update ) ){
		QString icon = QString( "qt-update-notifier-updating" ) ;
		this->showToolTip( icon,tr( "Status" ),tr( "Updating packages" ) ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::NeedsAttention ) ;
		this->logActivity( tr( "Automatic package update initiated" ) ) ;
		startSynaptic * s = new startSynaptic() ;
		connect( s,SIGNAL( result( int ) ),this,SLOT( autoUpdateResult( int ) ) ) ;
		s->start( QString( "--auto-update" ) ) ;
	}else{
		this->logActivity( this->logMsg() ) ;
	}
}

void qtUpdateNotifier::autoUpdateResult( int r )
{
	QString icon = QString( "qt-update-notifier" ) ;
	switch( r ){
		case 0 : this->showToolTip( icon,tr( "Automatic package update completed" ) ) ;	 break ;
		case 1 : this->showToolTip( icon,tr( "Automatic package update failed" ) )    ;	 break ;
		case 2 : this->showToolTip( icon,tr( "Automatic package update completed" ) ) ;	 break ;
		case 3 : this->showToolTip( icon,tr( "Automatic package update failed" ) )    ;	 break ;
	}

	KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
}

void qtUpdateNotifier::autoDownloadPackages( int r )
{
	QString icon = QString( "qt-update-notifier-updates-are-available" ) ;

	if( r == 0 ){
		this->showToolTip( icon,tr( "Downloading of packages completed" ) ) ;
	}else{
		this->showToolTip( icon,tr( "Downloading of packages failed" ) ) ;
	}

	KStatusNotifierItem::setStatus( KStatusNotifierItem::NeedsAttention ) ;
	this->autoUpdatePackages() ;
}

void qtUpdateNotifier::autoDownloadPackages()
{
	KStandardDirs k ;
	QString update = k.localxdgconfdir() + QString( "/qt-update-notifier/autoDownloadPackages.option" ) ;
	if( QFile::exists( update ) ){
		QString icon = QString( "qt-update-notifier-updating" ) ;
		this->showToolTip( icon,tr( "Status" ),tr( "Downloading packages" ) ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::NeedsAttention ) ;
		this->logActivity( tr( "Packages downloading initiated" ) ) ;
		startSynaptic * s = new startSynaptic() ;
		connect( s,SIGNAL( result( int ) ),this,SLOT( autoDownloadPackages( int ) ) ) ;
		s->start( QString( "--download-packages" ) ) ;
	}else{
		this->autoUpdatePackages() ;
	}
}

void qtUpdateNotifier::updateStatus( int r,QStringList list )
{
	m_threadIsRunning = false ;
	this->contextMenu()->setEnabled( true ) ;
	QString icon ;

	this->saveAptGetLogOutPut( list ) ;

	switch( check_updates::updateState( r ) ){
	case check_updates::updatesFound :

		icon = QString( "qt-update-notifier-updates-are-available" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::NeedsAttention ) ;
		this->showToolTip( icon,tr( "There are updates in the repository" ),list ) ;
		this->autoDownloadPackages() ;

		break ;
	case check_updates::inconsistentState :

		icon = QString( "qt-update-notifier" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
		this->showToolTip( icon, tr( "Update check complete, repository appears to be in an inconsistent state" ) ) ;
		this->checkForPackageUpdates() ;

		break ;
	case check_updates::noUpdatesFound :

		icon = QString( "qt-update-notifier" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
		this->showToolTip( icon,tr( "No updates found" ) ) ;
		this->checkForPackageUpdates() ;

		break ;
	case check_updates::noNetworkConnection :

		icon = QString( "qt-update-notifier" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
		this->showToolTip( icon,tr( "Check skipped, user is not connected to the internet" ) ) ;
		this->checkForPackageUpdates() ;

		break ;
	case check_updates::undefinedState :

		icon = QString( "qt-update-notifier" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
		this->showToolTip( icon,tr( "Update check complete, repository is in an unknown state" ) ) ;
		this->checkForPackageUpdates() ;

		break ;
	default:
		/*
		 * currently,we dont get here,added for completeness' sake
		 */
		icon = QString( "qt-update-notifier" ) ;
		KStatusNotifierItem::setStatus( KStatusNotifierItem::Passive ) ;
		this->showToolTip( icon,tr( "Update check complete, repository is in an unknown state" ) ) ;
		this->checkForPackageUpdates() ;
	}
}

void qtUpdateNotifier::checkForPackageUpdates()
{
	KStandardDirs k ;
	QString update = k.localxdgconfdir() + QString( "/qt-update-notifier/skipOldPackageCheck.option" ) ;
	if( QFile::exists( update ) ){
		;
	}else{
		checkoldpackages * c = new checkoldpackages() ;
		connect( c,SIGNAL( outdatedPackages( QStringList ) ),this,SLOT( checkOldPackages( QStringList ) ) ) ;
		c->start() ;
	}
}

void qtUpdateNotifier::objectGone( QObject * obj )
{
	qDebug() << "destroyed object" << obj->objectName() ;
}

void qtUpdateNotifier::checkOldPackages( QStringList list )
{
	QString	icon = QString( "qt-update-notifier-important-info" ) ;
	bool updateWindow = false ;

	QString kernelVersion = list.at( 0 ) ;

	if( !kernelVersion.isEmpty() ){
		updateWindow = true ;
		this->logActivity_1( kernelVersion ) ;
		this->showToolTip( icon,tr( "Outdated packages found" ) ) ;
	}

	QString libreofficeVersion = list.at( 1 ) ;
	if( !libreofficeVersion.isEmpty() ){
		updateWindow = true ;
		this->logActivity_1( libreofficeVersion ) ;
		this->showToolTip( icon,tr( "Outdated packages found" ) ) ;
	}

	QString virtualBoxVersion = list.at( 2 ) ;
	if( !virtualBoxVersion.isEmpty() ){
		updateWindow = true ;
		this->logActivity_1( virtualBoxVersion ) ;
		this->showToolTip( icon,tr( "Outdated packages found" ) ) ;
	}

	QString callibre = list.at( 3 ) ;
	if( !callibre.isEmpty() ){
		updateWindow = true ;
		this->logActivity_1( callibre ) ;
		this->showToolTip( icon,tr( "Outdated packages found" ) ) ;
	}

	if( updateWindow ){
		emit updateLogWindow() ;
	}
}

void qtUpdateNotifier::showToolTip( const QString& x,const QString& y,const QStringList& list )
{
	this->logActivity( y ) ;
	KStatusNotifierItem::setToolTip( x,tr( "Updates found" ),list.at( 0 ) ) ;
	this->changeIcon( x ) ;
}

void qtUpdateNotifier::showToolTip( const QString& x,const QString& y,const QString& z )
{
	KStatusNotifierItem::setToolTip( x,y,z ) ;
	this->changeIcon( x ) ;
}

void qtUpdateNotifier::showToolTip( const QString& x,const QString& y,int z )
{
	QString n = tr( "Next update check will be at %1" ).arg( this->nextUpdateTime( z ) ) ;
	KStatusNotifierItem::setToolTip( x,y,n ) ;
	this->changeIcon( x ) ;
}

void qtUpdateNotifier::showToolTip( const QString& x,const QString& y )
{
	QDateTime d ;
	d.setMSecsSinceEpoch( this->nextScheduledUpdateTime() ) ;

	QString n = tr( "Next update check will be at %1" ).arg( d.toString( Qt::TextDate ) ) ;

	if( y == tr( "No updates found" ) ){
		this->logActivity( y ) ;
		this->logActivity( this->logMsg() ) ;
		KStatusNotifierItem::setToolTip( x,y,n ) ;
	}else{
		QString msg = QString( "<table><tr><td><b>%1</b></tr></td><tr><td>%2</tr></td></table>" ).arg( y ).arg( n ) ;
		this->logActivity( y ) ;
		this->logActivity( this->logMsg() ) ;
		KStatusNotifierItem::setToolTip( x,tr( "Status" ),msg ) ;
	}

	this->changeIcon( x ) ;
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
	return tr( "Scheduled next check to be in %1 hours at %2" ).arg( QString( num ) ).arg( n ) ;
}

QString qtUpdateNotifier::logMsg( void )
{
	u_int64_t x = this->getCurrentTime() ;
	u_int64_t y = this->nextScheduledUpdateTime() ;
	u_int64_t e = y - x ;
	char num[ 64 ] ;
	float f = static_cast<float>( e ) ;
	snprintf( num,64,"%.2f",f / ( 1000 * 60 * 60 ) ) ;
	QString n = this->nextUpdateTime( e ) ;
	return tr( "Scheduled next check to be in %1 hours at %2" ).arg( QString( num ) ).arg( n ) ;
}

void qtUpdateNotifier::scheduleUpdates( int interval )
{
	this->logActivity( this->logMsg( interval ) ) ;
	m_timer->stop() ;
	m_timer->start( interval ) ;
}

void qtUpdateNotifier::setUpdateInterval( int interval )
{
	this->logActivity( tr( "Rescheduling update check" ) ) ;

	m_sleepDuration = interval;

	this->logActivity( this->logMsg( m_sleepDuration ) ) ;

	this->writeUpdateTimeToConfigFile( this->getCurrentTime() + m_sleepDuration ) ;

	m_timer->stop() ;
	m_timer->start( m_sleepDuration ) ;

	QString x = this->iconName() ;
	QString y = this->toolTipTitle() ;

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
}

void qtUpdateNotifier::closeApp()
{
	if( m_threadIsRunning ){
		m_updates->terminate() ;
	}else{
		QCoreApplication::exit( 0 ) ;
	}
}

void qtUpdateNotifier::closeApp( int st )
{
	if( m_threadIsRunning ){
		m_updates->terminate() ;
	}else{
		QCoreApplication::exit( st ) ;
	}
}

qtUpdateNotifier::~qtUpdateNotifier()
{
	if( m_trayMenu ){
		m_trayMenu->deleteLater() ;
	}
	if( m_timer ){
		m_timer->stop() ;
		m_timer->deleteLater() ;
	}
	this->logActivity( tr( "Qt-update-notifier quitting" ) ) ;
}

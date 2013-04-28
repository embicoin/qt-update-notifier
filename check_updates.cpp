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

#include "check_updates.h"

check_updates::check_updates( QString configPath,QObject * parent )
{
	Q_UNUSED( parent ) ;
	m_configPath = configPath ;
}

bool check_updates::online()
{
	QProcess exe ;
	exe.start( QString( "ping -c 1 8.8.8.8" ) ) ;
	exe.waitForFinished() ;
	int st = exe.exitCode() ;
	exe.close();
	return st == 0 ;
}

void check_updates::run()
{
	if( this->online() ){
		this->reportUpdates();
	}else{
		QStringList l ;
		emit updateStatus( NO_NET_CONNECTION,l );
	}
}

void check_updates::processUpdates( QByteArray data )
{
	QString updates ;

	int count ;

	QStringList l = QString( data ).split( "\n" ) ;

	int index = l.indexOf( QString( "The following packages will be upgraded" ) ) ;

	const char * threeSpaceCharacters = "   " ;

	if( index != -1 ){
		count = 0 ;
		while( true ){
			index++ ;
			if( l.at( index ).startsWith( threeSpaceCharacters ) ){
				count++ ;
			}else{
				break ;
			}
		}

		updates = QString( "pkgs to be upgraded: %1\n" ).arg( QString::number( count ) ) ;
	}

	index = l.indexOf( QString( "The following packages will be REPLACED:" ) ) ;

	if( index != -1 ){
		count = 0 ;
		while( true ){
			index++ ;
			if( l.at( index ).startsWith( threeSpaceCharacters ) ){
				count++ ;
			}else{
				break ;
			}
		}

		updates += QString( "pkgs to be replaced: %1 \n" ).arg( QString::number( count ) ) ;
	}

	index = l.indexOf( QString( "The following NEW packages will be installed:" ) ) ;

	if( index != -1 ){
		count = 0 ;
		while( true ){
			index++ ;
			if( l.at( index ).startsWith( threeSpaceCharacters ) ){
				count++ ;
			}else{
				break ;
			}
		}

		updates += QString( "pkgs to be installed: %1" ).arg( QString::number( count ) ) ;
	}

	int size = l.size() ;
	int i = 0 ;
	while( i < size ){
		if( l.at( i ).startsWith( QString( "Inst " ) ) ){
			l.removeAt( i );
			i = 0 ;
			size = l.size() ;
		}else{
			i++ ;
		}
	}

	size = l.size() ;
	i = 0 ;
	while( i < size ){
		if( l.at( i ).startsWith( QString( "Conf " ) ) ){
			l.removeAt( i );
			i = 0 ;
			size = l.size() ;
		}else{
			i++ ;
		}
	}

	l.prepend( updates );
	emit updateStatus( UPDATES_FOUND,l );
}

void check_updates::reportUpdates()
{
	QString aptUpdate = QString( "apt-get -s -o Debug::NoLocking=true -o dir::state=%1/apt update" ).arg( m_configPath ) ;
	QString aptUpgrade = QString( "apt-get -s -o Debug::NoLocking=true -o dir::state=%1/apt dist-upgrade" ).arg( m_configPath ) ;

	QDir dir ;
	dir.mkdir( m_configPath + QString( "/apt" ) ) ;
	dir.mkdir( m_configPath + QString( "/apt/lists" ) ) ;
	dir.mkdir( m_configPath + QString( "/apt/lists/partial" ) ) ;

	const char * error1 = "The following packages have unmet dependencies" ;
	const char * error2 = "E: Error, pkgProblemResolver::Resolve generated breaks, this may be caused by held packages." ;
	const char * error3 = "The following packages have been kept back" ;

	QStringList list ;
	QProcess exe ;
	exe.start( aptUpdate );
	exe.waitForFinished( -1 ) ;
	int st = exe.exitCode() ;
	exe.close();
	if( st == 0 ){
		exe.start( aptUpgrade );
		exe.waitForFinished( -1 ) ;
		QByteArray output = exe.readAllStandardOutput() ;
		exe.close();
		if( !output.isEmpty() ){
			if( output.contains( error1 ) || output.contains( error2 ) || output.contains( error3 ) ){
				list.append( output );
				emit updateStatus( INCONSISTENT_STATE,list );
			}else if( output.contains( "\nThe following packages will be" ) ){
				this->processUpdates( output );
			}else{
				list.append( output );
				emit updateStatus( NO_UPDATES_FOUND,list );
			}
		}else{
			list.append( output );
			emit updateStatus( NO_UPDATES_FOUND,list );
		}
	}else{
		/*
		 * I cant see how i will get here
		 */
		emit updateStatus( NO_UPDATES_FOUND,list );
	}
}

check_updates::~check_updates()
{
}

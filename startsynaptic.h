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


#ifndef STARTSYNAPTIC_H
#define STARTSYNAPTIC_H

#include <QObject>
#include <QRunnable>
#include <QProcess>
#include <QThreadPool>

#include "qt-update-synaptic-helper.h"

class startSynaptic : public QObject,public QRunnable
{
	Q_OBJECT
public:
	explicit startSynaptic( bool autoRefreshSynaptic,QObject * parent = 0 ) ;
	~startSynaptic() ;
	void start( void ) ;
private:
	void run( void ) ;
	bool m_autoRefresh ;
};

#endif // STARTSYNAPTIC_H

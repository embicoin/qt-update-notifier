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

#include "configuredialog.h"
#include "ui_configuredialog.h"

configureDialog::configureDialog( QStringList list,bool autoStart,bool autoRefresh,QWidget * parent ) :
	QDialog( parent ),m_ui( new Ui::configureDialog )
{
	m_ui->setupUi( this );

	this->setWindowTitle( tr( "Configuration window" ) );

	m_ui->checkBoxSynapticAutoRefresh->setChecked( autoRefresh ) ;

	m_ui->checkBoxAutoStart->setChecked( autoStart );

	m_ui->gbUpdateIntervalComboBoxDays->setCurrentIndex( -1 );
	m_ui->gbUpdateIntervalComboBoxHours->setCurrentIndex( -1 );
	m_ui->gbUpdateIntervalComboBoxMinutes->setCurrentIndex( -1 ) ;

	connect( m_ui->pbClose,SIGNAL( clicked() ),this,SLOT( closeUI() ) ) ;
	connect( m_ui->gbUpdateIntervalComboBoxDays,SIGNAL( currentIndexChanged( int ) ),this,SLOT( labelDays( int ) ) ) ;
	connect( m_ui->gbUpdateIntervalComboBoxHours,SIGNAL( currentIndexChanged( int ) ),this,SLOT( labelHours( int ) ) ) ;
	connect( m_ui->gbUpdateIntervalComboBoxMinutes,SIGNAL( currentIndexChanged( int ) ),this,SLOT(labelMinutes( int ) ) ) ;
	connect( m_ui->checkBoxSynapticAutoRefresh,SIGNAL( toggled( bool ) ),this,SLOT( autoRefreshSynaptic_1( bool ) ) ) ;

	m_CheckDelayOnStartUp = list.at( 0 ) ;
	m_updateCheckInterval = list.at( 1 ) ;

	m_autoRefreshSynaptic = autoRefresh ;
}

void configureDialog::autoRefreshSynaptic_1( bool b )
{
	emit autoReshreshSynaptic( b );
}

configureDialog::~configureDialog()
{
	delete m_ui;
}

void configureDialog::showUI( QString language )
{
	m_prefferedLanguage = language ;
	this->setIntervalBetweenUpdateChecks();
	this->setDelayTimeAtLogIn();
	this->setupLanguageList();
	this->show();
}

void configureDialog::closeUI()
{
	int days    = m_ui->gbUpdateIntervalComboBoxDays->currentText().toInt()    * 1 * 24 * 60 * 60 ;
	int hours   = m_ui->gbUpdateIntervalComboBoxHours->currentText().toInt()   * 60 * 60 ;
	int minutes = m_ui->gbUpdateIntervalComboBoxMinutes->currentText().toInt() * 60 ;

	m_duration = days + hours + minutes ;

	if( m_duration < 10 * 60 ){
		QMessageBox msg( this ) ;
		msg.setText( tr( "Update check interval must be at least 10 minutes" ) ) ;
		msg.exec() ;
	}else{
		this->autoStartToggled( m_ui->checkBoxAutoStart->isChecked() ) ;

		this->delayTimeChanged( m_ui->gbDelayStartIntervalComboBox->currentIndex() );

		QFile f( m_updateCheckInterval ) ;

		f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;

		QString e = QString::number( m_duration ) ;

		f.write( e.toAscii() ) ;
		f.close() ;

		if( m_duration != m_interval ){
			emit setUpdateInterval( m_duration * 1000 );
			emit configOptionsChanged();
		}else{
			;
		}

		if( m_ui->comboBoxLanguageList->currentText() != m_prefferedLanguage ){
			emit localizationLanguage( m_ui->comboBoxLanguageList->currentText() );
		}

		this->hide();
		this->deleteLater();
	}
}

void configureDialog::autoStartToggled( bool b )
{
	emit toggleAutoStart( b );
}

void configureDialog::delayTimeChanged( int index )
{
	QString interval ;
	switch( index ){
		case  0 : interval = QString( "0" )   ; break ;
		case  1 : interval = QString( "300" ) ; break ;
		case  2 : interval = QString( "600" ) ; break ;
		case  3 : interval = QString( "900" ) ; break ;
		case  4 : interval = QString( "1800" ); break ;
	}

	QFile f( m_CheckDelayOnStartUp ) ;
	f.open( QIODevice::WriteOnly | QIODevice::Truncate ) ;
	f.write( interval.toAscii() ) ;
}

void configureDialog::setupLanguageList()
{
	QDir d ;
	d.setPath( QT_UPDATE_NOTIFIER_TRANSLATION_PATH );
	QStringList list = d.entryList() ;

	list.removeOne( "." ) ;
	list.removeOne( ".." ) ;
	list.removeOne( "translations.ts" ) ;

	m_ui->comboBoxLanguageList->addItem( QString( "english_US" ) ) ;

	QString e ;
	int index ;
	int j = list.size() ;
	QChar c( '.' ) ;

	for( int i = 0 ; i < j ; i++ ){
		e = list.at( i ) ;
		index = e.indexOf( c ) ;
		if( index == -1 ){
			m_ui->comboBoxLanguageList->addItem( e ) ;
		}else{
			e.truncate( index ) ;
			m_ui->comboBoxLanguageList->addItem( e ) ;
		}
	}

	index = m_ui->comboBoxLanguageList->findText( m_prefferedLanguage ) ;
	if( index == -1 ){
		m_ui->comboBoxLanguageList->setCurrentIndex( 0 );
	}else{
		m_ui->comboBoxLanguageList->setCurrentIndex( index ) ;
	}
}

void configureDialog::labelDays( int index )
{
	if( index == 1 ){
		m_ui->labelDays->setText( tr( "Day" ) ) ;
	}else{
		m_ui->labelDays->setText( tr( "Days" ) ) ;
	}
}

void configureDialog::labelMinutes( int index )
{
	if( index == 1 ){
		m_ui->labelMinutes->setText( tr( "Minute" ) ) ;
	}else{
		m_ui->labelMinutes->setText( tr( "Minutes") ) ;
	}
}

void configureDialog::labelHours( int index )
{
	if( index == 1 ){
		m_ui->labelHours->setText( tr( "Hour" ) ) ;
	}else{
		m_ui->labelHours->setText( tr( "Hours") ) ;
	}
}

void configureDialog::setDelayTimeAtLogIn()
{
	QFile f( m_CheckDelayOnStartUp ) ;
	if( f.exists() ){
		f.open( QIODevice::ReadOnly ) ;
		int time = QString( f.readAll() ).toInt() ;

		time = time / 60 ;

		switch( time ){
			case  0  : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 0 ) ; break ;
			case  5  : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 1 ) ; break ;
			case  10 : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 2 ) ; break ;
			case  15 : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 3 ) ; break ;
			case  30 : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 4 ) ; break ;
			default  : m_ui->gbDelayStartIntervalComboBox->setCurrentIndex( 1 ) ; break ;
		}
	}
}

void configureDialog::setIntervalBetweenUpdateChecks()
{
	QFile f( m_updateCheckInterval ) ;
	f.open( QIODevice::ReadOnly ) ;
	QString data = f.readAll() ;

	m_interval = data.toULongLong() ;

	u_int32_t days = m_interval / ( 1 * 24 * 60 * 60 ) ;

	u_int32_t remainer = m_interval % ( 1 * 24 * 60 * 60 ) ;

	u_int32_t hours = remainer / ( 60 * 60 );

	remainer = remainer % ( 60 * 60 );

	u_int32_t minutes = remainer / 60 ;

	m_ui->gbUpdateIntervalComboBoxDays->setCurrentIndex( days );
	m_ui->gbUpdateIntervalComboBoxHours->setCurrentIndex( hours );
	m_ui->gbUpdateIntervalComboBoxMinutes->setCurrentIndex( minutes ) ;
}

void configureDialog::closeEvent( QCloseEvent * e )
{
	e->ignore();
	this->closeUI();
}

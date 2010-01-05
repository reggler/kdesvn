/***************************************************************************
 *   Copyright (C) 2009 by Rajko Albrecht  ral@alwins-world.de             *
 *   http://kdesvn.alwins-world.de/                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SVNTHREAD_H
#define SVNTHREAD_H

#include "ccontextlistener.h"
#include "frontendtypes.h"

#include <QThread>
#include <QEvent>
#include <QQueue>
#include <QMutex>
#include <QReadWriteLock>

//! Base class for creating threads holding an subversion connection
class SvnThread:public QThread
{
public:
    //! Creator
    /*!
     * \param parent A qobject derived class which should have a qt-slot slotNotifyMessage(const QString&)
     */
    SvnThread(QObject*parent);
    virtual ~SvnThread();
    virtual void run()=0;
    virtual void cancelMe();

protected:
    svn::Client* m_Svnclient;
    svn::ContextP m_CurrentContext;
    svn::smart_pointer<ThreadContextListener> m_SvnContextListener;
    QObject*m_Parent;

    //! a base method often needed
    /*!
     * Exceptions will NOT catched, the caller has to do it!
     */
    void itemInfo(const QString&what,svn::InfoEntry&target,const svn::Revision&_rev=svn::Revision::UNDEFINED,const svn::Revision&_peg = svn::Revision::UNDEFINED);
};

#endif
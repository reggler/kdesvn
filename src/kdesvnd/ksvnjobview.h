/***************************************************************************
*   Copyright (C) 2005-2009 by Rajko Albrecht  ral@alwins-world.de        *
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
#ifndef KSVN_JOBVIEW_H
#define KSVN_JOBVIEW_H

#include "src/ksvnwidgets/jobviewinterface.h"

class KsvnJobView:public org::kde::JobView
{
    Q_OBJECT

    public:
        KsvnJobView(qulonglong id, const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);
        virtual ~KsvnJobView(){}

        enum jobstate {
            STOPPED = 0,
            RUNNING,
            CANCELD
        };

        jobstate state()const{return m_state;}
        void setState(jobstate aState){m_state = aState;}
        qulonglong id()const{return m_id;}

        unsigned long percent(qulonglong amount);

        virtual void setTotal(qlonglong max);
        qlonglong max()const{return m_max;}

    protected Q_SLOTS:
        virtual void killJob();
    private:
        qulonglong m_id;
        jobstate m_state;
        qlonglong m_max;
};

#endif
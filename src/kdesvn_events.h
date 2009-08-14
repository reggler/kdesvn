/***************************************************************************
 *   Copyright (C) 2009 by Rajko Albrecht  ral@alwins-world.de             *
 *   http://kdesvn.alwins-world.de/                                        *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU Lesser General Public              *
 * License as published by the Free Software Foundation; either            *
 * version 2.1 of the License, or (at your option) any later version.      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Lesser General Public License for more details.                         *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this program (in the file LGPL.txt); if not,         *
 * write to the Free Software Foundation, Inc., 51 Franklin St,            *
 * Fifth Floor, Boston, MA  02110-1301  USA                                *
 *                                                                         *
 * This software consists of voluntary contributions made by many          *
 * individuals.  For exact contribution history, see the revision          *
 * history and logs, available at http://kdesvn.alwins-world.de.           *
 ***************************************************************************/
#ifndef KDESVN_EVENTS_H
#define KDESVN_EVENTS_H

#include <qevent.h>
#include "src/svnqt/svnqt_defines.hpp"

class FillCacheStatusEvent:public QEvent
{
    public:
        FillCacheStatusEvent(qlonglong current,qlonglong max);
        qlonglong current()const{return m_current;}
        qlonglong max()const{return m_max;}
    private:
        qlonglong m_current,m_max;
};

class DataEvent:public QEvent
{
    public:
        DataEvent(QEvent::Type);

        void setData(void*data);
        void*data()const;
    private:
        void*_data;
};

#endif

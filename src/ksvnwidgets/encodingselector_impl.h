/***************************************************************************
 *   Copyright (C) 2007 by Rajko Albrecht  ral@alwins-world.de             *
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
#ifndef ENCODINGSELECTOR_IMPL_H
#define ENCODINGSELECTOR_IMPL_H

#include "ui_encodingselector.h"

class QTextCodec;

class EncodingSelector_impl: public QWidget, public Ui::EncodingSelector {
Q_OBJECT
public:
    EncodingSelector_impl(QWidget *parent = 0);
    EncodingSelector_impl(const QString&cur, QWidget *parent = 0, const char *name = 0);
    virtual ~EncodingSelector_impl(){}

    void setCurrentEncoding(const QString&cur);

protected slots:
    virtual void itemActivated(int);

signals:
    void TextCodecChanged(const QString&);
};

#endif

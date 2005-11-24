/***************************************************************************
 *   Copyright (C) 2005 by Rajko Albrecht                                  *
 *   ral@alwins-world.de                                                   *
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
#ifndef HELPERSKTRANSLATEURL_H
#define HELPERSKTRANSLATEURL_H

#include <kurl.h>

namespace helpers {

/**
	@author Rajko Albrecht <ral@alwins-world.de>
*/
class KTranslateUrl{
public:
    KTranslateUrl();
    ~KTranslateUrl();

    static KURL translateSystemUrl(const KURL&);
    static bool parseURL(const KURL&,QString&name,QString&path);
    static KURL findSystemBase(const QString&name);
};

}

#endif
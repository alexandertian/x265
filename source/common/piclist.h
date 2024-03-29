/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_PICLIST_H
#define X265_PICLIST_H

#include <cstdlib>

namespace x265 {
class TComPic;

class PicList
{
protected:

    TComPic* m_start;
    TComPic* m_end;
    int      m_count;

public:

    PicList()
    {
        m_start = NULL;
        m_end   = NULL;
        m_count = 0;
    }

    /** Push picture to end of the list */
    void pushBack(TComPic& pic);

    /** Push picture to beginning of the list */
    void pushFront(TComPic& pic);

    /** Pop picture from end of the list */
    TComPic* popBack();

    /** Pop picture from beginning of the list */
    TComPic* popFront();

    /** Remove picture from list */
    void remove(TComPic& pic);

    TComPic* first()      { return m_start;   }

    TComPic* last()       { return m_end;     }

    int size()            { return m_count;   }

    bool empty() const    { return !m_count;  }

    operator bool() const { return !!m_count; }
};
}

#endif // ifndef X265_PICLIST_H

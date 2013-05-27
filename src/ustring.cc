/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Copyright (C) 2005-2006 Dodji Seketeli
 *               2007 Marko Anastasov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cstring> // for g++ 4.3
#include <glib.h>
#include "ustring.h"

namespace Glib {

namespace Util {

using std::vector;

vector<Glib::ustring>
split(const Glib::ustring& str, const Glib::ustring& delim)
{
    vector<Glib::ustring> result;

    if (str.size () == Glib::ustring::size_type(0)) { return result; }

    int len = str.size () + 1;

    gchar* buf = (gchar*) g_malloc (len);
    memset (buf, 0, len);
    memcpy (buf, str.c_str (), str.size ());

    gchar** splited = g_strsplit (buf, delim.c_str (), -1);

    try {
        for (gchar **cur = splited ; cur && *cur; ++cur) {
            result.push_back (Glib::ustring (*cur));
        }
    } catch (...) {}

    if (splited) {
        g_strfreev (splited);
    }

    g_free (buf);

    return result;
}

vector<Glib::ustring>
split(const Glib::ustring& str)
{
    vector<Glib::ustring> res;

    int bytes = str.bytes ();

    g_return_val_if_fail (bytes != Glib::ustring::size_type (0), res);
    g_return_val_if_fail (str.validate (), res);

    Glib::ustring tmp(str);

    trim (tmp);
    Glib::ustring::size_type chars = tmp.size ();
    g_return_val_if_fail (chars != Glib::ustring::size_type (0), res);

    Glib::ustring::size_type i, tail;
    i = 0;
    tail = 1;

    while (tail <= chars) {
        gunichar ch = tmp[tail];

        if ((g_unichar_isspace (ch) == TRUE) || (tail == chars)) {
            int chars_to_skip = 1;
            int pos;
            bool in_ws = true;

            while (in_ws) {
                pos = tail + chars_to_skip;
                ch = tmp[pos];
                (g_unichar_isspace (ch)) ? ++chars_to_skip : in_ws = false;
            }

            if (tail == chars) ++tail;

            res.push_back (tmp.substr (i, tail - i));

            i = tail + chars_to_skip;
            tail += chars_to_skip + 1;
        }

        ++tail;
    }

    return res;
}

void
trim_left(Glib::ustring& str)
{
    if (str.empty ()) return;

    Glib::ustring::iterator it(str.begin());
    Glib::ustring::iterator end(str.end());

    for ( ; it != end; ++it)
        if (! isspace(*it))
            break;

    if (it == str.end())
        str.clear();
    else
        str.erase(str.begin(), it);
}

void
trim_right(Glib::ustring& str)
{
    if (str.empty ()) return;

    Glib::ustring::iterator end(str.end());
    Glib::ustring::iterator it(--(str.end()));

    for ( ; ; --it) {
        if (! isspace(*it)) {
            Glib::ustring::iterator it_adv(it);
            str.erase(++it_adv, str.end());
            break;
        }

        if (it == str.begin()) {
            str.clear();
            break;
        }
    }
}

void
trim(Glib::ustring& str)
{
    trim_left(str);
    trim_right(str);
}

Glib::ustring
uprintf(const Glib::ustring& format, ...)
{
    va_list args;

    va_start(args, format);
    gchar* cstr = g_strdup_vprintf(format.c_str(), args);
    Glib::ustring str(cstr);
    g_free(cstr);
  
    return str;
}

} // namespace Util

} // namespace Glib


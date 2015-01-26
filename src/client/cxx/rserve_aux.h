/*
 *  C++ Interface to Rserve
 *  Copyright (C) 2004-8 Simon Urbanek, All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Leser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Although this code is licensed under LGPL v2.1, we strongly encourage
 *  everyone modifying this software to contribute back any improvements and
 *  bugfixes to the project for the benefit all other users. Thank you.
 *
 *  $Id$
 */

#ifndef __RSERVE_AUX_H__
#define __RSERVE_AUX_H__

#include <vector>
#include <string>
#include <boost/format.hpp>
#include "Rconnection.h"
using namespace std;

Rexp *parse_rexp(std::string data);

class DataFrame {
    // Wrapper around R's data frames as Rexps, to make handling them easier
    // This class owns the Rvector used to instantiate it
 public:
    // Make sure this is only called on a legitimate data frame
    explicit DataFrame(Rvector *rvec, bool set_own_rv = true) :
	rv(rvec),
        names((Rstrings *)((Rlist *)rv->attr)->entryByTagName("names")),
	own_rv(set_own_rv) {}
    explicit DataFrame(const vector<string> &colnames,
		       const vector<Rexp*> &coldata,
		       bool set_own_rv = true);
    explicit DataFrame(const vector<string> &colnames,
		       const vector<Rexp*> &coldata,
		       const Rstrings *rownames,
		       bool set_own_rv = true);
    ~DataFrame() { if (own_rv && rv) delete rv; }

    unsigned long ncol() const { return rv->length(); }
    unsigned long nrow() const;
    char *nameAt(unsigned int i) { return names->stringAt(i); }
    const char *nameAt(unsigned int i) const { return names->stringAt(i); }
    Rexp *expAt(unsigned int i) { return rv->expAt(i); }
    const Rexp *expAt(unsigned int i) const { return rv->expAt(i); }
    Rvector *getRv() { return rv; }
    bool has_string_rownames() const;
    const Rstrings *get_rownames() const;
 private:
    void df_const_helper(const vector<string> &colnames,
			 const vector<Rexp*> &coldata,
			 const Rstrings *rownames,
			 bool set_own_rv = true);
    Rlist *getAttr(const vector<string> &colnames, int nrow,
		   const Rstrings *rownames) const;
    unsigned long get_len(const Rexp *x) const;
    Rexp *get_rownames(int rnow, const Rstrings *rownames) const;
    Rvector *rv;
    Rstrings *names;
    bool own_rv; // True if this data frame should delete rv on destruction
};

bool is_data_frame(const Rexp *exp);
Rstrings *get_classes(const Rexp *exp);
bool has_class(const Rexp *exp, const std::string classname);
Rlist *make_class_attr(const std::string classname);
vector<string> Factor2Strings(const Rinteger *ri);

#endif

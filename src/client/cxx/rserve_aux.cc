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
 *  $Id: Rconnection.cc 301 2011-01-07 16:32:45Z urbanek $
 */

#include "rserve_aux.h"
#include "Rconnection.h"

bool is_data_frame(const Rexp *exp) {
    // True if exp points to a data frame
    if (!(exp && exp->type == XT_VECTOR))
	return false;
    if (!has_class(exp, "data.frame"))
	return false;

    // Finally check to make sure enough names
    Rstrings *names = (Rstrings *)((Rlist *)exp->attr)->entryByTagName("names");
    if (!(names && names->type == XT_ARRAY_STR
	  && names->count() == ((Rvector *)exp)->length()))
	return false;
    return true;
}

Rstrings *get_classes(const Rexp *exp) {
	// Return the classes of the given Rexp, or 0 if none.
	// The resulting object's memory is owned by exp
    if (!(exp->attr && exp->attr->type == XT_LIST_TAG))
		return 0;

	Rstrings *rclass = (Rstrings *)((Rlist *)exp->attr)->entryByTagName("class");
	if (!(rclass && rclass -> type == XT_ARRAY_STR))
		return 0;
	return rclass;
}

bool has_class(const Rexp *exp, const std::string classname) {
    // Return true if exp has the given class
	Rstrings *rclass = get_classes(exp);
	if (!rclass)
		return false;

    unsigned int class_size = rclass->count();
    unsigned int i;
    for (i = 0; i < class_size; i++)
		if (strcmp(rclass->stringAt(i), classname.c_str()) == 0)
		    break;
		if (i == class_size)
			return false;
    return true;
}

Rlist *make_class_attr(const string classname) {
	// Return attributes implying that the object is of the given class
	vector<Rexp*> tags;
	Rsymbol rsym("class");
	tags.push_back(&rsym);

	vector<std::string> vs;
	vs.push_back(classname);
	Rstrings rstring(vs);
	vector<Rexp*> entries;
	entries.push_back(&rstring);

	return new Rlist(tags, entries);
}

DataFrame::DataFrame(const vector<string> &colnames,
		     const vector<Rexp*> &coldata,
		     bool set_own_rv) : own_rv(set_own_rv) {
    // Create a data frame from column names and column data
    df_const_helper(colnames, coldata, 0, set_own_rv);
}

DataFrame::DataFrame(const vector<string> &colnames,
		     const vector<Rexp*> &coldata,
		     const Rstrings *rownames,
		     bool set_own_rv) : own_rv(set_own_rv) {
    // Create a data frame from column names and column data
    df_const_helper(colnames, coldata, rownames, set_own_rv);
}

void DataFrame::df_const_helper(const vector<string> &colnames,
				const vector<Rexp*> &coldata,
				const Rstrings *rownames,
				bool set_own_rv) {
    // Helper function, make data frames with and without rownames
    if (colnames.size() != coldata.size())
	throw RserveAuxException("DataFrame names and data different lengths");

    int nrow = colnames.size() > 0 ? get_len(coldata[0]) : 0;
    // Set class variable
    Rlist *attr = getAttr(colnames, nrow, rownames);
    this->rv = new Rvector(coldata, attr);
    this->names = (Rstrings *)(((Rlist *)rv->attr)->entryByTagName("names"));
}

Rlist *DataFrame::getAttr(const vector<string> &colnames, int nrow,
			  const Rstrings *rownames) const {
    // Return the attributes Rlist, used in construction
    Rsymbol rs1("names");
    Rsymbol rs2("row.names");
    Rsymbol rs3("class");
    vector<Rexp*> tags;
    tags.push_back(&rs1);
    tags.push_back(&rs2);
    tags.push_back(&rs3);

    Rstrings names_rs(colnames);
    Rexp *rownames_re = get_rownames(nrow, rownames);

    vector<string> vs;
    vs.push_back("data.frame");
    Rstrings class_rs(vs);

    vector<Rexp*> entries;
    entries.push_back(&names_rs);
    entries.push_back(rownames_re);
    entries.push_back(&class_rs);
    Rlist *result = new Rlist(tags, entries);
    delete rownames_re; // Rlist constructor copies arguments
    return result;
}

Rexp *DataFrame::get_rownames(int nrow, const Rstrings *rownames) const {
    // Return default row names equiv to seq(length=nrow)
	if (!rownames) {
		int *vi = new int[nrow];
		int count=1;
		for (int i = 0; i != nrow; i++)
			vi[i] = count++;
		return (Rexp *)new Rinteger(vi, nrow);
	}

	if (rownames->count() != nrow)
		throw RserveAuxException("Wrong length of rownames");
	return (Rexp *)rownames;
}

unsigned long DataFrame::get_len(const Rexp *x) const {
    // Return length of string/int/double x
	if (x->type == XT_ARRAY_STR)
		return ((Rstrings *)x)->count();
	else if (x->type == XT_ARRAY_INT)
		return ((Rinteger *)x)->length();
	else if (x->type == XT_ARRAY_DOUBLE)
		return ((Rdouble *)x)->length();
	throw RserveAuxException(str(boost::format(
		"Unrecognized type %d in data frame") % x->type));
}

unsigned long DataFrame::nrow() const {
    // Return the number of rows in the data frame
    if (ncol() == 0) return 0;
    return get_len(expAt(0));
}

bool DataFrame::has_string_rownames() const {
    // Return true if the rownames are strings and not just integers
	Rexp *rn = ((Rlist *)rv->attr)->entryByTagName("row.names");
	if (rn->type == XT_ARRAY_STR)
		return true;
    return false;
}

const Rstrings *DataFrame::get_rownames() const {
    // Return the rownames from the data frame, assuming strings
    Rexp *rn = ((Rlist *)rv->attr)->entryByTagName("row.names");
    if (rn->type != XT_ARRAY_STR)
	throw RserveAuxException("Data frame does not have string rownames");
    Rstrings *rs = (Rstrings *)rn;
    if (rs->count() != this->nrow())
	throw RserveAuxException("Data frame rownames don't match length");
    return rs;
}

// ---------------- Non-DataFrame code

vector<string> Factor2Strings(const Rinteger *ri) {
    // Return a vector of strings from an R factor
    if (!ri || ri->type != XT_ARRAY_INT)
	throw RserveAuxException("Received factor with bad format");
    Rstrings *levels = (Rstrings *)(((Rlist*)ri->attr)
				    ->entryByTagName("levels"));
    if (!levels || levels->type != XT_ARRAY_STR)
	throw RserveAuxException("Factor is missing string levels");

	vector<string> result;
    int curval, curindex, max_possible_index = levels->count() - 1;
    for (Rsize_t i = 0; i != ri->length(); i++) {
		curval = ri->intAt(i);
		if (ISNA(curval))
			result.push_back((char *)NaStringRepresentation);
		else {
			curindex = curval - 1; // R starts counting at 1
			if (curindex < 0 || curindex > max_possible_index)
				throw RserveAuxException("Factor has missing level");
			result.push_back(levels->stringAt(curindex));
		}
	}
    return result;
}


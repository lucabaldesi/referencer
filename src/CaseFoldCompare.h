
#ifndef CASEFOLDCOMPARE_H
#define CASEFOLDCOMPARE_H

struct casefoldCompare : public std::binary_function<Glib::ustring, Glib::ustring, bool> {
	bool operator () ( const Glib::ustring &lhs, const Glib::ustring &rhs ) const {
		return lhs.casefold() < rhs.casefold();
	}
};

#endif

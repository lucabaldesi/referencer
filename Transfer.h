
#ifndef TRANSFER_H
#define TRANSFER_H

#include <glibmm/ustring.h>
#include <glibmm/exception.h>

namespace Transfer {

Glib::ustring &getRemoteFile (
	Glib::ustring const &title,
	Glib::ustring const &messagetext,
	Glib::ustring const &filename);

class Exception : public Glib::Exception {

public:
  explicit Exception(Glib::ustring what) {what_ = what;}
  virtual ~Exception() throw() {};

  virtual Glib::ustring what() const {return what_;}

protected:
  Glib::ustring what_;
};

}

#endif

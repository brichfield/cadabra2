
#pragma once

#include "Props.hh"

class Tableau : public property {
	public:
		virtual ~Tableau() {};
		virtual std::string name() const;
		virtual bool parse(Ex&, Ex::iterator, Ex::iterator, keyval_t&);

		int dimension;
};


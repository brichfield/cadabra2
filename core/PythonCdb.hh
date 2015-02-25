/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/


#pragma once

#include <boost/python.hpp>
#include <stdexcept>
#include "Storage.hh"
#include "Kernel.hh"
#include "Algorithm.hh"

// Ex is essentially a wrapper around an exptree object, with additional
// functionality make it print nice in Python. It also contains logic
// to replace '@(abc)' nodes in the tree with the Python 'abc' expression.

class Ex {
	public:
		Ex(const Ex&); 
		Ex(std::string);
		Ex(int);
		~Ex();

		std::string str_() const;
		std::string repr_() const;
		std::string _repr_latex_() const;
		std::string _repr_html_() const;

		Ex& operator=(const Ex&);

		// Comparison operators.
		bool operator==(const Ex&) const;
		bool __eq__int(int) const;

		exptree tree;

		// Keeping track of what algorithms did to the expression
		// (corresponds to the return code of algorithms).
		// FIXME: the following should implement a stack of states,
		// so that it can be used with nested functions.
		// FIXME: perhaps put in exptree so that we can use it to
		// implement fixed-point logic there.
		Algorithm::result_t state() const;
		void                update_state(Algorithm::result_t);
		void                reset_state();

	private:
		// Functionality to pull in any '@(...)' expressions from the
		// Python side into a C++ expression.
		void                pull_in();
		std::shared_ptr<Ex> fetch_from_python(std::string nm);

		Algorithm::result_t state_;
};


// Property is a templated wrapper around a C++ property object. It 
// provides it with __str__ and __repr__ methods. In order to have
// a quick way to figure out in Python whether an object is a property,
// we derive it from BaseProperty, which is an empty placeholder (in
// Python this is called Property).

// Cadabra properties cannot be proper Python properties, because we
// need to give the latter names in order to prevent them from going
// out of scope. So Cadabra keeps a list of 'anonymous property objects 
// in the current scope'. 

// The question is now what we do when Python keeps a pointer to these
// objects, and let that pointer escape local scope (e.g. by returning
// the Python property object). How do we keep it in scope?

class BaseProperty {
};

template<class T>
class Property : public BaseProperty {
	public:
		Property(std::shared_ptr<Ex> obj, std::shared_ptr<Ex> params=0);

		std::string str_() const;
		std::string repr_() const;

	private:
		// We keep a pointer to the C++ property, so it is possible to 
		// query properties using the Python interface. However, this C++
		// object is owned by the C++ kernel and does not get destroyed
		// when the Python object goes out of scope.

		// When the Python object survives the local scope, results are
		// undefined. 
		T *prop;
};


// Setup of kernels in current scope, callable from Python.

Kernel *create_scope();
Kernel *create_scope_from_global();
Kernel *create_empty_scope();

// Setup default properties for the given kernel.
void    inject_defaults(Kernel *);

// Inject a property into the kernel in current scope. The property is
// then owned by the kernel.
void    inject_property(Kernel *, property *, std::shared_ptr<Ex>, std::shared_ptr<Ex>);

// Get a pointer to the currently visible kernel.
Kernel *get_kernel_from_scope();

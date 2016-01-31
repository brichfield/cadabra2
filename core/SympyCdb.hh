
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "Kernel.hh"

namespace sympy {

   /// \ingroup scalar
   ///
   /// Functionality to act with Sympy functions on (parts of) Cadabra Ex expressions
   /// and read the result back into the same Ex. This duplicates some of the 
   /// logic in PythonCdb.hh, in particular make_Ex_from_string, but it is best to
   /// keep these two completely separate.

	Ex::iterator apply(Kernel&, Ex&, Ex::iterator&, const std::string& head, const std::string& args);


   /// \ingroup scalar
   ///
   /// Use Sympy to invert a matrix, given a set of rules determining its
   /// sparse components. Will return a set of Cadabra rules for the
   /// inverse matrix.

	Ex invert_matrix(Kernel&, Ex& ex, Ex& rules);
	
};


#pragma once

#include <string>
#include <mutex>

#include "tree.hh"
#include <jsoncpp/json/json.h>

namespace cadabra {

	/// \ingroup clientserver
	///
	/// DataCells are the basic building blocks for a document. They are stored 
	/// in a tree inside the client. A user interface should read these cells
	/// and construct corresponding graphical output for them. All cell types are
	/// leaf nodes except for the section cell which generates a new nested structure.
	
	class DataCell {
		public:
			
			/// Cells are labelled with the data type of its contents, which is 
			/// stored in a textural representation but may need processing for
			/// proper display.
			
			enum class CellType { 
					   document,   ///< head node, only one per document
						python,     ///< code input in verbatim form
						output,     ///< latex-parseable output
						//						comment, 
						//						texcomment, 
						latex,      ///< user-entered LaTeX text
						error,      ///< latex-parseable error text
						// section
						};
			
			DataCell(CellType t=CellType::python, const std::string& str="", bool hidden=false);
			DataCell(const DataCell&);
			
			CellType                      cell_type;

			/// Textual representation of the cell content. For e.g. latex cells it is a bit of a
			/// waste to store this representation both in the input and in the output cell.
			/// However, this gives us the flexibility to do manipulations on the input (e.g. 
			/// resolving equation references) before feeding it to LaTeX. 

			std::string                   textbuf; 

			/// Flag indicating whether this cell should be hidden from
			/// view. The GUI should have a way to bring the cells back
			/// into view, typically by clicking on the output cell
			/// corresponding to the input cell. 

			bool                          hidden;
			bool                          sensitive;

			/// Indicator whether this cell is currently being evaluated by the server.
			/// The ComputeThread sets and clears this flag. 

			bool                          running;   

			/// Each cell is identified by a serial number 'id' which is used
			/// to keep track of it across network calls.

			uint64_t                      id() const;
			
		private:

			static std::mutex             serial_mutex;
			uint64_t                      serial_number;
			static uint64_t               max_serial_number;
	};

	typedef tree<DataCell> DTree;

	/// Serialise a document into .cj format, which is a JSON version of
	/// the document tree.

	std::string JSON_serialise(const DTree&);
	void        JSON_recurse(const DTree&, DTree::iterator, Json::Value&);

	/// Load a document from .cj format, i.e. the inverse of the above.

	void        JSON_deserialise(const std::string&, DTree&);
	void        JSON_in_recurse(DTree& doc, DTree::iterator loc, const Json::Value& cells);
}

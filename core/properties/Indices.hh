
#pragma once

#include "Props.hh"

class Indices : public list_property {
	public:
		Indices(); //const std::string& parent="");
		virtual bool parse(const Properties&, keyval_t&) override;
		virtual std::string name() const override;
		virtual std::string unnamed_argument() const override { return "name"; };
		virtual match_t equals(const property *) const override;
		
//		virtual void display(std::ostream&) const override; 
		virtual void latex(std::ostream&) const override; 
		
		std::string set_name, parent_name;
		enum position_t { free, fixed, independent } position_type;

		// List of possible values that indices of this type can take.
		std::vector<Ex> values;

	private:
		/// Given the right-hand side of a 'values={...}' node, generate
		/// a list of all index values in index_values.

		void collect_index_values(Ex::iterator ind_values);
};

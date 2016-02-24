
#include "Algorithm.hh"
#include "algorithms/tab_basics.hh"

class lr_tensor : public tab_basics  {
	public:
		lr_tensor(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

		sibling_iterator tab1, tab2;

	private:
		void do_tableau(iterator&);
		void do_filledtableau(iterator&);
};



#include "GammaMatrix.hh"
#include "Kernel.hh"

std::string GammaMatrix::name() const
	{
	return "GammaMatrix";
	}

void GammaMatrix::latex(std::ostream& str) const
	{
	Matrix::latex(str);
	}

bool GammaMatrix::parse(const Kernel& kernel, keyval_t& keyvals)
	{
	keyval_t::iterator kv=keyvals.find("metric");
	if(kv!=keyvals.end()) {
		metric=Ex(kv->second);
		keyvals.erase(kv);
		}

	ImplicitIndex::parse(kernel, keyvals);

//	kv=keyvals.find("delta");
//	if(kv!=keyvals.end()) delta=Ex(kv->second);
//
	return true;
	}

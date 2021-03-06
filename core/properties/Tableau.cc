
#include "properties/Tableau.hh"

std::string Tableau::name() const
	{
	return "Tableau";
	}

bool Tableau::parse(Ex& tr, Ex::iterator pat, Ex::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("dimension");
	if(kv!=keyvals.end()) dimension=to_long(*(kv->second->multiplier));
	else dimension=-1;
	return true;
	}


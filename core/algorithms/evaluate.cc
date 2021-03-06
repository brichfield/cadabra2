
#include "Functional.hh"
#include "Cleanup.hh"
#include "Permutations.hh"
#include "SympyCdb.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/substitute.hh"
#include "properties/PartialDerivative.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/Accent.hh"
#include <functional>

evaluate::evaluate(const Kernel& k, Ex& tr, const Ex& c)
	: Algorithm(k, tr), components(c)
	{
	}

bool evaluate::can_apply(iterator) 
	{
	return true;
	}

Algorithm::result_t evaluate::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// Descend down the tree. The general logic of the routines this
	// calls is that, instead of looping over all possible index value
	// sets, we look straight at the substitution rules, and check that
	// these are required for some index values (this is where symmetry
	// arguments should come in as well).
   //
	// The logic in Compare.cc helps us by matching component A_{t r}
	// in the rule to an abstract tensor A_{m n} in the expression, storing
	// the index name -> index value map.
	
	cadabra::do_subtree(tr, it, [&](Ex::iterator walk) {
			if(*(walk->name)=="\\sum")       handle_sum(walk);
			else if(*(walk->name)=="\\prod") handle_prod(walk);
			else {
				const PartialDerivative *pd = kernel.properties.get<PartialDerivative>(walk);
				if(pd) handle_derivative(walk);
				}
//			cleanup_dispatch(kernel, tr, walk);
			}
		);

	return res;
	}


void evaluate::handle_sum(iterator it)
	{
	index_map_t full_ind_free, full_ind_dummy;

	// First find the values that all indices will need to take. We do not loop over
	// them, but we need them in order to figure out which patterns in the rule can
	// match to patterns in the expression.

	classify_indices(it, full_ind_free, full_ind_dummy);
	for(auto i: full_ind_free) {
		const Indices *prop = kernel.properties.get<Indices>(i.second);
		if(prop==0)
			throw ArgumentException("evaluate: Index "+*(i.second->name)+" does not have an Indices property.");

		if(prop->values.size()==0)
			throw ArgumentException("evaluate: Do not know values of index "+*(i.second->name)+".");
		}

	// Iterate over all terms in the sum. These should be of three types: \component nodes,
	// nodes with only free indices, and nodes with internal contractions (e.g. A_{m}^{m}). 
	// The first type can, at this stage, be ignored. The second type needs to be converted
	// into a \component node using the component evaluation rules. The last type 
	// separate treatment, and is currently not handled yet; see the beginning of handle_factor.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;
		handle_factor(sib, full_ind_free);
		sib=nxt;
		}

	// Now all terms in the sum (which has its top node at 'it') are
	// \component nodes. We need to merge these together into a single
	// node.

	auto sib1=tr.begin(it);
//	merge_component_children(sib1);
	auto sib2=sib1;
	++sib2;
	while(sib2!=tr.end(it)) {
		merge_components(sib1, sib2);
		sib2=tr.erase(sib2);
		}
	cleanup_components(sib1);
	tr.flatten_and_erase(it);
	}

void evaluate::handle_factor(sibling_iterator& sib, const index_map_t& full_ind_free)
	{
	if(*sib->name=="\\components") return;

	// If this factor is an accent at the top level, descent further.
	const Accent *acc = kernel.properties.get<Accent>(sib);
	if(acc) {
		auto deeper=tr.begin(sib);
		handle_factor(deeper, full_ind_free);
		// Put the accent on each of the components.
		sibling_iterator cl = tr.end(deeper);
		--cl;
		cadabra::do_list(tr, cl, [&](Ex::iterator c) {
				auto towrap = tr.child(c, 1);
				tr.wrap(towrap, *sib);
				return true;
				});
		//tr.print_recursive_treeform(std::cerr, sib);
		// Move the component node up, outside the accent.
		sib=tr.flatten(sib);
		sib=tr.erase(sib);
		//tr.print_recursive_treeform(std::cerr, sib);
		return;
		}
	
	// Internal contractions.
	// FIXME: not yet handled.
	index_map_t ind_free, ind_dummy;
	classify_indices(sib, ind_free, ind_dummy);
	if(ind_dummy.size()>0) {
		std::cerr << "Internal contractions, not yet handled" << std::endl;
		return;
		}

	// Pure scalar nodes need to be wrapped in a \component node to make life
	// easier for the rest of the algorithm.
	if(ind_free.size()==0 && ind_dummy.size()==0) {
		// FIXME: would be good if we could write this in a more readable form.
		auto eq=tr.wrap(sib, str_node("\\equals"));
		tr.prepend_child(eq, str_node("\\comma"));
		eq=tr.wrap(eq, str_node("\\comma"));
		sib=tr.wrap(eq, str_node("\\components"));
		//std::cerr << tr << std::endl;
		return;
		}
	
	// Attempt to apply each component substitution rule on this term.
	Ex repl("\\components");
	for(auto& ind: ind_free) 
		repl.append_child(repl.begin(), ind.second);
	// If there are no free indices, add an empty first child anyway,
	// otherwise we need special cases in various other places.
	auto vl = repl.append_child(repl.begin(), str_node("\\comma"));
	cadabra::do_list(components, components.begin(), [&](Ex::iterator c) {
			Ex rule(c);
			Ex obj(sib);
			substitute subs(kernel, obj, rule);
			iterator oit=obj.begin();
			if(subs.can_apply(oit)) {
				auto el = repl.append_child(vl, str_node("\\equals"));
				auto il = repl.append_child(el, str_node("\\comma"));
				auto fi = full_ind_free.begin();
				// FIXME: need to do something sensible with indices on the lhs 
				// of rules which are not coordinates. You can have A_{m n} as expression,
				// A_{0 0} -> r, A_{i j} -> \delta_{i j} as rule, but at the moment the
				// second rule does not do the right thing.
				if(fi==full_ind_free.end()) {
					for(auto& r: subs.comparator.index_value_map) 
						repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none; 
					}
				else {
					while(fi!=full_ind_free.end()) {
						for(auto& r: subs.comparator.index_value_map) {
							if(fi->first == r.first) {
								repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none; 
								break;
								}
							}
						auto fiold(fi);
						while(fi!=full_ind_free.end() && fiold->first==fi->first)
							++fi;
						}
					}
				subs.apply(oit);
				repl.append_child(el, obj.begin());

				return true; // Cannot yet abort the do_list loop.
				}
			return true;
			});


	sib = tr.move_ontop(iterator(sib), repl.begin());
	}

void evaluate::merge_component_children(iterator it)
	{
	// Scan the entries of a single \components node for those
	// which carry the same index value for the free indices.
	// Such things can arise from e.g. A_{m} A_{m n} and the
	// rule { A_{r}=3, A_{t}=5, A_{r t}=1, A_{t t}=2 }, which
	// leads to two entries for the free index n=t.

	auto comma=tr.end(it);
	--comma;
	auto cv1=tr.begin(comma);
	while(cv1!=tr.end(comma)) {
		auto iv1=tr.begin(cv1); // index values \comma
		auto cv2=cv1;
		++cv2;
		while(cv2!=tr.end(comma)) {
			auto iv2=tr.begin(cv2); // index values \comma
			if(tr.equal_subtree(iv1, iv2)) {
				Ex::sibling_iterator tv1=iv1; // tensor component value
				++tv1;
				Ex::sibling_iterator tv2=iv2;
				++tv2;
				// std::cerr << "need to merge" << std::endl;
				tv1=tr.wrap(tv1, str_node("\\sum"));
				tr.append_child(tv1, tv2);
				cv2=tr.erase(cv2);
				}
			else ++cv2;
			}
		++cv1;
		}
	}

void evaluate::merge_components(iterator it1, iterator it2)
	{
	// Merge two component nodes which come from two terms in a sum (so that
	// we can be assured that the free indices match; they just may not be
	// in the same order).

	assert(*it1->name=="\\components");
	assert(*it2->name=="\\components");
	sibling_iterator sib1=tr.end(it1);
	--sib1;
	sibling_iterator sib2=tr.end(it2);
	--sib2;

	// tr.print_recursive_treeform(std::cerr, tr.begin());

	// We cannot directly compare the lhs of this equals node with the lhs
	// of the equals node of the other components node, because the index
	// order on the two components nodes may be different. We first
	// have to ensure that the orders are the same (but only, of course)
	// if we have anything to permutate in the first place.

	if(*tr.begin(it1)->name!="\\comma") {
		//std::cerr << "merging for " << *tr.begin(it1)->name << std::endl;
		Perm perm;
		perm.find(tr.begin(it2), sib2, tr.begin(it1), sib1);
//	for(auto p: perm.perm)
//		std::cerr << p << " ";
//	std::cerr << std::endl;
		
		//perm.apply(tr.begin(it2), sib2);
		//std::cerr << "after permutation" << std::endl;
		//tr.print_recursive_treeform(std::cerr, tr.begin());
		
		cadabra::do_list(tr, sib2, [&](Ex::iterator nd) {
				auto lhs2 = tr.begin(nd);
				perm.apply(tr.begin(lhs2), tr.end(lhs2));
				return true;
				});
		}
//	else std::cerr << "merging scalars" << std::endl;


	// Now all index orders match and we can simply compare index value sets.

	cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
			assert(*it2->name=="\\equals");

			auto lhs2 = tr.begin(it2);
			auto found = cadabra::find_in_list(tr, sib1, [&](Ex::iterator it1) {

					auto lhs1 = tr.begin(it1);
					//std::cerr << "comparing " << *lhs1->name << " with " << *lhs2->name << std::endl;
					if(tr.equal_subtree(lhs1, lhs2)) {
						auto sum1=lhs1;
						++sum1;
						auto sum2=lhs2;
						++sum2;
						if(*sum1->name!="\\sum") 
							sum1=tr.wrap(sum1, str_node("\\sum"));
						tr.append_child(sum1, sum2);
						return iterator(sum1);
						}

					return tr.end();
					});
			if(found==tr.end()) {
				tr.append_child(iterator(sib1), it2);
				}
			return true;
			});

	// Simplify the component by calling sympy.

	cadabra::do_list(tr, sib1, [&](Ex::iterator it1) {
			assert(*it1->name=="\\equals");
			auto rhs1 = tr.begin(it1);
			++rhs1;
			iterator nd=rhs1;
			sympy::apply(kernel, tr, nd, "", "", "");
			return true;
			});
	}

void evaluate::cleanup_components(iterator it) 
	{
	sibling_iterator sib=tr.end(it);
	--sib;

	cadabra::do_list(tr, sib, [&](Ex::iterator nd) {
			auto iv=tr.begin(nd);
			++iv;
			iterator p=iv;
			cleanup_dispatch(kernel, tr, p);
			return true;
			});
	}

void evaluate::handle_derivative(iterator it)
	{
	// In order to figure out which components to keep, we need to do two things:
	// expand into components the argument of the derivative, and then
	// figure out the dependence of that argument on the various coordinates.
	// There may be other orders (for e.g. situations where we want to keep entire
	// components unevaluated), but that's for later when we have practical use cases.
	
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->is_index()==false) {
			index_map_t empty;
			handle_factor(sib, empty);
			break;
			}
		++sib;
		}
	assert(sib!=tr.end(it));
	
	// Walk all the index value sets of the \components node inside the
	// argument.  For each, determine the dependencies, and generate
	// one element for each dependence.

	sibling_iterator ivalues = tr.end(sib);
	--ivalues;

	cadabra::do_list(tr, ivalues, [&](Ex::iterator iv) {
			sibling_iterator rhs = tr.begin(iv);
			++rhs;
			auto deps=dependencies(rhs);

			// FIXME: all indices on \partial can take any of the values of the 
			// dependencies. Need all permutations. 
			assert(number_of_direct_indices(it)==1);

			for(auto& obj: deps) {
				Ex eqcopy(iv);
				auto lhs=eqcopy.begin(eqcopy.begin());
				assert(*lhs->name=="\\comma");
				eqcopy.append_child(iterator(lhs), obj.begin());
				++lhs;
				multiplier_t mult=*lhs->multiplier;
				one(lhs->multiplier);
				lhs=eqcopy.wrap(lhs, str_node("\\partial"));
				multiply(lhs->multiplier, mult);
				multiply(lhs->multiplier, *it->multiplier);
				auto pch=tr.begin(it);
				auto arg=tr.begin(lhs);
				// FIXME: as above: need all permutations.
				while(pch!=tr.end(it)) {
					if(pch->is_index()) 
						eqcopy.insert_subtree(arg, obj.begin())->fl.parent_rel=str_node::p_sub;
					++pch;
					}
				tr.move_before(tr.begin(ivalues), eqcopy.begin());
				}
			tr.erase(iv);
			return true;
			});

	// Now move the partial indices to the components node, and then unwrap the
	// partial node.
	auto pch=tr.begin(it);
	while(pch!=tr.end(it)) {
		tr.move_before(ivalues, pch);
		++pch;
		}
	it=tr.flatten_and_erase(it);
	}

std::set<Ex, tree_exact_less_obj> evaluate::dependencies(iterator it)
	{
	tree_exact_less_obj comp(&kernel.properties);
	std::set<Ex, tree_exact_less_obj> ret(comp);

	// Determine explicit dependence on Coordinates.
	cadabra::do_subtree(tr, it, [&](Ex::iterator nd) {
			const Coordinate *cd = kernel.properties.get<Coordinate>(nd);
			if(cd) {
				Ex cpy(nd);
				cpy.begin()->fl.bracket=str_node::b_none;
				cpy.begin()->fl.parent_rel=str_node::p_none;
				ret.insert(cpy);
				}
			});

	// Determine implicit dependence via Depends.
	const Depends *dep = kernel.properties.get<Depends>(it);
	if(dep) {
		Ex deps(dep->dependencies(kernel, it));
		cadabra::do_list(deps, deps.begin(), [&](Ex::iterator nd) {
				Ex cpy(nd);
				cpy.begin()->fl.bracket=str_node::b_none;
				cpy.begin()->fl.parent_rel=str_node::p_none;
				ret.insert(cpy);
				return true;
				});
		}

	return ret;
	}

void evaluate::handle_prod(iterator it)
	{
	// All factors are either \component nodes, pure scalar nodes, or nodes which still need replacing.
	// The handle_factor function takes care of the latter two.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;

		index_map_t empty;
		handle_factor(sib, empty);

		sib=nxt;
		}
	
	// Now everything is a \component node.  The thing is effectively a
	// large sparse tensor product. We need to do the sums over the
	// dummy indices, turning this into a single \component node.
	
	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);

	auto di = ind_dummy.begin();
	// Since no factor can be a sum anymore, dummy indices always occur in pairs,
	// there is no need to account for anything more tricky. Every pair leads
	// to a sum.
	while(di!=ind_dummy.end()) {
		auto di2=di;
		++di2;
		int num1 = tr.index(di->second);
		int num2 = tr.index(di2->second);
		// std::cerr << *(di->first.begin()->name) 
		//           << " is index " << num1 << " in first and index " << num2 << " in second node " << std::endl;

		// three cases:
		//    two factors, single index in common. Merge is simple.
		//    two factors, more than one index in common. After merging this turns into:
		//    single factor, self-contraction

		auto cit1 = tr.parent(di->second);
		auto cit2 = tr.parent(di2->second);

		// Are the components objects cit1, cit2 on which these indices sit the same one?
		if(cit1 != cit2) {
			// Walk through all components of the first tensor, and for each check whether
			// any of the components of the second tensor matches the value for this dummy
			// index.
			sibling_iterator sib1=tr.end(cit1);
			--sib1;
			sibling_iterator sib2=tr.end(cit2);
			--sib2;

			// Move all indices of the second tensor to be indices of the first.
			sibling_iterator mv=tr.begin(cit2);
			while(mv!=sib2) {
				sibling_iterator nxt=mv;
				++nxt;
				tr.move_before(sib1, mv);
				mv=nxt;
				}

			cadabra::do_list(tr, sib1, [&](Ex::iterator it1) {
					assert(*it1->name=="\\equals");
					auto lhs1 = tr.begin(it1);
					auto ivalue1 = tr.begin(lhs1);
					ivalue1 += num1;
					cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
							assert(*it2->name=="\\equals");
							auto lhs2 = tr.begin(it2);
							auto ivalue2 = tr.begin(lhs2);
							ivalue2 += num2;

							// Compare the two index values in the two tensors, only continue if
							// these are the same.
							// std::cerr << "comparing value " << *ivalue1->name << " with " << *ivalue2->name << std::endl;
							// std::cerr << "                " << &(*ivalue1) << " vs " << &(*ivalue2) << std::endl;
							if(tr.equal_subtree(ivalue1,ivalue2)) {
								// Create new merged index value set.
								Ex ivs("\\equals");
								auto ivs_lhs = tr.append_child(ivs.begin(), str_node("\\comma"));
								auto ivs_rhs = tr.append_child(ivs.begin(), str_node("\\prod"));
								auto ci = tr.begin(lhs1);
								int n=0;
								while(ci!=tr.end(lhs1)) {
									if(n!=num1)
										ivs.append_child(ivs_lhs, iterator(ci));
									++ci; ++n;
									}
								ci = ivs.begin(lhs2);
								n=0;
								while(ci!=ivs.end(lhs2)) {
									if(n!=num2) 
										ivs.append_child(ivs_lhs, iterator(ci));
									++ci; ++n;
									}
								auto rhs1=lhs1;
								++rhs1;
								ivs.append_child(ivs_rhs, iterator(rhs1));
								auto rhs2=lhs2;
								++rhs2;
								ivs.append_child(ivs_rhs, iterator(rhs2));			
								cleanup_dispatch_deep(kernel, ivs);
								// Insert this new index value set before sib1, so that it will not get used
								// inside the outer loop.
								tr.move_before(it1, ivs.begin());
								}
							return true;
							});
					// This index value set can now be erased as all
					// possible combinations have been considered.
					tr.erase(it1);
					return true;
					});
			// Remove the dummy indices from the index set of tensor 1.
			tr.erase(di->second);
			tr.erase(di2->second);
			// Tensor 2 can now be removed from the product as well, as all information is now
			// part of tensor 1.
			tr.erase(cit2);
			}

		else {
			// Components objects cit1 and cit2 are actually the same. We just need to
			// do a single loop now, going over all index value sets and keeping those
			// for which the num1-th and num2-th value are identical.

			sibling_iterator sib1=tr.end(cit1);
			--sib1;

			cadabra::do_list(tr, sib1, [&](Ex::iterator it1) {
					assert(*it1->name=="\\equals");
					auto lhs = tr.begin(it1);
					auto ivalue1 = tr.begin(lhs);
					auto ivalue2 = ivalue1;
					ivalue1 += num1;
					ivalue2 += num2;
					if(tr.equal_subtree(ivalue1,ivalue2)) {
						tr.erase(ivalue1);
						tr.erase(ivalue2);
						}
					else {
						tr.erase(it1);
						}
					return true;
					}
				);
			tr.erase(di->second);
			tr.erase(di2->second);
			}			

		++di; ++di;
		}

	// FIXME: At this stage we have one or more components nodes in the product.
	// We need to do an outer multiplication, merging all index names into
	// one, and computing tensor component values for all possible index values.

	int n=tr.number_of_children(it);
	//std::cerr << Ex(it) << std::endl;
	if(n>1) {
		//std::cerr << "merging" << std::endl;
		auto first=tr.begin(it); // component node
		auto other=first;
		++other;
		auto fi=tr.end(first);
		--fi;
		// Add the free indices of 'other' to 'first'.
		while(other!=tr.end(it)) {
			auto oi=tr.begin(other);
			while(oi!=tr.end(other)) {
				if(oi->is_index()==false)
					break;
				tr.insert_subtree(fi, oi);
				++oi;
				}
			++other;
			}

		// Now do an outer combination of all possible indexvalue/componentvalue 
		// in the various component nodes.
		auto comma1=tr.end(first);
		--comma1;
		other=first;
		++other;
		while(other!=tr.end(it)) {
			Ex newcomma("\\comma"); // List of index value combinations and associated component values
			auto comma2=tr.end(other);
			--comma2;
			assert(*comma1->name=="\\comma");
			assert(*comma2->name=="\\comma");
			auto eq1=tr.begin(comma1);    // The \equals node
			while(eq1!=tr.end(comma1)) {
				auto eq2=tr.begin(comma2);
				while(eq2!=tr.end(comma2)) {
					// Collect all index values.
					auto neq = newcomma.append_child(newcomma.begin(), str_node("\\equals"));
					auto ncm = newcomma.append_child(neq, str_node("\\comma")); // List of index values
					auto iv=tr.begin(tr.begin(eq1));
					while(iv!=tr.end(tr.begin(eq1))) {
						newcomma.append_child(ncm, iterator(iv));
						++iv;
						}
					iv=tr.begin(tr.begin(eq2));
					while(iv!=tr.end(tr.begin(eq2))) {
						newcomma.append_child(ncm, iterator(iv));
						++iv;
						}
					// Multiply component values.
					Ex prod("\\prod");
					iv=tr.end(eq1);
					--iv;
					prod.append_child(prod.begin(), iterator(iv));
					iv=tr.end(eq2);
					--iv;
					prod.append_child(prod.begin(), iterator(iv));
					cleanup_dispatch_deep(kernel, prod);
					newcomma.append_child(neq, prod.begin());
					++eq2;
					}
				++eq1;
				}
			// Now replace the original comma1 node with newcomma, and re-iterate if there
			// are further factors in the tensor product.
			comma1 = tr.move_ontop(iterator(comma1), newcomma.begin());
			other=tr.erase(other);
			}
//		std::cerr << Ex(it) << std::endl;
		}

	// At this stage, there should be only one factor in the product, which 
	// should be a \components node. We do a cleanup, after which it should be
	// at the 'it' node.
	cleanup_dispatch(kernel, tr, it);
	assert(*it->name=="\\components");

	// We may have duplicate index value entries; merge them.
	merge_component_children(it);

	// Simplify the components of the now single \component node by calling sympy.
	// We just feed it the input; we do not call 'simplify'.
	sibling_iterator lst = tr.end(it);
	--lst;
	
	cadabra::do_list(tr, lst, [&](Ex::iterator eqs) {
			assert(*eqs->name=="\\equals");
			auto rhs1 = tr.begin(eqs);
			++rhs1;
			iterator nd=rhs1;
			sympy::apply(kernel, tr, nd, "", "", "");
			return true;
			});
	}

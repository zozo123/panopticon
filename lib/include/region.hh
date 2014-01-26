#include <type_traits>

#include <boost/range/any_range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range.hpp>
#include <boost/icl/right_open_interval.hpp>
#include <boost/optional.hpp>
#include <boost/icl/split_interval_map.hpp>

#include <panopticon/marshal.hh>
#include <panopticon/loc.hh>
#include <panopticon/digraph.hh>

#pragma once


namespace po
{
	using offset = uint64_t;
	using tryte = boost::optional<uint8_t>;
	using bound = boost::icl::discrete_interval<offset>;
	using slab = boost::any_range<tryte,boost::random_access_traversal_tag,tryte,std::ptrdiff_t>;

	struct map_layer
	{
		map_layer(const std::string &, std::function<tryte(tryte)> fn);

		bool operator==(const map_layer&) const;

		slab filter(const slab&) const;
		const std::string& name(void) const;

	private:
		struct adaptor
		{
			using result_type = tryte;

			adaptor(const map_layer *p = nullptr);
			tryte operator()(tryte) const;

			const map_layer *parent;
		};

		std::string _name;
		std::function<tryte(tryte)> _operation;
	};

	struct anonymous_layer
	{
		anonymous_layer(std::initializer_list<tryte> il, const std::string &n);
		anonymous_layer(offset sz, const std::string &n);

		bool operator==(const anonymous_layer &a) const;

		slab filter(const slab&) const;
		const std::string& name(void) const;

		std::vector<tryte> data;

	private:
		std::string _name;
	};

	struct mutable_layer
	{
		mutable_layer(const std::string &);

		slab filter(const slab&) const;
		const std::string& name(void) const;

		std::map<offset,tryte> data;

	private:
		std::string _name;
	};

	using layer = boost::variant<mutable_layer,anonymous_layer,map_layer>;
	using layer_loc = loc<layer>;
	using layer_wloc = wloc<layer>;

	layer_wloc operator+=(layer_wloc& a, const layer_wloc &b);

	template<>
	rdf::statements marshal(const layer*, const uuid&);

	template<>
	layer* unmarshal(const uuid&, const rdf::storage&);

	slab filter(const layer &l, const slab &s);
	std::string name(const layer& l);
}

namespace std
{
	std::ostream& operator<<(std::ostream&, const po::bound&);

	template<>
	struct hash<po::layer>
	{
		size_t operator()(const po::layer &a) const
		{
			return hash<string>()(name(a));
		}
	};

	template<>
	struct hash<po::bound>
	{
		size_t operator()(const po::bound &a) const
		{
			return hash<po::offset>()(boost::icl::first(a)) + hash<po::offset>()(boost::icl::last(a));
		}
	};
}

namespace po
{
	struct region
	{
		using image = boost::icl::interval_map<offset,layer_wloc>;
		using layers = digraph<layer_loc,bound>;

		region(const std::string&, size_t);
		void add(const bound&, layer_loc);

		const image& projection(void) const;
		const layers& graph(void) const;

		size_t size(void) const;
		slab read(void) const;
		const std::string& name(void) const;

	private:
		layers _graph;
		boost::graph_traits<digraph<layer_loc,bound>>::vertex_descriptor _root;
		std::string _name;
		size_t _size;

		// caches
		mutable boost::optional<image> _projection;

		slab read(layer_loc l) const;
	};

	template<>
	rdf::statements marshal(const region*, const uuid&);

	template<>
	region* unmarshal(const uuid&, const rdf::storage&);

	using region_loc = loc<region>;
	using region_wloc = wloc<region>;
	using regions = digraph<region_loc,bound>;

	std::unordered_map<region_wloc,region_wloc> spanning_tree(const regions&);
	std::list<std::pair<bound,region_wloc>> projection(const regions&);
}

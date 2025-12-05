module;

#include <cassert>
#include "../../../include/mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.quad_tree;

export import mo_yanxi.math.quad_tree.interface;
import std;
import mo_yanxi.utility;
import mo_yanxi.concepts;
import mo_yanxi.object_pool;
import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;
import mo_yanxi.math;

// namespace mo_yanxi::math{
// 	constexpr std::size_t get_region_count(const std::size_t layers) noexcept{
// 		return (static_cast<std::size_t>(1) << (layers * 2)) / static_cast<std::size_t>(3);
// 	}
//
// 	template <typename T>
// 	constexpr T get_region_size(T totalSize, std::size_t layers) noexcept{
// 		return totalSize / static_cast<T>(1 << (layers - 1));
// 	}
//
// 	constexpr auto layhers = get_region_size(512 * 512, 8);
// 	
// 	struct quad_tree{
// 	};
// }

namespace mo_yanxi::math{
	//TODO flattened quad tree with pre allocated sub trees
	constexpr std::size_t MaximumItemCount = 4;

	constexpr std::size_t top_lft_index = 0;
	constexpr std::size_t top_rit_index = 1;
	constexpr std::size_t bot_lft_index = 2;
	constexpr std::size_t bot_rit_index = 3;

	template <std::size_t layers>
	struct quad_tree_mdspan_trait{
		using index_type = unsigned;

		template <std::size_t>
		static constexpr std::size_t sub_size = 4;

		static consteval auto extent_impl(){
			return [] <std::size_t... I>(std::index_sequence<I...>){
				return std::extents<index_type, sub_size<I>...>{};
			}(std::make_index_sequence<layers>());
		}

		static consteval std::size_t expected_size(){
			return [] <std::size_t... I>(std::index_sequence<I...>){
				return (sub_size<I> * ...);
			}(std::make_index_sequence<layers>());
		}

		using extent_type = decltype(extent_impl());
		static constexpr std::size_t required_span_size = expected_size();
	};


	// using T = float;

	template <typename ItemTy, arithmetic T>
	struct quad_tree_evaluateable_traits{
		using vec_t = vector2<T>;
		using rect_type = rect_ortho<T>;

		static constexpr bool has_rough_intersect = requires(const ItemTy& value){
			{ value.quad_tree_rough_intersect_with(value) } -> std::same_as<bool>;
		};

		static constexpr bool has_exact_intersect = requires(const ItemTy& value){
			{ value.quad_tree_exact_intersect_with(value) } -> std::same_as<bool>;
		};

		static constexpr bool has_point_intersect = requires(const ItemTy& value, typename vec_t::const_pass_t p){
			{ value.quad_tree_contains(p) } -> std::same_as<bool>;
		};

		static rect_type bound_of(const ItemTy& cont) noexcept{
			static_assert(requires(const ItemTy& value){
				{ value.quad_tree_get_bound() } -> std::same_as<rect_type>;
			}, "QuadTree Requires ValueType impl at `Rect getBound()` member function");

			return cont.quad_tree_get_bound();
		}

		static bool is_intersected_between(const ItemTy& subject, const ItemTy& object) noexcept requires requires{
			requires derived<ItemTy, quad_tree_adaptor<ItemTy, T>>;
		}{
			//TODO equalTy support?
			if(&subject == &object) return false;

			bool intersected = quad_tree_evaluateable_traits::bound_of(subject).overlap_exclusive(quad_tree_evaluateable_traits::bound_of(object));

			if constexpr(has_rough_intersect){
				if(intersected) intersected &= subject.quad_tree_rough_intersect_with(object);
			}

			if constexpr(has_exact_intersect){
				if(intersected) intersected &= subject.quad_tree_exact_intersect_with(object);
			}

			return intersected;
		}

		static bool is_intersected_with_point(const vec_t point, const ItemTy& object) noexcept requires (has_point_intersect){
			return quad_tree_evaluateable_traits::bound_of(object).contains_loose(point) && object.quad_tree_contains(point);
		}
	};


	struct always_true{
		template <typename T>
		constexpr static bool operator()(const T&, const T&) noexcept{
			return true;
		}
	};

	template <typename ItemTy, arithmetic T = float>
	struct quad_tree_node{
		//OPTM if branch_size == 0 then skip some iteration
		using rect_type = rect_ortho<T>;
		using vec_t = vector2<T>;
		using trait = quad_tree_evaluateable_traits<ItemTy, T>;

		struct sub_nodes{
			std::array<quad_tree_node, 4> nodes{};

			[[nodiscard]] explicit sub_nodes(const quad_tree_node& parent, const std::array<rect_type, 4>& rects)
				: nodes{
					quad_tree_node{parent.node_pool, rects[bot_lft_index]},
					quad_tree_node{parent.node_pool, rects[bot_rit_index]},
					quad_tree_node{parent.node_pool, rects[top_lft_index]},
					quad_tree_node{parent.node_pool, rects[top_rit_index]},
				}{
			}

			void reserved_clear() noexcept{
				for (auto& node : nodes){
					node.reserved_clear();
				}
			}

			constexpr auto begin() noexcept{
				return nodes.begin();
			}

			constexpr auto end() noexcept{
				return nodes.end();
			}

			constexpr auto begin() const noexcept{
				return nodes.begin();
			}

			constexpr auto end() const noexcept{
				return nodes.end();
			}

			constexpr quad_tree_node& at(const unsigned i) noexcept{
				return nodes[i];
			}

			constexpr const quad_tree_node& at(const unsigned i) const noexcept{
				return nodes[i];
			}

		};

		using pool_type = single_thread_object_pool<sub_nodes>;
	protected:
		pool_type* node_pool{};
		rect_type boundary{};
		std::vector<ItemTy*> items{};
		unsigned branch_size = 0;
		bool leaf = true;

		typename pool_type::unique_ptr children{};

		[[nodiscard]] quad_tree_node() = default;

		[[nodiscard]] explicit quad_tree_node(pool_type* pool, const rect_type& boundary)
			: node_pool(pool), boundary(boundary){
		}
	public:
		[[nodiscard]] rect_type get_boundary() const noexcept{ return boundary; }

		[[nodiscard]] unsigned size() const noexcept{ return branch_size; }

		[[nodiscard]] bool is_leaf() const noexcept{ return leaf; }

		[[nodiscard]] bool has_valid_children() const noexcept{
			return !leaf && branch_size != items.size();
		}

		[[nodiscard]] auto& get_items() const noexcept{ return items; }

		[[nodiscard]] bool overlaps(const rect_type object) const noexcept{
			return boundary.overlap_exclusive(object);
		}

		[[nodiscard]] bool contains(const ItemTy& object) const noexcept{
			return boundary.contains_loose(trait::bound_of(object));
		}

		[[nodiscard]] bool overlaps(const ItemTy& object) const noexcept{
			return boundary.overlap_exclusive(trait::bound_of(object));
		}

		[[nodiscard]] bool contains(const vec_t object) const noexcept{
			return boundary.contains_loose(object);
		}

	private:
		bool withinBound(const ItemTy& object, const T dst) const noexcept{
			return trait::bound_of(object).get_center().within(this->boundary.get_center(), dst);
		}

		[[nodiscard]] bool is_children_cached() const noexcept{
			return children != nullptr;
		}

		void internalInsert(ItemTy* item){
			this->items.push_back(item);
			++this->branch_size;
		}

		void split(){
			if(!std::exchange(this->leaf, false)) return;

			if(!is_children_cached()){
				children = node_pool->obtain_unique(*this, this->boundary.split());
			}

			std::erase_if(this->items, [this](ItemTy* item){
				if(quad_tree_node* sub = this->get_wrappable_child(trait::bound_of(*item))){
					sub->internalInsert(item);
					return true;
				}

				return false;
			});
		}

		void unsplit() noexcept{
			if(std::exchange(this->leaf, true)) return;

			assert(is_children_cached());

			this->items.reserve(this->branch_size - this->items.size());
			for (quad_tree_node& node : children->nodes){
				this->items.append_range(node.items | std::views::as_rvalue);
				node.reserved_clear();
			}
		}

		[[nodiscard]] quad_tree_node* get_wrappable_child(const rect_type target_bound) const noexcept{
			assert(!this->is_leaf());
			assert(is_children_cached());

			auto [midX, midY] = this->boundary.get_center();

			// Object can completely fit within the top quadrants
			const bool topQuadrant = target_bound.get_src_y() > midY;
			// Object can completely fit within the bottom quadrants
			const bool bottomQuadrant = target_bound.get_end_y() < midY;

			// Object can completely fit within the left quadrants
			if(target_bound.get_end_x() < midX){
				if(topQuadrant){
					return &children->at(top_lft_index);
				}

				if(bottomQuadrant){
					return &children->at(bot_lft_index);
				}
			} else if(target_bound.get_src_x() > midX){
				// Object can completely fit within the right quadrants
				if(topQuadrant){
					return &children->at(top_rit_index);
				}

				if(bottomQuadrant){
					return &children->at(bot_rit_index);
				}
			}

			return nullptr;
		}

	public:
		void reserved_clear() noexcept{
			if(std::exchange(this->branch_size, 0) == 0) return;

			if(!this->leaf){
				children->reserved_clear();
				this->leaf = true;
			}

			this->items.clear();
		}

		void clear() noexcept{
			children.reset(nullptr);

			this->leaf = true;
			this->branch_size = 0;

			this->items.clear();
		}

	private:
		[[nodiscard]] constexpr bool isBranchEmpty() const noexcept{
			return this->branch_size == 0;
		}

		void insertImpl(ItemTy& box){
			//If this box is inbound, it must be added.
			++this->branch_size;

			// Otherwise, subdivide and then add the object to whichever node will accept it
			if(this->leaf){
				if(this->items.size() < MaximumItemCount){
					this->items.push_back(std::addressof(box));
					return;
				}

				split();
			}

			const rect_type rect = trait::bound_of(box);

			if(quad_tree_node* child = this->get_wrappable_child(rect)){
				child->insertImpl(box);
				return;
			}

			if(this->items.size() < MaximumItemCount * 2){
				this->items.push_back(std::addressof(box));
			}else{
				const auto [cx, cy] = rect.get_center();
				const auto [bx, by] = this->boundary.get_center();

				const bool left = cx < bx;
				const bool bottom = cy < by;

				if(bottom){
					if(left){
						children->at(bot_lft_index).insertImpl(box);
					}else{
						children->at(bot_rit_index).insertImpl(box);
					}
				}else{
					if(left){
						children->at(top_lft_index).insertImpl(box);
					}else{
						children->at(top_rit_index).insertImpl(box);
					}
				}

			}
		}

	public:
		bool insert(ItemTy& box){
			// Ignore objects that do not belong in this quad tree
			if(!this->overlaps(box)){
				return false;
			}

			this->insertImpl(box);
			return true;
		}

		bool erase(const ItemTy& box) noexcept{
			if(isBranchEmpty()) return false;

			bool result;
			if(this->is_leaf()){
				result = static_cast<bool>(std::erase_if(this->items, [ptr = std::addressof(box)](const ItemTy* o){
					return o == ptr;
				}));
			} else{
				if(quad_tree_node* child = this->get_wrappable_child(trait::bound_of(box))){
					result = child->erase(box);
				} else{
					result = static_cast<bool>(std::erase_if(this->items, [ptr = std::addressof(box)](const ItemTy* o){
						return o == ptr;
					}));
				}
			}

			if(result){
				--this->branch_size;
				if(this->branch_size < MaximumItemCount) unsplit();
			}

			return result;
		}

		// ------------------------------------------------------------------------------
		// external usage
		// ------------------------------------------------------------------------------

		template <std::regular_invocable<ItemTy&> Func>
		void each(Func func) const{
			for(auto item : this->items){
				std::invoke(func, *item);
			}

			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.each(func);
				}
			}
		}

		template <std::regular_invocable<quad_tree_node&> Func>
		void each(Func func) const{
			std::invoke(func, *this);

			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.each(mo_yanxi::pass_fn(func));
				}
			}
		}


		[[nodiscard]] ItemTy* intersect_any(const ItemTy& object) const noexcept{
			if(isBranchEmpty() || !this->overlaps(object)){
				return nullptr;
			}

			for(const auto item : this->items){
				if(trait::is_intersected_between(object, *item)){
					return item;
				}
			}

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					if(auto rst = node.intersect_any(object)) return rst;
				}
			}

			// Otherwise, the rectangle does not overlap with any rectangle in the quad tree
			return nullptr;
		}

		[[nodiscard]] ItemTy* intersect_any(const vec_t point) const noexcept requires (trait::has_point_intersect){
			if(isBranchEmpty() || !this->contains(point)){
				return nullptr;
			}

			for(const auto item : this->items){
				if(trait::is_intersected_with_point(point, *item)){
					return item;
				}
			}

			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					if(auto rst = node.intersect_any(point)) return rst;
				}
			}

			return nullptr;
		}

		[[nodiscard]] ItemTy* intersect_any(const rect_type region) const noexcept{
			if(isBranchEmpty() || !this->overlaps(region)){
				return nullptr;
			}

			for(const auto item : this->items){
				if(trait::bound_of(*item).overlap_exclusive(region)){
					return item;
				}
			}

			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					if(auto rst = node.intersect_any(region)) return rst;
				}
			}

			return nullptr;
		}

		template <
			std::invocable<ItemTy&, ItemTy&> Func,
			std::predicate<ItemTy&, ItemTy&> Filter = decltype(trait::is_intersected_between)>
		void intersect_test_all(ItemTy& object, Func func, Filter filter = trait::is_intersected_between) const{
			if(isBranchEmpty() || !this->overlaps(object)) return;

			for(ItemTy* element : this->items){
				if(std::invoke(mo_yanxi::pass_fn(filter), object, *element)){
					std::invoke(mo_yanxi::pass_fn(func), object, *element);
				}
			}

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node& node : children->nodes){
					node.intersect_test_all(object, mo_yanxi::pass_fn(func), mo_yanxi::pass_fn(filter));
				}
			}
		}

	private:
		template <std::predicate<const ItemTy&, const ItemTy&> Filter>
		void get_all_intersected_impl(const ItemTy& object, Filter filter, std::vector<ItemTy*>& out) const{
			if(isBranchEmpty() || !this->overlaps(object)) return;

			for(const auto element : this->items){
				if(trait::is_intersected_between(object, *element)){
					if(std::invoke(filter, object, *element)){
						out.push_back(element);
					}
				}
			}

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.get_all_intersected_impl(object, filter, out);
				}
			}
		}

	public:
		template <std::predicate<const ItemTy&, const ItemTy&> Filter>
		[[nodiscard]] std::vector<ItemTy*> get_all_intersected(const ItemTy& object, Filter filter) const{
			std::vector<ItemTy*> rst{};
			rst.reserve(this->branch_size / 4);

			this->get_all_intersected_impl<Filter>(object, mo_yanxi::pass_fn(filter), rst);
			return rst;
		}

		template <std::invocable<ItemTy&> Func>
		void intersect_then(const ItemTy& object, Func func) const{
			if(isBranchEmpty() || !this->overlaps(object)) return;

			for(auto* cont : this->items){
				if(trait::is_intersected_between(object, *cont)){
					std::invoke(mo_yanxi::pass_fn(func), *cont);
				}
			}

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.intersect_then(object, mo_yanxi::pass_fn(func));
				}
			}

		}

		template <std::invocable<ItemTy&, rect_type> Func>
		void intersect_then(const rect_type rect, Func func) const{
			if(isBranchEmpty() || !this->overlaps(rect)) return;

			//If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.intersect_then(rect, mo_yanxi::pass_fn(func));
				}
			}

			for(auto* cont : this->items){
				if(trait::bound_of(*cont).overlap_Exclusive(rect)){
					std::invoke(mo_yanxi::pass_fn(func), *cont, rect);
				}
			}
		}

		template <typename Region, std::predicate<const rect_type&, const Region&> Pred, std::invocable<ItemTy&, const Region&> Func>
			requires (!std::same_as<Region, rect_type>)
		void intersect_then(const Region& region, Pred boundCheck, Func func) const{
			if(isBranchEmpty() || !std::invoke(boundCheck, this->boundary, region)) return;

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.template intersect_then<Region>(region, mo_yanxi::pass_fn(boundCheck), mo_yanxi::pass_fn(func));
				}
			}

			for(auto cont : this->items){
				if(std::invoke(mo_yanxi::pass_fn(boundCheck), trait::bound_of(*cont), region)){
					std::invoke(mo_yanxi::pass_fn(func), *cont, region);
				}
			}
		}

		template <std::regular_invocable<ItemTy&, vec_t> Func>
		void intersect_then(const vec_t point, Func func) const{
			if(isBranchEmpty() || !this->contains(point)) return;

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.intersect_then(point, mo_yanxi::pass_fn(func));
				}
			}

			for(auto cont : this->items){
				if(trait::is_intersected_with_point(point, *cont)){
					std::invoke(mo_yanxi::pass_fn(func), *cont, point);
				}
			}
		}

		template <std::invocable<ItemTy&> Func>
		void within(const ItemTy& object, const T dst, Func func) const{
			if(isBranchEmpty() || !this->withinBound(object, dst)) return;

			// If this node has children, check if the rectangle overlaps with any rectangle in the children
			if(this->has_valid_children()){
				for (const quad_tree_node & node : children->nodes){
					node.within(object, dst, mo_yanxi::pass_fn(func));
				}
			}

			for(const auto* cont : this->items){
				std::invoke(mo_yanxi::pass_fn(func), *cont);
			}
		}
	};

	template <typename ItemTy, arithmetic T = float>
	struct quad_tree_root_base{
	protected:
		using node = quad_tree_node<ItemTy, T>;
		typename node::pool_type pool{};
	};

	export
	template <typename ItemTy, arithmetic T = float>
	struct quad_tree : quad_tree_root_base<ItemTy, T>, quad_tree_node<ItemTy, T>{
	private:
		using base = quad_tree_node<ItemTy, T>;

	public:
		[[nodiscard]] explicit quad_tree(typename base::rect_type boundary)
			: base(&this->pool, boundary)
		{

		}
	};

	// export
	template <typename ItemTy, arithmetic T = typename ItemTy::coordinate_type>
	struct quad_tree_with_buffer : quad_tree_node<ItemTy, T>{
	private:
		using buffer = std::vector<ItemTy*>;
		buffer toInsert{};
		buffer toRemove{};

		std::mutex insertMutex{};
		std::mutex removeMutex{};
		std::mutex consumeMutex{};

	public:
		void push_insert(ItemTy& item) noexcept{
			std::lock_guard guard{insertMutex};
			toInsert.push_back(item);
		}

		void push_remove(ItemTy& item) noexcept{
			std::lock_guard guard{removeMutex};
			toRemove.push_back(item);
		}

		void consume_buffer(){
			buffer inserts, removes;

			{
				std::lock_guard guard{insertMutex};
				inserts = std::move(toInsert);
			}

			{
				std::lock_guard guard{removeMutex};
				removes = std::move(toRemove);
			}

			std::lock_guard guard{consumeMutex};

			for(auto val : inserts){
				quad_tree_node<ItemTy, T>::insert(*val);
			}

			for(auto val : removes){
				quad_tree_node<ItemTy, T>::erase(*val);
			}
		}


		/**
		 * @warning Call this function if guarantees to be thread safe
		 */
		void consume_buffer_guaranteed(){
			for(auto val : toInsert){
				quad_tree_node<ItemTy, T>::insert(*val);
			}

			for(auto val : toRemove){
				quad_tree_node<ItemTy, T>::erase(*val);
			}


			toInsert.clear();
			toRemove.clear();
		}
	};

	//TODO quad tree with thread safe insert / remove buffer
}

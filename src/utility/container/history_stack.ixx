module;

#include <cassert>

export module mo_yanxi.history_stack;

import std;

namespace mo_yanxi{

export
template <typename T, typename Container = std::deque<T>>
struct history_stack{
	using container_type = Container;
	using value_type = std::ranges::range_value_t<container_type>;
	using size_type = typename container_type::size_type;
	using iterator_type = typename container_type::iterator;

	static_assert(std::is_same_v<T, value_type>);
	static_assert(std::ranges::bidirectional_range<container_type>);
	static_assert(std::bidirectional_iterator<iterator_type>);

private:
	size_type capacity_{64};
	container_type entries_{};
	iterator_type current_entry_{std::ranges::begin(entries_)};

	[[nodiscard]] constexpr size_type distance_to_current() const noexcept{
		return std::ranges::distance(current_entry_, std::ranges::end(entries_));
	}

	[[nodiscard]] constexpr size_type distance_to_initial() const noexcept{
		return std::ranges::distance(std::ranges::begin(entries_), current_entry_);
	}

	constexpr void relocate_current_to_last() noexcept{
		if(std::ranges::empty(entries_)){
			current_entry_ = std::ranges::begin(entries_);
		} else{
			current_entry_ = std::ranges::prev(std::ranges::end(entries_));
		}
	}

public:
	[[nodiscard]] constexpr history_stack() = default;

	template <typename... Args>
	[[nodiscard]] constexpr explicit(sizeof...(Args) == 0) history_stack(size_type capacity, Args&&... args)
	: capacity_(capacity), entries_{std::forward<Args>(args)...}{
	}

	[[nodiscard]] constexpr bool empty() const noexcept{
		return std::ranges::empty(entries_);
	}

	[[nodiscard]] constexpr auto size() const noexcept{
		return std::ranges::size(entries_);
	}

	constexpr void push(value_type&& val) noexcept(std::is_nothrow_move_constructible_v<value_type>){
		truncate();

		entries_.push_back(std::move(val));

		if(std::ranges::size(entries_) > capacity_){
			entries_.pop_front();
		}

		relocate_current_to_last();
	}

	constexpr void push(const value_type& val) requires (std::is_copy_constructible_v<value_type>){
		this->push(value_type{val});
	}

	template <typename... Args>
		requires (std::constructible_from<value_type, Args&&...>)
	constexpr void emplace(Args&&... args){
		this->push(value_type{std::forward<Args>(args)...});
	}

	[[nodiscard]] constexpr bool has_prev() const noexcept{
		return current_entry_ != std::ranges::begin(entries_);
	}

	[[nodiscard]] constexpr bool has_next() const noexcept{
		return !std::ranges::empty(entries_) && current_entry_ != std::ranges::prev(std::ranges::end(entries_));
	}

	constexpr const value_type* to_prev() noexcept{
		if(current_entry_ == std::ranges::begin(entries_)) return nullptr;
		return std::to_address(current_entry_--);
	}

	constexpr const value_type* to_next() noexcept{
		if(std::ranges::empty(entries_) || current_entry_ == std::ranges::prev(std::ranges::end(entries_))) return
			nullptr;
		return std::to_address(current_entry_++);
	}

	[[nodiscard]] constexpr const value_type& current() const noexcept{
		assert(!std::ranges::empty(entries_));
		return *current_entry_;
	}

	[[nodiscard]] constexpr const value_type* try_get() const noexcept{
		if(std::ranges::empty(entries_)) return nullptr;
		return std::to_address(current_entry_);
	}

	constexpr void pop() noexcept{
		if(!std::ranges::empty(entries_)){
			entries_.pop_back();
		}

		relocate_current_to_last();
	}

	constexpr std::optional<value_type> pop_and_get() noexcept{
		if(std::ranges::empty(entries_)){
			return std::nullopt;
		}

		std::optional<value_type> v{std::move(*std::ranges::rbegin(entries_))};

		entries_.pop_back();
		relocate_current_to_last();

		return v;
	}

	constexpr void clear() noexcept{
		entries_.clear();
		current_entry_ = std::ranges::begin(entries_);
	}

	constexpr void truncate() noexcept{
		if(current_entry_ == std::ranges::end(entries_)) return;
		entries_.erase(std::ranges::next(current_entry_), std::ranges::end(entries_));
	}
};

export
template <typename T, typename Container = std::deque<T>>
struct procedure_history_stack{
	using container_type = Container;
	using value_type = std::ranges::range_value_t<container_type>;
	using size_type = typename container_type::size_type;
	using iterator_type = typename container_type::iterator;

	static_assert(std::is_same_v<T, value_type>);
	static_assert(std::ranges::bidirectional_range<container_type>);
	static_assert(std::bidirectional_iterator<iterator_type>);

private:
	size_type capacity_{64};
	container_type entries_{};

	// current_entry_ 现在作为一个“游标”。
	// 它指向“下一个可以 Redo 的元素”。
	// 如果它等于 end()，说明没有由于 Undo 而产生的待重做项（即处于最新状态）。
	iterator_type current_entry_{std::ranges::begin(entries_)};

public:
	[[nodiscard]] constexpr procedure_history_stack() = default;

	template <typename... Args>
	[[nodiscard]] constexpr explicit(sizeof...(Args) == 0) procedure_history_stack(size_type capacity, Args&&... args)
	: capacity_(capacity), entries_{std::forward<Args>(args)...}{
		// 初始化时，默认处于最新状态，指向 end()
		current_entry_ = std::ranges::end(entries_);
	}

	[[nodiscard]] constexpr bool empty() const noexcept{
		return std::ranges::empty(entries_);
	}

	[[nodiscard]] constexpr auto size() const noexcept{
		return std::ranges::size(entries_);
	}


	const container_type& get_base() const noexcept{
		return entries_;
	}
	// --- Push 操作 ---

	constexpr void push(value_type&& val) noexcept(std::is_nothrow_move_constructible_v<value_type>){
		// 1. 如果当前不在末尾，说明中间有被撤销的操作，需要截断（Discard Redo history）
		truncate();

		// 2. 推入新操作
		entries_.push_back(std::move(val));

		// 3. 检查容量
		if(std::ranges::size(entries_) > capacity_){
			entries_.pop_front();
			// 注意：deque 的 pop_front 会使所有迭代器失效，需要重新定位
			// 但因为我们刚 push 且 truncate 过，当前位置一定是 end()
		}

		// 4. 将游标重置为 end()，表示最新状态
		current_entry_ = std::ranges::end(entries_);
	}

	constexpr void push(const value_type& val) requires (std::is_copy_constructible_v<value_type>){
		this->push(value_type{val});
	}

	template <typename... Args>
		requires (std::constructible_from<value_type, Args&&...>)
	constexpr void emplace(Args&&... args){
		this->push(value_type{std::forward<Args>(args)...});
	}

	// --- 状态查询 ---

	// 是否可以撤销：只要游标不在 begin()，说明前面有操作可以撤销
	[[nodiscard]] constexpr bool has_prev() const noexcept{
		return current_entry_ != std::ranges::begin(entries_);
	}

	// 是否可以重做：只要游标不在 end()，说明后面有操作可以重做
	[[nodiscard]] constexpr bool has_next() const noexcept{
		return current_entry_ != std::ranges::end(entries_);
	}

	// --- 核心移动操作 ---

	// 撤销 (Undo)：返回“上一个”操作，并将游标左移
	constexpr const value_type* to_prev() noexcept{
		if(current_entry_ == std::ranges::begin(entries_)) return nullptr;

		// 移动游标：从 "end" (或某位置) 移到 "要被撤销的元素" 上
		--current_entry_;
		return std::to_address(current_entry_);
	}

	// 重做 (Redo)：返回“当前”操作（即待重做的操作），并将游标右移
	constexpr const value_type* to_next() noexcept{
		if(current_entry_ == std::ranges::end(entries_)) return nullptr;

		// 获取当前指向的元素（待重做）
		const value_type* ptr = std::to_address(current_entry_);

		// 移动游标：指向下一个待重做元素（或者 end）
		++current_entry_;

		return ptr;
	}

	// 获取当前指向的元素（仅用于查看下一个 Redo 是什么，不移动）
	// 如果在 end()，则返回 nullptr
	[[nodiscard]] constexpr const value_type* try_get_current() const noexcept{
		if(current_entry_ == std::ranges::end(entries_)) return nullptr;
		return std::to_address(current_entry_);
	}

	// 获取刚被执行过的元素（即上一个元素，用于查看当前状态是由什么操作导致的）
	// 如果在 begin()，说明是初始状态，返回 nullptr
	[[nodiscard]] constexpr const value_type* try_get_applied() const noexcept{
		if(current_entry_ == std::ranges::begin(entries_)) return nullptr;
		return std::to_address(std::prev(current_entry_));
	}

	// --- 其他辅助 ---

	constexpr void clear() noexcept{
		entries_.clear();
		current_entry_ = std::ranges::end(entries_);
	}

	// 截断：删除从 current_entry_ 开始到 end 的所有元素
	constexpr void truncate() noexcept{
		if(current_entry_ == std::ranges::end(entries_)) return;

		// erase 返回的是删除后的下一个有效迭代器（即 end()）
		current_entry_ = entries_.erase(current_entry_, std::ranges::end(entries_));
	}
};
}

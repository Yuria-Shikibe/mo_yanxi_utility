module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.flat_set;

import std;

namespace mo_yanxi{
// 定义一个概念，要求容器支持 push_back, pop_back, back 以及随机/双向访问
template<typename C>
concept sequence_container = requires(C c, typename C::value_type v) {
    c.push_back(v);
    c.pop_back();
    c.back();
    requires std::ranges::random_access_range<C> || std::ranges::bidirectional_range<C>;
};

export
template <
    sequence_container Container,
    typename Compare = std::equal_to<>
>
requires std::equality_comparable_with<typename Container::value_type, typename Container::value_type>
class linear_flat_set {
public:
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // --- 构造函数 ---

    constexpr linear_flat_set() noexcept(std::is_nothrow_default_constructible_v<Container>) = default;

    constexpr explicit linear_flat_set(Container c, Compare cmp = Compare{})
        noexcept(std::is_nothrow_move_constructible_v<Container>)
        : container_(std::move(c)), comparator_(cmp) {
        this->make_unique();
    }

	template <typename ...Args>
		requires(std::constructible_from<Container, Args&&...>)
	constexpr explicit linear_flat_set(Args&& ...args) : container_(std::forward<Args>(args)...) {

    }

    // --- 查 (Read) ---

    [[nodiscard]] constexpr bool contains(const value_type& value) const {
        return this->find_impl(value) != this->container_.end();
    }

    [[nodiscard]] constexpr iterator find(const value_type& value) {
        return this->find_impl(value);
    }

    [[nodiscard]] constexpr const_iterator find(const value_type& value) const {
        return this->find_impl(value);
    }

    // --- 增 (Create) ---

    constexpr bool insert(const value_type& value) {
        if (this->contains(value)) {
            return false;
        }
        this->container_.push_back(value);
        return true;
    }

    constexpr bool insert(value_type&& value) {
        if (this->contains(value)) {
            return false;
        }
        this->container_.push_back(std::move(value));
        return true;
    }

    // --- 删 (Delete) ---

	constexpr void clear() noexcept{
	    container_.clear();
    }

    // noexcept 条件：如果交换元素不抛出异常，则删除操作也不抛出异常
    constexpr bool erase(const value_type& value) noexcept(std::is_nothrow_swappable_v<value_type>) {
        auto it = this->find_impl(value);
        if (it == this->container_.end()) {
            return false;
        }

        // 优化：Swap-and-Pop
        if (it != std::prev(this->container_.end())) {
            std::ranges::swap(*it, this->container_.back());
        }
        this->container_.pop_back();
        return true;
    }

    // --- 改 (Update) ---

    constexpr bool update(const value_type& old_val, const value_type& new_val) {
        auto it = this->find_impl(old_val);
        if (it == this->container_.end()) {
            return false;
        }

        // 如果根据比较器视为相等，则直接更新 payload
        if (this->comparator_(*it, new_val)) {
            *it = new_val;
            return true;
        }

        // 检查新值冲突
        if (this->contains(new_val)) {
            return false;
        }

        *it = new_val;
        return true;
    }

	template <std::predicate<value_type&> Predicate>
	constexpr void modify_and_erase(Predicate pred) noexcept(std::is_nothrow_move_assignable_v<value_type> && std::is_nothrow_invocable_v<Predicate, value_type&>){

    	auto it = std::ranges::begin(this->container_);
    	auto end = std::ranges::end(this->container_);

    	while (it != end) {
    		if (std::invoke(pred, *it)) {
    			--end;
    			if (it != end) {
    				*it = std::ranges::iter_move(end);
    			}
    		} else {
    			++it;
    		}
    	}

    	this->container_.erase(end, this->container_.end());
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return this->container_.empty();
    }

    [[nodiscard]] constexpr size_type size() const noexcept {
        return this->container_.size();
    }

    [[nodiscard]] constexpr const Container& data() const noexcept {
        return this->container_;
    }

    constexpr iterator begin() noexcept {
        return this->container_.begin();
    }

    constexpr iterator end() noexcept {
        return this->container_.end();
    }

    constexpr const_iterator begin() const noexcept {
        return this->container_.begin();
    }

    constexpr const_iterator end() const noexcept {
        return this->container_.end();
    }

private:
    Container container_{};
    ADAPTED_NO_UNIQUE_ADDRESS Compare comparator_{};

    // 使用全限定名调用 ranges 算法
    constexpr auto find_impl(const value_type& value) {
        return std::ranges::find_if(this->container_, [&](const auto& elem) {
            return this->comparator_(elem, value);
        });
    }

    constexpr auto find_impl(const value_type& value) const {
        return std::ranges::find_if(this->container_, [&](const auto& elem) {
            return this->comparator_(elem, value);
        });
    }

    constexpr void make_unique() {
        auto curr = this->container_.begin();
        while (curr != this->container_.end()) {
            // 使用 std::next 限定名
            auto dup = std::find_if(std::next(curr), this->container_.end(), [&](const auto& e){
                return this->comparator_(*curr, e);
            });

            if (dup != this->container_.end()) {
                if (dup != std::prev(this->container_.end())) {
                    std::ranges::swap(*dup, this->container_.back());
                }
                this->container_.pop_back();
            } else {
                ++curr;
            }
        }
    }
};
}

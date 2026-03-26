module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.flat_set;

import std;

namespace mo_yanxi {

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
    typename EqualTo = std::equal_to<>,
    typename Compare = std::less<typename Container::value_type>
>
requires std::equality_comparable_with<typename Container::value_type, typename Container::value_type>
class linear_flat_set {
public:
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // --- 策略常量与约束检查 ---
    static constexpr size_type kSortedThreshold = 32;
    static constexpr size_type kLinearThreshold = 16;
    static constexpr bool supports_sorted_strategy =
        std::ranges::random_access_range<Container> &&
        std::strict_weak_order<Compare, value_type, value_type>;

    // --- 构造函数 ---
    constexpr linear_flat_set() noexcept(std::is_nothrow_default_constructible_v<Container>) = default;

    constexpr explicit linear_flat_set(Container c, EqualTo eq = EqualTo{}, Compare cmp = Compare{})
        noexcept(std::is_nothrow_move_constructible_v<Container>)
        : container_(std::move(c)), comparator_(eq), compare_(cmp) {
        this->make_unique();
        this->check_state_transition();
    }

    template <typename ...Args>
        requires(std::constructible_from<Container, Args&&...>)
    constexpr explicit linear_flat_set(Args&& ...args) : container_(std::forward<Args>(args)...) {
        this->make_unique();
        this->check_state_transition();
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
        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                auto it = std::ranges::lower_bound(this->container_, value, this->compare_);
                if (it != this->container_.end() && this->comparator_(*it, value)) {
                    return false; // 元素已存在
                }
                auto dist = std::ranges::distance(this->container_.begin(), it);
                this->container_.push_back(value);
                // 使用 rotate 将插入末尾的元素移动到正确位置，保持有序性
                std::ranges::rotate(this->container_.begin() + dist, this->container_.end() - 1, this->container_.end());
                this->check_state_transition();
                return true;
            }
        }

        if (this->contains(value)) return false;
        this->container_.push_back(value);
        this->check_state_transition();
        return true;
    }

    constexpr bool insert(value_type&& value) {
        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                auto it = std::ranges::lower_bound(this->container_, value, this->compare_);
                if (it != this->container_.end() && this->comparator_(*it, value)) {
                    return false;
                }
                auto dist = std::ranges::distance(this->container_.begin(), it);
                this->container_.push_back(std::move(value));
                std::ranges::rotate(this->container_.begin() + dist, this->container_.end() - 1, this->container_.end());
                this->check_state_transition();
                return true;
            }
        }

        if (this->contains(value)) return false;
        this->container_.push_back(std::move(value));
        this->check_state_transition();
        return true;
    }

    // --- 删 (Delete) ---
    constexpr void clear() noexcept {
        container_.clear();
        this->check_state_transition(); // 将触发回退到线性模式
    }

    constexpr bool erase(const value_type& value) noexcept(std::is_nothrow_swappable_v<value_type>) {
        auto it = this->find_impl(value);
        if (it == this->container_.end()) {
            return false;
        }

        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                // 有序模式下不可 Swap-and-Pop，必须平移覆盖
                std::ranges::move(it + 1, this->container_.end(), it);
                this->container_.pop_back();
                this->check_state_transition();
                return true;
            }
        }

        // 线性模式：Swap-and-Pop 优化
        if (it != std::prev(this->container_.end())) {
            std::ranges::swap(*it, this->container_.back());
        }
        this->container_.pop_back();
        this->check_state_transition();
        return true;
    }

    // --- 改 (Update) ---
    constexpr bool update(const value_type& old_val, const value_type& new_val) {
        auto it = this->find_impl(old_val);
        if (it == this->container_.end()) {
            return false;
        }

        if (this->comparator_(*it, new_val)) {
            *it = new_val;
            return true;
        }

        if (this->contains(new_val)) {
            return false;
        }

        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                // 更新可能会破坏严格顺序。直接删去旧值，再有序插入新值。
                std::ranges::move(it + 1, this->container_.end(), it);
                this->container_.pop_back();

                auto insert_it = std::ranges::lower_bound(this->container_, new_val, this->compare_);
                auto dist = std::ranges::distance(this->container_.begin(), insert_it);
                this->container_.push_back(new_val);
                std::ranges::rotate(this->container_.begin() + dist, this->container_.end() - 1, this->container_.end());
                return true;
            }
        }

        *it = new_val;
        return true;
    }

	template <std::predicate<value_type&> Predicate>
	 constexpr void modify_and_erase(Predicate pred) noexcept(std::is_nothrow_move_assignable_v<value_type> && std::is_nothrow_invocable_v<Predicate, value_type&>) {
    	if constexpr (supports_sorted_strategy) {
    		if (this->is_sorted_mode_) {
    			// 手动实现保序的 remove_if
			    auto first = std::ranges::begin(this->container_);
			    auto last = std::ranges::end(this->container_);

    			// 找到第一个需要删除的元素（满足 pred 返回 true）
    			while (first != last && !std::invoke(pred, *first)) {
    				++first;
    			}

    			// 如果找到了需要删除的元素，则开始数据前移
    			if (first != last) {
    				auto result = first; // result 指向将被覆盖的位置（写指针）
    				++first;             // first 继续向后遍历（读指针）

    				for (; first != last; ++first) {
    					if (!std::invoke(pred, *first)) {
    						// 如果不需要删除，则将其移动到前面的 result 位置，并推进写指针

    						*result = std::ranges::iter_move(first);
    						++result;
    					}
    				}
    				// 擦除尾部无用的元素
    				this->container_.erase(result, last);
    			}

    			this->check_state_transition();
    			return;
    		}
    	}

    	// 线性模式：允许破坏顺序的 Swap-and-Pop 逻辑
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
    	this->check_state_transition();
    }

    // --- 观察者 (Observer) ---
    [[nodiscard]] constexpr bool empty() const noexcept { return this->container_.empty(); }
    [[nodiscard]] constexpr size_type size() const noexcept { return this->container_.size(); }
    [[nodiscard]] constexpr const Container& data() const noexcept { return this->container_; }
    constexpr iterator begin() noexcept { return this->container_.begin(); }
    constexpr iterator end() noexcept { return this->container_.end(); }
    constexpr const_iterator begin() const noexcept { return this->container_.begin(); }
    constexpr const_iterator end() const noexcept { return this->container_.end(); }

    constexpr void difference_update(const linear_flat_set& other) noexcept(
            std::is_nothrow_move_assignable_v<value_type>
        ) {
        this->modify_and_erase([&other](const value_type& elem) {
            return other.contains(elem);
        });
    }

private:
    Container container_{};
    ADAPTED_NO_UNIQUE_ADDRESS EqualTo comparator_{};
    ADAPTED_NO_UNIQUE_ADDRESS Compare compare_{};
    bool is_sorted_mode_ = false; // 当前内部数据是否维持有序

    constexpr void check_state_transition() {
        if constexpr (supports_sorted_strategy) {
            if (!this->is_sorted_mode_ && this->container_.size() > kSortedThreshold) {
                std::ranges::sort(this->container_, this->compare_);
                this->is_sorted_mode_ = true;
            } else if (this->is_sorted_mode_ && this->container_.size() < kLinearThreshold) {
                this->is_sorted_mode_ = false;
            }
        }
    }

    constexpr auto find_impl(const value_type& value) {
        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                auto it = std::ranges::lower_bound(this->container_, value, this->compare_);
                if (it != this->container_.end() && this->comparator_(*it, value)) {
                    return it;
                }
                return this->container_.end();
            }
        }
        return std::ranges::find_if(this->container_, [&](const auto& elem) {
            return this->comparator_(elem, value);
        });
    }

    constexpr auto find_impl(const value_type& value) const {
        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                auto it = std::ranges::lower_bound(this->container_, value, this->compare_);
                if (it != this->container_.end() && this->comparator_(*it, value)) {
                    return it;
                }
                return this->container_.end();
            }
        }
        return std::ranges::find_if(this->container_, [&](const auto& elem) {
            return this->comparator_(elem, value);
        });
    }

    constexpr void make_unique() {
        if constexpr (supports_sorted_strategy) {
            // 如果初始化时体量庞大，直接采用排序去重，时间复杂度由 O(N^2) 降级为 O(N log N)
            if (this->container_.size() > kSortedThreshold) {
                std::ranges::sort(this->container_, this->compare_);
                auto ret = std::ranges::unique(this->container_, this->comparator_);
                this->container_.erase(ret.begin(), this->container_.end());
                this->is_sorted_mode_ = true;
                return;
            }
        }

        auto curr = this->container_.begin();
        while (curr != this->container_.end()) {
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

} // namespace mo_yanxi
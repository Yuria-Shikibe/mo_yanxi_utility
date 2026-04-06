module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.flat_set;

import std;

namespace mo_yanxi {

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
                    return false;
                }
                auto dist = std::ranges::distance(this->container_.begin(), it);
                this->container_.push_back(value);
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
        this->check_state_transition();
    }

    constexpr bool erase(const value_type& value) noexcept(std::is_nothrow_swappable_v<value_type>) {
        auto it = this->find_impl(value);
        if (it == this->container_.end()) {
            return false;
        }

        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                std::ranges::move(it + 1, this->container_.end(), it);
                this->container_.pop_back();
                this->check_state_transition();
                return true;
            }
        }

        if (it != std::prev(this->container_.end())) {
            std::ranges::swap(*it, this->container_.back());
        }
        this->container_.pop_back();
        this->check_state_transition();
        return true;
    }

	constexpr iterator erase(const_iterator pos) noexcept(std::is_nothrow_swappable_v<value_type>) {
        // 将 const_iterator 安全地转换为 iterator
        auto it = this->container_.begin();
        std::ranges::advance(it, std::ranges::distance(std::as_const(this->container_).begin(), pos));

        if (it == this->container_.end()) {
            return it;
        }

        if constexpr (supports_sorted_strategy) {
            // 有序模式：保持原有相对顺序，将后续元素前移
            if (this->is_sorted_mode_) {
                std::ranges::move(std::ranges::next(it), this->container_.end(), it);
                this->container_.pop_back();
                this->check_state_transition();
                return it;
            }
        }

        // 线性/混合模式：不需要保持顺序，直接与尾部元素交换后 pop_back
        if (it != std::ranges::prev(this->container_.end())) {
            std::ranges::swap(*it, this->container_.back());
            this->container_.pop_back();
            this->check_state_transition();
            return it; // 此时 it 指向被交换过来的原本在尾部的元素
        }

        // 删除的正好是尾部元素
        this->container_.pop_back();
        this->check_state_transition();
        return this->container_.end();
    }

    // 如果需要更方便的范围删除 (erase a range)，也可以添加这个：
    constexpr iterator erase(const_iterator first, const_iterator last) {
        auto it_first = this->container_.begin();
        std::ranges::advance(it_first, std::ranges::distance(std::as_const(this->container_).begin(), first));

        auto it_last = this->container_.begin();
        std::ranges::advance(it_last, std::ranges::distance(std::as_const(this->container_).begin(), last));

        if (it_first == it_last) {
            return it_first;
        }

        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_) {
                auto new_end = std::ranges::move(it_last, this->container_.end(), it_first).out;
                this->container_.erase(new_end, this->container_.end());
                this->check_state_transition();
                return it_first;
            }
        }

        // 线性模式下批量擦除，直接调用底层容器的 erase 即可，因为反正无序
        auto ret = this->container_.erase(it_first, it_last);
        this->check_state_transition();
        return ret;
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
                auto first = std::ranges::begin(this->container_);
                auto last = std::ranges::end(this->container_);

                while (first != last && !std::invoke(pred, *first)) {
                    ++first;
                }

                if (first != last) {
                    auto result = first;
                    ++first;

                    for (; first != last; ++first) {
                        if (!std::invoke(pred, *first)) {
                            *result = std::ranges::iter_move(first);
                            ++result;
                        }
                    }
                    this->container_.erase(result, last);
                }

                this->check_state_transition();
                return;
            }
        }

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

    // --- 合并与相减 (Merge & Difference) ---

    // 拷贝版本的 merge
    constexpr void merge(const linear_flat_set& other) {
        if (this == &other) return;

        if constexpr (supports_sorted_strategy) {
            // 当双方都处于有序模式时，使用 $O(N+M)$ 的就地归并算法
            if (this->is_sorted_mode_ && other.is_sorted_mode_) {
                auto mid_idx = this->container_.size();
                for (const auto& elem : other.container_) {
                    this->container_.push_back(elem);
                }

                // iterators 必须在 push_back 后重新获取以防失效
                auto mid_it = this->container_.begin() + mid_idx;
                std::ranges::inplace_merge(this->container_.begin(), mid_it, this->container_.end(), this->compare_);

                auto ret = std::ranges::unique(this->container_, this->comparator_);
                this->container_.erase(ret.begin(), this->container_.end());
                this->check_state_transition();
                return;
            }
        }

        // 线性/混合模式降级处理：逐个查找插入
        for (const auto& elem : other.container_) {
            this->insert(elem);
        }
    }

    // 移动版本的 merge
    constexpr void merge(linear_flat_set&& other) {
        if (this == &other) return;

        if constexpr (supports_sorted_strategy) {
            if (this->is_sorted_mode_ && other.is_sorted_mode_) {
                auto mid_idx = this->container_.size();
                for (auto& elem : other.container_) {
                    this->container_.push_back(std::move(elem));
                }

                auto mid_it = this->container_.begin() + mid_idx;
                std::ranges::inplace_merge(this->container_.begin(), mid_it, this->container_.end(), this->compare_);

                auto ret = std::ranges::unique(this->container_, this->comparator_);
                this->container_.erase(ret.begin(), this->container_.end());
                other.clear();
                this->check_state_transition();
                return;
            }
        }

        for (auto& elem : other.container_) {
            this->insert(std::move(elem));
        }
        other.clear();
    }

    // 优化后的相减模式
    constexpr void difference_update(const linear_flat_set& other) noexcept(
            std::is_nothrow_move_assignable_v<value_type>
        ) {
        if (this == &other) {
            this->clear();
            return;
        }

        if constexpr (supports_sorted_strategy) {
            // 在双方皆有序的情况下，使用双指针原地擦除算法，避免额外开销
            if (this->is_sorted_mode_ && other.is_sorted_mode_) {
                auto it1 = this->container_.begin();
                auto end1 = this->container_.end();
                auto it2 = other.container_.begin();
                auto end2 = other.container_.end();

                auto out = it1; // 写入指针

                while (it1 != end1 && it2 != end2) {
                    if (this->compare_(*it1, *it2)) {
                        if (out != it1) {
                            *out = std::ranges::iter_move(it1);
                        }
                        ++out;
                        ++it1;
                    } else if (this->compare_(*it2, *it1)) {
                        ++it2; // other 中的元素更小，步进 other 的指针
                    } else {
                        // 元素相等，存在于 other 中，从当前容器剔除（直接跳过，不写入 out）
                        ++it1;
                        ++it2;
                    }
                }

                // 处理剩余不会被排除的元素
                while (it1 != end1) {
                    if (out != it1) {
                        *out = std::ranges::iter_move(it1);
                    }
                    ++out;
                    ++it1;
                }

                this->container_.erase(out, this->container_.end());
                this->check_state_transition();
                return;
            }
        }

        // 降级：线性模式通过 O(N) 遍历结合 other.contains() 判断
        this->modify_and_erase([&other](const value_type& elem) {
            return other.contains(elem);
        });
    }

    // --- 观察者 (Observer) ---
    [[nodiscard]] constexpr bool empty() const noexcept { return this->container_.empty(); }
    [[nodiscard]] constexpr size_type size() const noexcept { return this->container_.size(); }
    [[nodiscard]] constexpr const Container& data() const noexcept { return this->container_; }
    constexpr iterator begin() noexcept { return this->container_.begin(); }
    constexpr iterator end() noexcept { return this->container_.end(); }
    constexpr const_iterator begin() const noexcept { return this->container_.begin(); }
    constexpr const_iterator end() const noexcept { return this->container_.end(); }

private:
    Container container_{};
    ADAPTED_NO_UNIQUE_ADDRESS EqualTo comparator_{};
    ADAPTED_NO_UNIQUE_ADDRESS Compare compare_{};
    bool is_sorted_mode_ = false;

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

export module mo_yanxi.mpsc_queue;

import std;
import mo_yanxi.condition_variable_single;


namespace mo_yanxi{

export
template <typename T, typename Cont = std::deque<T>>
class mpsc_queue{
	static_assert(std::same_as<std::ranges::range_value_t<Cont>, T>);

public:
	using value_type = T;
	using container_type = Cont;

	[[nodiscard]] mpsc_queue() = default;

	[[nodiscard]] explicit mpsc_queue(const typename Cont::allocator_type& alloc) noexcept(std::is_nothrow_constructible_v<Cont, const typename Cont::allocator_type&>) requires requires{
		typename Cont::allocator_type;
		requires std::constructible_from<Cont, const typename Cont::allocator_type&>;
	}
		: m_queue(alloc){
	}

	template <std::predicate<const value_type&> Pred>
	void erase_if(Pred pred) noexcept {
		std::lock_guard lock(m_mutex);
		std::erase_if(m_queue, pred);
	}

	void push(T&& value) noexcept(std::is_nothrow_move_constructible_v<value_type>){
		{
			std::lock_guard lock(m_mutex);
			m_queue.push_back(std::move(value));
		}
		m_cond.notify_one();
	}

	void push(const value_type& value) noexcept(std::is_nothrow_copy_constructible_v<value_type>){
		{
			std::lock_guard lock(m_mutex);
			m_queue.push_back(value);
		}
		m_cond.notify_one();
	}

	template <typename... Args>
		requires std::constructible_from<value_type, Args...>
	void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) {
		{
			std::lock_guard lock(m_mutex);
			m_queue.emplace_back(std::forward<Args>(args)...);
		}
		m_cond.notify_one();
	}

	value_type consume() noexcept(std::is_nothrow_move_constructible_v<value_type>){
		std::unique_lock lock(m_mutex);
		m_cond.wait(lock, [this]{
			return !m_queue.empty();
		});

		const auto value = std::move(m_queue.front());
		m_queue.pop_front();

		return value;
	}

	void notify() noexcept{
		m_cond.notify_one();
	}

	template <std::predicate<> ExitPred>
	[[nodiscard]] std::optional<value_type> consume(ExitPred exit_pred) noexcept(std::is_nothrow_move_constructible_v<value_type>){
		std::unique_lock lock(m_mutex);
		m_cond.wait(lock, [&, this]{
			return !m_queue.empty() || exit_pred();
		});

		if(m_queue.empty()){
			return std::nullopt;
		}

		std::optional<value_type> value = std::move(m_queue.front());
		m_queue.pop_front();

		return value;
	}

	[[nodiscard]] std::optional<value_type> try_consume() noexcept(std::is_nothrow_move_constructible_v<Cont>){
		if(m_mutex.try_lock()){
			std::lock_guard lock(m_mutex, std::adopt_lock);
			if(m_queue.empty()){
				return std::nullopt;
			}

			std::optional value = std::move(m_queue.front());
			m_queue.pop_front();

			return value;
		}

		return std::nullopt;
	}

	[[nodiscard]] Cont dump() noexcept(std::is_nothrow_move_constructible_v<value_type>){
		std::lock_guard lock(m_mutex);

		auto alloc = m_queue.get_allocator();
		return std::exchange(m_queue, Cont{std::move(alloc)});
	}

	[[nodiscard]] bool swap(Cont& cont) noexcept(std::is_nothrow_move_constructible_v<value_type>){
		std::lock_guard lock(m_mutex);

		if(m_queue.empty())return false;
		std::swap(m_queue, cont);
		return true;
	}

private:
	Cont m_queue{};
	std::mutex m_mutex{};
	condition_variable_single m_cond{};
};
}
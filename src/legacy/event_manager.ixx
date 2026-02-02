module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.event;

import std;
import mo_yanxi.concepts;
import mo_yanxi.meta_programming;
import mo_yanxi.type_map;
import mo_yanxi.heterogeneous;


//TODO delayed event submitter
namespace mo_yanxi::events{
	template <class T>
	class def_reference_wrapper{
	public:
		static_assert(std::is_object_v<T> || std::is_function_v<T>,
			"reference_wrapper<T> requires T to be an object type or a function type.");

		using type = T;

		[[nodiscard]] constexpr def_reference_wrapper() = default;

		template <class U>
			requires (std::same_as<std::remove_cvref_t<U>, T>)
		explicit(false) constexpr def_reference_wrapper(U&& v){
			T& r = static_cast<U&&>(v);
			ptr      = std::addressof(r);
		}

		constexpr operator T&() const noexcept {
			assert(ptr);
			return *ptr;
		}

		[[nodiscard]] constexpr T& get() const noexcept {
			assert(ptr);
			return *ptr;
		}

	private:
		T* ptr{};
	};

	template <typename Ty>
	using decay_ref = std::conditional_t<std::is_reference_v<Ty>, def_reference_wrapper<std::remove_reference_t<Ty>>, std::decay_t<Ty>>;

	template <typename Tuple>
	constexpr auto decay(){
		if constexpr (std::tuple_size_v<Tuple> == 0){
			return std::tuple<>{};
		}else{
			return [] <std::size_t... Indices>(std::index_sequence<Indices...>){
				return std::tuple<decay_ref<std::tuple_element_t<Indices, Tuple>> ...>{};
			}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}
	}

	export struct event_type_tag{};


	export
	template <typename T>
	constexpr std::type_index index_of(){
		return std::type_index(typeid(T));
	}

	export
	template <typename T>
	concept event_argument = /*std::derived_from<T, eventTpe> &&*/ std::is_final_v<T> || std::is_fundamental_v<T>;

	struct EventProjection{
		template <std::ranges::range Rng>
		static decltype(auto) projection(const Rng& range){
			return range;
		}
	};

	struct event_registry{
		[[nodiscard]] event_registry() noexcept(!MO_YANXI_UTILITY_ENABLE_CHECK) = default;

#if MO_YANXI_UTILITY_ENABLE_CHECK
		std::set<std::type_index> registered{};
#endif

		[[nodiscard]] event_registry(const std::initializer_list<std::type_index> registeredList)
			noexcept(!MO_YANXI_UTILITY_ENABLE_CHECK)
#if MO_YANXI_UTILITY_ENABLE_CHECK
			: registered(registeredList)
#endif
		{}

		template <typename T>
		void checkRegister() const{
#if MO_YANXI_UTILITY_ENABLE_CHECK
			assert(registered.empty() || registered.contains(index_of<T>()));
#endif
		}

		template <typename... T>
		void registerType(){
#if MO_YANXI_UTILITY_ENABLE_CHECK
			(registered.insert(index_of<T>()), ...);
#endif
		}
	};


	template <template <typename...> typename Container = std::vector, typename Proj = EventProjection>
	struct BasicEventManager : event_registry{
	protected:
		using FuncType = std::function<void(const void*)>;
		using FuncContainer = Container<FuncType>;

		std::unordered_map<std::type_index, FuncContainer> events{};

	public:
		using event_registry::event_registry;

		template <std::derived_from<event_type_tag> T>
			requires (std::is_final_v<T>)
		void fire(const T& event) const{
			checkRegister<T>();

			if(const auto itr = events.find(index_of<T>()); itr != events.end()){
				for(const auto& listener : Proj::projection(itr->second)){
					listener(&event);
				}
			}
		}

		template <std::derived_from<event_type_tag> T, typename... Args>
			requires requires(Args&&... args){
				requires std::is_final_v<T>;
				T{std::forward<Args>(args)...};
			}
		void emplace_fire(Args&&... args) const{
			this->fire<T>(T{std::forward<Args>(args)...});
		}
	};

	export
	struct legacy_event_manager : BasicEventManager<std::vector>{
		// using BasicEventManager<std::vector>::BasicEventManager;
		template <std::derived_from<event_type_tag> T, std::invocable<const T&> Func>
			requires std::is_final_v<T>
		void on(Func&& func){
			checkRegister<T>();

			events[index_of<T>()].emplace_back([fun = std::forward<decltype(func)>(func)](const void* event){
				fun(*static_cast<const T*>(event));
			});
		}
	};

	export
	struct legacy_named_event_manager : BasicEventManager<string_hash_map, legacy_named_event_manager>{
		// using BasicEventManager<StringHashMap, NamedEventManager>::BasicEventManager;

		template <std::derived_from<event_type_tag> T, std::invocable<const T&> Func>
			requires std::is_final_v<T>
		void on(const std::string_view name, Func&& func){
			checkRegister<T>();

			events[index_of<T>()].insert_or_assign(name, [fun = std::forward<decltype(func)>(func)](const void* event){
				fun(*static_cast<const T*>(event));
			});
		}

		template <std::derived_from<event_type_tag> T>
			requires std::is_final_v<T>
		std::optional<FuncType> erase(const std::string_view name){
			checkRegister<T>();

			if(const auto itr = events.find(index_of<T>()); itr != events.end()){
				if(const auto eitr = itr->second.find(name); eitr != itr->second.end()){
					std::optional opt{eitr->second};
					itr->second.erase(eitr);
					return opt;
				}
			}

			return std::nullopt;
		}

		static decltype(auto) projection(const FuncContainer& range){
			return range | std::views::values;
		}
	};

	/**
	 * @brief THE VALUE OF THE ENUM MEMBERS MUST BE CONTINUOUS
	 * @tparam T Used Enum Type
	 * @tparam maxsize How Many Items This Enum Contains
	 */
	export
	template <mo_yanxi::enum_type T, std::underlying_type_t<T> maxsize>
	class signal_manager{
		std::array<std::vector<std::function<void()>>, maxsize> events{};

	public:
		using SignalType = std::underlying_type_t<T>;
		template <T index>
		static constexpr bool isValid = requires{
			requires static_cast<SignalType>(index) < maxsize;
			requires static_cast<SignalType>(index) >= 0;
		};

		[[nodiscard]] static constexpr SignalType max() noexcept{
			return maxsize;
		}

		void fire(const T signal){
			for(const auto& listener : events[std::to_underlying(signal)]){
				listener();
			}
		}

		template <std::invocable Func>
		void on(const T signal, Func&& func){
			events.at(std::to_underlying(signal)).push_back(std::function<void()>{std::forward<Func>(func)});
		}

		template <T signal, std::invocable Func>
			requires isValid<signal>
		void on(Func&& func){
			events.at(std::to_underlying(signal)).push_back(std::function<void()>{std::forward<Func>(func)});
		}

		template <T signal>
			requires isValid<signal>
		void fire(){
			for(const auto& listener : events[std::to_underlying(signal)]){
				listener();
			}
		}
	};

	template <typename T>
	concept non_const = !std::is_const_v<T>;

	export
	enum class CycleSignalState{
		begin, end,
	};
	export
	using CycleSignalManager = signal_manager<CycleSignalState, 2>;

	export
	template <bool allow_pmr = false>
	struct event_submitter : event_registry{
		std::unordered_map<std::type_index, std::vector<void*>> subscribers{};

		using event_registry::event_registry;

		template <non_const T, typename ArgT>
			requires requires{
				requires
				(!allow_pmr && std::convertible_to<ArgT, T> && std::same_as<std::remove_cv_t<ArgT>, std::remove_cv_t<T>>) ||
				(allow_pmr && std::derived_from<ArgT, T> && std::has_virtual_destructor_v<T>);
			}

		void submit(ArgT* ptr){
			checkRegister<std::decay_t<T>>();

			subscribers[index_of<T>()].push_back(static_cast<void*>(ptr));
		}

		template <non_const T, std::invocable<T&> Func>
			requires (!std::is_member_pointer_v<Func>)
		void invoke(Func&& func) const{
			checkRegister<std::decay_t<T>>();

			if(const auto itr = at<T>(); itr != subscribers.end()){
				for(auto second : itr->second){
					std::invoke(std::ref(std::as_const(func)), *static_cast<T*>(second));
				}
			}
		}

		template <non_const T, std::invocable<T*> Func>
		void invoke(Func&& func) const{
			checkRegister<std::decay_t<T>>();

			if(const auto itr = at<T>(); itr != subscribers.end()){
				for(auto second : itr->second){
					std::invoke(std::ref(std::as_const(func)), static_cast<T*>(second));
				}
			}
		}


		template <non_const T>
		void clear(){
			checkRegister<std::decay_t<T>>();

			if(const auto itr = at<T>(); itr != subscribers.end()){
				itr->second.clear();
			}
		}

		template <non_const T, std::invocable<T&> Func>
			requires (!std::is_member_pointer_v<Func>)
		void invoke_then_clear(Func&& func){
			checkRegister<std::decay_t<T>>();

			if(auto itr = at<T>(); itr != subscribers.end()){
				for(auto second : itr->second){
					std::invoke(std::ref(std::as_const(func)), *static_cast<T*>(second));
				}
				itr->second.clear();
			}
		}

		template <non_const T, std::invocable<T*> Func>
		void invoke_then_clear(Func&& func){
			checkRegister<std::decay_t<T>>();

			if(auto itr = at<T>(); itr != subscribers.end()){
				for(auto second : itr->second){
					std::invoke(std::ref(std::as_const(func)), static_cast<T*>(second));
				}
				itr->second.clear();
			}
		}

	private:
		template <typename T>
		decltype(subscribers)::const_iterator at() const{
			return subscribers.find(index_of<T>());
		}

		template <typename T>
		decltype(subscribers)::iterator at(){
			return subscribers.find(index_of<T>());
		}
	};
}


namespace mo_yanxi::events{
	// using event_erased_funcTpe = void(const void*);
	// using T = std::add_const_t<event_erased_funcTpe>;


	template <template<typename > typename T>
	using wrapped_func =
	T<std::conditional_t<
		std::same_as<T<void()>, std::move_only_function<void()>>,
		void(const void*) const,
		void(const void*)>>;

	template <typename T, typename Append>
	using select_wrapped_func =
	std::conditional_t<
		spec_of<T, std::move_only_function>,
		std::move_only_function<void(const void*, Append&) const>,
		std::function<void(const void*, Append&)>>;

	template <typename T, typename Append>
	using replace_wrapped_func =
	std::conditional_t<
		spec_of<T, std::move_only_function>,
		std::move_only_function<Append const>,
		std::function<Append>>;

	template <
		typename Cont,
		typename Proj,
		typename EventsTuple,
		typename ContextTuple
	>
	struct event_manager_base{
		// static_assert(requires{
		// 	typename wrapped_func<FuncWrapperTy>;
		// }, "invalid function wrapper template type");

		// using valueTpe = wrapped_func<FuncWrapperTy>;
		using containerTpe = Cont;
		using mapping_type = type_map<containerTpe, EventsTuple>;

	protected:
		using context_type = decltype(decay<ContextTuple>());
		context_type context{};
		mapping_type events{};

		ADAPTED_NO_UNIQUE_ADDRESS Proj proj{};

	public:
		template <typename ...T>
			requires std::constructible_from<context_type, T...>
		void set_context(T&& ...args) noexcept(std::is_nothrow_constructible_v<context_type, T...>){
			context = context_type{std::forward<T>(args)...};
		}

		template <event_argument T>
			// requires (mappingTpe::template isTpe_valid<T>)
		const containerTpe& group_at() const noexcept{
			return events.template at<T>();
		}

		template <event_argument T>
			// requires (mappingTpe::template isTpe_valid<T>)
		containerTpe& group_at() noexcept{
			return events.template at<T>();
		}

	public:
		template <event_argument T>
			// requires (mappingTpe::template isTpe_valid<T>)
		void fire(const T& event) const{
			//fuck msvc

			for (const auto& listener : std::invoke(proj, group_at<T>())){
				listener(const_cast<void*>(static_cast<const void*>(&event)), context);
			}
		}

		template <event_argument T>
		void operator()(const T& event) const{
			this->fire(event);
		}

	protected:
		template <typename T, typename Fn>
		static auto convert(Fn&& fn) noexcept{
			return [fun = std::forward<Fn>(fn)](const void* event, const context_type& context){
				if constexpr (std::tuple_size_v<context_type>){
					[&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
						fun(*static_cast<const T*>(event), std::get<Idx>(context)...);
					}(std::make_index_sequence<std::tuple_size_v<context_type>>{});
				}else{
					fun(*static_cast<const T*>(event));
				}
			};
		}
	};

	template <typename T, typename ...Rest>
	struct drop_first : std::type_identity<std::tuple<Rest...>>{};

	template <typename ...TupleTy>
	struct drop_first<std::tuple<TupleTy...>> : drop_first<TupleTy...>{};

	template <typename Fn>
	struct unwrap_fn{
		static_assert(false, "unsupported function type");
	};

	template <template<typename > typename Wrap, typename Fn>
	struct unwrap_fn<Wrap<Fn>>{
		using funcTpe = Fn;
	};

	template <typename Fn, typename ...Args>
	struct unwrap_fn<std::move_only_function<Fn, Args...>>{
		using funcTpe = Fn;
	};

	template <typename Fn>
	struct unwrap_fn<std::function<Fn>>{
		using funcTpe = Fn;
	};


	export
	template <typename TupleTy>
	using drop_first_t = typename drop_first<TupleTy>::type;

	export
	template <typename Wrap>
	using unwrap_fn_t = typename unwrap_fn<Wrap>::funcTpe;

	template <typename FnWrap>
	auto getFnTy(){
		using args = typename function_traits<FnWrap>::args_type;

		if constexpr (std::tuple_size_v<args> == 1){
			return std::type_identity<void(const void*)>{};
		}else{
			return std::type_identity<void(const void*, const drop_first_t<args>&)>{};
		}
	}

	template <typename FnWrap>
	using Fnty = decltype(events::getFnTy<FnWrap>())::type;

	template <template<typename > typename Cont, typename FnWrap, typename Proj, typename ...Events>
	using ebase = event_manager_base<
		Cont<select_wrapped_func<FnWrap, const typename function_traits<unwrap_fn_t<FnWrap>>::args_type&>>,
		Proj,
		std::tuple<Events...>,
		typename function_traits<unwrap_fn_t<FnWrap>>::args_type
	>;

	export
	template<typename FuncWrapperTy, typename... EventTs>
	struct event_manager : ebase<std::vector, FuncWrapperTy, std::identity, EventTs...>{
	private:
		using base = ebase<std::vector, FuncWrapperTy, std::identity, EventTs...>;

	public:
		template <event_argument T, typename Func>
			requires requires(Func&& func){
				base::template convert<T>(std::forward<Func>(func));
			}
		void on(Func&& func){
			auto& tgt = this->template group_at<T>();

			tgt.emplace_back(base::template convert<T>(std::forward<Func>(func)));
		}
	};

	export
	template<typename FuncWrapperTy, typename... EventTs>
	struct named_event_manager : ebase<string_hash_map, FuncWrapperTy, std::decay_t<decltype(std::views::values)>, EventTs...>{
	protected:
		using base = ebase<string_hash_map, FuncWrapperTy, std::decay_t<decltype(std::views::values)>, EventTs...>;

		using fn = select_wrapped_func<FuncWrapperTy, const typename function_traits<unwrap_fn_t<FuncWrapperTy>>::args_type&>;
		using cont = string_hash_map<fn>;

	public:
		template <event_argument T, typename Func>
			requires requires(Func&& func){
				base::template convert<T>(std::forward<Func>(func));
			}
		void on(const std::string_view name, Func&& func){
			cont& tgt = this->template group_at<T>();

			tgt.insert_or_assign(name, base::template convert<T>(std::forward<Func>(func)));
		}

		template <event_argument T>
		std::optional<fn> erase(const std::string_view name){
			cont& tgt = this->template group_at<T>();

			if(const auto eitr = tgt.find(name); eitr != tgt.end()){
				std::optional opt{std::move(eitr->second)};
				tgt.erase(eitr);
				return opt;
			}

			return std::nullopt;
		}
	};
}

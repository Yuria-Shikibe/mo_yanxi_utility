/*
module;

#include <cassert>

export module ext.json;

import std;
import mo_yanxi.heterogeneous.open_addr_hash;
import mo_yanxi.meta_programming;

namespace mo_yanxi::json{
	export struct parse_string_t{};
	export constexpr parse_string_t parse_string{};

	export class json_value;

	export using json_integer = std::int64_t;
	export using json_float = std::double_t;
	export using json_object = string_open_addr_hash_map<json_value>;
	export using json_array = std::vector<json_value>;

	export template <typename T>
	using json_scalar_t =
		std::conditional_t<std::same_as<T, bool>, bool,
		std::conditional_t<std::same_as<T, std::nullptr_t>, std::nullptr_t,
		std::conditional_t<std::is_floating_point_v<T>, json_float, json_integer>>>;

	export template <typename T>
	using JsonArithmeticType = std::conditional_t<std::is_floating_point_v<T>, json_float, json_integer>;

	export namespace keys{
		/**
		 * @brief Indeicate this json info refer to a Polymorphic Class
		 #1#
		constexpr std::string_view Typename = "$ty"; //type
		constexpr std::string_view Tag = "$tag";       //tag
		constexpr std::string_view ID = "$id";        //id
		constexpr std::string_view Data = "$d";      //data
		constexpr std::string_view Version = "$ver";   //version
		constexpr std::string_view Pos = "$pos";       //position
		constexpr std::string_view Size2D = "$sz2";   //size 2D
		constexpr std::string_view Offset = "$off";   //offset
		constexpr std::string_view Trans = "$trs";   //transform
		constexpr std::string_view Size = "$sz1";     //size 1D
		constexpr std::string_view Bound = "$b";     //bound

		constexpr std::string_view Key = "$key";   //key
		constexpr std::string_view Value = "$val"; //value

		constexpr std::string_view First = "$fst";   //first
		constexpr std::string_view Secound = "$sec"; //second
		constexpr std::string_view Count = "$cnt"; //second
	}

	//TODO arth exception
	json_integer parseInt(const std::string_view str, const int base = 10) noexcept{
		json_integer val{};
		std::from_chars(str.data(), str.data() + str.size(), val, base);
		return val;
	}

	json_float parseFloat(const std::string_view str) noexcept{
		json_float val{};
		std::from_chars(str.data(), str.data() + str.size(), val);
		return val;
	}

	export enum struct jval_tag : std::size_t{
		null,
		arithmetic_int,
		arithmetic_float,
		boolean,
		string,
		array,
		object,
		illegal = std::variant_npos
	};



	static_assert(std::same_as<std::underlying_type_t<jval_tag>, std::size_t>);

	template <jval_tag Tag>
	struct TagBase : std::integral_constant<std::underlying_type_t<jval_tag>, std::to_underlying(Tag)>{
		static constexpr auto tag = Tag;
		static constexpr auto index = std::to_underlying(tag);
	};

	export
	template <typename T>
	struct json_type_to_index : TagBase<jval_tag::null>{};

	template <std::integral T>
	struct json_type_to_index<T> : TagBase<jval_tag::arithmetic_int>{};

	template <std::floating_point T>
	struct json_type_to_index<T> : TagBase<jval_tag::arithmetic_float>{};

	template <>
	struct json_type_to_index<bool> : TagBase<jval_tag::boolean>{};

	template <>
	struct json_type_to_index<json_array> : TagBase<jval_tag::array>{};

	template <>
	struct json_type_to_index<json_object> : TagBase<jval_tag::object>{};;

	template <>
	struct json_type_to_index<std::string> : TagBase<jval_tag::string>{};

	template <>
	struct json_type_to_index<std::string_view> : TagBase<jval_tag::string>{};

	export template <typename T>
	constexpr jval_tag jval_tag_of = json_type_to_index<T>::tag;

	export template <typename T>
	constexpr std::size_t jval_index_of = json_type_to_index<T>::index;

	export using enum jval_tag;

	export
	class json_value{
	private:
		using args = type_to_index<std::tuple<std::nullptr_t, json_integer, json_float, bool, std::string, json_array, json_object>>;
		using VariantTypeTuple = args::args_type;
		using VariantType = tuple_to_variant_t<VariantTypeTuple>;
		VariantType data{json_object{}};

	public:
		constexpr json_value() = default;

		template <typename T>
			requires args::is_type_valid<std::remove_cvref_t<T>> && !std::is_scalar_v<std::remove_cvref_t<T>>
		constexpr explicit json_value(T&& val) : data{std::forward<T>(val)}{}

		template <typename T>
			requires (std::is_scalar_v<T>)
		constexpr explicit json_value(const T val) : data{static_cast<json_scalar_t<T>>(val)}{}

		constexpr explicit json_value(const std::string_view val) : data{std::string{val}}{}
		json_value(parse_string_t, std::string_view json_str);

		void set(const std::string_view str){
			data = static_cast<std::string>(str);
		}

		template <typename T>
			requires args::is_type_valid<std::decay_t<T>> && !std::is_arithmetic_v<T>
		void set(T&& val){
			data = std::forward<T>(val);
		}

		template <typename T>
			requires args::is_type_valid<json_scalar_t<T>> && std::is_arithmetic_v<T>
		void set(const T val){
			data = static_cast<json_scalar_t<T>>(val);
		}

		template <jval_tag tag>
		[[nodiscard]] constexpr bool is() const noexcept{
			return get_tag() == tag;
		}

		[[nodiscard]] constexpr bool is_str() const noexcept{
			return is<string>();
		}

		[[nodiscard]] constexpr bool is_arr() const noexcept{
			return is<array>();
		}

		[[nodiscard]] constexpr bool is_obj() const noexcept{
			return is<object>();
		}

		template <typename T>
			requires args::is_type_valid<T>
		[[nodiscard]] constexpr bool is() const noexcept{
			return std::to_underlying(get_tag()) == args::index_of<T>;
		}

		void set(const bool val) noexcept{
			data = val;
		}

		void set(const std::nullptr_t) noexcept{
			data = nullptr;
		}

		[[nodiscard]] constexpr auto get_tag() const noexcept{
			const auto idx = data.index();
			return idx == std::variant_npos ? jval_tag::null : jval_tag{idx};
		}

		[[nodiscard]] constexpr auto get_index() const noexcept{
			return data.index();
		}

		json_value& operator[](const std::string_view key){
			if(!is<object>()){
				throw std::bad_variant_access{};
			}

			return std::get<json_object>(data)[key];
		}

		const json_value& operator[](const std::string_view key) const{
			if(!is<object>()){
				throw std::bad_variant_access{};
			}

			return std::get<json_object>(data).at(key);
		}

		template <typename T>
			requires (std::is_arithmetic_v<T> && args::is_type_valid<json_scalar_t<T>>)
		[[nodiscard]] constexpr json_scalar_t<T> get_or(const T def) const noexcept{
			auto* val = try_get<json_scalar_t<T>>();
			if(val)return *val;
			return def;
		}

		template <typename T>
			requires args::is_type_valid<T> || (std::is_arithmetic_v<T> && args::is_type_valid<json_scalar_t<T>>)
		[[nodiscard]] constexpr decltype(auto) as(){
			if constexpr(std::is_arithmetic_v<T>){
				return static_cast<T>(std::get<json_scalar_t<T>>(data));
			} else{
				return std::get<T>(data);
			}
		}

		template <typename T>
			requires args::is_type_valid<T> || (std::is_arithmetic_v<T> && args::is_type_valid<json_scalar_t<T>>)
		[[nodiscard]] constexpr decltype(auto) as() const{
			if constexpr(std::is_arithmetic_v<T>){
				return static_cast<T>(std::get<json_scalar_t<T>>(data));
			} else{
				return std::get<T>(data);
			}
		}

		template <jval_tag tag>
		[[nodiscard]] constexpr decltype(auto) as(){
			return std::get<std::to_underlying(tag)>(data);
		}

		template <jval_tag tag>
		[[nodiscard]] constexpr decltype(auto) as() const{
			return std::get<std::to_underlying(tag)>(data);
		}

		json_object& as_obj(){
			if(get_tag() != object){
				set(json_object{});
			}

			return std::get<json_object>(data);
		}
		std::string_view as_str() const{
			if(get_tag() != string){
				return {""};
			}

			return std::get<std::string>(data);
		}

		[[nodiscard]] const json_object& as_obj() const{
			return std::get<json_object>(data);
		}

		decltype(auto) as_arr(){
			if(get_tag() != array){
				set(json_array{});
			}

			return std::get<json_array>(data);
		}

		[[nodiscard]] decltype(auto) as_arr() const{
			return std::get<json_array>(data);
		}

		//TODO move these to something adapter like
		template <typename T>
			requires (std::same_as<std::remove_cvref_t<T>, json_value> || args::is_type_valid<std::remove_cvref_t<T>>)
		void append(const char* name, T&& val){
			this->append(std::string{name}, std::forward<T>(val));
		}

		template <typename T>
			requires (std::same_as<std::remove_cvref_t<T>, json_value> || args::is_type_valid<std::remove_cvref_t<T>>)
		void append(const std::string_view name, T&& val){
			if(get_tag() != object){
				throw std::bad_variant_access{};
			}

			if constexpr(std::same_as<T, json_value>){
				as<object>().insert_or_assign(name, std::forward<T>(val));
			} else{
				as<object>().insert_or_assign(name, json_value{std::forward<T>(val)});
			}
		}

		template <typename T>
			requires (std::same_as<std::remove_cvref_t<T>, json_value> || args::is_type_valid<std::remove_cvref_t<T>>)
		void append(std::string&& name, T&& val){
			if(get_tag() != object){
				throw std::bad_variant_access{};
			}

			if constexpr(std::same_as<T, json_value>){
				as<object>().insert_or_assign(std::move(name), std::forward<T>(val));
			} else{
				as<object>().insert_or_assign(std::move(name), json_value{std::forward<T>(val)});
			}
		}

		template <typename T>
			requires (std::same_as<std::remove_cvref_t<T>, json_value> || args::is_type_valid<std::remove_cvref_t<T>>)
		void push_back(T&& val){
			if(get_tag() != array){
				throw std::bad_variant_access{};
			}

			if constexpr(std::same_as<T, json_value>){
				as<array>().push_back(std::forward<T>(val));
			} else{
				as<array>().emplace_back(std::forward<T>(val));
			}
		}

		friend constexpr bool operator==(const json_value& lhs, const json_value& rhs) noexcept  = default;
		constexpr auto operator<=>(const json_value& rhs) const noexcept = default;


		template <typename T>
			requires (args::is_type_valid<std::remove_cvref_t<T>> && !std::is_scalar_v<std::remove_cvref_t<T>>)
		json_value& operator=(T&& val){
			this->set(std::forward<T>(val));
			return *this;
		}

		template <typename T>
			requires (std::is_scalar_v<T>)
		json_value& operator=(const T val){
			this->set(static_cast<json_scalar_t<T>>(val));
			return *this;
		}

		json_value& operator=(const std::string_view str){
			this->set(std::string{str});
			return *this;
		}


		template <typename T>
			requires args::is_type_valid<T>
		[[nodiscard]] constexpr std::add_pointer_t<T> try_get() noexcept{
			return std::get_if<T>(&data);
		}

		template <typename T>
			requires args::is_type_valid<T>
		[[nodiscard]] constexpr std::add_pointer_t<const T> try_get() const noexcept{
			return std::get_if<T>(&data);
		}

		template <jval_tag tag>
		[[nodiscard]] constexpr std::add_pointer_t<args::arg_at<std::to_underlying(tag)>> try_get() noexcept{
			return std::get_if<std::to_underlying(tag)>(&data);
		}

		template <jval_tag tag>
		[[nodiscard]] constexpr std::add_pointer_t<const args::arg_at<std::to_underlying(tag)>> try_get() const noexcept{
			return std::get_if<std::to_underlying(tag)>(&data);
		}

		//TODO better format
		void print(std::ostream& os, const bool flat = false, const bool noSpace = false, const unsigned padSize = 1,
			const unsigned depth = 1) const{
			const std::string pad(flat ? 0 : depth * padSize, ' ');
			const std::string_view endRow =
				flat
					? noSpace
						  ? ""
						  : " "
					: "\n";

			switch(get_tag()){
				case arithmetic_int :{
					os << std::to_string(as<arithmetic_int>());
					break;
				}

				case arithmetic_float :{
					os << std::to_string(as<arithmetic_float>());
					break;
				}

				case boolean :{
					os << (as<boolean>() ? "true" : "false");
					break;
				}

				case string :{
					os << '"' << as<string>() << '"';
					break;
				}

				case array :{
					os << '[' << endRow;
					const auto data = as<array>();

					if(!data.empty()){
						int index = 0;
						for(; index < data.size() - 1; ++index){
							os << pad;
							data[index].print(os, flat, noSpace, padSize, depth + 1);
							os << ',' << endRow;
						}

						os << pad;
						data[index].print(os, flat, noSpace, padSize, depth + 1);
						os << endRow;
					}

					os << std::string_view(pad).substr(0, pad.size() - 4) << ']';
					break;
				}

				case object :{
					os << '{' << endRow;

					const auto data = as<object>();

					int count = 0;
					for(const auto& [k, v] : data){
						os << pad << '"' << k << '"' << (noSpace ? ":" : " : ");
						v.print(os, flat, noSpace, padSize, depth + 1);
						count++;
						if(count == data.size()) break;
						os << ',' << endRow;
					}

					os << endRow;
					os << std::string_view(pad).substr(0, pad.size() - 4) << '}';
					break;
				}

				default : os << "null";
			}
		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		[[nodiscard]] constexpr  std::optional<T> get_optional() const noexcept{
			if(is_arithmetic()){
				return std::optional{this->as_arithmetic<T>()};
			}
			return std::nullopt;
		}

		template <std::same_as<std::string_view>>
		[[nodiscard]] constexpr  std::optional<std::string_view> get_optional() const noexcept{
			if(is<string>()){
				return std::optional{this->as_str()};
			}
			return std::nullopt;
		}

		[[nodiscard]] constexpr std::optional<std::string_view> get_optional_string() const noexcept{
			return get_optional<std::string_view>();
		}

		json_value* nested_find(std::string_view name, const char splitter = '.') noexcept{
			auto* current = this;
			for (const std::string_view entry : name | std::views::split(splitter) | std::views::transform(mo_yanxi::convert_to<std::string_view>{})){
				if(!current)return nullptr;
				if(!current->is<object>())return nullptr;
				current = current->as_obj().try_find(entry);
			}

			return current;
		}

		const json_value* nested_find(const std::string_view name, const char splitter = '.') const noexcept{
			return const_cast<json_value*>(this)->nested_find(name, splitter);
		}

		friend std::ostream& operator<<(std::ostream& os, const json_value& obj){
			obj.print(os);

			return os;
		}

		[[nodiscard]] constexpr bool is_arithmetic() const noexcept{
			return
				is<arithmetic_int>() ||
				is<arithmetic_float>();
			//TODO or is bool??
		}

		[[nodiscard]] constexpr bool is_scalar() const noexcept{
			return
				is_arithmetic() ||
				is<boolean>() ||
				is<null>();
		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		[[nodiscard]] constexpr T as_arithmetic(const T def = T{}) const noexcept{
			if(is<json_integer>()){
				return static_cast<T>(as<json_integer>());
			}

			if(is<json_float>()){
				return static_cast<T>(as<json_float>());
			}

			if(is<bool>()){
				return static_cast<T>(as<bool>());
			}

			return def;
		}

		template <typename T>
			requires (std::is_arithmetic_v<T> || std::same_as<T, std::nullptr_t>)
		[[nodiscard]] constexpr T as_scalar(const T def = T{}) const noexcept{
			if constexpr (std::is_arithmetic_v<T>){
				return this->as_arithmetic<T>(def);
			}

			if constexpr (std::same_as<T, std::nullptr_t>){
				return nullptr;
			}

			std::unreachable();
		}

		constexpr explicit operator bool() const noexcept{
			return !data.valueless_by_exception() && !as<std::nullptr_t>();
		}
	};

	export
	constexpr json_value null_jval{nullptr};
}

namespace mo_yanxi::json{
	constexpr std::string_view trim(const std::string_view view) noexcept{
		constexpr std::string_view Empty = " \t\n\r\f\v";
		const auto first = view.find_first_not_of(Empty);

		if(first == std::string_view::npos){
			return {};
		}

		const auto last = view.find_last_not_of(Empty);
		return view.substr(first, last - first + 1);
	}

	export
	enum struct parse_error_code{
		syntax_error,
		empty_key
	};

	export
	struct parse_error final : std::exception{
		parse_error_code errc{};

		[[nodiscard]] explicit parse_error(const parse_error_code errc)
			: errc(errc){
		}

		[[nodiscard]] char const* what() const override{
			switch(errc){
			case parse_error_code::syntax_error : return "syntax error";
			case parse_error_code::empty_key : return R"(using empty("") key)";
			default : return "unknown error";
			}
		}
	};

	constexpr std::string_view shrink(const std::string_view text, const std::string_view::size_type begin, const std::string_view::size_type end) noexcept{
		return trim(text.substr(begin, end - begin));
	}

	constexpr std::string_view unwrap_quote(const std::string_view str){
		if(!str.starts_with('"') || !str.ends_with('"')){
			throw parse_error{parse_error_code::syntax_error};
		}

		return str.substr(1, str.size() - 2);
	}

	json_value parseBasicValue(const std::string_view view){
		if(view.empty()){
			return json_value(nullptr);
		}

		const auto frontChar = view.front();

		switch(frontChar){
			case '"' :{
				if(view.back() != '"' || view.size() == 1)
					throw parse_error{parse_error_code::syntax_error};

				std::string s{};
				std::ispanstream {view} >> std::quoted(s);

				return json_value{std::move(s)};
			}

			case 't' : return json_value(true);
			case 'f' : return json_value(false);
			case 'n' : return json_value(nullptr);
			default : break;
		}

		//TODO support nan?
		if(view.find_first_of(".fFeEiI") != std::string_view::npos){
			return json_value(parseFloat(view));
		}

		if(frontChar == '0'){
			if(view.size() >= 2){
				switch(view[1]){
					case 'x' : return json_value(parseInt(view.substr(2), 16));
					case 'b' : return json_value(parseInt(view.substr(2), 2));
					default : return json_value(parseInt(view.substr(1), 8));
				}
			} else{
				return json_value(static_cast<json_integer>(0));
			}
		} else{
			return json_value(parseInt(view));
		}
	}

	static constexpr auto InvalidSplitMark = std::string_view::npos;

	export
	json_value parse(std::string_view text){
		if(text.empty())return {};

		bool escapeTheNext{false};
		bool parsingString{false};
		std::string_view currentObjectKey{};

		struct Layer{
			std::size_t lastBeginIndex{};
			json_value value{};
			std::string_view targetKeyName{};

			void insert(const std::string_view key, json_value&& val){
				value.as<object>().insert_or_assign(key, std::move(val));
			}

			void push_back(json_value&& val){
				value.as<array>().push_back(std::move(val));
			}
		};

		std::vector<Layer> layers{};
		layers.reserve(8);

		constexpr auto storeCurrent = [](Layer& top, Layer&& popped){

			if(top.value.is<object>()){
				if(popped.targetKeyName.empty()){
					throw parse_error{parse_error_code::empty_key};
				}
				top.insert(popped.targetKeyName, std::move(popped.value));
			}else if(top.value.is<array>()){
				top.push_back(std::move(popped.value));
			}

			//Mark it invalid so jump next ',' check
			top.lastBeginIndex = InvalidSplitMark;
		};

		for(const auto [index, character] : text | std::views::enumerate){
			if(escapeTheNext){
				escapeTheNext = false;
				continue;
			}

			if(character == '\\'){
				escapeTheNext = true;
				continue;
			}

			if(character == '"'){
				if(!parsingString){
					parsingString = true;
				} else{
					parsingString = false;
				}
			}

			if(parsingString) continue;

			switch(character){
			case '[' :{
				layers.emplace_back(index, json_value{json_array{}}, std::exchange(currentObjectKey, {}));
				break;
			}

			case '{' :{
				layers.emplace_back(index, json_value{json_object{}}, std::exchange(currentObjectKey, {}));
				break;
			}

			case ':' :{
				if(layers.empty())throw parse_error{parse_error_code::syntax_error};

				auto& top = layers.back();
				currentObjectKey = unwrap_quote(shrink(text, top.lastBeginIndex + 1, index));

				if(currentObjectKey.empty()){
					throw parse_error{parse_error_code::empty_key};
				}

				top.lastBeginIndex = index;

				break;
			}

			case ']' :{
				if(layers.empty())throw parse_error{parse_error_code::syntax_error};

				auto popped = std::move(layers.back());
				layers.pop_back();

				if(!popped.value.is<array>()){
					throw parse_error{parse_error_code::syntax_error};
				}

				if(popped.lastBeginIndex != InvalidSplitMark){
					const auto lastCode = shrink(text, popped.lastBeginIndex + 1, index);
					popped.push_back(parseBasicValue(lastCode));
				}

				if(layers.empty()) return std::move(popped.value);
				auto& top = layers.back();

				storeCurrent(top, std::move(popped));

				break;
			}
			case '}' :{
				if(layers.empty())throw parse_error{parse_error_code::syntax_error};

				auto popped = std::move(layers.back());
				layers.pop_back();

				if(!popped.value.is<object>()){
					throw parse_error{parse_error_code::syntax_error};
				}

				if(!currentObjectKey.empty() && popped.lastBeginIndex != InvalidSplitMark){
					const auto lastCode = shrink(text, popped.lastBeginIndex + 1, index);
					popped.insert(std::exchange(currentObjectKey, {}), parseBasicValue(lastCode));
				}

				if(layers.empty()) return std::move(popped.value);
				auto& top = layers.back();

				storeCurrent(top, std::move(popped));

				break;
			}

			case ',' :{
				if(layers.empty())throw parse_error{parse_error_code::syntax_error};

				auto& top = layers.back();
				if(top.lastBeginIndex != InvalidSplitMark){
					if(!currentObjectKey.empty()){
						if(top.value.is<object>()){
							const auto lastCode = shrink(text, top.lastBeginIndex + 1, index);
							top.insert(std::exchange(currentObjectKey, {}), parseBasicValue(lastCode));
						}
					} else{
						if(top.value.is<array>()){
							const auto lastCode = shrink(text, top.lastBeginIndex + 1, index);
							top.push_back(parseBasicValue(lastCode));
						}
					}
				}

				top.lastBeginIndex = index;
			}

			default : break;
			}
		}

		throw parse_error{parse_error_code::syntax_error};

	}

	// export
	// struct json_optional{
	// 	json_value* value;
	//
	// 	template <typename T>
	// 	T value_or(T&& val){
	//
	// 	}
	// };

}

export
template <>
struct ::std::formatter<mo_yanxi::json::json_value>{
	constexpr auto parse(std::format_parse_context& context) const{
		return context.begin();
	}

	bool flat{false};
	bool noSpace{false};
	int pad{2};

	constexpr auto parse(std::format_parse_context& context){
		auto it = context.begin();
		if(it == context.end()) return it;

		if(*it == 'n'){
			noSpace = true;
			++it;
		}

		if(*it == 'f'){
			flat = true;
			++it;
		}

		if(*it >= '0' && *it <= '9'){
			pad = *it - '0';
			++it;
		}

		if(it != context.end() && *it != '}') throw std::format_error("Invalid format");

		return it;
	}

	auto format(const mo_yanxi::json::json_value& json, auto& context) const{
		//TODO ...
		std::ostringstream ss{};
		json.print(ss, flat, noSpace);

		return std::format_to(context.out(), "{}", std::move(ss).str());
	}
};

namespace mo_yanxi::json{
	export void json_test(std::string_view pathFile, std::initializer_list<std::string_view> queryList){
		std::string str{};

		{
			std::ifstream file_stream(pathFile.data(), std::ios::ate);
			if(!file_stream.is_open()) return;

			const std::size_t length = file_stream.tellg();
			file_stream.seekg(std::ios::beg);


			str.resize(length);
			file_stream.read(str.data(), static_cast<std::streamsize>(length));
		}

		try{
			json_value jval{parse_string, str};

			auto printer = [&](std::string_view entry){
				if(auto obj =  jval.nested_find(entry)){
					if(obj->is_obj()){
						std::println("{}", "OBJECT");
					}else if(obj->is_str()){
						std::println("STRING {}", obj->as_str());
					}else{
						std::println("OTHER TYPE: [{}]", std::to_underlying(obj->get_tag()));
					}
				}else{
					std::println("{}", "NOTEXIST");
				}
			};

			for (auto && entry : queryList){
				printer(entry);
			}

		}catch(const parse_error& e){
			std::cerr << e.what() << std::endl;
		}
	}
}

module : private;

mo_yanxi::json::json_value::json_value(parse_string_t, const std::string_view json_str){
	this->operator=(parse(json_str));
}
*/

export module mo_yanxi.io.file;

import mo_yanxi.concepts;

import std;

namespace fs = std::filesystem;

namespace mo_yanxi::io{
	//TODO...
	struct access_error : std::exception{
		using std::exception::exception;
	};

	export class file{
	protected:
		fs::path rawPath{};

	public:
		static constexpr std::string_view EMPTY_EXTENSION = "[empty]";

		file() = default;

		explicit(false) file(const decltype(rawPath)::string_type& p) : rawPath{p}{}

		explicit(false) file(const std::string& p) : rawPath{p}{}

		explicit(false) file(decltype(rawPath)::string_type&& p) : rawPath{std::move(p)}{}

		explicit(false) file(const char* p) : rawPath{p}{}

		explicit(false) file(const std::string_view p) : rawPath{p}{}

		explicit(false) file(const fs::path& p) : rawPath{p}{}

		explicit(false) file(fs::path&& p) : rawPath{std::move(p)}{}

		explicit(false) file(const fs::directory_entry& p) : rawPath(p){}

		file& operator=(const fs::path& other){
			rawPath = other;
			return *this;
		}

		file& operator=(fs::path&& other){
			rawPath = std::move(other);
			return *this;
		}

		// explicit operator fs::directory_entry() const{
		// 	return fs::directory_entry{rawPath};
		// }

		explicit(false) operator const fs::path&() const{
			return rawPath;
		}

		friend bool operator==(const file& lhs, const file& rhs) noexcept = default;

		[[nodiscard]] auto get_size() const{
			return fs::file_size(rawPath);
		}

		[[nodiscard]]fs::path absolutePath() const{
			return absolute(path());
		}

		[[nodiscard]] std::string extension() const{
			return rawPath.extension().string();
		}

		[[nodiscard]] std::string stem() const{
			return rawPath.stem().string();
		}

		[[nodiscard]] std::string filename() const{
			return rawPath.filename().string();
		}

		[[nodiscard]] std::string filename_full() const{
			std::string name = rawPath.filename().string();
			if(filename().empty()){
				name = absolutePath().string();
			}
			return (is_dir() ? "<Dir> " : "<Fi> ") + name;
		}

		[[nodiscard]] std::string filename_full_pure() const{
			std::string name = rawPath.filename().string();
			if(filename().empty()){
				name = absolutePath().string();
			}
			return name;
		}

		[[nodiscard]] fs::path& path(){
			return rawPath;
		}

		[[nodiscard]] const fs::path& path() const{
			return rawPath;
		}

		[[nodiscard]] bool exist() const{
			return exists(rawPath);
		}

		[[nodiscard]] bool delete_file() const{
			return exist() && (is_dir()
				                   ? remove(absolutePath())
				                   : remove_all(absolutePath()));
		}

		void delete_file_quiet() const{
			(void)delete_file();
		}

		[[nodiscard]] bool copy(const fs::path& dest) const{
			try{
				fs::copy(path(), dest);
				return true;
			} catch([[maybe_unused]] std::error_code& ignore){
				return false;
			}
		}

		[[nodiscard]] bool copy(const file& dest) const{
			return copy(dest.path());
		}

		[[nodiscard]] bool is_dir() const{
			return is_directory(path());
		}

		[[nodiscard]] bool is_regular() const{
			return is_regular_file(path());
		}

		[[nodiscard]] bool is_hidden() const{
			return stem().starts_with('.');
		}

		[[nodiscard]] bool empty_extension() const{
			return extension().empty();
		}

		[[nodiscard]] bool create_dir(const bool autoCreateParents = true) const{
			return autoCreateParents ? create_directories(path()) : create_directory(path());
		}

		void create_dir_quiet(const bool autoCreateParents = true) const{
			(void)create_dir(autoCreateParents);
		}

		[[nodiscard]] bool create_file(const bool autoCreateParents = false) const{
			if(exist()) return false;

			if(autoCreateParents){
				if(const file parent = file{get_parent()}; !parent.exist()){
					(void)parent.create_dir();
				}
			}

			const std::ofstream ofStream(path());
			const bool valid = ofStream.is_open();

			return valid;
		}

		void create_file_quiet(const bool autoCreateParents = false) const{
			(void)create_file(autoCreateParents);
		}

		[[nodiscard]] file get_parent() const{
			return file{rawPath.parent_path()};
		}

		[[nodiscard]] file get_root() const{
			return file{rawPath.root_path()};
		}

		[[nodiscard]] bool has_parent() const{
			return rawPath.has_parent_path();
		}

		[[nodiscard]] bool has_extension() const{
			return rawPath.has_extension();
		}

		file& goto_parent(){
			rawPath = rawPath.parent_path();

			return *this;
		}

		[[nodiscard]] file sub_file(const std::string_view name) const{
			if(!is_dir()) throw access_error{""};

			return file{absolutePath().append(name)};
		}

		[[nodiscard]] file sub_file(const std::string& name) const{
			if(!is_dir()) throw access_error{""};

			return file{absolutePath().append(name)};
		}

		[[nodiscard]] file sub_file(const fs::path& path) const{
			if(!is_dir()) throw access_error{""};

			return file{absolutePath() / path};
		}

		[[nodiscard]] file sub_file(const char* name) const{
			if(!is_dir()) throw access_error{""};

			return file{absolutePath().append(name)};
		}

		[[nodiscard]] file find(const std::string_view name) const{
			if(file ret = sub_file(name); ret.exist()){
				return ret;
			}

			throw access_error{""};
		}

		/**
		 * Warning: This does not do append but erase and write!
		 * */
	    template <std::ranges::contiguous_range Rng>
		void writeByte(const Rng& range) const{
			if(std::ofstream ofStream(path(), std::ios::binary); ofStream.is_open()){
			    ofStream.write(reinterpret_cast<const char *>(std::ranges::data(range)), sizeof(std::ranges::range_value_t<Rng>) * std::ranges::size(range));
			}
		}

	    /**
		 * Warning: This does not do append but erase and write!
		 * */
		void writeString(const std::string_view string) const{
			if(std::ofstream ofStream(path()); ofStream.is_open()){
			    ofStream << string;
			}
		}

		template <bool careDirs = false>
		[[nodiscard]] std::vector<file> subs() const{
			std::vector<file> files;
			for(const auto& item : fs::directory_iterator(path())){
				if constexpr (careDirs){
					files.emplace_back(item);
				}else{
					if(!item.is_directory()){
						files.emplace_back(item);
					}
				}
			}

			return files;
		}

		template <std::predicate<const file&> Pred>
		[[nodiscard]] std::vector<file> subs(Pred pred) const{
			std::vector<file> files;
			for(const auto& item : fs::directory_iterator(path())){
				file file{item};
				if (pred(file)){
					files.push_back(std::move(file));
				}
			}

			return files;
		}

		void for_subs(std::invocable<file&&> auto consumer) const{
			for(const auto& item : fs::directory_iterator(path())){
				consumer(file{item});
			}
		}

		template <bool careDirs = false, std::invocable<file&&> Fn>
		void for_all_subs(Fn consumer) const{
			for(const auto& item : fs::recursive_directory_iterator(path())){
				if(file f{item}; f.is_regular()){
					consumer(std::move(f));
				} else{
					if constexpr (careDirs){
						consumer(std::move(f));
					}
				}
			}
		}

		template <bool careDirs = false>
		void allSubs(std::vector<file>& container) const{
			for(const auto& item : fs::recursive_directory_iterator(path())){
				if(file f{item}; f.is_regular()){
					container.emplace_back(std::move(f));
				} else{
					if constexpr(careDirs){
						container.emplace_back(std::move(f));
					}
				}
			}
		}

		[[nodiscard]] std::string read_string() const{
			std::ifstream file_stream(path(), std::ios::binary | std::ios::ate);
			if(!file_stream.is_open()) return "";

			const std::size_t length = file_stream.tellg();
			file_stream.seekg(std::ios::beg);

			std::string str{};

			str.resize_and_overwrite(length, [&file_stream](char* buf, const std::size_t buf_size) noexcept{
				file_stream.read(buf, static_cast<std::streamsize>(buf_size));
				return buf_size;
			});

			return str;
		}

	    template <typename T = std::byte>
		[[nodiscard]] std::vector<T> read_bytes() const{
			std::ifstream file_stream(path(), std::ios::binary | std::ios::ate);
			if(!file_stream.is_open()) {
				return {};
			}

			const std::size_t length = file_stream.tellg();
			file_stream.seekg(std::ios::beg);

			std::vector<T> str((length - 1) / sizeof(T) + 1);

		    file_stream.read(reinterpret_cast<char *>(str.data()), static_cast<std::streamsize>(str.extent() * sizeof(T)));

			return str;
		}

		void writeByte(std::invocable<std::ofstream&> auto&& func){
			if(std::ofstream ofStream(absolutePath(), std::ios::binary); ofStream.is_open()){
				func(ofStream);
			}
		}

		friend std::ostream& operator<<(std::ostream& os, const file& file){
			os << file.filename_full();
			return os;
		}

		using Filter = std::pair<std::string, std::function<bool(const file&)>>;

		// template <bool careDirs = false>
		// [[nodiscard]] ext::string_hash_map<std::vector<file>> sortSubs() const{
		// 	ext::string_hash_map<std::vector<file>> map;
		// 	this->for_all_subs<careDirs>([&map](file&& file){
		// 		const std::string& extension = file.extension();
		// 		map[extension.empty() ? static_cast<std::string>(EMPTY_EXTENSION) : extension].push_back(file);
		// 	});
		//
		// 	return map;
		// }
		//
		// template <bool careDirs = false>
		// [[nodiscard]] ext::string_hash_map<std::vector<file>> sortSubsBy(const std::span<Filter>& standards) const{
		// 	ext::string_hash_map<std::vector<file>> map;
		//
		// 	this->for_all_subs<careDirs>([&standards, &map](file&& file){
		// 		if(const auto it = std::ranges::find_if(standards, [&file](const Filter& pair){
		// 			return pair.second(file);
		// 		}); it != standards.end()){
		// 			map[it->first].push_back(file);
		// 		}
		// 	});
		//
		// 	return map;
		// }
		//
		// static ext::string_hash_map<std::vector<file>> sortBy(
		// 	const std::span<file>& files, const std::span<Filter>& standards){
		// 	ext::string_hash_map<std::vector<file>> map;
		//
		// 	for(const file& file : files){
		// 		if(
		// 			auto it = std::ranges::find_if(standards, [&file](const Filter& pair){
		// 				return pair.second(file);
		// 			});
		// 			it != standards.end()
		// 		){
		// 			map[it->first].push_back(file);
		// 		}
		// 	}
		//
		// 	return map;
		// }

		friend auto operator<=>(const file& lhs, const file& rhs){
			if(lhs.is_dir()){
				if(rhs.is_dir()){
					return lhs.rawPath <=> rhs.rawPath;
				}
				return std::strong_ordering::less;
			}

			if(rhs.is_dir()){
				return std::strong_ordering::greater;
			}

			return lhs.rawPath <=> rhs.rawPath;
		}

		friend file operator/(const file& l, const file& fi){
			return file{l.rawPath / fi.path()};
		}

		friend file operator/(const file& l, const std::string_view path){
			return file{l.rawPath / path};
		}

		friend file operator/(const file& l, const char* path){
			return file{l.rawPath / path};
		}

		file& operator/=(const file& file){
			rawPath /= file.path();
			return *this;
		}

		file& operator/=(const std::string_view path){
			rawPath /= path;
			return *this;
		}

		file& operator/=(const char* path){
			rawPath /= path;
			return *this;
		}
	};
}

export
template <>
struct std::hash<mo_yanxi::io::file>{
	std::size_t operator()(const mo_yanxi::io::file& p) const noexcept{
		static constexpr std::hash<std::filesystem::path> hasher{};
		return hasher(p.path());
	}
};

export
template <>
struct std::formatter<mo_yanxi::io::file>{
	template <typename Context>
	constexpr auto parse(Context& context) const{
		return context.begin();
	}

	auto format(const mo_yanxi::io::file& p, auto& context) const{
		return std::format_to(context.out(), "{}", p.filename_full());
	}
};
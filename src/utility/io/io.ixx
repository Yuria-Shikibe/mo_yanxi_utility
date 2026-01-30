export module mo_yanxi.io;

import std;

namespace mo_yanxi::io{

	export
	/**
	 * Warning: This does not do append but erase and write!
	 * */
	template <std::ranges::contiguous_range Rng, typename PathTy>
		requires (std::constructible_from<std::ifstream, const PathTy&>)
	bool write_bytes(const PathTy& path, const Rng& range) {
		if(std::ofstream ofStream(path, std::ios::binary); ofStream.is_open()){
			ofStream.write(reinterpret_cast<const char *>(std::ranges::data(range)), sizeof(std::ranges::range_value_t<Rng>) * std::ranges::size(range));
			return true;
		}else{
			return false;
		}
	}

	export
	template <typename PathTy>
		requires (std::constructible_from<std::ifstream, const PathTy&>)
	[[nodiscard]] std::optional<std::string> read_string(const PathTy& path) noexcept{
		try{
			std::ifstream file_stream(path, std::ios::binary | std::ios::ate);

			const std::size_t length = file_stream.tellg();
			file_stream.seekg(std::ios::beg);

			std::string str{};

			str.resize_and_overwrite(length, [&file_stream](char* buf, const std::size_t buf_size) noexcept{
				file_stream.read(buf, static_cast<std::streamsize>(buf_size));
				return buf_size;
			});

			return str;
		} catch(...){
			return std::nullopt;
		}
	}

	export
	template <typename T = std::byte, typename PathTy>
		requires (std::constructible_from<std::ifstream, const PathTy&>)
	[[nodiscard]] std::optional<std::vector<T>> read_bytes(const PathTy& path) noexcept{
		try{
			std::ifstream file_stream(path, std::ios::binary | std::ios::ate);

			const std::size_t length = file_stream.tellg();
			file_stream.seekg(std::ios::beg);

			std::vector<T> str(length / sizeof(T));

			file_stream.read(reinterpret_cast<char*>(str.data()), static_cast<std::streamsize>(str.size() * sizeof(T)));

			return str;
		} catch(...){
			return std::nullopt;
		}
	}

}

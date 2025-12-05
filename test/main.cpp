import std;
import mo_yanxi.math.vector2;
import mo_yanxi.event;
import mo_yanxi.type_register;

struct T final : mo_yanxi::events::event_type_tag{

};

int main(){
	mo_yanxi::events::event_manager<std::move_only_function<void() const>, T> manager;
	std::println("{}", mo_yanxi::unstable_type_identity_of<int>()->name());
	mo_yanxi::math::vec2 vec2{1, 2};
	std::cout << vec2.x << vec2.y << std::endl;
	return 0;
}
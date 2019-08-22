#include <functional>
#include <iostream>

void f(int& n1, int& n2, const int& n3){
	std::cout << "In func: " << n1 << " " << n2 << " " << n3 << std::endl;
	++n1;
	++n2;
}

int main(){
	int n1 = 1, n2 = 2, n3 = 3;
	// 默认的 std::bind 使用的是参数的拷贝，因此如果一定要用引用，需要加上 ref/cref。
	// 因为 bind 生成的是函数模板，所以 bind 之后的函数模板不知道在其被调用时，参数还是否有效，因此默认用参数拷贝的形式传参。
	std::function<void()> bind_f = std::bind(f, n1, std::ref(n2), std::cref(n3));
	n1 = 11;
	n2 = 12;
	n3 = 13;
	std::cout << "Before func: " << n1 << " " << n2 << " " << n3 << std::endl;
	bind_f();
	std::cout << "After func: " << n1 << " " << n2 << " " << n3 << std::endl;
	return 0;
}


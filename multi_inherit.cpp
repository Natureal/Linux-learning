#include <stdio.h>

class A{
public:
	virtual void vf1(){}
	virtual void vf2(){}
};

class B{
public:
	virtual void vf1(){}
	virtual void vf2(){}
	virtual void vf4(){}
};

class C : public A, public B{
public:
	virtual void vf1(){}
	virtual void vf2(){}
	virtual void vf3(){}
};

int main(){
	typedef void (*Func)();
	C c;
	printf("C add: %p\n", 
	printf("C vf1: %p\n", (Func)&C::vf1);
	printf("C vf2: %p\n", (Func)&C::vf2);
	printf("C vf3: %p\n", (Func)&C::vf3);
	printf("C vf4: %p\n", (Func)&C::vf4);
	
	return 0;
}

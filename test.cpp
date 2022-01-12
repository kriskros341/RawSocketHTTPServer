#include <iostream>
#include <functional>

/*
template <typename T>
struct LList {
	T val;
	LList* next;
	void printList() {
		for(auto *i = this; i != nullptr; i = i->next) {
			std::cout << i->val << std::endl;
		}
	}
};

template <typename T>
void insert(LList<T> &head, int data) {
	LList<T>* temp = new LList<T>;
	temp->val = data;
	temp->next = &head;
	head = *temp;

}

template <typename T>
struct Queue {
	T val;
	Queue* head;
};

int main() {
	LList<int>* n1 = new LList<int>;
	n1->val = 20;
	n1->next = nullptr;
	insert(*n1, 3);
	insert(*n1, 4);
	insert(*n1, 5);
	n1->printList();
	n1->printList();
}
*/


class klasa {
	public:
		int mfn(const char* slowo) {
			return 4;
		}
};

int mfn(klasa k, const char* slowo) {
	return 4;
};

int fn(const char* slowo) {
	return 4;
}



using f1 = int (*)();
using f2 = std::function<int()>;

/*
Functor, czyli

Functor {
	private:
		vals;
	public:

		operator (args) {
			fn(...vals, ...args)
		}
}


Functor jednoczy:
	bindowane metody:  type bind(klasa, objekt, ...args)
	wskaźniki:  type (*){}
	lambdy:  type []() {}
*/

int main() {
	klasa objekt;
	
  std::cout << fn("gg") << std::endl;
	objekt.mfn("gg");

	f2 bfn = std::bind(&fn, "gg");
	f2 bmfn = std::bind(&klasa::mfn, objekt, "gg");
	
	bfn();
	bmfn();


}







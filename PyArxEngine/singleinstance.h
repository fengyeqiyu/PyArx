#pragma once
#include <forward_list>


template <typename T>
class SingleInstance {
private:
	inline static T* m_instance = nullptr;
public:
	template <typename... Args>
	inline static T* instance(Args&&... args) {
		if (m_instance == nullptr) {
			m_instance = new T(std::forward<Args>(args)...);
		}
		return m_instance;
	}
	inline static void release() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = nullptr;
		}
	}
	inline static T* getPtr() {
		return m_instance;
	}

	~SingleInstance() {
		m_instance = nullptr;
	}
};

#define DECLARE_SINGLETON(ClassName) \
  class ClassName : public SingleInstance<ClassName>
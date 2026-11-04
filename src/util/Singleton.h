#pragma once

// 앱 전역에 하나만 있는 객체의 공통 바탕 (CRTP).
//
//   class Foo : public QObject, public Singleton<Foo> {   // QObject 는 꼭 첫 번째로
//       Q_OBJECT
//       SINGLETON(Foo)
//   public:
//       void bar();
//   };
//
//   Foo::instance().bar();
template <typename T>
class Singleton {
public:
    static T& instance()
    {
        // 첫 호출 때 만든다 (C++11 부터 초기화는 스레드 안전).
        // 일부러 지우지 않는다: 정적 객체는 QApplication 보다 늦게 소멸되므로
        // 그때 QObject 정리를 하면 죽을 수 있다. 앱이 끝날 때 OS 가 회수하게 둔다.
        static T* inst = new T();
        return *inst;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton() = default;
    ~Singleton() = default;
};

// 비공개 기본 생성자를 만들고 Singleton<Class> 만 부를 수 있게 한다.
// 생성자를 막아야 instance() 말고는 객체를 만들 수 없다.
// Q_OBJECT 처럼 끝나면 private 구역이므로 뒤에 public: 을 적는다.
#define SINGLETON(Class)           \
    friend class Singleton<Class>; \
                                   \
private:                           \
    Class() = default;

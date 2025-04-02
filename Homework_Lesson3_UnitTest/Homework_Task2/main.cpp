//подключаем макросы catch2
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

//проверяемая функция
#include <iostream>

struct ListNode
{
public:
    ListNode(int value, ListNode* prev = nullptr, ListNode* next = nullptr)
        : value(value), prev(prev), next(next)
    {
        if (prev != nullptr) prev->next = this;
        if (next != nullptr) next->prev = this;
    }

public:
    int value;
    ListNode* prev;
    ListNode* next;
};


class List
{
public:
    List()
        : m_head(new ListNode(static_cast<int>(0))), m_size(0),
        m_tail(new ListNode(0, m_head))
    {
    }

    virtual ~List()
    {
        Clear();
        delete m_head;
        delete m_tail;
    }

    bool Empty() { return m_size == 0; }

    unsigned long Size() { return m_size; }

    void PushFront(int value)
    {
        new ListNode(value, m_head, m_head->next);
        ++m_size;
    }

    void PushBack(int value)
    {
        new ListNode(value, m_tail->prev, m_tail);
        ++m_size;
    }

    int PopFront()
    {
        if (Empty()) throw std::runtime_error("list is empty");
        auto node = extractPrev(m_head->next->next);
        int ret = node->value;
        delete node;
        return ret;
    }

    int PopBack()
    {
        if (Empty()) throw std::runtime_error("list is empty");
        auto node = extractPrev(m_tail);
        int ret = node->value;
        delete node;
        return ret;
    }

    void Clear()
    {
        auto current = m_head->next;
        while (current != m_tail)
        {
            current = current->next;
            delete extractPrev(current);
        }
    }

private:
    ListNode* extractPrev(ListNode* node)
    {
        auto target = node->prev;
        target->prev->next = target->next;
        target->next->prev = target->prev;
        --m_size;
        return target;
    }

private:
    ListNode* m_head;
    ListNode* m_tail;
    unsigned long m_size;
};




TEST_CASE("PushBack adds elements to the end of the list", "[PushBack]") {
    List list;
    list.PushBack(10);
    list.PushBack(20);
    list.PushBack(30);

    REQUIRE(list.Size() == 3);

    REQUIRE(list.PopFront() == 10);
    REQUIRE(list.PopFront() == 20);
    REQUIRE(list.PopFront() == 30);
    REQUIRE(list.Empty());
}


TEST_CASE("PushFront adds elements to the beginning of the list", "[PushFront]") {
    List list;
    list.PushFront(10);
    list.PushFront(20);
    list.PushFront(30);

    REQUIRE(list.Size() == 3);

    REQUIRE(list.PopBack() == 10);
    REQUIRE(list.PopBack() == 20);
    REQUIRE(list.PopBack() == 30);
    REQUIRE(list.Empty());
}


TEST_CASE("PopBack throws an exception if the list is empty", "[PopBack]") {
    List list;
    REQUIRE_THROWS_AS(list.PopBack(), std::runtime_error);
}


TEST_CASE("PopFront throws an exception if the list is empty", "[PopFront]") {
    List list;
    REQUIRE_THROWS_AS(list.PopFront(), std::runtime_error);
}


TEST_CASE("Complex scenario of list operations", "[ComplexScenario]") {
    List list;

    list.PushBack(1);
    list.PushBack(2);
    list.PushFront(0); 

    REQUIRE(list.PopBack() == 2);  
    REQUIRE(list.PopFront() == 0); 
    REQUIRE(list.PopFront() == 1); 

    REQUIRE(list.Empty()); 

    list.PushFront(100);
    list.PushBack(200);
    list.PushFront(50); 

    REQUIRE(list.Size() == 3);
    REQUIRE(list.PopFront() == 50);
    REQUIRE(list.PopBack() == 200);
    REQUIRE(list.PopFront() == 100);

    REQUIRE(list.Empty());
}



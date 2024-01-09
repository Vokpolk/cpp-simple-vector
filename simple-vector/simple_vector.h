#pragma once
#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity_to_reserve) : capacity_to_reserve_(capacity_to_reserve) {}
    size_t GetCapacity() { return capacity_to_reserve_; }
private:
    size_t capacity_to_reserve_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    SimpleVector(ReserveProxyObj obj) : items_(obj.GetCapacity()), size_(0), capacity_(obj.GetCapacity()) {}

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : items_(size), size_(size), capacity_(size) {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : items_(size), size_(size), capacity_(size) {
        for (size_t i = 0; i < size_; i++) {
            items_[i] = value;
        }
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) :
        items_(init.size()),
        size_(init.size()),
        capacity_(init.size()) {
        for (size_t i = 0; i < init.size(); i++) {
            items_[i] = *(init.begin() + i);
        }
    }

    SimpleVector(const SimpleVector& other) {
        SimpleVector temp(other.size_);
        items_.swap(temp.items_);
        size_ = temp.size_;
        capacity_ = temp.capacity_;
        for (size_t i = 0; i < size_; i++) {
            items_[i] = other.items_[i];
        }
    }

    SimpleVector(SimpleVector&& other) {
        SimpleVector temp(other.size_);
        items_.swap(temp.items_);

        for (size_t i = 0; i < other.size_; i++) {
            items_[i] = std::move(other.items_[i]);
        }

        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        other.Resize(0);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            auto rhs_copy(rhs);
            this->swap(rhs_copy);
        }
        return *this;
    }

    SimpleVector&& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            auto rhs_copy(std::move(rhs));
            this->swap(rhs_copy);
        }
        return std::move(*this);
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ >= capacity_) {
            Resize(size_ + 1);
            items_[size_ - 1] = item;
        }
        else {
            items_[size_] = item;
            ++size_;
        }
    }

    void PushBack(Type&& item) {
        if (size_ >= capacity_) {
            Resize(size_ + 1);
            items_[size_ - 1] = std::move(item);
        }
        else {
            items_[size_] = std::move(item);
            ++size_;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        if (capacity_ == 0) {
            size_ = 1;
            capacity_ = 1;
            auto* temp = new SimpleVector(1);
            *this = *temp;
            items_[0] = value;
            return &items_[0];
        }
        else if (end() == pos) {
            PushBack(value);
            return end() - 1;
        }
        else if (size_ >= capacity_) {
            size_t temp_position = 0;
            for (auto it = begin(); it != end(); it++) {
                if (it == pos) {
                    break;
                }
                ++temp_position;
            }
            Resize(size_ + 1);
            std::copy_backward(&items_[temp_position], end() - 1, end());
            items_[temp_position] = value;

            return &items_[temp_position];
        }
        else {
            ConstIterator temp_value = pos;
            Iterator new_pos = this->begin();
            size_++;
            int count = 0;
            for (auto it = begin(); it != end(); it++) {
                if (pos == it) {
                    new_pos = &items_[count];
                    break;
                }
                count++;
            }

            std::copy_backward(new_pos, end() - 1, end());
            items_[count] = value;
            return &items_[count];
        }
    }

    Iterator Insert(Iterator pos, Type&& value) {
        if (capacity_ == 0) {
            size_ = 1;
            capacity_ = 1;
            auto* temp = new SimpleVector(1);
            *this = std::move(*temp);
            items_[0] = std::move(value);
            return &items_[0];
        }
        else if (end() == pos) {
            PushBack(std::move(value));
            return end() - 1;
        }
        else if (size_ >= capacity_) {
            size_t temp_position = 0;
            for (auto it = begin(); it != end(); it++) {
                if (it == pos) {
                    break;
                }
                ++temp_position;
            }
            Resize(size_ + 1);
            std::copy_backward(std::make_move_iterator(&items_[temp_position]), std::make_move_iterator(end() - 1), end());
            items_[temp_position] = std::move(value);

            return &items_[temp_position];
        }
        else {
            Iterator new_pos = this->begin();
            size_++;
            int count = 0;
            for (auto it = begin(); it != end(); it++) {
                if (pos == it) {
                    new_pos = &items_[count];
                    break;
                }
                count++;
            }

            std::copy_backward(std::make_move_iterator(new_pos), std::make_move_iterator(end() - 1), end());
            items_[count] = std::move(value);
            return &items_[count];
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        Iterator new_pos = this->begin();
        size_t count = 0;
        for (size_t i = 0; i < size_; i++) {
            if (&items_[i] == pos) {
                new_pos = &items_[i];
                count = i;
                break;
            }
        }

        std::copy(std::make_move_iterator(new_pos + 1), std::make_move_iterator(end()), begin());
        --size_;

        return &items_[count];
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {

        this->items_.swap(other.items_);

        auto temp_size = this->size_;
        auto temp_capacity = this->capacity_;

        this->size_ = other.size_;
        this->capacity_ = other.capacity_;

        other.size_ = temp_size;
        other.capacity_ = temp_capacity;

    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0 ? true : false;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        ArrayPtr<Type> temp(new_size > capacity_ ? new_size * 2 : capacity_);

        size_t temp_size = size_ > new_size ? new_size : size_;
        for (size_t i = 0; i < temp_size; i++) {
            temp[i] = std::move(items_[i]);
        }

        size_ = new_size;
        if (size_ > capacity_) {
            capacity_ = size_ * 2;
        }

        items_.swap(temp);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> temp(new_capacity);
            for (size_t i = 0; i < size_; i++) {
                temp[i] = items_[i];
            }
            items_.swap(temp);
            capacity_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return &items_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return const_cast<ConstIterator>(&items_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return &items_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return const_cast<ConstIterator>(&items_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return &items_[size_];
    }
private:
    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
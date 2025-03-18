#pragma once
#include "framework.h"

#include <cstddef>
#include <memory>
#include <algorithm>
#include <typeindex>
#include <type_traits>

template <typename Allocator = std::allocator<unsigned char>>
class FlexibleVector {
private:
	using AllocTraits = std::allocator_traits<Allocator>;

	unsigned char* storage;

	size_t _capacity;
	size_t _size;

	size_t itemSize;

	Allocator allocator;

	RC_DBG_VAR(std::type_index FLEXIBLE_VECTOR_DBG_TYPE = typeid(void));

	template <typename Ty>
	void reserveImpl(const size_t elementsAmmount) {
		try {
			RC_DBG_CODE({
				if (this->FLEXIBLE_VECTOR_DBG_TYPE != typeid(Ty)) {
					throw std::runtime_error(
						std::format("Type is not valid! Attempted type: {}, Right type: {}", this->FLEXIBLE_VECTOR_DBG_TYPE.name(), typeid(Ty).name()).c_str()
					);
				}
			});

			using ReboundAlloc = typename AllocTraits::template rebind_alloc<Ty>;
			ReboundAlloc reboundAllocator(this->allocator);
			Ty* reboundStorage = reinterpret_cast<Ty*>(this->storage);

			const auto allocationSize = this->_capacity + elementsAmmount;

			auto buff = reboundAllocator.allocate(allocationSize);

			if (!buff) return;
			if (reboundStorage) {
				if constexpr (std::is_trivially_copyable_v<Ty>) memcpy(buff, reboundStorage, sizeof(Ty) * this->_size);
				else if constexpr (std::is_copy_constructible_v<Ty>) {
					std::uninitialized_copy(reboundStorage, reboundStorage + this->_size, buff);
				}
				else {
					std::uninitialized_move(reboundStorage, reboundStorage + this->_size, buff);
				}

				reboundAllocator.deallocate(reboundStorage, this->_capacity);
			}

			this->storage = reinterpret_cast<unsigned char*>(buff);

			this->_capacity += elementsAmmount;
		}
		catch (const std::exception& e) {
			std::cout << "Exception: " << e.what() << '\n';
			std::cout << "Allocation size: " << this->_capacity + elementsAmmount << '\n';
			std::cout << "Item size: " << sizeof(Ty) << " bytes \n";
			std::cout << "Allocated type: " << typeid(Ty).name() << '\n';
			std::cout << "Total size: " << (this->_capacity + elementsAmmount) * sizeof(Ty) << " bytes\n";
		}
	}

public:
	FlexibleVector() = default;
	FlexibleVector(const FlexibleVector& other) :
		_capacity(other._capacity),
		_size(other._size),
		itemSize(other.itemSize)
	{
		RC_DBG_CODE(
			this->FLEXIBLE_VECTOR_DBG_TYPE = other.FLEXIBLE_VECTOR_DBG_TYPE;
		)

		this->reserveImpl<unsigned char>(other._capacity);
		memcpy(this->storage, other.storage, other._size * other.itemSize);
	}
	FlexibleVector(FlexibleVector&& other) noexcept :
		storage(other.storage),
		_capacity(other._capacity),
		_size(other._size),
		itemSize(other.itemSize)
	{
		RC_DBG_CODE(
			this->FLEXIBLE_VECTOR_DBG_TYPE = other.FLEXIBLE_VECTOR_DBG_TYPE;
		)

		other.storage = nullptr;
	}

	~FlexibleVector() {
		if (this->storage) this->allocator.deallocate(this->storage, this->itemSize * this->_capacity);
	}

	template <typename Ty>
	void build() {
		RC_DBG_CODE(
			this->FLEXIBLE_VECTOR_DBG_TYPE = typeid(Ty);
		)

		this->storage = nullptr;

		this->_capacity = 0;
		this->_size = 0;

		this->itemSize = sizeof(Ty);
	}
	template <typename Ty>
	void build(const size_t initialSize) {
		RC_DBG_CODE(
			this->FLEXIBLE_VECTOR_DBG_TYPE = typeid(Ty);
		)

		this->storage = nullptr;

		this->_capacity = initialSize;
		this->_size = initialSize;

		this->itemSize = sizeof(Ty);

		this->reserveImpl<Ty>(initialSize);
	}

	size_t size() { return this->_size; }
	size_t capacity() { return this->_capacity; }
	const size_t size() const { return this->_size; }
	const size_t capacity() const { return this->_capacity; }

	template <typename Ty>
	void reserve(const size_t elementsAmmount) {
		this->reserveImpl<Ty>(elementsAmmount);
	}

	template <typename Ty, typename = std::enable_if_t<std::is_copy_constructible_v<Ty>>>
	void push(const Ty& element) {
		using ReboundAlloc = typename AllocTraits::template rebind_alloc<Ty>;
		ReboundAlloc reboundAllocator(this->allocator);

		if (this->_size + 1 > this->_capacity) this->reserveImpl<Ty>(1);
		this->_size++;

		Ty* reboundStorage = reinterpret_cast<Ty*>(this->storage);
		const auto correctSize = this->_size - 1;
		Ty* location = reboundStorage + correctSize;
		std::construct_at(location, element);
	}
	template <typename Ty, typename = std::enable_if_t<!std::is_lvalue_reference<Ty>::value>>
	void push(Ty&& element) {
		using ValueType = std::remove_reference_t<Ty>;

		using ReboundAlloc = typename AllocTraits::template rebind_alloc<ValueType>;
		ReboundAlloc reboundAllocator(this->allocator);

		if (this->_size + 1 >= this->_capacity) this->reserveImpl<ValueType>(1);
		this->_size++;

		ValueType* reboundStorage = reinterpret_cast<ValueType*>(this->storage);
		const auto correctSize = this->_size - 1;
		ValueType* location = reboundStorage + correctSize;
		std::construct_at(location, std::forward<Ty>(element));
	}

	template <typename Ty>
	void erase(const Ty* where) {
		auto begin = reinterpret_cast<Ty*>(this->storage);
		auto end = begin + this->_size;
		auto pos = const_cast<Ty*>(where);

		if (pos >= begin && pos < end) {
			std::move(pos + 1, end, pos);
			if constexpr (!std::is_trivially_destructible_v<Ty>) std::destroy_at(end - 1);
			--this->_size;
		}
	}
	template <typename Ty>
	void pop() {
		if (this->_size > 0) {
			--this->_size;
			reinterpret_cast<Ty*>(this->storage)[this->_size].~Ty();
		}
	}

	template <typename Ty>
	void destroy() {
		using ReboundAlloc = typename AllocTraits::template rebind_alloc<Ty>;
		ReboundAlloc reboundAllocator(this->allocator);
		Ty* reboundStorage = reinterpret_cast<Ty*>(this->storage);

		for (int i = 0; i < this->_size; i++) {
			Ty* location = reboundStorage + i;
			std::destroy_at(location);
		}

		reboundAllocator.deallocate(reboundStorage, this->_capacity);
		this->storage = nullptr;
	}

	template <typename Ty>
	Ty* begin() { return reinterpret_cast<Ty*>(this->storage); }
	template <typename Ty>
	Ty* end() { return reinterpret_cast<Ty*>(this->storage) + this->_size; }

	template <typename Ty>
	const Ty* begin() const { return reinterpret_cast<const Ty*>(this->storage); }
	template <typename Ty>
	const Ty* end() const { return reinterpret_cast<const Ty*>(this->storage) + this->_size; }

	template <typename Ty>
	Ty* at(const size_t idx) {
		Ty* location = &reinterpret_cast<Ty*>(this->storage)[idx];
		return &reinterpret_cast<Ty*>(this->storage)[idx];
	}

	template <typename Ty>
	FlexibleVector& copy(const FlexibleVector& other) {
		using ReboundAlloc = typename AllocTraits::template rebind_alloc<Ty>;
		ReboundAlloc reboundAllocator(this->allocator);
		Ty* reboundStorage = reinterpret_cast<Ty*>(this->storage);

		if (this != &other) {
			this->_size = other._size;
			this->_capacity = other._capacity;
			this->itemSize = other.itemSize;

			if (other.storage) {
				if constexpr (std::is_trivially_copyable_v<Ty>) {
					reserveImpl(other._capacity);
					memcpy(this->storage, other.storage, other._size * other.itemSize);
				}
				else if constexpr (std::is_copy_constructible_v<Ty>) {
					reserveImpl(other._capacity);
					std::uninitialized_copy(reinterpret_cast<Ty*>(other.storage), reinterpret_cast<Ty*>(reboundStorage) + other._size, reboundStorage);
				}
				else {
					reserveImpl(other._capacity);
					std::uninitialized_move(reinterpret_cast<Ty*>(other.storage), reinterpret_cast<Ty*>(reboundStorage) + other._size, reboundStorage);
				}
			}
		}
		return *this;
	}
	template <typename Ty>
	FlexibleVector& move(FlexibleVector&& other) noexcept {
		if (this != &other) {
			this->storage   = other.storage;
			this->_size     = other._size;
			this->_capacity = other._capacity;
			this->itemSize  = other.itemSize;

			other.storage = nullptr;
		}
		return *this;
	}

	void* operator[](size_t idx) {
		unsigned char* bytePtr = this->storage + (idx * this->itemSize);
		return static_cast<void*>(bytePtr);
	}

	FlexibleVector& operator=(const FlexibleVector& other) {
		if (this != &other) {
			this->_size = other._size;
			this->_capacity = other._capacity;
			this->itemSize = other.itemSize;

			if (other.storage) {
				reserveImpl(other._capacity * other.itemSize);
				memcpy(this->storage, other.storage, other._size * other.itemSize);
			}
		}
		return *this;
	}
	FlexibleVector& operator=(FlexibleVector&& other) noexcept {
		if (this != &other) {
			this->storage   = other.storage;
			this->_size     = other._size;
			this->_capacity = other._capacity;
			this->itemSize  = other.itemSize;

			other.storage = nullptr;
		}
		return *this;
	}
};
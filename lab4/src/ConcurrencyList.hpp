#pragma once

#include <memory>
#include <type_traits>
#include <mutex>
#include <atomic>
#include <list>
#include <iostream>

namespace cl
{
	template<class T>
	class Node
	{
		using value_type = T;
		using nodeptr_type = Node*;
	public:

		Node(const value_type& _Value)
			:
			mNext(nullptr),
			mData(_Value)
		{		}

		Node(value_type&& _Value)
			:
			mNext(nullptr),
			mData(std::move(_Value))
		{		}

		template<class... Args>
		Node(Args&&... _Args)
			:
			mNext(nullptr),
			mData(std::forward<_Args>(_Args)...)
		{		}

		~Node()
		{
			mNext = nullptr;
		}

		value_type mData;
		std::atomic<nodeptr_type> mNext;
	};


	template<class T>
	class readonly_iterator
	{
	public:

		using value_type = T;
		using node_type = Node<T>;
		using nodeptr_type = Node<T>*;
		using this_type = readonly_iterator;

		readonly_iterator(const nodeptr_type& _Ptr)
			:
			mPtr(_Ptr)
		{		}

		readonly_iterator& operator++()
		{
			mPtr = mPtr->mNext.load(std::memory_order_relaxed);
			return *this;
		}

		readonly_iterator operator++(int)
		{
			auto result(*this);
			mPtr = mPtr->mNext.load(std::memory_order_relaxed);
			return result;
		}

		value_type operator*()
		{
			return value_type(mPtr->mData);
		}

		bool operator!=(const readonly_iterator& _OtherIt) const
		{
			return mPtr != _OtherIt.mPtr;
		}

		nodeptr_type mutable mPtr;
	};

	template<class T, class Alloc = std::allocator<T>>
	class ConcurrencyList
	{
		using node_allocator = std::_Rebind_alloc_t<Alloc, Node<T>>;

	public:

		using value_type = T;
		using node_type = Node<T>;
		using nodeptr_type = Node<T>*;
		using atomicnode_type = std::atomic<nodeptr_type>;
		using iterator = readonly_iterator<T>;

		ConcurrencyList(Alloc& _Al)
			:
			mValPair(std::_One_then_variadic_args_t{}, _Al)
		{
			mEmptyHead = mValPair._Get_first().allocate(1);
			mEmptyTail = mValPair._Get_first().allocate(1);
			construct_at(mEmptyHead->mNext, mEmptyTail);
			construct_at(mEmptyTail->mNext, nullptr);

			mValPair._Myval2.first.store(mEmptyHead, std::memory_order_relaxed);
			mValPair._Myval2.second.store(mEmptyHead, std::memory_order_relaxed);
		}

		ConcurrencyList()
			:
			mValPair(std::_Zero_then_variadic_args_t{})
		{
			mEmptyHead = mValPair._Get_first().allocate(1);
			mEmptyTail = mValPair._Get_first().allocate(1);
			construct_at(mEmptyHead->mNext, mEmptyTail);
			construct_at(mEmptyTail->mNext, nullptr);
			
			mValPair._Myval2.first.store(mEmptyHead, std::memory_order_relaxed);
			mValPair._Myval2.second.store(mEmptyHead, std::memory_order_relaxed);
		}

		~ConcurrencyList()
		{
			for (; !empty(); pop());

			mValPair._Get_first().deallocate(mEmptyHead, 1);
			mValPair._Get_first().deallocate(mEmptyTail, 1);
		}

		ConcurrencyList(ConcurrencyList&& _Other) noexcept
			:
			mValPair(std::_One_then_variadic_args_t{}, _Other.mValPair._Get_first())
		{
			mEmptyHead = _Other.mEmptyHead; 
			mEmptyTail = _Other.mEmptyTail; 

			mValPair._Myval2.first.store(mEmptyHead, std::memory_order_relaxed);
			mValPair._Myval2.second.store(mEmptyHead, std::memory_order_relaxed);

			_Other.mEmptyHead = _Other.mValPair._Get_first().allocate(1);
			_Other.mEmptyTail = _Other.mValPair._Get_first().allocate(1);

			construct_at(_Other.mEmptyHead->mNext, _Other.mEmptyTail);
			construct_at(_Other.mEmptyTail->mNext, nullptr);

			_Other.mValPair._Myval2.first.store(_Other.mEmptyHead, std::memory_order_relaxed);
			_Other.mValPair._Myval2.second.store(_Other.mEmptyHead, std::memory_order_relaxed);
		}

		bool empty()
		{
			return mEmptyHead->mNext == mEmptyTail;
		}

		void push(const value_type& _Value)
		{
			if (empty())
				mValPair._Myval2.second.store(mEmptyHead, std::memory_order_release);

			auto node = mValPair._Get_first().allocate(1);
			construct_at(node->mData, _Value);

			emplace_node(node);
		}

		void push(value_type&& _Value)
		{
			if (empty())
				mValPair._Myval2.second.store(mEmptyHead, std::memory_order_release);

			auto node = mValPair._Get_first().allocate(1);
			construct_at(node->mData, std::move(_Value));
			
			emplace_node(node);
		}

		template<class... ArgTypes>
		void emplace(ArgTypes&&... _Args)
		{
			if (empty())
				mValPair._Myval2.second.store(mEmptyHead, std::memory_order_release);

			auto node = mValPair._Get_first().allocate(1);
			construct_at(node->mData, std::forward<ArgTypes>(_Args)...);
			
			emplace_node(node);
		}

		value_type pop()
		{
			auto node = mValPair._Myval2.first.load(std::memory_order_relaxed)->
				mNext.load(std::memory_order_relaxed);
			
			for (auto exchange_node = mValPair._Myval2.first.load(std::memory_order_relaxed);
				!exchange_node->mNext.compare_exchange_weak(
					node, exchange_node->mNext.load(std::memory_order_relaxed)->mNext.load(std::memory_order_relaxed),
					std::memory_order_release, std::memory_order_relaxed););

			auto data = std::move(node->mData);
			mValPair._Get_first().deallocate(node, 1);
			return std::move(data);
		}

		iterator begin()
		{
			return iterator(mEmptyHead->mNext.load(std::memory_order_relaxed));
		}

		iterator end()
		{
			return iterator(mEmptyTail);
		}

		size_t size()
		{
			size_t result = 0;
			for (auto it = begin(); it != end(); ++it)
				result++;
			return result;
		}

	private:

		void emplace_node(nodeptr_type _Node)
		{
			construct_at(_Node->mNext, mEmptyTail);

			std::unique_lock<std::mutex> locker(mTailMutex);

			mValPair._Myval2.second.load(std::memory_order_relaxed)->
				mNext.store(_Node, std::memory_order_relaxed);

			mValPair._Myval2.second.store(_Node, std::memory_order_relaxed);
		}

		template<class Obj, class... Args>
		static void construct_at(Obj& _Obj, Args&&... _Args)
		{
			new (std::addressof(_Obj)) Obj(std::forward<Args>(_Args)...);
		}

		std::_Compressed_pair<node_allocator, std::pair<atomicnode_type, atomicnode_type>> mValPair;
		nodeptr_type mEmptyHead, mEmptyTail;
		std::mutex mTailMutex;
	};
}
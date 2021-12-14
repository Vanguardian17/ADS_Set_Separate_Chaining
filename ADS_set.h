#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <initializer_list>
#include <vector>

template <typename Key, size_t N = 7>
class ADS_set {
public:
	class Iterator;
	using value_type = Key;
	using key_type = Key;
	using reference = key_type&;
	using const_reference = const key_type&;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using iterator = Iterator;
	using const_iterator = Iterator;
	using key_compare = std::less<key_type>;   // B+-Tree
	using key_equal = std::equal_to<key_type>; // Hashing
	using hasher = std::hash<key_type>;        // Hashing
private:
	struct Node
	{
		key_type key;
		Node* next{ nullptr };
		Node* head{ nullptr };
	};
	Node* table{ nullptr };
	size_type table_size{ 0 }, curr_size{ 0 };
	size_type h(const key_type& key) const { return hasher{}(key) % table_size; }
	void rehash(size_type n);
	void reserve(size_type n);
	Node* insert_(const key_type& key);
	Node* find_(const key_type& key) const;
public:
	ADS_set() { rehash(N); }
	ADS_set(std::initializer_list<key_type> ilist) : ADS_set{}
	{
		insert(ilist);
	}
	template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{}
	{
		insert(first, last);
	}
	ADS_set(const ADS_set& other) : ADS_set{}
	{
		rehash(N);
		for (const auto& k : other)
		{
			insert_(k);
		}
	}
	~ADS_set()
	{
		for (size_type idx{ 0 }; idx < table_size; ++idx) {
			for (Node* no = table[idx].head; no != nullptr; no = no->next) {
				delete no;
			}
		}
		delete[] table;
	}

	ADS_set& operator=(const ADS_set& other)
	{
		if (this == &other) return *this;
		ADS_set tmp{ other };
		swap(tmp);
		return *this;
	}
	ADS_set& operator=(std::initializer_list<key_type> ilist)
	{
		//std::cerr << "ads =ilist operator";
		ADS_set tmp{ ilist };
		swap(tmp);
		return *this;
	}

	size_type size() const { return curr_size; }
	bool empty() const { return curr_size == 0; }

	size_type count(const key_type& key) const { return !!find_(key); };
	iterator find(const key_type& key) const
	{
		if (auto p{ find_(key) }) return iterator(p, this->table, h(key), table_size);
		return end();
	}

	void clear()
	{
		ADS_set tmp;
		swap(tmp);
	}
	void swap(ADS_set& other)
	{
		//std::cerr << "swap";
		using std::swap;
		swap(table, other.table);
		swap(curr_size, other.curr_size);
		swap(table_size, other.table_size);
	}

	void insert(std::initializer_list<key_type> ilist) { insert(std::begin(ilist), std::end(ilist)); }
	std::pair<iterator, bool> insert(const key_type& key)
	{
		if (auto p{ find_(key) }) return std::make_pair(const_iterator(p, this->table, h(key), table_size), false); /// >
		insert_(key);
		return std::make_pair(const_iterator(find_(key), this->table, h(key), table_size), true);
	}

	template<typename InputIt> void insert(InputIt first, InputIt last);

	size_type erase(const key_type& key)
	{
		//std::cerr << "erase";
		if (auto p{ find_(key) })
		{
			auto idx = h(key);
			Node* help{ table[idx].head };
			Node* help2{ table[idx].head->next };
			if (p != table[idx].head)
			{
				while (help2 != p)
				{
					help = help->next;
					help2 = help2->next;
				}
				help->next = help2->next;
				delete help2;
				--curr_size;
				return 1;
			}
			table[idx].head = table[idx].head->next;
			help->next = nullptr;
			delete help;
			help2 = nullptr;
			--curr_size;
			return 1;
		}
		return 0;
	}

	const_iterator begin() const
	{
		//std::cerr << "begin";
		for (size_t i = 0; i < table_size; ++i)
			if (table[i].head != nullptr) {
				return const_iterator(table[i].head, this->table, i, table_size);
			}
		return end();
	}
	const_iterator end() const
	{
		//std::cerr << "end";
		return const_iterator(nullptr);
	}

	void dump(std::ostream& o = std::cerr) const;

	friend bool operator==(const ADS_set& lhs, const ADS_set& rhs)
	{
		//std::cerr << "operator ==";
		if (lhs.curr_size != rhs.curr_size) return false;
		for (const auto& k : rhs)
		{
			if (!lhs.count(k)) return false;
		}
		return true;
	}
	friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) { return !(lhs == rhs); }
};

template <typename Key, size_t N>
typename ADS_set<Key, N>::Node* ADS_set<Key, N>::insert_(const key_type& key)
{
	size_type idx = h(key);
	Node* help = new Node;

	help->next = table[idx].head;
	help->key = key;
	table[idx].head = help;
	++curr_size;
	reserve(curr_size);
	return table + idx;

}

template <typename Key, size_t N>
typename ADS_set<Key, N>::Node* ADS_set<Key, N>::find_(const key_type& key) const
{
	//std::cerr << "find_";
	size_type idx = h(key);
	if (table[idx].head == nullptr) return nullptr;
	for (Node* elem{ table[idx].head }; elem != nullptr; elem = elem->next)
	{
		if (key_equal{}(elem->key, key)) return elem;
	}
	return nullptr;
}


template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last)
{
	//std::cerr << "insert";
	for (auto it{ first }; it != last; ++it)
	{
		if (!count(*it))
		{
			insert_(*it);
		}
	}
}

template <typename Key, size_t N>
void ADS_set<Key, N>::reserve(size_type n)
{
	if (n > table_size * 2)
	{
		size_type new_table_size{ table_size };
		do
		{
			new_table_size = new_table_size * 2 + 1;
		} while (n > new_table_size * 2);
		rehash(new_table_size);
	}
}


template <typename Key, size_t N>
void ADS_set<Key, N>::rehash(size_type n)
{
	//std::cerr << "rehashs";
	std::vector<key_type> v;
	size_type new_table_size{ std::max(N, std::max(n,curr_size)) };
	Node* old_table = table;
	Node* new_table = new Node[new_table_size];
	size_type old_table_size = table_size;
	for (size_type idx{ 0 }; idx < old_table_size; ++idx)
	{
		for (Node* no{ table[idx].head }; no != nullptr; no = no->next)
		{
			v.push_back(no->key);
		}
	}

	table_size = new_table_size;
	curr_size = 0;

	table = new_table;
	for (size_type idx{ 0 }; idx < v.size(); ++idx)
	{
		insert_(v[idx]);
	}

	for (size_type idx{ 0 }; idx < old_table_size; ++idx)
	{
		for (Node* no{ old_table[idx].head }; no != nullptr; no = no->next)
		{
			delete no;
		}
	}
	delete[] old_table;
}



template <typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream& o) const
{
	o << "curr_size = " << curr_size << ", table_size = " << table_size << "\n";
	for (size_type idx{ 0 }; idx < table_size; ++idx)
	{
		o << idx << ": ";
		if (table[idx].head == nullptr)
		{
			o << "--free";
		}
		for (Node* no{ table[idx].head }; no != nullptr; no = no->next)
		{
			o << no->key;
			if (no->next == nullptr) break;
			o << " --> ";
		}
		o << "\n";
	}

}




template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
public:
	using value_type = Key;
	using difference_type = std::ptrdiff_t;
	using reference = const value_type&;
	using pointer = const value_type*;
	using iterator_category = std::forward_iterator_tag;
private:
	Node* pos;
	Node* table;
	size_type idx;
	size_type table_size;
public:
	explicit Iterator(Node* pos = nullptr, Node* table = nullptr, size_type idx = 0, size_type table_size = 0) : pos(pos),
		table(table), idx(idx), table_size(table_size)
	{
		//std::cerr << "iterator constructor";
	}
	reference operator*() const { return pos->key; }
	pointer operator->() const { return &pos->key; }

	Iterator& operator++()
	{
		while(idx < table_size)
		{
			if (pos->next != nullptr)
			{
				pos = pos->next;
				return *this;
			}
			++idx;
			if (idx == table_size) { pos = nullptr; return *this; }
			if(table[idx].head!=nullptr)
			{
				pos = table[idx].head;
				return *this;
			}
		}
		return *this;
	}


	Iterator operator++(int)
	{
		auto rc(*this); operator++(); return rc;
	}
	friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return lhs.pos == rhs.pos; }
	friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return !(lhs == rhs); }
};

template <typename Key, size_t N> void swap(ADS_set<Key, N>& lhs, ADS_set<Key, N>& rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H

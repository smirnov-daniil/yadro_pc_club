#pragma once

#include "bimap-base.h"
#include "bimap-element.h"
#include "bimap-iterator.h"

#include <cstddef>
#include <functional>
#include <utility>

template <
    typename Left,
    typename Right,
    typename CompareLeft = std::less<Left>,
    typename CompareRight = std::less<Right>>
class bimap
    : protected bimap_impl::bimap_base<bimap_impl::left_iterator<Left, Right>, CompareLeft>
    , protected bimap_impl::bimap_base<bimap_impl::right_iterator<Left, Right>, CompareRight> {
public:
  using left_t = Left;
  using right_t = Right;

private:
  using node_t = bimap_impl::binode<Left, Right>;
  using left = bimap_impl::bimap_base<bimap_impl::left_iterator<Left, Right>, CompareLeft>;
  using right = bimap_impl::bimap_base<bimap_impl::right_iterator<Left, Right>, CompareRight>;
  using node_no_data_t = bimap_impl::binode_no_data;

public:
  using left_iterator = typename left::iterator;
  using right_iterator = typename right::iterator;

  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight())
      : left(std::move(compare_left))
      , right(std::move(compare_right)) {
    init_base();
  }

  bimap(const bimap& other)
      : left(other)
      , right(other) {
    init_base();
    try {
      for (auto it = other.begin_left(); it != other.end_left(); ++it) {
        insert(*it, *it.flip());
      }
    } catch (...) {
      _free();
      throw;
    }
  }

  bimap(bimap&& other) noexcept
      : left(std::move(other))
      , right(std::move(other))
      , _size(std::exchange(other._size, 0)) {
    init_base();
    std::swap(_sentinel, other._sentinel);
    move_base(other._sentinel);
  }

  bimap& operator=(const bimap& other) {
    if (this != &other) {
      bimap copy(other);
      swap(*this, copy);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap moved(std::move(other));
      swap(*this, moved);
    }
    return *this;
  }

  ~bimap() {
    _free();
  }

  friend void swap(bimap& lhs, bimap& rhs) {
    std::swap(lhs._sentinel, rhs._sentinel);
    std::swap(lhs._size, rhs._size);
    swap_base(lhs, rhs);
  }

  left_iterator insert(const left_t& left, const right_t& right) {
    return _try_emplace(left, right);
  }

  left_iterator insert(const left_t& left, right_t&& right) {
    return _try_emplace(left, std::move(right));
  }

  left_iterator insert(left_t&& left, const right_t& right) {
    return _try_emplace(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return _try_emplace(std::move(left), std::move(right));
  }

private:
  template <typename L, typename R>
  left_iterator _try_emplace(L&& left, R&& right) {
    if (auto lit = lower_bound_left(left); lit == end_left() || left::compare(left, *lit)) {
      if (auto rit = lower_bound_right(right); rit == end_right() || right::compare(right, *rit)) {
        node_t* new_node = new node_t(std::forward<L>(left), std::forward<R>(right));
        ++_size;
        right::emplace_hint(rit, new_node);
        return left::emplace_hint(lit, new_node);
      }
    }
    return end_left();
  }

  void _free() noexcept {
    auto rs = &static_cast<typename right::node_no_data_t&>(_sentinel);
    rs->parent->right = nullptr;
    _free(rs->right);
  }

  static void _free(bimap_impl::node_base* node) noexcept {
    if (node) {
      _free(node->left);
      _free(node->right);
      delete static_cast<node_t*>(static_cast<typename right::node_t*>(node));
    }
  }

private:
  void init_base() {
    left::init(&_sentinel);
    right::init(&_sentinel);
  }

  void move_base(node_no_data_t& other_sentinel) {
    left::move(&_sentinel, &other_sentinel);
    right::move(&_sentinel, &other_sentinel);
  }

  static void swap_base(bimap& lhs, bimap& rhs) {
    left::swap(lhs, rhs);
    right::swap(lhs, rhs);
  }

public:
  left_iterator erase_left(left_iterator it) {
    right::erase(it.flip());
    auto next = left::erase(it);
    delete static_cast<node_t*>(static_cast<typename left::node_t*>(it._node));
    --_size;
    return next;
  }

  right_iterator erase_right(right_iterator it) {
    left::erase(it.flip());
    auto next = right::erase(it);
    delete static_cast<node_t*>(static_cast<typename right::node_t*>(it._node));
    --_size;
    return next;
  }

  bool erase_left(const left_t& left) {
    if (auto it = find_left(left); it != end_left()) {
      erase_left(it);
      return true;
    }
    return false;
  }

  bool erase_right(const right_t& right) {
    if (auto it = find_right(right); it != end_right()) {
      erase_right(it);
      return true;
    }
    return false;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    do {
      first = erase_left(first);
    } while (last != first);
    return first;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    do {
      first = erase_right(first);
    } while (last != first);
    return first;
  }

  left_iterator find_left(const left_t& left) const {
    return left::find(left);
  }

  right_iterator find_right(const right_t& right) const {
    return right::find(right);
  }

  const right_t& at_left(const left_t& key) const {
    return left::at(key);
  }

  const left_t& at_right(const right_t& key) const {
    return right::at(key);
  }

  const right_t& at_left_or_default(const left_t& key)
    requires std::is_default_constructible_v<right_t>
  {
    auto lbl = left::lower_bound(key);
    if (lbl != end_left() && !left::compare(key, *lbl)) {
      return *lbl.flip();
    }
    auto def = right_t{};
    if (auto it = find_right(def); it != end_right()) {
      node_t* new_node = new node_t(key, def);
      right::emplace_hint(erase_right(it), new_node);
      ++_size;
      return *left::emplace_hint(lbl, new_node).flip();
    }
    return *insert(key, std::move(def)).flip();
  }

  const left_t& at_right_or_default(const right_t& key)
    requires std::is_default_constructible_v<left_t>
  {
    auto lbr = right::lower_bound(key);
    if (lbr != end_right() && !right::compare(key, *lbr)) {
      return *lbr.flip();
    }

    auto def = left_t{};
    if (auto it = find_left(def); it != end_left()) {
      node_t* new_node = new node_t(def, key);
      left::emplace_hint(erase_left(it), new_node);
      ++_size;
      return *right::emplace_hint(lbr, new_node).flip();
    }
    return *insert(std::move(def), key);
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left::lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left::upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right::lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right::upper_bound(right);
  }

  left_iterator begin_left() const {
    return left::begin();
  }

  left_iterator end_left() const {
    return left::end();
  }

  right_iterator begin_right() const {
    return right::begin();
  }

  right_iterator end_right() const {
    return right::end();
  }

  bool empty() const {
    return size() == 0;
  }

  std::size_t size() const {
    return _size;
  }

  friend bool operator==(const bimap& lhs, const bimap& rhs) {
    if (&lhs == &rhs) {
      return true;
    }
    if (lhs._size != rhs._size) {
      return false;
    }
    auto left_neq = [&lhs](const left_t& l, const left_t& r) {
      return lhs.left::compare(l, r) || lhs.left::compare(r, l);
    };
    auto right_neq = [&lhs](const right_t& l, const right_t& r) {
      return lhs.right::compare(l, r) || lhs.right::compare(r, l);
    };
    for (auto lhs_it = lhs.begin_left(), rhs_it = rhs.begin_left(); lhs_it != lhs.end_left(); ++lhs_it, ++rhs_it) {
      if (left_neq(*lhs_it, *rhs_it) || right_neq(*lhs_it.flip(), *rhs_it.flip())) {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(const bimap& lhs, const bimap& rhs) {
    return !operator==(lhs, rhs);
  }

private:
  node_no_data_t _sentinel{};
  std::size_t _size{};
};

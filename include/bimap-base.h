#pragma once

#include "bimap-element.h"
#include "bimap-iterator.h"

#include <stdexcept>

#ifdef _MSC_VER
#define MY_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define MY_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace bimap_impl {
template <typename Iterator, typename Compare>
class bimap_base {
  using tag = typename Iterator::tag;
  using type = typename tag::type;
  using flipped_type = typename Iterator::Flipped::type;

protected:
  using iterator = Iterator;
  using node_no_data_t = node_no_data<typename tag::tag>;
  using node_t = node<tag>;

  explicit bimap_base(Compare&& compare)
      : _compare(std::move(compare)) {}

  bimap_base(const bimap_base& other) = default;

  bimap_base(bimap_base&& other) = default;

  iterator erase(iterator it) noexcept {
    auto node = it._node, parent = node->parent, left = node->left, right = node->right;
    auto next = std::next(it);
    if (_sentinel->left == node) {
      _sentinel->left = next._node;
    }
    if (parent == nullptr) {
      _sentinel->right = left ? left : right;
    }
    if (left) {
      if (right) {
        auto prev = std::prev(it);
        prev._node->right = right;
        right->parent = prev._node;
      }
      left->parent = parent;
      if (parent) {
        (parent->left == node ? parent->left : parent->right) = left;
      }
    } else if (right) {
      right->parent = parent;
      if (parent) {
        (parent->right == node ? parent->right : parent->left) = right;
      } else if (right == _sentinel) {
        right->parent = _sentinel;
      }
    } else {
      (parent->left == node ? parent->left : parent->right) = nullptr;
    }
    return next;
  }

  iterator emplace_hint(iterator hint, node_t* node) noexcept {
    if (_sentinel->right == _sentinel) { // empty
      _sentinel->right = _sentinel->left = _sentinel->parent = node;
      node->parent = nullptr;
      node->right = _sentinel;
    } else if (hint == end()) { // most right
      node->parent = _sentinel->parent;
      hint._node->parent->right = node;
      _sentinel->parent = node;
      node->right = _sentinel;
    } else {
      if (hint == begin()) { // most left
        hint._node->left = node;
        _sentinel->left = node;
      } else if (!hint._node->left) {
        hint._node->left = node;
      } else {
        --hint;
        hint._node->right = node;
      }
      node->parent = hint._node;
    }
    return iterator(node);
  }

  iterator lower_bound(const type& value) const {
    node_base *current = _sentinel->right, *bound = _sentinel;
    if (_sentinel->right == _sentinel || compare(*std::prev(end()), value)) {
      return end();
    }
    while (current) {
      const auto& data = static_cast<node_t*>(current)->data;
      if (!compare(data, value)) {
        bound = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }
    return iterator(bound);
  }

  iterator upper_bound(const type& value) const {
    node_base *current = _sentinel->right, *bound = _sentinel;
    if (_sentinel->right == _sentinel || compare(*std::prev(end()), value)) {
      return end();
    }
    while (current) {
      const auto& data = static_cast<node_t*>(current)->data;
      if (compare(value, data)) {
        bound = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }
    return iterator(bound);
  }

  iterator find(const type& value) const {
    auto it = lower_bound(value);
    return it == end() || compare(value, *it) ? end() : it;
  }

  bool compare(const type& left, const type& right) const {
    return _compare(left, right);
  }

  iterator begin() const {
    return iterator(_sentinel->left);
  }

  iterator end() const {
    return iterator(const_cast<node_no_data_t*>(_sentinel));
  }

  const flipped_type& at(const type& key) const {
    if (auto it = find(key); it != end()) {
      return *it.flip();
    }
    throw std::out_of_range("bimap::at");
  }

  void init(node_no_data_t* sentinel) {
    _sentinel = sentinel;
    sentinel->parent->right = sentinel;
  }

  static void swap(bimap_base& lhs, bimap_base& rhs) {
    std::swap(lhs._compare, rhs._compare);
    if (lhs._sentinel->left == rhs._sentinel) {
      lhs._sentinel->left = lhs._sentinel;
    }
    if (lhs._sentinel->right == rhs._sentinel) {
      lhs._sentinel->right = lhs._sentinel;
    }
    if (lhs._sentinel->parent == rhs._sentinel) {
      lhs._sentinel->parent = lhs._sentinel;
    }
    lhs._sentinel->parent->right = lhs._sentinel;
    if (rhs._sentinel->left == lhs._sentinel) {
      rhs._sentinel->left = rhs._sentinel;
    }
    if (rhs._sentinel->right == lhs._sentinel) {
      rhs._sentinel->right = rhs._sentinel;
    }
    if (rhs._sentinel->parent == lhs._sentinel) {
      rhs._sentinel->parent = rhs._sentinel;
    }
    rhs._sentinel->parent->right = rhs._sentinel;
  }

  static void move(node_no_data_t* ths, node_no_data_t* thr) {
    if (ths->left == thr) {
      ths->left = ths;
    }
    if (ths->right == thr) {
      ths->right = ths;
    }
    if (ths->parent == thr) {
      ths->parent = ths;
    }
    ths->parent->right = ths;
    thr->parent = thr->left = thr->right = thr;
  }

  node_no_data_t* sentinel() {
    return _sentinel;
  }

private:
  node_no_data_t* _sentinel{};
  MY_NO_UNIQUE_ADDRESS mutable Compare _compare;
};
} // namespace bimap_impl

#undef MY_NO_UNIQUE_ADDRESS

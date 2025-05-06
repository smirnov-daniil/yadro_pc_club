#pragma once

#include "bimap-element.h"

#include <iterator>

template <typename, typename, typename, typename>
class bimap;

namespace bimap_impl {
template <typename T, typename Left, typename Right>
class iterator {
  template <typename, typename, typename>
  friend class iterator;

  template <typename, typename>
  friend class bimap_base;

  template <typename, typename, typename, typename>
  friend class ::bimap;

  using Flipped = std::conditional_t<std::is_same_v<T, Left>, Right, Left>;

  using tag = T;
  using left_t = typename Left::type;
  using right_t = typename Right::type;

  using binode_t = binode<left_t, right_t>;
  using binode_no_data_t = binode_no_data;
  using node_t = node<T>;
  using node_no_data_t = node_no_data<typename tag::tag>;

private:
  explicit iterator(node_base* node)
      : _node(node) {}

  bool is_end() const {
    return (_node->right && _node->right->parent == nullptr) || _node->right == _node;
  }

public:
  using value_type = typename tag::type;

  using reference = const value_type&;
  using const_reference = const value_type&;

  using difference_type = std::ptrdiff_t;
  using pointer = const value_type*;

  using iterator_category = std::bidirectional_iterator_tag;

public:
  iterator() = default;
  iterator(const iterator& other) = default;
  iterator(iterator&& other) = default;
  iterator& operator=(const iterator& other) = default;
  iterator& operator=(iterator&& other) = default;
  ~iterator() = default;

  const_reference operator*() const {
    return static_cast<node_t*>(_node)->data;
  }

  pointer operator->() const {
    return &static_cast<node_t*>(_node)->data;
  }

  iterator& operator++() {
    if (_node->right) {
      _node = _node->right;
      if (!is_end()) {
        while (_node->left) {
          _node = _node->left;
        }
      }
    } else {
      auto parent = _node->parent;
      while (parent && _node == parent->right) {
        _node = parent;
        parent = _node->parent;
      }
      _node = parent;
    }
    return *this;
  }

  iterator& operator--() {
    if (_node->left && !is_end()) {
      _node = _node->left;
      while (_node->right) {
        _node = _node->right;
      }
    } else {
      auto parent = _node->parent;
      while (parent && _node == parent->left) {
        _node = parent;
        parent = _node->parent;
      }
      _node = parent;
    }
    return *this;
  }

  iterator operator++(int) {
    iterator tmp = *this;
    ++*this;
    return tmp;
  }

  iterator operator--(int) {
    iterator tmp = *this;
    --*this;
    return tmp;
  }

  iterator<Flipped, Left, Right> flip() const {
    if (is_end()) {
      return iterator<Flipped, Left, Right>(static_cast<typename iterator<Flipped, Left, Right>::node_no_data_t*>(
          static_cast<binode_no_data_t*>(static_cast<node_no_data_t*>(_node))
      ));
    }
    return iterator<Flipped, Left, Right>(static_cast<typename iterator<Flipped, Left, Right>::node_t*>(
        static_cast<binode_t*>(static_cast<node_t*>(_node))
    ));
  }

  friend bool operator==(const iterator& lhs, const iterator& rhs) {
    return lhs._node == rhs._node;
  }

  friend bool operator!=(const iterator& lhs, const iterator& rhs) {
    return lhs._node != rhs._node;
  }

private:
  node_base* _node;
};

template <typename Left, typename Right>
using left_iterator = iterator<tagged_t<Left, left>, tagged_t<Left, left>, tagged_t<Right, right>>;

template <typename Left, typename Right>
using right_iterator = iterator<tagged_t<Right, right>, tagged_t<Left, left>, tagged_t<Right, right>>;

} // namespace bimap_impl

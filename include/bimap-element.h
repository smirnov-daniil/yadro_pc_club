#pragma once
#include <utility>

namespace bimap_impl {
class left;
class right;

struct node_base {
  node_base *left{this}, *right{this}, *parent{this};

  node_base() = default;

  explicit node_base(std::nullptr_t)
      : left{nullptr}
      , right{nullptr}
      , parent{nullptr} {}
};

template <typename T>
struct node : node_base {
  typename T::type data;

  template <typename U>
    requires (!std::is_same_v<std::remove_cvref_t<U>, node>)
  explicit node(U&& data)
      : node_base(nullptr)
      , data(std::forward<U>(data)) {}
};

template <typename>
struct node_no_data : node_base {
  node_no_data() = default;
};

template <typename T, typename Tag>
struct tagged_t {
  using type = T;
  using tag = Tag;
};

struct binode_no_data
    : node_no_data<left>
    , node_no_data<right> {
  binode_no_data() = default;
};

template <typename Tl, typename Tr>
struct binode
    : node<tagged_t<Tl, left>>
    , node<tagged_t<Tr, right>> {
  template <typename L, typename R>
  binode(L&& tl, R&& tr)
      : node<tagged_t<Tl, left>>(std::forward<L>(tl))
      , node<tagged_t<Tr, right>>(std::forward<R>(tr)) {}
};
} // namespace bimap_impl

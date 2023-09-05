/*
 * helper.hpp
 *
 * Miscellaneous helpful little tidbits...
 */

/*
 * Copyright (c) 2015, James Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __HELPER_HPP__
#define __HELPER_HPP__

#include <string>
#include <cstring>
#include <iomanip>
#include <map>

// *************************************************************************

namespace Helper {

struct string_repr_out {
  explicit string_repr_out(std::string str) : s(std::move(str)) {}

  void write_on(std::ostream &) const;

  std::string s;
};

inline std::ostream &operator<<(std::ostream &o, const string_repr_out &sro) {
  sro.write_on(o);
  return o;
}

template <class T> inline const T &repr(const T &v) { return v; }

inline string_repr_out repr(const char *s) { return string_repr_out(s); }

inline string_repr_out repr(const std::string &s) { return string_repr_out(s); }

// *************************************************************************

class NotFoundException : public std::exception {
public:
  virtual const char *what() noexcept { return nullptr; }
};

template <class KEYT, class VALT>
inline const VALT &find_or_throw(const std::map<KEYT, VALT> &m,
                                 const KEYT &target) {
  auto i = m.find(target);
  if (i == m.end()) {
    throw NotFoundException();
  }
  return i->second;
}

template <class KEYT, class VALT>
inline VALT &find_or_throw(std::map<KEYT, VALT> &m, const KEYT &target) {
  auto i = m.find(target);
  if (i == m.end()) {
    throw NotFoundException();
  }
  return i->second;
}

// *************************************************************************

template <class T> class Coord {
public:
  Coord() : x(), y() {}

  Coord(T _x, T _y) : x(_x), y(_y) {}

  T x;
  T y;
};

template <class T>
std::ostream &operator<<(std::ostream &o, const Coord<T> &c) {
  o << "{ " << c.x << " x " << c.y << " }";
  return o;
}

template <class T> class Bounds {
public:
  typedef Coord<T> CT;

  Bounds(const CT &_tl, const CT &_br) : tl(_tl), br(_br) {}

  Bounds(T x1, T y1, T x2, T y2) : tl(x1, y1), br(x2, y2) {}

  bool contains(const CT &pos) const {
    return tl.x <= pos.x && tl.y <= pos.y && pos.x <= br.x && pos.y <= br.y;
  }

  void expand(const CT &pos) {
    if (pos.x < tl.x)
      tl.x = pos.x;
    if (pos.y < tl.y)
      tl.y = pos.y;
    if (pos.x > br.x)
      br.x = pos.x;
    if (pos.y > br.y)
      br.y = pos.y;
  }

  CT tl;
  CT br;
};

template <class T>
std::ostream &operator<<(std::ostream &o, const Bounds<T> &b) {
  o << "{ " << b.tl.x << " x " << b.tl.y << " / " << b.br.x << " x " << b.br.y
    << " }";
  return o;
}

// *************************************************************************

typedef const char *CCP;

inline const char *ltrim(const char *string, const char *ws = " \t") {
  return string + strspn(string, ws);
}

inline const char *advance_ws(CCP &source, std::string &dest) {
  size_t l;
  source = ltrim(source);
  l = strcspn(source, "# \t");
  dest = std::string(source, l);
  source = !source[l] || source[l] == '#'? "": source + l + 1;
  return source;
}

// *************************************************************************

template <class MAP_T> struct _map_keys_out {
  _map_keys_out(const MAP_T &c, std::string s)
      : container(c), sep(std::move(s)) {}

  const MAP_T &container;
  std::string sep;
};

template <class STREAM_T, class MAP_T>
STREAM_T &operator<<(STREAM_T &o, const _map_keys_out<MAP_T> &_mko) {
  bool first = true;
  for (auto i = _mko.container.begin(); i != _mko.container.end(); i++) {
    if (first) {
      first = false;
      o << i->first;
    } else {
      o << _mko.sep << i->first;
    }
  }
  return o;
}

template <class MAP_T>
_map_keys_out<MAP_T> map_keys_out(const MAP_T &c,
                                  const std::string &sep = " ") {
  return _map_keys_out<MAP_T>(c, sep);
}

// *************************************************************************

// This is from http://www.cplusplus.com/faq/sequences/strings/split
// TODO: decltype

struct split {
  enum empties_t { empties_ok, no_empties };
};

/*
template <typename Container>
auto split(Container& target,
                 const typename Container::value_type& srcStr,
                 const typename Container::value_type& delimiters,
                 split::empties_t empties = split::empties_ok) {

    Container result = split(srcStr,delimiters,empties);
    std::swap(target,result);
    return target;
}
*/
// template <typename T>
// typename std::add_rvalue_reference<T>::type declval(); // no definition
// required

// decltype(*std::declval<T>()) operator*() { /* ... */ }
template <typename Container>
auto split(const typename Container::value_type &srcStr,
           const typename Container::value_type &delimiters,
           split::empties_t empties = split::empties_ok) {

  Container result;
  size_t current;
  auto next = (size_t)-1;
  do {
    if (empties == split::no_empties) {
      next = srcStr.find_first_not_of(delimiters, next + 1);
      if (next == Container::value_type::npos)
        break;
      next -= 1;
    }
    current = next + 1;
    next = srcStr.find_first_of(delimiters, current);
    result.push_back(srcStr.substr(current, next - current));
  } while (next != Container::value_type::npos);
  return result;
}

// *************************************************************************
// Ignore GCC Unused Result warning

inline void IGUR(...) {
}

// *************************************************************************
// Translate a glob pattern into a regular expression

std::string glob2regex(const char *glob);

// *************************************************************************

} // namespace Helper

// *************************************************************************

#endif // __HELPER_HPP__

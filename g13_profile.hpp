//
// Created by khampf on 13-05-2020.
//

#ifndef G13_G13_PROFILE_HPP
#define G13_G13_PROFILE_HPP

#include <regex>
#include "g13.hpp"
#include "g13_action.hpp"
#include "g13_device.hpp"

namespace G13 {
class G13_Key;
/*!
 * Represents a set of configured key mappings
 *
 * This allows a keypad to have multiple configured
 * profiles and switch between them easily
 */
class G13_Profile {
public:
  G13_Profile(G13::G13_Device &keypad, std::string name_arg)
      : _keypad(keypad), _name(std::move(name_arg)) {
    _init_keys();
  }

  G13_Profile(const G13_Profile &other, std::string name_arg)
      : _keypad(other._keypad), _keys(other._keys), _name(std::move(name_arg)) {
  }

  // search key by G13 keyname
  G13::G13_Key *FindKey(const std::string &keyname);

  std::vector<std::string> FilteredKeyNames(const std::regex &pattern,
                                            bool all = false);

  void dump(std::ostream &o) const;

  void ParseKeys(unsigned char *buf);

  [[nodiscard]] const std::string &name() const { return _name; }

  // [[maybe_unused]] [[nodiscard]] const G13::G13_Manager &manager() const;

protected:
  G13::G13_Device &_keypad;
  std::vector<G13::G13_Key> _keys;
  std::string _name;

  void _init_keys();
};
} // namespace G13

#endif // G13_G13_PROFILE_HPP

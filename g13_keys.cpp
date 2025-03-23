/* This file contains code for managing keys an profiles
 *
 */

// TODO: check out why g13.h needs to be included before libevdev
// clang-format off
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_keys.hpp"
#include "helper.hpp"
// clang-format on

namespace G13 {

// clang-format off
const char* G13_Key_Tables::G13_KEY_STRINGS[] = {
  /* byte 3 */
  "G1", "G2", "G3", "G4", "G5", "G6", "G7", "G8",
  /* byte 4 */
  "G9", "G10", "G11", "G12", "G13", "G14", "G15", "G16",
  /* byte 5 */
  "G17", "G18", "G19", "G20", "G21", "G22", "UNDEF1", "LIGHT_STATE",
  /* byte 6 */
  "BD", "L1", "L2", "L3", "L4", "M1", "M2", "M3",
  /* byte 7 */
  "MR", "LEFT", "DOWN", "TOP", "UNDEF3", "LIGHT", "LIGHT2", "MISC_TOGGLE",
  nullptr
};

const char *G13_Key_Tables::G13_NONPARSED_KEYS[] = {
    "UNDEF1", "LIGHT_STATE", "UNDEF3",     "LIGHT",
    "LIGHT2", "UNDEF3",      "MISC_TOGGLE",
    nullptr
};

const char* G13_Key_Tables::G13_BTN_SEQ[] = {
  "LEFT", "RIGHT", "MIDDLE", "SIDE", "EXTRA",
  nullptr
};
// clang-format on


void G13_Key::dump(std::ostream &o) const {
  o << G13_Manager::Instance()->FindG13KeyName(index()) << "(" << index()
    << ") : ";
  if (action()) {
    action()->dump(o);
  } else {
    o << "(no action)";
  }
}

// *************************************************************************

void G13_Key::ParseKey(const unsigned char *byte, G13_Device *g13) {
  bool key_is_down = byte[_index.offset] & _index.mask;
  auto key_state_changed = g13->update(_index.index, key_is_down);

  if (key_state_changed && _action) {
    _action->act(*g13, key_is_down);
  }
}

// *************************************************************************

} // namespace G13

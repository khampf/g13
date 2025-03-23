//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_KEYS_HPP
#define G13_G13_KEYS_HPP

#include <string>
#include <vector>

namespace G13 {

typedef int G13_KEY_INDEX;
typedef int LINUX_KEY_VALUE;
const LINUX_KEY_VALUE BAD_KEY_VALUE = -1;


/*!
 * Various static key tables.
 */
class G13_Key_Tables {
public:
  /*! sequence containing the
   * G13 keys.  The order is very specific, with the position of each
   * item corresponding to a specific bit in the G13's USB message
   * format.  Do NOT remove or insert items in this list.
   */
  static const char* G13_KEY_STRINGS[];  // formerly G13_KEY_SEQ

  /*! sequence containing the
   * G13 keys that shouldn't be tested input.  These aren't actually keys,
   * but they are in the bitmap defined by G13_KEY_SEQ.
   */
  static const char *G13_NONPARSED_KEYS[];  // formerly G13_NONPARSED_KEY_SEQ

  /*! m_INPUT_BTN_SEQ is a Boost Preprocessor sequence containing the
   * names of button events we can send through binding actions.
   * These correspond to BTN_xxx value definitions in <linux/input.h>,
   * i.e. LEFT is BTN_LEFT, RIGHT is BTN_RIGHT, etc.
   *
   * The binding names have prefix M to avoid naming conflicts.
   * e.g. LEFT keyboard button and LEFT mouse button
   * i.e. LEFT mouse button is named MLEFT, MIDDLE mouse button is MMIDDLE
   */
  static const char* G13_BTN_SEQ[];  // formerly M_INPUT_BTN_SEQ
};


/*!
 * a key code with up/down indicator
 */
class G13_State_Key {
public:
  G13_State_Key(LINUX_KEY_VALUE key = 0, bool down = true) {
    _key = key;
    _down = down;
  }
  LINUX_KEY_VALUE key(void) const { return _key; }
  bool is_down(void) const { return _down; }

private:
  LINUX_KEY_VALUE _key;
  bool _down;
};

} // namespace G13
#endif // G13_G13_KEYS_HPP

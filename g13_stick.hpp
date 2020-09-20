//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_STICK_HPP
#define G13_G13_STICK_HPP

#include <vector>
#include "helper.hpp"

namespace G13 {
class G13_Device;

typedef Helper::Coord<int> G13_StickCoord;
typedef Helper::Bounds<int> G13_StickBounds;
typedef Helper::Coord<unsigned char> G13_ZoneCoord;
typedef Helper::Bounds<unsigned char> G13_ZoneBounds;

// *************************************************************************

class G13_StickZone;

enum stick_mode_t {
  STICK_ABSOLUTE,
  // STICK_RELATIVE,
  STICK_KEYS,
  STICK_CALCENTER,
  STICK_CALBOUNDS,
  STICK_CALNORTH
};

class G13_Stick {
public:
  explicit G13_Stick(G13_Device &keypad);

  void ParseJoystick(const unsigned char *buf);

  void set_mode(stick_mode_t);
  G13_StickZone *zone(const std::string &, bool create = false);
  void RemoveZone(const G13_StickZone &zone);

  /*
    [[nodiscard]] const std::vector<G13_StickZone> &zones() const {
      return m_zones;
    }
  */

  void dump(std::ostream &) const;

protected:
  void RecalcCalibrated();

  G13_Device &_keypad;
  std::vector<G13_StickZone> m_zones;

  G13_StickBounds m_bounds;
  G13_StickCoord m_center_pos;
  G13_StickCoord m_north_pos;

  G13_StickCoord m_current_pos;

  stick_mode_t m_stick_mode;
};

} // namespace G13

#endif // G13_G13_STICK_HPP

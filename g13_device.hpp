//
// Created by khampf on 07-05-2020.
//
#ifndef G13_G13_DEVICE_HPP
#define G13_G13_DEVICE_HPP

#include "g13_lcd.hpp"
#include "g13_manager.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"
#include <functional>
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <map>
#include <vector>
#include <memory>
#include <regex>

namespace G13 {
// *************************************************************************

class G13_Profile;
class G13_Action;
class G13_Manager;

class G13_Font;

typedef std::shared_ptr<G13_Profile> ProfilePtr;
typedef std::shared_ptr<G13_Action> G13_ActionPtr;
typedef std::shared_ptr<G13_Font> FontPtr;

const size_t G13_NUM_KEYS = 40;

class G13_Device {
public:
  G13_Device(libusb_device *dev, libusb_context *ctx,
             libusb_device_handle *handle, int m_id);
  ~G13_Device();

  G13_LCD &lcd() { return m_lcd; }

  // [[nodiscard]] const G13_LCD &lcd() const { return m_lcd; }

  G13_Stick &stick() { return m_stick; }

  // [[nodiscard]] const G13_Stick &stick() const { return m_stick; }

  FontPtr SwitchToFont(const std::string &name);

  void SwitchToProfile(const std::string &name);

  std::vector<std::string> FilteredProfileNames(const std::regex &pattern);

  ProfilePtr Profile(const std::string &name);

  void Dump(std::ostream &o, int detail = 0);

  void Command(char const *str, const char *info = nullptr);

  void ReadCommandsFromPipe();

  void ReadConfigFile(const std::string &filename);

  int ReadKeypresses();

  void parse_joystick(unsigned char *buf);

  G13_ActionPtr MakeAction(const std::string &action);

  void SetKeyColor(int red, int green, int blue);

  void SetModeLeds(int leds);

  void SendEvent(int type, int code, int val);

  void OutputPipeWrite(const std::string &out) const;

  void LcdWrite(unsigned char *data, size_t size);

  // bool is_set(int key);

  bool update(int key, bool v);

  // used by G13_Manager
  void Cleanup();


  void RegisterContext(libusb_context *libusbContext);

  void LcdWriteFile(const std::string &filename);

  G13_Font &current_font() { return *m_currentFont; }

  // G13_Profile &current_profile() { return *m_currentProfile; }

  [[nodiscard]] int id_within_manager() const { return m_id_within_manager; }

  static std::string DescribeLibusbErrorCode(int code);

  // typedef boost::function<void(const char*)> COMMAND_FUNCTION;
  typedef std::function<void(const char *)> COMMAND_FUNCTION;
  typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

  /*
          void setManager(G13_Manager manager) {
              _manager = manager;
          }
  */
  // libusb_device_handle *Handle() const;
  libusb_device *Device() const;

protected:
  void InitFonts();

  void LcdInit();

  void InitCommands();

  // typedef void (COMMAND_FUNCTION)( G13_Device*, const char *, const char * );
  CommandFunctionTable _command_table;

  // struct timeval _event_time;
  struct input_event m_event {};

  int m_id_within_manager;
  libusb_context *m_ctx;

  int m_uinput_fid;

  int m_input_pipe_fid{};
  std::string m_input_pipe_name;
  std::string m_input_pipe_fifo;
  int m_output_pipe_fid{};
  std::string m_output_pipe_name;

  std::map<std::string, FontPtr> pFonts;
  FontPtr m_currentFont;
  std::map<std::string, ProfilePtr> m_profiles;
  ProfilePtr m_currentProfile;

  G13_LCD m_lcd;
  G13_Stick m_stick;

  bool keys[G13_NUM_KEYS]{};

private:
  libusb_device_handle *handle;
  libusb_device *device;
};

/*
inline bool G13_Device::is_set(int key) {
    return keys[key];
}
*/

inline bool G13_Device::update(int key, bool v) {
  bool old = keys[key];
  keys[key] = v;
  return old != v;
}
} // namespace G13

#endif // G13_G13_DEVICE_HPP

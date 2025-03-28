//
// Created by khampf on 07-05-2020.
//

#include "g13_device.hpp"
#include "g13.hpp"
#include "g13_fonts.hpp"
#include "g13_log.hpp"
#include "g13_manager.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"
#include <fstream>
#include <regex>
#include <filesystem>
#include <unistd.h>

namespace G13 {
// *************************************************************************

G13_Device::G13_Device(libusb_device *dev, libusb_context *ctx,
                       libusb_device_handle *handle, int m_id)
    : m_id_within_manager(m_id), m_ctx(ctx), m_uinput_fid(-1),
      m_lcd(*this), m_stick(*this), handle(handle), device(dev) {
  m_currentProfile = std::make_shared<G13_Profile>(*this, "default");
  m_profiles["default"] = m_currentProfile;

  for (bool &key : keys) {
    key = false;
  }

  lcd().image_clear();

  InitFonts();
  InitCommands();
}

// *************************************************************************

std::string G13_Device::DescribeLibusbErrorCode(int code) {
  /*
    auto description = std::string(libusb_error_name(code)) + " (" +
    std::to_string(code) + ") - " +
                       std::string(libusb_strerror((libusb_error)code));
  */
  auto description = std::string(libusb_strerror((libusb_error)code));
  return description;
}

static int G13CreateFifo(const char *fifo_name, mode_t umask) {
  int fd;

  umask |= std::stoi(std::string("0") +
                     G13_Manager::Instance()->getStringConfigValue("umask"),
                     nullptr, 8);
  // mkfifo(g13->fifo_name(), 0777); - didn't work
  mkfifo(fifo_name, 0666);
  fd = open(fifo_name, O_RDWR | O_NONBLOCK);
  chmod(fifo_name, 0777 & ~umask);

  return fd;
}

int G13CreateUinput(G13_Device *g13) {
  struct uinput_user_dev uinp {};
  const char *dev_uinput_fname =
      access("/dev/input/uinput", F_OK) == 0
          ? "/dev/input/uinput"
          : access("/dev/uinput", F_OK) == 0 ? "/dev/uinput" : nullptr;
  if (!dev_uinput_fname) {
    G13_ERR("Could not find an uinput device");
    return -1;
  }
  if (access(dev_uinput_fname, W_OK) != 0) {
    G13_ERR(dev_uinput_fname << " doesn't grant write permissions");
    return -1;
  }
  int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
  if (ufile <= 0) {
    G13_ERR("Could not open uinput");
    return -1;
  }
  memset(&uinp, 0, sizeof(uinp));
  char name[] = "G13";
  memcpy(uinp.name, name, sizeof(name));
  uinp.id.version = 1;
  uinp.id.bustype = BUS_USB;
  uinp.id.product = G13_PRODUCT_ID;
  uinp.id.vendor = G13_VENDOR_ID;
  uinp.absmin[ABS_X] = 0;
  uinp.absmin[ABS_Y] = 0;
  uinp.absmax[ABS_X] = 0xff;
  uinp.absmax[ABS_Y] = 0xff;
  //  uinp.absfuzz[ABS_X] = 4;
  //  uinp.absfuzz[ABS_Y] = 4;
  //  uinp.absflat[ABS_X] = 0x80;
  //  uinp.absflat[ABS_Y] = 0x80;

  ioctl(ufile, UI_SET_EVBIT, EV_KEY);
  ioctl(ufile, UI_SET_EVBIT, EV_ABS);
  /*  ioctl(ufile, UI_SET_EVBIT, EV_REL);*/
  ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
  ioctl(ufile, UI_SET_ABSBIT, ABS_X);
  ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
  /*  ioctl(ufile, UI_SET_RELBIT, REL_X);
   ioctl(ufile, UI_SET_RELBIT, REL_Y);*/
  for (int i = 0; i < 256; i++) {
    ioctl(ufile, UI_SET_KEYBIT, i);
  }

  // Mouse buttons
  for (int i = 0x110; i < 0x118; i++) {
    ioctl(ufile, UI_SET_KEYBIT, i);
  }
  ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

  int retcode = write(ufile, &uinp, sizeof(uinp));
  if (retcode < 0) {
    G13_ERR("Could not write to uinput device (" << retcode << ")");
    return -1;
  }
  retcode = ioctl(ufile, UI_DEV_CREATE);
  if (retcode) {
    G13_ERR("Error creating uinput device for G13");
    return -1;
  }
  return ufile;
}

// *************************************************************************

void G13_Device::SendEvent(int type, int code, int val) {
  using Helper::IGUR;
  memset(&m_event, 0, sizeof(m_event));
  gettimeofday(&m_event.time, nullptr);
  m_event.type = type;
  m_event.code = code;
  m_event.value = val;
  IGUR(write(m_uinput_fid, &m_event, sizeof(m_event)));
}

void G13_Device::OutputPipeWrite(const std::string &out) const {
  using Helper::IGUR;
  IGUR(write(m_output_pipe_fid, out.c_str(), out.size()));
}

void G13_Device::SetModeLeds(int leds) {
  unsigned char usb_data[] = {5, 0, 0, 0, 0};
  usb_data[1] = leds;
  int error = libusb_control_transfer(
      handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x305,
      0, usb_data, 5, 1000);
  if (error != 5) {
    G13_ERR("Problem setting mode LEDs: " + DescribeLibusbErrorCode(error));
    return;
  }
}

void G13_Device::SetKeyColor(int red, int green, int blue) {
  int error;
  unsigned char usb_data[] = {5, 0, 0, 0, 0};
  usb_data[1] = red;
  usb_data[2] = green;
  usb_data[3] = blue;

  error = libusb_control_transfer(
      handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307,
      0, usb_data, 5, 1000);
  if (error != 5) {
    G13_ERR("Problem changing color: " + DescribeLibusbErrorCode(error));
    return;
  }
}

/*! reads and processes key state report from G13
 *
 */
int G13_Device::ReadKeypresses() {
  unsigned char buffer[G13_REPORT_SIZE];
  int size = 0;
  int error =
      libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT,
                                buffer, G13_REPORT_SIZE, &size, 100);

  if (error && error != LIBUSB_ERROR_TIMEOUT) {
    G13_ERR("Error while reading keys: " << DescribeLibusbErrorCode(error));
    if (error == LIBUSB_ERROR_NO_DEVICE || error == LIBUSB_ERROR_IO) {
      G13_DBG("Giving libusb a nudge");
      libusb_handle_events(m_ctx);
    }
  }
  if (size == G13_REPORT_SIZE) {
    parse_joystick(buffer);
    m_currentProfile->ParseKeys(buffer);
    SendEvent(EV_SYN, SYN_REPORT, 0);
  }
  return 0;
}

void G13_Device::ReadCommandsFromFile(const std::string &filename,
                                      const char *info) {
  using std::filesystem::path;

  class in_use {        // Handle file name stack.
  public:
    in_use(G13_Device *dev, const std::string &filename) {
      m_dev = dev;
      m_dev->m_filesLoading.emplace_back(filename);
    }
    ~in_use() { m_dev->m_filesLoading.pop_back(); }
  private:
    G13_Device *m_dev;
  };

  auto filepath = path(filename);

  // If relative and loaded from a file, use previous file directory as base.
  if (filepath.is_relative() && m_filesLoading.size()) {
    auto p = path(m_filesLoading.back());
    filepath = p.replace_filename(filepath);
  }

  filepath = filepath.lexically_normal();
  std::string fn(filepath);

  // Check for load recursion.
  for (auto &f: m_filesLoading)
    if (f == fn) {
      G13_ERR(filename << " loading recursion");
      return;
    }

  in_use autoclean(this, fn);
  std::ifstream s(fn);

  if (s.fail()) G13_LOG(log4cpp::Priority::ERROR << strerror(errno));
  else while (s.good()) {
    // grab a line
    char buf[1024];
    buf[0] = 0;
    buf[sizeof(buf) - 1] = 0;
    s.getline(buf, sizeof(buf) - 1);

    // send it
    Command(buf, info);
  }
}

void G13_Device::ReadConfigFile(const std::string &filename) {

  G13_OUT("reading configuration from " << filename);
  ReadCommandsFromFile(filename, "  cfg");
}

void G13_Device::ReadCommandsFromPipe() {
  fd_set set;
  FD_ZERO(&set);
  FD_SET(m_input_pipe_fid, &set);
  struct timeval tv {};
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  int ret = select(m_input_pipe_fid + 1, &set, nullptr, nullptr, &tv);
  if (ret > 0) {
    auto end = m_input_pipe_fifo.length();
    char buf[1024 * 1024];
    memcpy(buf, m_input_pipe_fifo.c_str(), end);
    ret = read(m_input_pipe_fid, buf + end, sizeof buf - end);
    G13_LOG(log4cpp::Priority::DEBUG << "read " << ret << " characters");

    if (ret < 0)
      ; // Read error: should not occur after successful select().
    else if (ret + end ==
        960) { // TODO probably image, for now, don't test, just assume image
      lcd().Image(reinterpret_cast<unsigned char *>(buf), ret + end);
    } else {
      size_t beg = 0;
      for (ret += end; end < (size_t) ret; end++)
        if (buf[end] == '\r' || buf[end] == '\n') {
          if (end != beg) {
            buf[end] = '\0';
            Command(buf + beg, "command");
          }
          beg = end + 1;
        }
      m_input_pipe_fifo.clear();
      if (ret - beg < sizeof buf)   // Drop too long lines.
        m_input_pipe_fifo = std::string(buf + beg, ret - beg);
    }
  }
}

FontPtr G13_Device::SwitchToFont(const std::string &name) {
  FontPtr rv = pFonts[name];
  if (rv) {
    m_currentFont = rv;
  }
  return rv;
}

void G13_Device::SwitchToProfile(const std::string &name) {
  m_currentProfile = Profile(name);
}

std::vector<std::string>
G13_Device::FilteredProfileNames(const std::regex &pattern) {
  std::vector<std::string> names;

  for (auto &profile: m_profiles)
    if (std::regex_match(profile.first, pattern))
      names.emplace_back(profile.first);
  return names;
}

ProfilePtr G13_Device::Profile(const std::string &name) {
  ProfilePtr rv = m_profiles[name];
  if (!rv) {
    rv = std::make_shared<G13_Profile>(*m_currentProfile, name);
    m_profiles[name] = rv;
  }
  return rv;
}

G13_ActionPtr G13_Device::MakeAction(const std::string &action) {
  if (action.empty()) {
    throw G13_CommandException("empty action string");
  }
  if (action[0] == '>') {
    return G13_ActionPtr(new G13_Action_PipeOut(*this, &action[1]));
  } else if (action[0] == '!') {
    return G13_ActionPtr(new G13_Action_Command(*this, &action[1]));
  } else {
    return G13_ActionPtr(new G13_Action_Keys(*this, action));
  }
  // UNREACHABLE: throw G13_CommandException("can't create action for " +
  // action);
}

// *************************************************************************

void G13_Device::Dump(std::ostream &o, int detail) {
  o << "G13 id=" << id_within_manager() << std::endl;
  o << "   input_pipe_name=" << Helper::repr(m_input_pipe_name) << std::endl;
  o << "   output_pipe_name=" << Helper::repr(m_output_pipe_name) << std::endl;
  o << "   current_profile=" << m_currentProfile->name() << std::endl;
  o << "   current_font=" << m_currentFont->name() << std::endl;

  if (detail > 0) {
    o << "STICK" << std::endl;
    stick().dump(o);
    if (detail == 1) {
      m_currentProfile->dump(o);
    } else {
      for (auto &_profile : m_profiles) {
        _profile.second->dump(o);
      }
    }
  }
}

struct commandAdder {
  commandAdder(G13_Device::CommandFunctionTable &t, const char *name)
      : _t(t), _name(name) {}

  commandAdder(G13_Device::CommandFunctionTable &t, const char *name,
               G13_Device::COMMAND_FUNCTION f)
      : _t(t), _name(name) {
    _t[_name] = std::move(f);
  }

  G13_Device::CommandFunctionTable &_t;
  std::string _name;

  commandAdder &operator+=(G13_Device::COMMAND_FUNCTION f) {
    _t[_name] = std::move(f);
    return *this;
  };
};

void G13_Device::InitCommands() {
  using Helper::advance_ws;
  using Helper::ltrim;
  using Helper::glob2regex;
  // const char *remainder;

  commandAdder add_out(_command_table, "out", [this](const char *remainder) {
    lcd().WriteString(remainder);
  });

  commandAdder add_pos(_command_table, "pos", [this](const char *remainder) {
    int row, col;
    if (sscanf(remainder, " %i %i", &row, &col) == 2) {
      lcd().WritePos(row, col);
    } else {
      G13_ERR("bad pos : " << remainder);
    }
  });

  commandAdder add_bind(_command_table, "bind", [this](const char *remainder) {
    const char *rawaction;
    std::string keyname, action, actionup;
    advance_ws(remainder, keyname);
    rawaction = ltrim(remainder);
    advance_ws(remainder, action);
    advance_ws(remainder, actionup);
    if (!action.empty() && strchr("!>", action[0]))
      action = std::string(rawaction);
    else if (!actionup.empty())
      action += std::string(" ") + actionup;
    try {
      if (auto key = m_currentProfile->FindKey(keyname)) {
        key->set_action(MakeAction(action));
      } else if (auto stick_key = m_stick.zone(keyname)) {
        stick_key->set_action(MakeAction(action));
      } else {
        G13_ERR("bind key " << keyname << " unknown");
        return;
      }
      G13_LOG(log4cpp::Priority::DEBUG << "bind " << keyname << " [" << action
                                       << "]");
    } catch (const std::exception &ex) {
      G13_ERR("bind " << keyname << " " << action << " failed : " << ex.what());
    }
  });

  commandAdder add_profile(
      _command_table, "profile", [this](const char *remainder) {
        std::string profile;
        advance_ws(remainder, profile);
        SwitchToProfile(profile.c_str());
      });

  commandAdder add_font(_command_table, "font", [this](const char *remainder) {
    std::string font;
    advance_ws(remainder, font);
    SwitchToFont(font.c_str());
  });

  commandAdder add_mod(_command_table, "mod", [this](const char *remainder) {
    SetModeLeds(atoi(remainder));
  });

  commandAdder add_textmode(
      _command_table, "textmode",
      [this](const char *remainder) { lcd().text_mode = atoi(remainder); });

  commandAdder add_rgb(_command_table, "rgb", [this](const char *remainder) {
    int red, green, blue;
    if (sscanf(remainder, " %i %i %i", &red, &green, &blue) == 3) {
      SetKeyColor(red, green, blue);
    } else {
      G13_ERR("rgb bad format: <" << remainder << ">");
    }
  });

  commandAdder add_stickmode(
      _command_table, "stickmode", [this](const char *remainder) {
        std::string mode;
        advance_ws(remainder, mode);
        // TODO: this could be part of a G13::Constants class I think
        const std::string modes[] = {"ABSOLUTE",  "KEYS",
                                     "CALCENTER", "CALBOUNDS",
                                     "CALNORTH"};
        int index = 0;
        for (auto &test : modes) {
          if (test == mode) {
            m_stick.set_mode((G13::stick_mode_t)index);
            return;
          }
          index++;
        }
        G13_ERR("unknown stick mode : <" << mode << ">");
      });

  commandAdder add_stickzone(
      _command_table, "stickzone", [this](const char *remainder) {
        std::string operation, zonename;
        advance_ws(remainder, operation);
        advance_ws(remainder, zonename);
        if (operation == "add") {
          /* G13_StickZone* zone = */
          m_stick.zone(zonename, true);
        } else {
          G13_StickZone *zone = m_stick.zone(zonename);
          if (!zone) {
            throw G13_CommandException("unknown stick zone");
          }
          if (operation == "action") {
            zone->set_action(MakeAction(remainder));
          } else if (operation == "bounds") {
            double x1, y1, x2, y2;
            if (sscanf(remainder,
                       " %lf %lf %lf %lf", &x1, &y1, &x2, &y2) != 4) {
              throw G13_CommandException("bad bounds format");
            }
            zone->set_bounds(G13_ZoneBounds(x1, y1, x2, y2));

          } else if (operation == "del") {
            m_stick.RemoveZone(*zone);
          } else {
            G13_ERR("unknown stickzone operation: <" << operation << ">");
          }
        }
      });

  commandAdder add_dump(_command_table, "dump", [this](const char *remainder) {
    std::string target;
    advance_ws(remainder, target);
    if (target == "all") {
      Dump(std::cout, 3);
    } else if (target == "current") {
      Dump(std::cout, 1);
    } else if (target == "summary") {
      Dump(std::cout, 0);
    } else {
      G13_ERR("unknown dump target: <" << target << ">");
    }
  });

  commandAdder add_log_level(_command_table, "log_level",
                             [this](const char *remainder) {
                               std::string level;
                               advance_ws(remainder, level);
                               G13_Manager::Instance()->SetLogLevel(level);
                             });

  commandAdder add_refresh(
      _command_table, "refresh",
      [this](const char *remainder) { lcd().image_send(); });

  commandAdder add_clear(_command_table, "clear",
                         [this](const char *remainder) {
                           lcd().image_clear();
                           lcd().image_send();
                         });

  commandAdder add_delete(_command_table, "delete",
                          [this](const char *remainder) {
    std::string target;
    std::string glob;
    bool found = false;
    advance_ws(remainder, target);
    advance_ws(remainder, glob);
    std::regex re(glob2regex(glob.c_str()));

    if (target == "profile") {
      for (auto &profile: FilteredProfileNames(re)) {
        m_profiles.erase(profile);
        G13_OUT("profile " << profile << " deleted");
        found = true;
      }
    } else if (target == "key") {
      for (auto key: m_currentProfile->FilteredKeyNames(re)) {
        m_currentProfile->FindKey(key)->set_action(nullptr);
        G13_OUT("key " << key << " unbound");
        found = true;
      }
    } else if (target == "zone") {
      for (auto &zone: m_stick.FilteredZoneNames(re)) {
        m_stick.RemoveZone(*m_stick.zone(zone));
        G13_OUT("stickzone " << zone << " unbound");
        found = true;
      }
    } else {
      G13_ERR("unknown delete target: <" << target << ">");
      found = true;
    }
    if (!found)
      G13_OUT("no " << target << " name matches <" << glob << ">");
  });

  commandAdder add_load(_command_table, "load", [this](const char *remainder) {
    std::string filename;
    advance_ws(remainder, filename);
    ReadCommandsFromFile(filename,
                         std::string(1 + m_filesLoading.size(), '>').c_str());
  });
}

void G13_Device::Command(char const *str, const char *info) {
  const char *remainder = str;

  try {
    using Helper::advance_ws;
    using Helper::ltrim;

    std::string cmd;
    advance_ws(remainder, cmd);

    if (cmd != "") {
      auto i = _command_table.find(cmd);
      if (info)
        G13_OUT(info << ": " << ltrim(str));
      if (i == _command_table.end()) {
        G13_ERR("unknown command : " << cmd);
      } else {
        COMMAND_FUNCTION f = i->second;
        f(remainder);
      }
    }
  } catch (const std::exception &ex) {
    G13_ERR("command failed : " << ex.what());
  }
}

void G13_Device::RegisterContext(libusb_context *libusbContext) {
  m_ctx = libusbContext;

  int leds = 0;
  int red = 0;
  int green = 0;
  int blue = 255;
  LcdInit();

  SetModeLeds(leds);
  SetKeyColor(red, green, blue);

  m_uinput_fid = G13CreateUinput(this);
  m_input_pipe_name = G13_Manager::Instance()->MakePipeName(this, true);
  m_input_pipe_fid = G13CreateFifo(m_input_pipe_name.c_str(),
                                   S_IRGRP | S_IROTH);
  if (m_input_pipe_fid == -1) {
    G13_ERR("failed opening input pipe " << m_input_pipe_name);
  }
  m_output_pipe_name = G13_Manager::Instance()->MakePipeName(this, false);
  m_output_pipe_fid = G13CreateFifo(m_output_pipe_name.c_str(),
                                    S_IWGRP | S_IWOTH);
  if (m_output_pipe_fid == -1) {
    G13_ERR("failed opening output pipe " << m_output_pipe_name);
  }
}

void G13_Device::Cleanup() {
  SetKeyColor(0, 0, 0);
  remove(m_input_pipe_name.c_str());
  remove(m_output_pipe_name.c_str());
  ioctl(m_uinput_fid, UI_DEV_DESTROY);
  close(m_uinput_fid);
  libusb_release_interface(handle, 0);
  libusb_close(handle);
}

G13_Device::~G13_Device() {
  Cleanup();
}

// libusb_device_handle *G13_Device::Handle() const { return handle; }

libusb_device *G13_Device::Device() const { return device; }

} // namespace G13

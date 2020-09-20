//
// Created by khampf on 13-05-2020.
//

#include "g13_manager.hpp"
#include "g13_device.hpp"
#include "g13_keys.hpp"
#include "helper.hpp"
#include <csignal>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <log4cpp/OstreamAppender.hh>
#include <memory>

namespace G13 {

// definitions
bool G13_Manager::running = true;
std::map<std::string, std::string> G13_Manager::stringConfigValues;
libusb_context *G13_Manager::libusbContext;
std::vector<G13::G13_Device *> G13_Manager::g13s;
libusb_hotplug_callback_handle G13_Manager::hotplug_cb_handle[3];
const int G13_Manager::class_id = LIBUSB_HOTPLUG_MATCH_ANY;

std::map<G13_KEY_INDEX, std::string> G13_Manager::g13_key_to_name;
std::map<std::string, G13_KEY_INDEX> G13_Manager::g13_name_to_key;
std::map<LINUX_KEY_VALUE, std::string> G13_Manager::input_key_to_name;
std::map<std::string, LINUX_KEY_VALUE> G13_Manager::input_name_to_key;

libusb_device **G13_Manager::devs;
std::string G13_Manager::logoFilename;

G13_Manager::G13_Manager() /* : libusbContext(nullptr), devs(nullptr)*/ {
  InitKeynames();
}

void G13_Manager::Cleanup() {
  G13_OUT("Cleaning up");
  for (auto handle : hotplug_cb_handle) {
    libusb_hotplug_deregister_callback(libusbContext, handle);
  }
  // TODO: This might be better with an iterator and also g13s.erase(iter)
  for (auto g13 : g13s) {

    // g13->Cleanup();
    delete g13;
  }
  libusb_exit(libusbContext);
}

void G13_Manager::InitKeynames() {

  int key_index = 0;

  // setup maps to let us convert between strings and G13 key names
  for (auto &name : G13::G13_KEY_STRINGS) {
    g13_key_to_name[key_index] = name;
    g13_name_to_key[name] = key_index;
    G13_OUT("mapping G13 " << name << " = " << key_index);
    key_index++;
  }

  // setup maps to let us convert between strings and linux key names
  for (auto &symbol : G13::G13_SYMBOLS) {
    auto keyname = std::string("KEY_" + std::string(symbol));

    int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
    if (code < 0) {
      G13_ERR("No input event code found for " << keyname);
    } else {
      // TODO: this seems to map ok but the result is off
      // assert(keyname.compare(libevdev_event_code_get_name(EV_KEY,code)) ==
      // 0); linux/input-event-codes.h

      input_key_to_name[code] = symbol;
      input_name_to_key[symbol] = code;
      G13_OUT("mapping " << symbol << " " << keyname << "=" << code);
    }
  }

  // setup maps to let us convert between strings and linux button names
  for (auto &symbol : G13::G13_BTN_SEQ) {
    auto name = std::string("M" + std::string(symbol));
    auto keyname = std::string("BTN_" + std::string(symbol));
    int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str());
    if (code < 0) {
      G13_ERR("No input event code found for " << keyname);
    } else {
      input_key_to_name[code] = name;
      input_name_to_key[name] = code;
      G13_OUT("mapping " << name << " " << keyname << "=" << code);
    }
  }
}

void G13_Manager::SignalHandler(int signal) {
  G13_OUT("Caught signal " << signal << " (" << strsignal(signal) << ")");
  running = false;
  // TODO: Should we break usblib handling with a reset?
}

std::string G13_Manager::getStringConfigValue(const std::string &name) {
  try {
    return Helper::find_or_throw(stringConfigValues, name);
  } catch (...) {
    return "";
  }
}

void G13_Manager::setStringConfigValue(const std::string &name,
                                       const std::string &value) {
  G13_DBG("setStringConfigValue " << name << " = " << Helper::repr(value));
  stringConfigValues[name] = value;
}

std::string G13_Manager::MakePipeName(G13::G13_Device *d, bool is_input) {
  if (is_input) {
    std::string config_base = getStringConfigValue("pipe_in");
    if (!config_base.empty()) {
      if (d->id_within_manager() == 0) {
        return config_base;
      } else {
        return config_base + "-" + std::to_string(d->id_within_manager());
      }
    }
    return CONTROL_DIR + "g13-" + std::to_string(d->id_within_manager());
  } else {
    std::string config_base = getStringConfigValue("pipe_out");
    if (!config_base.empty()) {
      if (d->id_within_manager() == 0) {
        return config_base;
      } else {
        return config_base + "-" + std::to_string(d->id_within_manager());
      }
    }

    return CONTROL_DIR + "g13-" + std::to_string(d->id_within_manager()) +
           "_out";
  }
}

G13::LINUX_KEY_VALUE G13_Manager::FindG13KeyValue(const std::string &keyname) {
  auto i = g13_name_to_key.find(keyname);
  if (i == g13_name_to_key.end()) {
    return G13::BAD_KEY_VALUE;
  }
  return i->second;
}

G13::LINUX_KEY_VALUE
G13_Manager::FindInputKeyValue(const std::string &keyname) {
  // if there is a KEY_ prefix, strip it off
  if (!strncmp(keyname.c_str(), "KEY_", 4)) {
    return FindInputKeyValue(keyname.c_str() + 4);
  }

  auto i = input_name_to_key.find(keyname);
  if (i == input_name_to_key.end()) {
    return G13::BAD_KEY_VALUE;
  }
  return i->second;
}

std::string G13_Manager::FindInputKeyName(G13::LINUX_KEY_VALUE v) {
  try {
    return Helper::find_or_throw(input_key_to_name, v);
  } catch (...) {
    return "(unknown linux key)";
  }
}

std::string G13_Manager::FindG13KeyName(G13::G13_KEY_INDEX v) {
  try {
    return Helper::find_or_throw(g13_key_to_name, v);
  } catch (...) {
    return "(unknown G13 key)";
  }
}

void G13_Manager::DisplayKeys() {
  typedef std::map<std::string, int> mapType;
  G13_OUT("Known keys on G13:");
  G13_OUT(Helper::map_keys_out(g13_name_to_key));

  G13_OUT("Known keys to map to:");
  G13_OUT(Helper::map_keys_out(input_name_to_key));
}

int G13_Manager::Run() {
  DisplayKeys();

  ssize_t cnt;
  int error;

  error = libusb_init(&libusbContext);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("libusb initialization error: "
            << G13_Device::DescribeLibusbErrorCode(error));
    Cleanup();
    return EXIT_FAILURE;
  }
  libusb_set_option(libusbContext, LIBUSB_OPTION_LOG_LEVEL, 3);

  if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
    cnt = libusb_get_device_list(libusbContext, &devs);
    if (cnt < 0) {
      G13_ERR("Error while getting device list");
      Cleanup();
      return EXIT_FAILURE;
    }

    DiscoverG13s(devs, cnt);
    libusb_free_device_list(devs, 1);
    G13_OUT("Found " << g13s.size() << " G13s");
    if (g13s.empty()) {
      G13_ERR("Unable to open any device");
      Cleanup();
      return EXIT_FAILURE;
    }
  } else {
    ArmHotplugCallbacks();
  }

  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  for (auto g13 : g13s) {
    // This can not be done from the event handler (will give LIBUSB_ERROR_BUSY)
    SetupDevice(g13);
  }

  do {
    if (g13s.empty()) {
      G13_OUT("Waiting for device to show up ...");
      error = libusb_handle_events(libusbContext);
      G13_DBG("USB Event wakeup with " << g13s.size() << " devices registered");
      if (error != LIBUSB_SUCCESS) {
        G13_ERR("Error: " << G13_Device::DescribeLibusbErrorCode(error));
      } else {
        for (auto g13 : g13s) {
          // This can not be done from the event handler (will give
          // LIBUSB_ERROR_BUSY)
          SetupDevice(g13);
        }
      }
    }

    // Main loop
    for (auto g13 : g13s) {
      int status = g13->ReadKeypresses();
      if (!g13s.empty()) {
        // Cleanup might have removed the object before this loop has run
        // TODO: This will not work with multiplt devices and can be better
        g13->ReadCommandsFromPipe();
      };
      if (status < 0) {
        running = false;
      }
    }
  } while (running);

  Cleanup();
  G13_OUT("Exit");
  return EXIT_SUCCESS;
}

/*
    libusb_context *G13_Manager::getCtx() {
        return libusbContext;
    }

    void G13_Manager::setCtx(libusb_context *newLibusbContext) {
        libusbContext = newLibusbContext;
    }
*/

/*
    const std::string &G13_Manager::getLogoFilename() {
        return logoFilename;
    }
*/

void G13_Manager::setLogoFilename(const std::string &newLogoFilename) {
  logoFilename = newLogoFilename;
}

} // namespace G13

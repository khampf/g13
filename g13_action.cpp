//
// Created by khampf on 07-05-2020.
//

#include "g13_action.hpp"
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_manager.hpp"

// *************************************************************************
namespace G13 {
G13_Action::~G13_Action() = default;

G13_Action_Keys::G13_Action_Keys(G13_Device &keypad,
                                 const std::string &keys_string)
    : G13_Action(keypad) {

  auto scan = [](std::string in, std::vector<G13::G13_State_Key> &out) {
    auto keys = Helper::split<std::vector<std::string>>(in, "+");
    for (auto &key : keys) {
      auto kval = G13_Manager::Instance()->FindInputKeyValue(key);
      if (kval.key() == BAD_KEY_VALUE) {
        throw G13_CommandException("create action unknown key : " + key);
      }
      out.push_back(kval);
    }
  };

  auto keydownup = Helper::split<std::vector<std::string>>(keys_string, " ");

  scan(keydownup[0], _keys);
  if (keydownup.size()>1)
    scan(keydownup[1], _keysup);
}

G13_Action_Keys::~G13_Action_Keys() = default;

void G13_Action_Keys::act(G13_Device &g13, bool is_down) {
  auto downkeys = std::vector<bool>(G13_Manager::InputKeyMax(), false);

  auto send_key = [&](LINUX_KEY_VALUE key, bool down) {
    g13.SendEvent(EV_KEY, key, down);
    downkeys[key] = down;
    G13_LOG(log4cpp::Priority::DEBUG << "sending KEY " <<
            (down? "DOWN ": "UP ") << key);
  };
  auto send_keys = [&](std::vector<G13_State_Key> &keys) {
    for (auto &key : keys) {
      if (key.is_down() && downkeys[key.key()])
        send_key(key.key(), false);
      send_key(key.key(), key.is_down());
    }
  };
  auto release_keys = [&](std::vector<G13_State_Key> &keys) {
    for (auto i = keys.size(); i--;)
      if (downkeys[keys[i].key()])
        send_key(keys[i].key(), false);
  };

  if (is_down) {
    send_keys(_keys);
    if (!_keysup.empty())
      release_keys(_keys);
  }
  else if(_keysup.empty()) {
    for (auto &key : _keys)
      downkeys[key.key()] = key.is_down();
    release_keys(_keys);
  }
  else {
    send_keys(_keysup);
    release_keys(_keysup);
  }
}

void G13_Action_Keys::dump(std::ostream &out) const {
  out << " SEND KEYS: ";

  for (size_t i = 0; i < _keys.size(); i++) {
    if (i)
      out << " + ";
    if (!_keys[i].is_down())
      out << "-";
    out << G13_Manager::Instance()->FindInputKeyName(_keys[i].key());
  }
}

G13_Action_PipeOut::G13_Action_PipeOut(G13_Device &keypad,
                                       const std::string &out)
    : G13_Action(keypad), _out(out + "\n") {}

G13_Action_PipeOut::~G13_Action_PipeOut() = default;

void G13_Action_PipeOut::act(G13_Device &kp, bool is_down) {
  if (is_down) {
    kp.OutputPipeWrite(_out);
  }
}

void G13_Action_PipeOut::dump(std::ostream &o) const {
  o << "WRITE PIPE : " << Helper::repr(_out);
}

G13_Action_Command::G13_Action_Command(G13_Device &keypad, std::string cmd)
    : G13_Action(keypad), _cmd(std::move(cmd)) {}

G13_Action_Command::~G13_Action_Command() = default;

void G13_Action_Command::act(G13_Device &kp, bool is_down) {
  if (is_down) {
    keypad().Command(_cmd.c_str());
  }
}

void G13_Action_Command::dump(std::ostream &o) const {
  o << "COMMAND : " << Helper::repr(_cmd);
}

/*
    // inlines
    inline G13_Manager& G13_Action::manager() {
        return m_keypad.manager();
    }
*/

/*inline const G13_Manager& G13_Action::manager() const {
    return m_keypad.manager();
}*/
} // namespace G13

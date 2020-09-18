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
  auto keydownup = Helper::split<std::vector<std::string>>(keys_string, " ");
  auto keys = Helper::split<std::vector<std::string>>(keydownup[0], "+");

  for (auto &key : keys) {
    auto kval = G13_Manager::Instance()->FindInputKeyValue(key);
    if (kval == BAD_KEY_VALUE) {
      throw G13_CommandException("create action unknown key : " + key);
    }
    _keys.push_back(kval);
  }

  if (keydownup.size()>1){
    auto keysup = Helper::split<std::vector<std::string>>(keydownup[1], "+");
    for (auto &key : keysup) {
      auto kval = G13_Manager::Instance()->FindInputKeyValue(key);
      if (kval == BAD_KEY_VALUE) {
        throw G13_CommandException("create action unknown key : " + key);
      }
      _keysup.push_back(kval);
    }
  }
  // are these required?
  //std::vector<int> _keys_local;
  //std::vector<int> _keysup_local;
}

G13_Action_Keys::~G13_Action_Keys() = default;

void G13_Action_Keys::act(G13_Device &g13, bool is_down) {
  if (is_down) {
    for (int &_key : _keys) {
      g13.SendEvent(EV_KEY, _key, is_down);
      G13_LOG(log4cpp::Priority::DEBUG << "sending KEY DOWN " << _key);
    }
    if (!_keysup.empty()) for (int i = _keys.size() - 1; i >= 0; i--) {
      g13.SendEvent(EV_SYN, SYN_REPORT, 0);
      g13.SendEvent(EV_KEY, _keys[i], !is_down);
      G13_LOG(log4cpp::Priority::DEBUG << "sending KEY UP " << _keys[i]);
    }
  } else {
    if(_keysup.empty()) for (int i = _keys.size() - 1; i >= 0; i--) {
      g13.SendEvent(EV_KEY, _keys[i], is_down);
      G13_LOG(log4cpp::Priority::DEBUG << "sending KEY UP " << _keys[i]);
    } else {
      for (int &_key : _keysup) {
        g13.SendEvent(EV_KEY, _key, !is_down);
        G13_LOG(log4cpp::Priority::DEBUG << "sending KEY DOWN " << _key);
      }
      g13.SendEvent(EV_SYN, SYN_REPORT, 0);
      for (int i = _keysup.size() - 1; i >= 0; i--) {
        g13.SendEvent(EV_KEY, _keysup[i], is_down);
        G13_LOG(log4cpp::Priority::DEBUG << "sending KEY UP " << _keysup[i]);
      }
    }
  }
}

void G13_Action_Keys::dump(std::ostream &out) const {
  out << " SEND KEYS: ";

  for (size_t i = 0; i < _keys.size(); i++) {
    if (i) out << " + ";
    out << G13_Manager::Instance()->FindInputKeyName(_keys[i]);
  }

  if (!_keysup.empty()) {
    out << ", ";
    for (size_t i = 0; i < _keysup.size(); i++) {
      if (i) out << " + ";
      out << G13_Manager::Instance()->FindInputKeyName(_keysup[i]);
    }
  }
}

G13_Action_PipeOut::G13_Action_PipeOut(G13_Device &keypad,
       	                               const std::string &out,
                                       const bool split)
    : G13_Action(keypad) {
  if (split) {
    std::size_t up = out.find('|');
    if (up!=std::string::npos && up < out.length() - 1)
      _outup = out.substr(up+1,std::string::npos) + "\n";
    if (up) _out = out.substr(0,up) + "\n";
  } else _out = out + "\n";
  // are these required?
  //std::string _out_local;
  //std::string _outup_local;
}

G13_Action_PipeOut::~G13_Action_PipeOut() = default;

void G13_Action_PipeOut::act(G13_Device &kp, bool is_down) {
  if (is_down) {
    if(!_out.empty()) kp.OutputPipeWrite(_out);
  } else if(!_outup.empty()) {
    kp.OutputPipeWrite(_outup);
  }
}

void G13_Action_PipeOut::dump(std::ostream &o) const {
  o << "WRITE PIPE : " << Helper::repr(_out);
  if (!_outup.empty()) o << ", " << Helper::repr(_outup);
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

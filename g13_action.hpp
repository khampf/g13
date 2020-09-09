//
// Created by khampf on 07-05-2020.
//

#ifndef G13_G13_ACTION_HPP
#define G13_G13_ACTION_HPP

#include "g13.hpp"
#include "g13_keys.hpp"
#include "g13_manager.hpp"
#include "g13_stick.hpp"
#include <memory>
#include <vector>

namespace G13 {

class G13_Device;
class G13_Manager;
class G13_Profile;

// typedef std::shared_ptr<G13_Profile> ProfilePtr;
// class G13_Manager;

// *************************************************************************

/*! holds potential actions which can be bound to G13 activity
 *
 */
class G13_Action {
public:
  explicit G13_Action(G13_Device &keypad) : _keypad(keypad) {}
  virtual ~G13_Action();

  virtual void act(G13_Device &, bool is_down) = 0;
  virtual void dump(std::ostream &) const = 0;

  void act(bool is_down) { act(keypad(), is_down); }

  G13_Device &keypad() { return _keypad; }

  // [[maybe_unused]] [[nodiscard]] const G13_Device& keypad() const { return
  // m_keypad; }

  // [[nodiscard]] const G13_Manager& manager() const;

private:
  G13_Device &_keypad;
};

typedef std::shared_ptr<G13_Action> G13_ActionPtr;

/*!
 * action to send one or more keystrokes
 */
class G13_Action_Keys : public G13_Action {
public:
  G13_Action_Keys(G13_Device &keypad, const std::string &keys);
  ~G13_Action_Keys() override;

  void act(G13_Device &, bool is_down) override;
  void dump(std::ostream &) const override;

  std::vector<LINUX_KEY_VALUE> _keys;
  std::vector<LINUX_KEY_VALUE> _keysup;
};

/*!
 * action to send a string to the output pipe
 */
class G13_Action_PipeOut : public G13_Action {
public:
  G13_Action_PipeOut(G13_Device &keypad, const std::string &out, const bool split);
  ~G13_Action_PipeOut() override;

  void act(G13_Device &, bool is_down) override;
  void dump(std::ostream &) const override;

  std::string _out;
  std::string _outup;
};

/*!
 * action to send a command to the g13
 */
class G13_Action_Command : public G13_Action {
public:
  G13_Action_Command(G13_Device &keypad, std::string cmd);
  ~G13_Action_Command() override;

  void act(G13_Device &, bool is_down) override;
  void dump(std::ostream &) const override;

  std::string _cmd;
};

// *************************************************************************
template <class PARENT_T> class G13_Actionable {
public:
  G13_Actionable(PARENT_T &parent_arg, std::string name)
      : _parent_ptr(&parent_arg), _name(std::move(name)) {}
  virtual ~G13_Actionable() { _parent_ptr = nullptr; }

  [[nodiscard]] G13_ActionPtr action() const { return _action; }
  [[nodiscard]] const std::string &name() const { return _name; }
  // PARENT_T& parent() { return *_parent_ptr; }
  // [[nodiscard]] const PARENT_T& parent() const { return *_parent_ptr; }
  // G13_Manager& manager() { return _parent_ptr->manager(); }
  // [[nodiscard]] const G13_Manager& manager() const { return
  // _parent_ptr->manager(); }

  virtual void set_action(const G13_ActionPtr &action) { _action = action; }

protected:
  std::string _name;
  G13_ActionPtr _action;

private:
  PARENT_T *_parent_ptr;
};

// *************************************************************************
/*! manages the bindings for a G13 key
 *
 */
class G13_Key : public G13_Actionable<G13_Profile> {
public:
  void dump(std::ostream &o) const;
  [[nodiscard]] G13_KEY_INDEX index() const { return _index.index; }

  void ParseKey(const unsigned char *byte, G13_Device *g13);

protected:
  struct KeyIndex {
    explicit KeyIndex(int key)
        : index(key), offset(key / 8u), mask(1u << (key % 8u)) {}

    int index;
    unsigned char offset;
    unsigned char mask;
  };

  // G13_Profile is the only class able to instantiate G13_Keys
  friend class G13_Profile;

  G13_Key(G13_Profile &mode, const std::string &name, int index)
      : G13_Actionable<G13_Profile>(mode, name), _index(index),
        _should_parse(true) {}

  G13_Key(G13_Profile &mode, const G13_Key &key)
      : G13_Actionable<G13_Profile>(mode, key.name()), _index(key._index),
        _should_parse(key._should_parse) {
    set_action(
        key.action()); // TODO: do not invoke virtual member function from ctor
  }

  KeyIndex _index;
  bool _should_parse;
};

// *************************************************************************

class G13_StickZone : public G13_Actionable<G13_Stick> {
public:
  G13_StickZone(G13_Stick &, const std::string &name, const G13_ZoneBounds &,
                const G13_ActionPtr& = nullptr);

  bool operator==(const G13_StickZone &other) const {
    return _name == other._name;
  }

  void dump(std::ostream &) const;

  // void ParseKey(unsigned char* byte, G13_Device* g13);
  void test(const G13_ZoneCoord &loc);
  void set_bounds(const G13_ZoneBounds &bounds) { _bounds = bounds; }

protected:
  bool _active;

  G13_ZoneBounds _bounds;
};

} // namespace G13
#endif // G13_G13_ACTION_HPP

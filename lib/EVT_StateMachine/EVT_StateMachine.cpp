#include <EVT_StateMachine.hpp>

namespace Signals {
  StateMachine::StateMachine(States initstate) {
    currentState = initstate;
    setStateDefaults();
  }


  void StateMachine::setState(States newState) {
    if (stateMap.count(newState) == 1) {
      currentState = newState;
    } else {
      currentState = States::IDLE;
    }

    printState();
  }


  void StateMachine::runState() {
    stateMap[currentState].func();
  }


  void StateMachine::defineState(States state, const char * name, function<void(void)> func) {
    stateMap[state] = {state, name, func};
  }


  void StateMachine::defineState(States state, function<void(void)> func) {
    stateMap[state].func = func;
  }


  void StateMachine::printState() {
    snprintf(
      statePrint,
      sizeof(statePrint),
      "Current State: %s [%u]",
      toString(currentState),
      currentState
    );

    Queues::logWrite(statePrint);
  }


  void StateMachine::setStateDefaults() {
    defineState(States::NONE,  "NONE",  [&] () {});
    defineState(States::IDLE,  "IDLE",  [&] () {});
    defineState(States::RC,    "RC",    [&] () {});
    defineState(States::AUTO,  "AUTO",  [&] () {});
    defineState(States::STOP,  "STOP",  [&] () {});
    defineState(States::RESET, "RESET", [&] () {});
    defineState(States::ERROR, "ERROR", [&] () {});
  }


  void StateMachine::setErrorState() {
    setState(States::ERROR);
  }


  States StateMachine::getState() const {
    return currentState;
  }


  bool StateMachine::isInState(States checkState) const {
    return (currentState == checkState);
  }


  const char * StateMachine::toString(States state) {
    return stateMap[state].name;
  }
}
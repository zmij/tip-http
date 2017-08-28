/*
 * connection_fsm_observer.hpp
 *
 *  Created on: Aug 28, 2017
 *      Author: zmij
 */

#ifndef PUSHKIN_HTTP_CLIENT_CONNECTION_FSM_OBSERVER_HPP_
#define PUSHKIN_HTTP_CLIENT_CONNECTION_FSM_OBSERVER_HPP_

#include <afsm/fsm.hpp>
#include <pushkin/util/demangle.hpp>
#include <iostream>
#include <sstream>
#include <pushkin/log.hpp>

namespace psst {
namespace http {
namespace client {
namespace observer {

LOCAL_LOGGING_FACILITY(CONNFSM, TRACE);

struct conection_fsm_observer : ::afsm::detail::null_observer {

    ::std::string
    cut_type_name(::std::string const& name) const noexcept
    {
        auto pos = name.find_last_of("::");
        if (pos != ::std::string::npos) {
            return name.substr(pos + 1);
        }
        return name;
    }

    template < typename FSM, typename SourceState, typename TargetState, typename Event>
    void
    state_changed(FSM const& fsm, SourceState const&, TargetState const&, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection state changed "
                << cut_type_name(demangle< typename SourceState::state_definition_type >())
                << " -> "
                << cut_type_name(demangle< typename TargetState::state_definition_type >())
                << " (" << cut_type_name(demangle<Event>()) << ")";
        local_log() << os.str();
    }
    template < typename FSM, typename State >
    void
    state_cleared(FSM const& fsm, State const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection state cleared "
                << cut_type_name(demangle<typename State::state_definition_type>());
        local_log() << os.str();
    }

    template < typename FSM, typename Event >
    void
    start_process_event(FSM const& fsm, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection start process event "
                << cut_type_name(demangle<Event>());
        local_log() << os.str();
    }

    template < typename FSM, typename Event >
    void
    processed_in_state(FSM const& fsm, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection processed event "
                << cut_type_name(demangle<Event>()) << " in state";
        local_log() << os.str();
    }

    template < typename FSM, typename Event >
    void
    enqueue_event(FSM const& fsm, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection enqueue event "
                << cut_type_name(demangle<Event>());
        local_log() << os.str();
    }
    template < typename FSM, typename Event >
    void
    defer_event(FSM const& fsm, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection defer event "
                << cut_type_name(demangle<Event>());
        local_log() << os.str();
    }
    template < typename FSM, typename Event >
    void
    reject_event(FSM const& fsm, Event const&) const noexcept
    {
        using ::psst::util::demangle;
        ::std::ostringstream os;
        root_machine(fsm)->tag(os) << " Connection reject event "
                << cut_type_name(demangle<Event>());
        local_log() << os.str();
    }
};

} /* namespace observer */
} /* namespace client */
} /* namespace http */
} /* namespace psst */



#endif /* PUSHKIN_HTTP_CLIENT_CONNECTION_FSM_OBSERVER_HPP_ */

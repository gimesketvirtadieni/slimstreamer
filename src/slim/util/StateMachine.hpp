/*
 * Copyright 2017, Andrej Kislovskij
 *
 * This is PUBLIC DOMAIN software so use at your own risk as it comes
 * with no warranties. This code is yours to share, use and modify without
 * any restrictions or obligations.
 *
 * For more information see conwrap/LICENSE or refer refer to http://unlicense.org
 *
 * Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)
 */

#pragma once

#include <algorithm>
#include <functional>
#include <vector>


namespace slim
{
	namespace util
	{
		template <typename EventType, typename StateType>
		struct Transition
		{
			EventType                      event;
			StateType                      fromState;
			StateType                      toState;
			std::function<void(EventType)> action;
			std::function<bool()>          guard;
		};

		template <typename EventType, typename StateType>
		struct StateMachine
		{
			StateType                                     state;
			std::vector<Transition<EventType, StateType>> transitions;

			template <typename ErrorHandlerType>
			void processEvent(EventType event, ErrorHandlerType errorHandler)
			{
				// searching for a transition based on the event and the current state
				auto found = std::find_if(transitions.begin(), transitions.end(), [&](const auto& transition)
				{
					return (transition.fromState == state && transition.event == event);
				});

				// if found then perform a transition if guard allows
				if (found != transitions.end())
				{
					// invoking transition action if guar is satisfied
					if ((*found).guard())
					{
						(*found).action(event);

						// changing state of the state machine
						state = (*found).toState;
					}
				}
				else
				{
					errorHandler(event, state);
				}
			}
		};
	}
}

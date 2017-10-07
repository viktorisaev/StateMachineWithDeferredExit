#include "pch.h"
#include "StateMachineWithDeferredExit.h"




namespace StateMachineWithDeferredExit
{

#pragma region Transition

	Transition::Transition(ICondition *_ConditionToJump, ISingleState *_StateToJumpTo)
	{
		m_ConditionToJump = _ConditionToJump;
		m_StateToJumpTo = _StateToJumpTo;
	}

	bool Transition::IsConditionMet() const
	{
		return m_ConditionToJump->IsConditionActive();
	}

	ISingleState* Transition::GetStateToJumpTo() const
	{
		return m_StateToJumpTo;
	}

#pragma endregion



#pragma region SingleState

	SingleState::SingleState(IStateMachine *_machine)
	{
		m_Machine = _machine;
	}


	void SingleState::AddConnection(const Transition &_Connection)
	{
		m_Connections.push_back(_Connection);
	}


	// null=no active connection, State=newxt active state
	PossibleNextStateType SingleState::GetPossibleNewState()
	{
		for (int i = 0, ei = m_Connections.size(); i < ei; ++i)
		{
			if (m_Connections[i].IsConditionMet())
			{
				return PossibleNextStateType(true, m_Connections[i].GetStateToJumpTo());
			}
		}

		// nothing is met, return "no jump"
		return PossibleNextStateType(false, nullptr);
	}



	// inform a machine that state is finalized its exit and could be safely switched
	void SingleState::ConfirmExitRequestFinalized()
	{
		m_Machine->OnStateExited();
	}


#pragma endregion



#pragma region StateMachine

	StateMachine::StateMachine()
	{
		m_CurrentState = nullptr;
	}

	void StateMachine::SetStartState(ISingleState *_StartState)
	{
		m_StartState = _StartState;
	}


	void StateMachine::RunFromStartState()
	{
		m_IsExitRequested = false;
		GotoState(m_StartState);
	}


	void StateMachine::SetDoneCondition(ICondition *_MachineDoneCondition)
	{
		m_MachineDoneCondition = _MachineDoneCondition;   // to be called when machine done in OnStateExited
	}


	void StateMachine::SomeConditionUpdated()
	{
		if (m_CurrentState != nullptr)	// could be nullptr if pre-setup condition
		{
			// scan through condition of active state and check for possible transitions
			PossibleNextStateType _possibleNewState = m_CurrentState->GetPossibleNewState();

			if (std::get<0>(_possibleNewState))    // if some condition met
			{
				GotoState(std::get<1>(_possibleNewState));   // null=done as next state
			}
		}
	}


	void StateMachine::GotoState(ISingleState *_newState)
	{
		// 1) initiate exit from current state (to be continued in "ExitIsDone")
		// 2) save next state
		if (m_CurrentState == nullptr)
		{
			m_CurrentState = _newState;
			OnStateExited();
		}
		else
		{
			RequestExitIfAvailable(_newState);
		}
	}


	void StateMachine::RequestExitIfAvailable(ISingleState *_newState)
	{
		ISingleState *_lastState = m_CurrentState;
		m_CurrentState = _newState;     // replace state even if already in exit
		if (!m_IsExitRequested)
		{
			m_IsExitRequested = true;
			_lastState->OnExit();
			// to be continued in "OnStateExited"
		}
	}



	// used by parent machine to inform sub-machine
	void StateMachine::ExitCurrentState(OnMachineIsDoneDelegate _OnMachineExitCurrentState)
	{
		m_OnMachineExitCurrentState = _OnMachineExitCurrentState;   // to be called when exited

		if (m_CurrentState != nullptr)     // current state could be "null" if machine is done
		{
			RequestExitIfAvailable(nullptr);   // request exit and go to "null" (done) state
		}
		else
		{
			OnStateExited();
		}
	}



	// callback after current state finalized its exit and could be switched safe
	// m_CurrentState already points to a new state to enter
	void StateMachine::OnStateExited()
	{
		// 3) enter new state, m_CurrentState is already points to new state to enter
		if (m_CurrentState != nullptr)
		{
			m_CurrentState->OnEnter();	// enter new state
			SomeConditionUpdated();		// check for possible immediate transaction and perform it if exists
		}
		else
		{
			if (m_OnMachineExitCurrentState != nullptr)    // a callback could be assigned to report state exit
			{
				m_OnMachineExitCurrentState();  // report state exited
				m_OnMachineExitCurrentState = nullptr;
			}
			else
			{
				m_MachineDoneCondition->SignalCondition(true);      // report machine is done
			}
		}

		m_IsExitRequested = false;
	}

#pragma endregion



#pragma region TriggerMachineCondition

	TriggerMachineCondition::TriggerMachineCondition(IStateMachine *_Machine, bool _IsAutoReset/* = true*/)
	{
		m_Machine = _Machine;
		m_IsAutoReset = _IsAutoReset;
		m_Condition = false;
	}


	bool TriggerMachineCondition::IsConditionActive()
	{
		return m_Condition;
	}


	void TriggerMachineCondition::SignalCondition(bool _cond)
	{
		// store condition value
		SetCondition(_cond);

		// react on value changed
		m_Machine->SomeConditionUpdated();
		if (m_IsAutoReset)
		{
			m_Condition = false;
		}
	}


	void TriggerMachineCondition::SetCondition(bool _cond)
	{
		m_Condition = _cond;
	}

#pragma endregion

} // namespace StateMachineWithDeferredExit

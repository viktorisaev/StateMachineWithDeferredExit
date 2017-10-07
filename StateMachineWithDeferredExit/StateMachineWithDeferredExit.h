// State Machine

#pragma once

// std
#include <tuple>
#include <vector>


namespace StateMachineWithDeferredExit
{
	///////////////////////////////////////////////////////////////////////////
	// Interfaces

	// condition to jump
	class ICondition abstract
	{
	public:		// func
		virtual bool IsConditionActive() = 0;
		virtual void SetCondition(bool _cond) = 0;
	};


	class Transition;	// used in ISingleState

	// single state
	class ISingleState abstract
	{
	public:
		typedef std::tuple<bool, ISingleState*> PossibleNextStateType;

	public:		// func
		virtual void AddConnection(const Transition &_Transition) = 0;
		virtual PossibleNextStateType GetPossibleNewState() = 0;      // null=no active connection, State=newxt active state

		virtual void OnEnter() = 0;
		virtual void OnExit() = 0;  // should call OnExitDone when done
	};



	typedef void (*OnMachineIsDoneDelegate)();

	// state machine
	class IStateMachine abstract
	{
	public:		// func
		virtual void SetStartState(ISingleState *_startState) = 0;
		virtual void RunFromStartState() = 0;
		virtual void OnStateExited() = 0;
		virtual void SomeConditionUpdated() = 0;
		virtual void ExitCurrentState(OnMachineIsDoneDelegate _OnMachineIsDone) = 0;    // used for sub machine
		virtual void SetDoneCondition(ICondition *_MachineDoneCondition) = 0;     // machine set this condition when done, i.e. current state to jump to is "null"
	};



	// transition
	class Transition
	{
	public:		// func
		Transition(ICondition *_ConditionToJump, ISingleState *_StateToJumpTo)
		{
			m_ConditionToJump = _ConditionToJump;
			m_StateToJumpTo = _StateToJumpTo;
		}

		bool IsConditionMet() const
		{
			return m_ConditionToJump->IsConditionActive();
		}

		ISingleState* GetStateToJumpTo() const
		{
			return m_StateToJumpTo;
		}

	private:	// data
		ICondition *m_ConditionToJump;
		ISingleState *m_StateToJumpTo;

	};



	///////////////////////////////////////////////////////////////////////////
	// Classes

	class SingleState : public ISingleState
	{
	public:		// func
		SingleState(IStateMachine *_machine)
		{
			m_Machine = _machine;

//			m_Connections = new List<Transition>();
		}


		// real state methods
		virtual void OnEnter() = 0;
		virtual void OnExit() = 0;



		void AddConnection(const Transition &_Connection) override
		{
			m_Connections.push_back(_Connection);
		}




		// null=no active connection, State=newxt active state
		PossibleNextStateType GetPossibleNewState() override
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
		void ConfirmExitRequestFinalized()
		{
			m_Machine->OnStateExited();
		}



	protected:	// data
		IStateMachine *m_Machine;  // a machine to inform the state is done and want to exit

	private:	// data
		std::vector<Transition> m_Connections;

	};   // class SingleState






	class StateMachine : public IStateMachine
	{
	public:		// func

#pragma region Setup

		StateMachine()
		{
			m_CurrentState = nullptr;
		}

		void SetStartState(ISingleState *_StartState)
		{
			m_StartState = _StartState;
		}


		void RunFromStartState()
		{
			m_IsExitRequested = false;
			GotoState(m_StartState);
		}


		void SetDoneCondition(ICondition *_MachineDoneCondition)
		{
			m_MachineDoneCondition = _MachineDoneCondition;   // to be called when machine done in OnStateExited
		}

#pragma endregion


#pragma region ChangeState

		void SomeConditionUpdated()
		{
			if (m_CurrentState)	// could be nullptr if pre-setup condition
			{
				// scan through condition of active state and check for possible transitions
				ISingleState::PossibleNextStateType _possibleNewState = m_CurrentState->GetPossibleNewState();

				if (std::get<0>(_possibleNewState))    // if some condition met
				{
					GotoState(std::get<1>(_possibleNewState));   // null=done as next state
				}
			}
		}


		void GotoState(ISingleState *_newState)
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


		void RequestExitIfAvailable(ISingleState *_newState)
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


#pragma endregion


#pragma region Exit

			// used by parent machine to inform sub-machine
		void ExitCurrentState(OnMachineIsDoneDelegate _OnMachineExitCurrentState)
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
		void OnStateExited()
		{
			// 3) enter new state, m_CurrentState is already points to new state to enter
			if (m_CurrentState != nullptr)
			{
				m_CurrentState->OnEnter();   // enter new state
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
					m_MachineDoneCondition->SetCondition(true);      // report machine is done
				}
			}

			m_IsExitRequested = false;
		}




#pragma endregion


	protected:	// data
		ISingleState *m_CurrentState;
		ISingleState *m_StartState;

	private:	// data
		OnMachineIsDoneDelegate m_OnMachineExitCurrentState;
		bool m_IsExitRequested;  // true=exit requested, false=done
		ICondition *m_MachineDoneCondition;


	};	// class StateMachine




	class TriggerMachineCondition : public ICondition
	{
	public:		// func
		TriggerMachineCondition(IStateMachine *_Machine, bool _IsAutoReset = true)
		{
			m_Machine = _Machine;
			m_IsAutoReset = _IsAutoReset;
			m_Condition = false;
		}

		bool IsConditionActive() override
		{
			return m_Condition;
		}

		void SetCondition(bool _cond) override
		{
			// store condition value
			SetupCondition(_cond);

			// react on value changed
			m_Machine->SomeConditionUpdated();
			if (m_IsAutoReset)
			{
				m_Condition = false;
			}
		}

		void SetupCondition(bool _cond)
		{
			m_Condition = _cond;
		}

	private:	// data
		bool m_Condition;
		IStateMachine *m_Machine;
		bool m_IsAutoReset;

	};



}	// namespace StateMachineWithDeferredExit
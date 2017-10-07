// State Machine

#pragma once

#include "pch.h"



namespace StateMachineWithDeferredExit
{
	///////////////////////////////////////////////////////////////////////////
	// Interfaces

	// condition to jump
	class ICondition abstract
	{
	public:		// func
		virtual bool IsConditionActive() = 0;
		virtual void SignalCondition(bool _cond) = 0;	// activate condition with a value, causing state change
	};



	class Transition;	// used in ISingleState
	class ISingleState;
	typedef std::tuple<bool, ISingleState*> PossibleNextStateType;

	// single state
	class ISingleState abstract
	{
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
		Transition(ICondition *_ConditionToJump, ISingleState *_StateToJumpTo);
		bool IsConditionMet() const;
		ISingleState* GetStateToJumpTo() const;

	private:	// data
		ICondition *m_ConditionToJump;
		ISingleState *m_StateToJumpTo;

	};



	///////////////////////////////////////////////////////////////////////////
	// Classes

	class SingleState : public ISingleState
	{
	public:		// func
		SingleState(IStateMachine *_machine);

		// real state methods
		virtual void OnEnter() = 0;
		virtual void OnExit() = 0;

		void AddConnection(const Transition &_Connection) override;
		PossibleNextStateType GetPossibleNewState() override;
		
		// inform a machine that state is finalized its exit and could be safely switched
		void ConfirmExitRequestFinalized();

	protected:	// data
		IStateMachine *m_Machine;  // a machine to inform the state is done and want to exit

	private:	// data
		std::vector<Transition> m_Connections;

	};   // class SingleState



	class StateMachine : public IStateMachine
	{
	public:		// func
		StateMachine();
		void SetStartState(ISingleState *_StartState);
		void RunFromStartState();
		void SetDoneCondition(ICondition *_MachineDoneCondition);
		void SomeConditionUpdated();
		void GotoState(ISingleState *_newState);
		void RequestExitIfAvailable(ISingleState *_newState);
		void ExitCurrentState(OnMachineIsDoneDelegate _OnMachineExitCurrentState);	// used by parent machine to inform sub-machine

		// callback after current state finalized its exit and could be switched safe
		// m_CurrentState already points to a new state to enter
		void OnStateExited();

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
		TriggerMachineCondition(IStateMachine *_Machine, bool _IsAutoReset = true);
		bool IsConditionActive() override;
		void SignalCondition(bool _cond) override;
		void SetCondition(bool _cond);	// set condition flag but do not request for state change

	private:	// data
		bool m_Condition;
		IStateMachine *m_Machine;
		bool m_IsAutoReset;

	};



}	// namespace StateMachineWithDeferredExit
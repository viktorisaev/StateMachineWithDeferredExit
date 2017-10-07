#include "stdafx.h"
#include "CppUnitTest.h"

#include "StateMachineWithDeferredExit.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace StateMachineWithDeferredExit;	// use testee namespace


namespace UnitTestStateMachineWithDeferredExit
{



// define real states and machines


// simple state
class TestState1 : public SingleState
{
public:	//tests
	int state;
public:
	TestState1(IStateMachine *_machine) : SingleState(_machine)
	{ 
		state = 0;
	}

	virtual void OnEnter() override
	{
		state = 1;
	}

	virtual void OnExit() override
	{
		state = 2;
		ConfirmExitRequestFinalized();	// inform state machine on exit procedure is finalized
	}
};










	// simple state + transition
	TEST_CLASS(UnitTest1)
	{
	public:


		TEST_METHOD_INITIALIZE(UnitTest1Init)
		{
			m_StateMachine = new StateMachine();
		}


		TEST_METHOD_CLEANUP(UnitTest1Cleanup)
		{
			delete m_StateMachine;
		}


		// state machine with single infinite state
		TEST_METHOD(CreateStateAndEnter)
		{
			TestState1 state = TestState1(m_StateMachine);
			m_StateMachine->SetStartState(&state);
			m_StateMachine->RunFromStartState();

			Assert::AreEqual(state.state, 1);
		}


		
		// state machine with two states and transition between them
		TEST_METHOD(TransitionBetweenTwoStates)
		{
			TestState1 state1 = TestState1(m_StateMachine);
			TestState1 state2 = TestState1(m_StateMachine);

			TriggerMachineCondition jumpCond = TriggerMachineCondition(m_StateMachine, true);
			Transition transition = Transition(&jumpCond, &state2);
			state1.AddConnection(transition);

			m_StateMachine->SetStartState(&state1);
			m_StateMachine->RunFromStartState();

			Assert::AreEqual(state1.state, 1);

			// change state
			jumpCond.SetCondition(true);

			Assert::AreEqual(2, state1.state);
			Assert::AreEqual(1, state2.state);
		}


	private:	// data
		StateMachine *m_StateMachine;

	};
}	// namespace UnitTestStateMachineWithDeferredExit
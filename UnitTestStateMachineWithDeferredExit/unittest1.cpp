#include "stdafx.h"
#include "CppUnitTest.h"

#include "StateMachineWithDeferredExit.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace StateMachineWithDeferredExit;	// use testee namespace


namespace UnitTestStateMachineWithDeferredExit
{



// define real states and machines


class TestState1 : public SingleState
{
public:	//tests
	int state = 0;
public:
	TestState1(IStateMachine *_machine) : SingleState(_machine)
	{

	}


	virtual void AddConnection(Transition * _Transition) override
	{
	}

	virtual PossibleNextStateType GetPossibleNewState() const override
	{
		return PossibleNextStateType();
	}

	virtual void OnEnter() override
	{
		state = 1;
	}

	virtual void OnExit() override
	{
		state = 2;
	}
};











	TEST_CLASS(UnitTest1)
	{
	public:


		TEST_CLASS_INITIALIZE(UnitTest1Init)
		{
		}


		TEST_CLASS_CLEANUP(UnitTest1Cleanup)
		{

		}


		TEST_METHOD(CreateState)
		{
			// TODO: Your test code here

			TestState1 state = TestState1(&m_StateMachine);
			m_StateMachine.SetStartState(&state);
			m_StateMachine.RunFromStartState();

			Assert::AreEqual(state.state, 1);

		}

	private:	// data
		StateMachine m_StateMachine;

	};
}	// namespace UnitTestStateMachineWithDeferredExit
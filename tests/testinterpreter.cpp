#include "testinterpreter.h"

#include "fungespace.h"
#include "interpreter.h"
#include "stackstack.h"

QTEST_MAIN(TestInterpreter);

void TestInterpreter::init()
{
	m_model = new StackStackCollectionModel(NULL);
	m_space = new FungeSpace(2);
	m_interpreter = new Interpreter(m_model, m_space);
}

void TestInterpreter::cleanup()
{
	delete m_model;
	delete m_space;
	delete m_interpreter;
}

void TestInterpreter::testPushItem()
{
	m_interpreter->pushItem(42);
	QVERIFY(m_interpreter->popItem());
}

void TestInterpreter::testAdd()
{
	QVERIFY(m_interpreter->ip()->m_pos == Coord());
	m_interpreter->ip()->stack()->push(1);
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->add();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 3);
}

void TestInterpreter::testSubtract()
{
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->ip()->stack()->push(1);
	m_interpreter->subtract();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 1);
}

void TestInterpreter::testMultiply()
{
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->ip()->stack()->push(3);
	m_interpreter->multiply();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 6);
}

void TestInterpreter::testDivide()
{
	m_interpreter->ip()->stack()->push(4);
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->divide();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 2);
}

void TestInterpreter::testModulo()
{
	m_interpreter->ip()->stack()->push(3);
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->modulo();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 1);
}

void TestInterpreter::testGreaterThan()
{
	m_interpreter->ip()->stack()->push(2);
	m_interpreter->ip()->stack()->push(1);
	m_interpreter->greaterThan();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 1);
}

void TestInterpreter::testNotf()
{
	m_interpreter->ip()->stack()->push(1);
	m_interpreter->notf();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 0);
}

void TestInterpreter::testUp()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = -1;

	m_interpreter->up();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testDown()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = 1;

	m_interpreter->down();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testLeft()
{
	Coord expected;
	expected[0] = -1;
	expected[1] = 0;

	m_interpreter->left();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testRight()
{
	Coord expected;
	expected[0] = 1;
	expected[1] = 0;

	m_interpreter->right();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testTurnLeft()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = -1;

	m_interpreter->turnLeft();
	QVERIFY(m_interpreter->ip()->m_direction == expected);

	expected[0] = -1;
	expected[1] = 0;
	m_interpreter->turnLeft();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testTurnRight()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = 1;

	m_interpreter->turnRight();
	QVERIFY(m_interpreter->ip()->m_direction == expected);

	expected[0] = -1;
	expected[1] = 0;
	m_interpreter->turnRight();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testHigher()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = 0;
	expected[2] = 1;

	m_interpreter->higher();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testLower()
{
	Coord expected;
	expected[0] = 0;
	expected[1] = 0;
	expected[2] = -1;

	m_interpreter->lower();
	QVERIFY(m_interpreter->ip()->m_direction == expected);
}

void TestInterpreter::testSimpleBeginBlock()
{
	m_interpreter->pushItem(0);
	Stack* temp = m_interpreter->ip()->stack();
	m_interpreter->beginBlock();
	QVERIFY(m_interpreter->ip()->stack());
	QVERIFY(m_interpreter->ip()->stack() != temp);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack() == temp);
	QVERIFY(temp->count() == 2);
	QVERIFY(m_interpreter->ip()->stack()->count() == 0);
}

void TestInterpreter::testCopyItemsBeginBlock()
{
	m_interpreter->pushItem(42);
	m_interpreter->pushItem(43);
	m_interpreter->pushItem(2);
	Stack* temp = m_interpreter->ip()->stack();

	m_interpreter->beginBlock();
	QVERIFY(m_interpreter->ip()->stack());
	QVERIFY(m_interpreter->ip()->stack()->count() == 2);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 43);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 42);

	QVERIFY(temp->count() == 2);
	QVERIFY(temp->pop() == 0);
	QVERIFY(temp->pop() == 0);
}

void TestInterpreter::testEmptyBlockDoesNothing()
{
	Stack* temp = m_interpreter->ip()->stack();

	m_interpreter->pushItem(0);
	m_interpreter->beginBlock();
	m_interpreter->endBlock();

	QVERIFY(m_interpreter->ip()->stack() == temp);
	QVERIFY(temp->count() == 0);
}

void TestInterpreter::testCopyItemsEndBlock()
{
	m_interpreter->pushItem(0);
	m_interpreter->beginBlock();
	m_interpreter->pushItem(42);
	m_interpreter->pushItem(43);
	m_interpreter->pushItem(2);
	m_interpreter->endBlock();

	QVERIFY(m_interpreter->ip()->stack()->count() == 2);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 43);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 42);
}

void TestInterpreter::testCopyZeroesBeginBlock()
{
	m_interpreter->pushItem(-2);
	m_interpreter->beginBlock();
	
	QVERIFY(m_interpreter->ip()->stack()->count() == 0);
	// Two zeroes copied as well as the 0,0 storage offset.
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->count() == 4);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->pop() == 0);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->pop() == 0);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->pop() == 0);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->pop() == 0);
}

void TestInterpreter::testCopyTooManyItemsBeginBlock()
{
	m_interpreter->pushItem(42);
	m_interpreter->pushItem(2);
	m_interpreter->beginBlock();

	QVERIFY(m_interpreter->ip()->stack()->count() == 2);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 42);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 0);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->count() == 2);
}

void TestInterpreter::testMultiBeginEndBlock()
{
	m_interpreter->pushItem(1);
	m_interpreter->pushItem(2);
	m_interpreter->pushItem(3);
	m_interpreter->pushItem(4);
	m_interpreter->pushItem(5);
	m_interpreter->pushItem(5);

	m_interpreter->beginBlock();
	QVERIFY(m_interpreter->ip()->stack()->count() == 5);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 5);

	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->count() == 2);
	QVERIFY(m_interpreter->ip()->m_stackStack->secondStack()->peek() == 0);

	m_interpreter->pushItem(2);
	m_interpreter->endBlock();
	QVERIFY(m_interpreter->ip()->stack()->count() == 2);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 5);
	QVERIFY(m_interpreter->ip()->stack()->pop() == 4);
}

void TestInterpreter::testPut()
{
	m_interpreter->pushItem(42);
	m_interpreter->pushItem(1);
	m_interpreter->pushItem(1);

	m_interpreter->putFunge();
	QVERIFY(m_interpreter->ip()->stack()->count() == 0);
	Coord c;
	c[0] = 1;
	c[1] = 1;
	QVERIFY(m_interpreter->m_space->getChar(c) == 42);
}

void TestInterpreter::testGet()
{
	m_interpreter->pushItem(1);
	m_interpreter->pushItem(1);
	Coord c;
	c[0] = 1;
	c[1] = 1;
	m_interpreter->m_space->setChar(c, 42);

	m_interpreter->getFunge();
	QVERIFY(m_interpreter->ip()->stack()->count() == 1);
	QVERIFY(m_interpreter->ip()->stack()->peek() == 42);
}

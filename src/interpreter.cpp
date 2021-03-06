#include "interpreter.h"
#include "sysinfo.h"

#include <QDebug>
#include <QStringList>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

Interpreter::InstructionPointer::InstructionPointer(StackStackCollectionModel* model, Coord position, Coord direction, Coord storageOffset, int id)
	:m_pos(position), m_direction(direction), m_storageOffset(storageOffset),
	m_stringMode(false), m_commentMode(false), m_waitingForInput(NotWaiting), m_id(id),
	m_usingSecondStack(false)
{
	init();
	m_stackStack = model->newStackStack(this);
}

Interpreter::InstructionPointer::InstructionPointer(StackStackCollectionModel* model, const Interpreter::InstructionPointer& ip, int id)
	:m_pos(ip.m_pos), m_direction(ip.m_direction), m_storageOffset(ip.m_storageOffset),
	m_stringMode(ip.m_stringMode), m_commentMode(ip.m_commentMode), m_waitingForInput(ip.m_waitingForInput),
	m_id(id), m_usingSecondStack(false)
{
	init();
	m_stackStack = model->deepCopy(this, ip.m_stackStack);
}

void Interpreter::InstructionPointer::init()
{
	static QList<QColor> colorList;
	
	if (colorList.isEmpty())
	{
		colorList.append(Qt::red);
		colorList.append(Qt::cyan);
		colorList.append(Qt::yellow);
		colorList.append(Qt::magenta);
	}
	
	m_color = colorList[m_id % colorList.count()];
}

Interpreter::InstructionPointer::~InstructionPointer()
{
}

Stack* Interpreter::InstructionPointer::stack()
{
	if (m_usingSecondStack)
		return m_stackStack->secondStack();
	else
		return m_stackStack->topStack();
}

Interpreter::Interpreter(StackStackCollectionModel* stackModel, FungeSpace* space, QObject* parent)
	: QObject(parent),m_space(space),m_ipid(0)
{
	m_version = "1";
	
	m_stackModel = stackModel;

	Coord direction;
	direction[0] = 1;
	Coord pos;
	pos[0] = 0;
	pos[1] = 0;
	m_ip = new InstructionPointer(m_stackModel, pos, direction, Coord(), m_ipid++);

	m_ip->m_stringMode = false;
	m_ip->m_commentMode = false;

	m_ips << m_ip;
}

Interpreter::~Interpreter()
{
	foreach(InstructionPointer* ip, m_ips)
	{
		delete ip;
	}
}

void Interpreter::jumpSpaces()
{
	QChar t = m_space->getChar(m_ip->m_pos);

	if(t.category() == QChar::Separator_Space || (m_ip->m_commentMode && t != ';') || (!m_ip->m_stringMode && t == 'z'))
	{
		m_ip->m_jumpedSpace = true;	
		move();
	}
}

void Interpreter::move(bool skipSpaces)
{
	if (m_ip == NULL)
		return;
	
	for(uint i = 0; i < m_space->dimensions(); ++i)
	{
		m_ip->m_pos[i] += m_ip->m_direction[i];
		if(m_ip->m_pos[i] > m_space->getPositiveEdge(i))
			m_ip->m_pos[i] = m_space->getNegativeEdge(i);
		else if(m_ip->m_pos[i] < m_space->getNegativeEdge(i))
			m_ip->m_pos[i] = m_space->getPositiveEdge(i);
	}

	if (skipSpaces)
		jumpSpaces();

	//qDebug() << "Moved to:" << m_pos;
	emit ipChanged(m_ip);
}

Coord Interpreter::getNextInstruction(const Coord& c)
{
	Coord temp = c;
	for (uint i = 0; i < m_space->dimensions(); ++i)
	{
		temp[i] += m_ip->m_direction[i];
		if (m_ip->m_pos[i] > m_space->getPositiveEdge(i))
			m_ip->m_pos[i] = m_space->getNegativeEdge(i);
		else if(m_ip->m_pos[i] < m_space->getNegativeEdge(i))
			m_ip->m_pos[i] = m_space->getPositiveEdge(i);
	}

	if (!isRealInstruction(temp))
		return getNextInstruction(temp);

	return temp;
}

bool Interpreter::isRealInstruction(Coord c) {
	if (QString(" ;\")(").contains(m_space->getChar(c)))
		return false;
	
	return true;
}

Interpreter::Status Interpreter::step()
{
	if(m_ip->m_jumpedSpace)
	{
		m_ip->m_jumpedSpace = false;
		if(m_ip->m_stringMode)
			pushItem(QChar(' ').unicode());
	}
	QChar c(m_space->getChar(m_ip->m_pos));
	//qDebug() << m_ip->m_pos << c;
	Interpreter::Status ret = compute(c);

	//qDebug() << "Direction: " << m_direction;
	if(ret == Success || ret == Invalid || ret == Again)
	{
		move();
	}

	if (ret == Again)
		return step();

	return ret;
}

QList<Interpreter::Status> Interpreter::stepAll()
{
	QList<Interpreter::Status> status;
	foreach(InstructionPointer* ip, m_ips)
	{
		m_ip = ip;
		Interpreter::Status ret = step();

		if (ret == End)
			m_ips.removeAll(ip);

		status << ret;
	}

	return status;
}

void Interpreter::run()
{
	Interpreter::Status s;
	do
	{
		s = step();
	} while (s == Success || s == Invalid);
}

// Call this with a QChar Array
Interpreter::Status Interpreter::compute(QChar command)
{
	//qDebug() << "Instruction:" << command;
	if(m_ip->m_stringMode && command != '"')
	{
		pushItem(command.unicode());
		return Success;
	}
	
	if (m_ip->m_waitingForInput != NotWaiting)
		return SuspendForInput;

	if(command == '+')
		add();
	else if(command == '-')
		subtract();
	else if(command == '*')
		multiply();
	else if(command == '/')
		divide();
	else if(command == '%')
		modulo();
	else if(command == '!')
		notf();
	else if(command == '`')
		greaterThan();
	else if(command == '^')
		up();
	else if(command == '>')
		right();
	else if(command == '<')
		left();
	else if(command == 'v')
		down();
	else if(command == 'h')
		higher();
	else if(command == 'l')
		lower();
	else if(command == '?')
		random();
	else if(command == '[')
		turnLeft();
	else if(command == ']')
		turnRight();
	else if(command == 'Z')
		upDimension();
	else if(command == 'Y')
		downDimension();
	else if(command == 'z')
		return Success;// Nop
	else if(command == 'y')
		getSysInfo();
	else if(command == 'r')
		reverse();
	else if(command == 'x')
		absolute();
	else if(command == '"')
		string();
	else if(command == ';')
	{
		comment();
		return Again;
	}
	else if(command == '\'')
		character();
	else if(command == 's')
		storeCharacter();
	else if(command == ':')
		duplicate();
	else if(command == '$')
		pop();
	else if(command == '\\')
		swap();
	else if(command == 'n')
		clear();
	else if(command == '|')
		vertIf();
	else if(command == '_')
		horizIf();
	else if(command == 'w')
		compare();
	else if(command == ',')
		printChar();
	else if(command == '.')
		printDec();
	else if(command == '&')
		return inputDec();
	else if(command == '~')
		return inputChar();
	else if(command == '#')
		trampoline();
	else if(command == 'j')
		jump();
	else if(command == 'k')
		iterate();
	else if(command == '{')
		beginBlock();
	else if(command == '}')
		endBlock();
	else if(command == 'u')
		stackUnderStack();
	else if(command == 'p')
		putFunge();
	else if(command == 'g')
		getFunge();
	else if(command == 't')
		split();
	else if(command.isNumber())
		pushNumber(command);
	else if(command == 'a')
		pushItem(10);
	else if(command == 'b')
		pushItem(11);
	else if(command == 'c')
		pushItem(12);
	else if(command == 'd')
		pushItem(13);
	else if(command == 'e')
		pushItem(14);
	else if(command == 'f')
		pushItem(15);
	else if(command == '(')
		loadSemantics();
	else if(command == '@')
	{
		if (end())
			return End;
	}
	else
	{
		// Reflect on invalid character.
		reverse();
		return Invalid;
		//panic("Don't understand character: " + QString(command));
	}

	return Success;
}

//Instructions
void Interpreter::add()
{
	int x = popItem();
	int y = popItem();

	int z = y + x;

	pushItem(z);
}

void Interpreter::subtract()
{
	int x = popItem();
	int y = popItem();

	int z = y - x;

	pushItem(z);
}

void Interpreter::multiply()
{
	int x = popItem();
	int y = popItem();

	int z = y * x;

	pushItem(z);
}

void Interpreter::divide()
{
	int x = popItem();
	int y = popItem();

	int z = 0;
	if (x != 0)
		z = y / x;

	pushItem(z);
}

void Interpreter::modulo()
{
	int x = popItem();
	int y = popItem();

	int z = 0;
	if (x != 0)
		z = y % x;

	pushItem(z);
}

void Interpreter::notf()
{
	int x = popItem();
	if(x)
		pushItem(0);
	else
		pushItem(1);
}

void Interpreter::greaterThan()
{
	int x = popItem();
	int y = popItem();

	if(y > x)
		pushItem(1);
	else
		pushItem(0);
}

void Interpreter::up()
{
	m_ip->m_direction[0] = 0;
	m_ip->m_direction[1] = -1;

	for(uint i = 2; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::right()
{
	m_ip->m_direction[0] = 1;

	for(uint i = 1; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::left()
{
	m_ip->m_direction[0] = -1;

	for(uint i = 1; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::down()
{
	m_ip->m_direction[0] = 0;
	m_ip->m_direction[1] = 1;

	for(uint i = 2; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::higher()
{
	m_ip->m_direction[0] = 0;
	m_ip->m_direction[1] = 0;
	m_ip->m_direction[2] = 1;

	for(uint i = 3; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::lower()
{
	m_ip->m_direction[0] = 0;
	m_ip->m_direction[1] = 0;
	m_ip->m_direction[2] = -1;

	for(uint i = 3; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
}

void Interpreter::random()
{
	for(int i=0 ; i<m_space->dimensions() ; i++)
		m_ip->m_direction[i] = 0;
	m_ip->m_direction[rand()%m_space->dimensions()] = (rand()%2)?1:-1;
}

void Interpreter::turnRight()
{
	int oldXDirection = m_ip->m_direction[0];
	m_ip->m_direction[0] = -m_ip->m_direction[1];
	m_ip->m_direction[1] = oldXDirection;
}

void Interpreter::turnLeft()
{
	int oldXDirection = m_ip->m_direction[0];
	m_ip->m_direction[0] = m_ip->m_direction[1];
	m_ip->m_direction[1] = -oldXDirection;
}

void Interpreter::upDimension()
{
	int dim = popItem();

	for(uint i = 0; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;
	
	m_ip->m_direction[dim] = 1;
}

void Interpreter::downDimension()
{
	int dim = popItem();

	for(uint i = 0; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] = 0;

	m_ip->m_direction[dim] = -1;
}

void Interpreter::reverse()
{
	for(uint i = 0; i < m_space->dimensions(); ++i)
		m_ip->m_direction[i] *= -1;
}

void Interpreter::absolute()
{
	for(int i = m_space->dimensions() - 1; i >= 0; --i)
	{
		m_ip->m_direction[i] = popItem();
	}
}

void Interpreter::string()
{
	// For null-terminated strings?
	//if(!m_ip->m_stringMode)
	//	pushItem('\0');

	m_ip->m_stringMode = !m_ip->m_stringMode;
}

void Interpreter::comment()
{
	m_ip->m_commentMode = !m_ip->m_commentMode;
}

void Interpreter::character()
{
	move(false);
	pushItem(m_space->getChar(m_ip->m_pos));
}

void Interpreter::storeCharacter()
{
	move(false);
	m_space->setChar(m_ip->m_pos, popItem());
}

void Interpreter::duplicate()
{
	int x = popItem();
	pushItem(x);
	pushItem(x);
}

void Interpreter::pop()
{
	popItem();
}

void Interpreter::swap()
{
	int a = popItem();
	int b = popItem();

	pushItem(a);
	pushItem(b);
}

void Interpreter::clear()
{
	m_ip->m_stackStack->topStack()->clear();
}

void Interpreter::vertIf()
{
	int x = popItem();
	if(x)
		up();
	else
		down();
}

void Interpreter::horizIf()
{
	int x = popItem();
	if(x)
		left();
	else
		right();
}

void Interpreter::compare()
{
	int b = popItem();
	int a = popItem();

	if(a < b)
		turnLeft();
	else if(a > b)
		turnRight();
}

void Interpreter::printChar()
{
	QChar outputChar = QChar(popItem());
	emit(output(outputChar));
}

void Interpreter::printDec()
{
	QString outputStr = QString::number(popItem()) + ' ';
	emit(output(outputStr));
}

Interpreter::Status Interpreter::inputChar()
{
	m_ip->m_waitingForInput = WaitingForChar;
	emit input(m_ip->m_waitingForInput);
	
	return SuspendForInput;
}

Interpreter::Status Interpreter::inputDec()
{
	m_ip->m_waitingForInput = WaitingForInteger;
	emit input(m_ip->m_waitingForInput);
	
	return SuspendForInput;
}

void Interpreter::provideInput(QChar c)
{
	provideInput(c.unicode());
}

void Interpreter::provideInput(int i)
{
	if (m_ip->m_waitingForInput == NotWaiting)
		return;
	pushItem(i);
	
	m_ip->m_waitingForInput = NotWaiting;
	
	move();
}

void Interpreter::trampoline()
{
	for(uint i = 0; i < m_space->dimensions(); ++i)
	{
		m_ip->m_pos[i] += m_ip->m_direction[i];
		if(m_ip->m_pos[i] > m_space->getPositiveEdge(i))
			m_ip->m_pos[i] = m_space->getNegativeEdge(i);
		else if(m_ip->m_pos[i] < m_space->getNegativeEdge(i))
			m_ip->m_pos[i] = m_space->getPositiveEdge(i);
	}

	emit ipChanged(m_ip);
}

void Interpreter::jump()
{
	int x = popItem();

	if(x > 0)
	{
		for(int i = 0; i < x; ++i)
			move(false);
	}
	else if(x < 0)
	{
		reverse();
		for(int i = 0; i < abs(x); ++i)
			move(false);

		reverse();
	}
}

void Interpreter::iterate()
{
	int x = popItem();
	Coord old = m_ip->m_pos;
	Coord c = getNextInstruction(m_ip->m_pos);

	for(int i = 0; i < x; ++i)
		compute(m_space->getChar(c));

	//if (old == m_ip->m_pos)
	//	move();
}

void Interpreter::beginBlock()
{
	int n = m_ip->stack()->pop();
	
	Stack* newStack = m_ip->m_stackStack->pushEmptyStack();
	
	if (n >= 0)
	{
		int max = m_ip->m_stackStack->secondStack()->childCount();
		QStack<int> temp;
		for (int i=0 ; i<n ; i++)
		{
			if (i >= max)
				newStack->pushToBottom(0);
			else
				temp.push(m_ip->m_stackStack->secondStack()->pop());
		}
		while (!temp.isEmpty())
			newStack->push(temp.pop());
	}
	else
	{
		n = abs(n);
		for (int i=0 ; i<n ; i++)
			m_ip->m_stackStack->secondStack()->push(0);
	}

	m_ip->m_usingSecondStack = true;
	pushVector(m_ip->m_storageOffset);
	m_ip->m_usingSecondStack = false;

	for(uint i = 0; i < m_space->dimensions(); ++i)
		m_ip->m_storageOffset[i] = m_ip->m_pos[i] + m_ip->m_direction[i];
}

void Interpreter::endBlock()
{
	if (m_ip->m_stackStack->count() == 1)
	{
		reverse();
		return;
	}

	int n = popItem();
	
	m_ip->m_usingSecondStack = true;
	m_ip->m_storageOffset = popVector();
	m_ip->m_usingSecondStack = false;
	
	if (n >= 0)
	{
		QStack<int> tempStack;
		for (int i=0 ; i<n ; i++)
			tempStack << popItem();

		m_ip->m_stackStack->removeTopStack();
		while (tempStack.count() > 0)
			pushItem(tempStack.pop());
	}
	else
	{
		m_ip->m_stackStack->removeTopStack();
		for (int i=0 ; i<abs(n) ; i++)
			popItem();
	}
}

void Interpreter::stackUnderStack()
{
	if(m_ip->m_stackStack->count() == 1)
	{
		reverse();
		return;
	}

	int n = popItem();
	
	Stack* source = m_ip->m_stackStack->secondStack();
	if (!source)
	{
		reverse();
		return;
	}

	Stack* dest = m_ip->m_stackStack->topStack();
	
	if (n < 0)
	{
		source = dest;
		dest = m_ip->m_stackStack->secondStack();
		n = abs(n);
	}
	
	for (int i=0 ; i<n ; i++)
		dest->push(source->pop());
}

void Interpreter::getFunge()
{
	Coord c = popVector();
	for(uint i = 0; i < m_space->dimensions(); ++i)
		c[i] += m_ip->m_storageOffset[i];

	int fetched_char = m_space->getChar(c);

	pushItem(fetched_char);
}

void Interpreter::putFunge()
{
	Coord c = popVector();
	for(uint i = 0; i < m_space->dimensions(); ++i)
		c[i] += m_ip->m_storageOffset[i];

	int popped = popItem();

	m_space->setChar(c, popped);
}

void Interpreter::pushNumber(QChar n)
{
	pushItem(QString(n).toInt());
}

void Interpreter::pushItem(int c)
{
	m_ip->stack()->push(c);
}

void Interpreter::pushVector(Coord c)
{
	pushItem(c[0]);
	pushItem(c[1]);
}

Coord Interpreter::popVector()
{
	Coord c;
	c[1] = popItem();
	c[0] = popItem();

	return c;
}

int Interpreter::popItem()
{
	return m_ip->stack()->pop();
}

void Interpreter::split()
{
	InstructionPointer* t = new InstructionPointer(m_stackModel, *m_ip, m_ipid++);

	Q_ASSERT(m_ips.contains(m_ip));
	for (QList<InstructionPointer*>::iterator it = m_ips.begin(); it != m_ips.end(); ++it) {
		if (*it == m_ip) {
			m_ips.insert(it, t);
			break;
		}
	}
	Q_ASSERT(m_ips.contains(t));

	qSwap(m_ip, t);
	reverse();
	move();
	qSwap(m_ip, t);

	emit ipCreated(m_ips.indexOf(t), t);
}

bool Interpreter::end()
{
	int index = m_ips.indexOf(m_ip);
	m_ips.removeAt(index);
	emit ipDestroyed(m_ip);
	delete m_ip;
	m_ip = NULL;
	if (m_ips.count() == 0)
		return true;
	
	/*if (index >= m_ips.count())
		m_ip = m_ips.first();
	else
		m_ip = m_ips[index];*/
	
	return false;
}

void Interpreter::panic(QString message)
{
	message = "PANIC!: " + message;
	qDebug() << m_ip->m_pos[0] << m_ip->m_pos[1];
	qFatal(message.toAscii());
}

void Interpreter::loadSemantics()
{
	// Load Semantics unimplemented... reflect.
	reverse();
}

void Interpreter::getSysInfo()
{
	int t = popItem();

	int currentStackSize = m_ip->stack()->count();

	using namespace SysInfo;

	if(t <= 0)
	{
		pushEnvVariables(m_ip->stack());
		pushCommandLineArgs(m_ip->stack());
		pushStackSizes(m_ip->stack(), m_ip->m_stackStack, currentStackSize);
		pushStackStackSize(m_ip->stack(), m_ip->m_stackStack);
		pushTime(m_ip->stack());
		pushDate(m_ip->stack());
		pushGreatestPoint(m_ip->stack(), *m_space);
		pushLeastPoint(m_ip->stack(), *m_space);
		pushStorageOffset(m_ip->stack(), *m_ip);
		pushDirection(m_ip->stack(), *m_ip);
		pushPosition(m_ip->stack(), *m_ip);
		pushTeam(m_ip->stack(), *m_ip);
		pushUUID(m_ip->stack(), *m_ip);
		pushDimensions(m_ip->stack(), *m_space);
		pushSeparator(m_ip->stack());
		pushOperatingParadigm(m_ip->stack());
		pushVersion(m_ip->stack());
		pushHandprint(m_ip->stack());
		pushBytesPerCell(m_ip->stack());
		pushFlags(m_ip->stack());
	}
	else if (t > 20)
	{
		qDebug() << __PRETTY_FUNCTION__ << t << m_ip->stack()->count();
		if (t-20 > m_ip->stack()->count())
		{
			reverse();
			return;
		}

		pushItem(((DataCellItem*)m_ip->stack()->child(t-20))->value());
	}
	else
	{
		switch(t)
		{
			case 1:
				pushFlags(m_ip->stack());
				break;
			case 2:
				pushBytesPerCell(m_ip->stack());
				break;
			case 3:
				pushHandprint(m_ip->stack());
				break;
			case 4:
				pushVersion(m_ip->stack());
				break;
			case 5:
				pushOperatingParadigm(m_ip->stack());
				break;
			case 6:
				pushSeparator(m_ip->stack());
				break;
			case 7:
				pushDimensions(m_ip->stack(), *m_space);
				break;
			case 8:
				pushUUID(m_ip->stack(), *m_ip);
				break;
			case 9:
				pushTeam(m_ip->stack(), *m_ip);
				break;
			case 10:
				pushPosition(m_ip->stack(), *m_ip);
				break;
			case 11:
				pushDirection(m_ip->stack(), *m_ip);
				break;
			case 12:
				pushStorageOffset(m_ip->stack(), *m_ip);
				break;
			case 13:
				pushLeastPoint(m_ip->stack(), *m_space);
				break;
			case 14:
				pushGreatestPoint(m_ip->stack(), *m_space);
				break;
			case 15:
				pushDate(m_ip->stack());
				break;
			case 16:
				pushTime(m_ip->stack());
				break;
			case 17:
				pushStackStackSize(m_ip->stack(), m_ip->m_stackStack);
				break;
			case 18:
				pushStackSizes(m_ip->stack(), m_ip->m_stackStack, currentStackSize);
				break;
			case 19:
				pushCommandLineArgs(m_ip->stack());
				break;
			case 20:
				pushEnvVariables(m_ip->stack());
				break;
		}
	}
}


#include "mainwindow.h"
#include "glview.h"
#include "ui_mainwindow.h"
#include "stringmodewidget.h"
#include "interpreter.h"

#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent),
	  m_executionFungeSpace(NULL),
	  m_interpreter(NULL),
	  m_fullSpeedExecution(false),
	  m_settings("BeQunge", "BeQunge", this)
{
	m_ui.setupUi(this);
  showMaximized();
	
	setWindowIcon(QIcon(":/icons/beq.png"));
	
	m_ui.docBrowser->setSource(QUrl("qrc:/docs/index.html"));
	
	// Create empty funge space
	m_fungeSpace = new FungeSpace(3);
	
	// Setup the GL view
	QGLFormat format;
	format.setSampleBuffers(true);
	m_glView = new GLView(m_fungeSpace, format, this);
	setCentralWidget(m_glView);
	m_glView->setFocus(Qt::OtherFocusReason);
	connect(m_glView, SIGNAL(cursorDirectionChanged(int)), SLOT(cursorDirectionChanged(int)));
	connect(m_glView, SIGNAL(copyChangeToCodeFungeSpace(Coord)), SLOT(slotCopyChangeToCodeFungeSpace(Coord)));
	
	// Cursor direction statusbar label
	m_cursorDirectionLabel = new QLabel(this);
	m_cursorDirectionLabel->setToolTip("Use alt+arrowkey to change the cursor direction");
	statusBar()->addPermanentWidget(m_cursorDirectionLabel);
	cursorDirectionChanged(m_glView->cursorDirection());
	
	// Setup the String Mode statusbar widget
	StringModeWidget* stringMode = new StringModeWidget(this);
	statusBar()->addPermanentWidget(stringMode);
	connect(stringMode, SIGNAL(stringModeChanged(bool)), m_glView, SLOT(setStringMode(bool)));
	connect(m_glView, SIGNAL(stringModeChanged(bool)), stringMode, SLOT(setStringMode(bool)));
	
	// Hide the docks by default
  //m_ui.stackDock->hide();
  //m_ui.consoleDock->hide();
  //m_ui.debuggerDock->hide();
	m_ui.documentationDock->hide();
	
	m_ui.displayFungeSpace->setEnabled(false);

	// Connect actions
	connect(m_ui.actionOpen, SIGNAL(triggered(bool)), SLOT(slotOpen()));
	connect(m_ui.actionNew, SIGNAL(triggered(bool)), SLOT(slotNew()));
	connect(m_ui.actionDebug, SIGNAL(triggered(bool)), SLOT(slotDebug()));
	connect(m_ui.actionStep, SIGNAL(triggered(bool)), SLOT(slotStep()));
	connect(m_ui.actionStop, SIGNAL(triggered(bool)), SLOT(slotStop()));
	connect(m_ui.actionUndo, SIGNAL(triggered(bool)), m_glView->getUndo(), SLOT(undo()));
	connect(m_ui.actionRedo, SIGNAL(triggered(bool)), m_glView->getUndo(), SLOT(redo()));
	connect(m_ui.actionSave, SIGNAL(triggered(bool)), SLOT(slotSave()));
	connect(m_ui.actionAbout, SIGNAL(triggered(bool)), SLOT(slotAbout()));
	connect(m_ui.actionAboutQt, SIGNAL(triggered(bool)), SLOT(slotAboutQt()));
	connect(m_ui.actionCut, SIGNAL(triggered(bool)), m_glView, SLOT(slotCut()));
	connect(m_ui.actionCopy, SIGNAL(triggered(bool)), m_glView, SLOT(slotCopy()));
	connect(m_ui.actionPaste, SIGNAL(triggered(bool)), m_glView, SLOT(slotPaste()));
	connect(m_ui.actionPasteTransparent, SIGNAL(triggered(bool)), m_glView, SLOT(slotPasteTransparent()));
	connect(m_ui.actionToggleBreakpoint, SIGNAL(triggered(bool)), SLOT(slotToggleBreakpoint()));
	connect(m_ui.actionToggleWatchpoint, SIGNAL(triggered(bool)), SLOT(slotToggleWatchpoint()));
	connect(m_ui.actionClearAllBreakpoints, SIGNAL(triggered(bool)), SLOT(slotClearAllBreakpoints()));
	connect(m_ui.actionClearAllWatchpoints, SIGNAL(triggered(bool)), SLOT(slotClearAllWatchpoints()));
	
	connect(m_ui.speedSlider, SIGNAL(sliderMoved(int)), SLOT(speedSliderMoved(int)));
	connect(m_ui.displayFungeSpace, SIGNAL(activated(int)), SLOT(slotDisplayFungeSpaceChanged(int)));
	connect(m_ui.pauseButton, SIGNAL(clicked(bool)), SLOT(pauseButtonClicked(bool)));
	connect(m_ui.fullSpeedButton, SIGNAL(clicked(bool)), SLOT(fullSpeedButtonClicked(bool)));

	m_ui.actionUndo->setEnabled(false);
	m_ui.actionRedo->setEnabled(false);
	connect(m_glView->getUndo(), SIGNAL(canUndoChanged(bool)), m_ui.actionUndo, SLOT(setEnabled(bool)));
	connect(m_glView->getUndo(), SIGNAL(canRedoChanged(bool)), m_ui.actionRedo, SLOT(setEnabled(bool)));
	
	connect(m_ui.actionBloom, SIGNAL(triggered(bool)), m_glView, SLOT(setBloom(bool)));
	
	// Setup the stack list
	
	// Setup the auto-step timer
	m_autoStepTimer = new QTimer(this);
	connect(m_autoStepTimer, SIGNAL(timeout()), SLOT(slotStep()));

	if(!m_settings.contains("filedir"))
	{
		m_settings.setValue("filedir", QDir::home().absolutePath());
		m_settings.sync();
	}
	
	// Disable the stop and step actions
	m_ui.actionStep->setEnabled(false);
	m_ui.actionStop->setEnabled(false);
}

MainWindow::~MainWindow()
{
}

void MainWindow::cursorDirectionChanged(int direction)
{
	QString text("Cursor direction: ");
	text += (direction < 0) ? "negative " : "positive ";
	
	switch (abs(direction))
	{
		case 1: text += "x"; break;
		case 2: text += "y"; break;
		case 3: text += "z"; break;
	}
	
	m_cursorDirectionLabel->setText(text);
}

void MainWindow::slotOpen()
{
	QString filter;
	QString fileName = QFileDialog::getOpenFileName(
		this,
		"Open file",
		m_settings.value("filedir").toString(),
		"BeQunge source (*.beq);;Befunge '98 source (*.b98);;Befunge '93 source (*.bf)",
		&filter);

	if (fileName.isNull())
		return;
	
	slotStop();
	
	QFile file(fileName);
	QDir dir(fileName);
	m_settings.setValue("filedir", dir.absolutePath());

	FungeSpace::SourceType type;
  if (fileName.endsWith("beq"))
		type = FungeSpace::Bequnge;
  else if (fileName.endsWith("b98"))
		type = FungeSpace::Befunge98;
  else if (fileName.endsWith("bf"))
		type = FungeSpace::Befunge93;
	else
		Q_ASSERT(0);
	
	FungeSpace* space = new FungeSpace(&file, type);
	if (space->dimensions() < 3)
		space->setDimensions(3);
	
	delete m_fungeSpace;
	m_fungeSpace = space;
	
	showExecutionSpace(false);
	m_glView->resetView();
	
	delete m_interpreter;
	m_interpreter = NULL;
	m_ui.stackList->clear();
}

void MainWindow::slotNew()
{
	delete m_fungeSpace;
	m_fungeSpace = new FungeSpace(3);
	
	showExecutionSpace(false);
	m_glView->resetView();
	
	delete m_interpreter;
	m_interpreter = NULL;
	m_ui.stackList->clear();
}

void MainWindow::slotDebug()
{
	// Setup UI
	m_ui.stackDock->show();
	m_ui.consoleDock->show();
	m_ui.debuggerDock->show();
	m_ui.displayFungeSpace->setEnabled(true);
	
	// Copy the funge space
	delete m_executionFungeSpace;
	m_executionFungeSpace = new FungeSpace(m_fungeSpace);
	m_executionFungeSpace->trackChanges(true);
	m_ui.stackList->clear();
	
	// Make an interpreter
	delete m_interpreter;
	m_interpreter = new Interpreter(m_ui.stackList, m_executionFungeSpace, this);
	connect(m_interpreter, SIGNAL(ipCreated(int, Interpreter::InstructionPointer*)), SLOT(slotIpCreated(int, Interpreter::InstructionPointer*)));
	connect(m_interpreter, SIGNAL(ipChanged(Interpreter::InstructionPointer*)), SLOT(slotIpChanged(Interpreter::InstructionPointer*)));
	connect(m_interpreter, SIGNAL(ipDestroyed(Interpreter::InstructionPointer*)), SLOT(slotIpDestroyed(Interpreter::InstructionPointer*)));
	connect(m_executionFungeSpace, SIGNAL(watchpointTriggered(Coord, int)), SLOT(slotWatchpointTriggered(Coord, int)), Qt::DirectConnection);
	connect(m_interpreter, SIGNAL(output(QChar)), SLOT(slotOutput(QChar)));
	connect(m_interpreter, SIGNAL(output(QString)), SLOT(slotOutput(QString)));
	connect(m_interpreter, SIGNAL(input(Interpreter::WaitingForInput)), m_ui.consoleBox, SLOT(getInput(Interpreter::WaitingForInput)));
	connect(m_ui.consoleBox, SIGNAL(charEntered(QChar)), SLOT(provideInput(QChar)));
	connect(m_ui.consoleBox, SIGNAL(intEntered(int)), SLOT(provideInput(int)));

	m_glView->clearIps();
	slotIpCreated(0, m_interpreter->ip());
	
	showExecutionSpace(true);
	
	m_ui.actionStep->setEnabled(true);
	m_ui.actionStop->setEnabled(true);
	m_ui.actionDebug->setEnabled(false);
	
	if (!m_ui.pauseButton->isChecked())
	{
		if (m_ui.fullSpeedButton->isChecked())
		{
			m_fullSpeedExecution = true;
			doFullSpeedExecution();
		}
		else
			speedSliderMoved(m_ui.speedSlider->value());
	}
}

void MainWindow::slotStep()
{
	if (m_interpreter == NULL)
		return;

	QList<Interpreter::Status> status = m_interpreter->stepAll();

	foreach (Interpreter::Status s, status)
	{
		switch (s)
		{
			case Interpreter::End:
				slotStop();
				break;
			case Interpreter::Invalid:
				break;
			case Interpreter::SuspendForInput:
				break;
			case Interpreter::Success:
				break;
		}
	}
}

void MainWindow::slotIpCreated(int index, Interpreter::InstructionPointer* ip)
{
	m_glView->ipCreated(index, ip);
	m_glView->ipChanged(ip);
}

void MainWindow::slotIpChanged(Interpreter::InstructionPointer* ip)
{
	m_glView->ipChanged(ip);
	
	if (m_fungeSpace->isBreakpoint(ip->m_pos))
	{
		if (!m_ui.pauseButton->isChecked())
			m_ui.pauseButton->click();
		m_glView->followIp(ip);
	}
}

void MainWindow::slotIpDestroyed(Interpreter::InstructionPointer* ip)
{
	m_glView->ipDestroyed(ip);
}

void MainWindow::showExecutionSpace(bool execution)
{
	if ((m_executionFungeSpace == NULL) && (execution))
		return;
	m_glView->setFungeSpace(execution ? m_executionFungeSpace : m_fungeSpace);
	m_glView->setExecution(execution);
	m_ui.displayFungeSpace->setCurrentIndex(execution ? 1 : 0);
	m_glView->displayChanges(false);
}

void MainWindow::slotDisplayFungeSpaceChanged(int index)
{
	if ((m_executionFungeSpace == NULL) && (index != 0))
		return;
	m_glView->setFungeSpace((index != 0) ? m_executionFungeSpace : m_fungeSpace);
	m_glView->setExecution((index != 0));
	m_glView->displayChanges(index == 2);
}

void MainWindow::slotStop()
{
	if (m_fullSpeedExecution)
		m_fullSpeedExecution = false;
	
	delete m_interpreter;
	m_interpreter = NULL;
	showExecutionSpace(false);
	
	m_ui.actionStep->setEnabled(false);
	m_ui.actionStop->setEnabled(false);
	m_ui.actionDebug->setEnabled(true);
	
	m_autoStepTimer->stop();
	
	m_glView->clearIps();
	m_glView->followIp(NULL);
}

void MainWindow::slotOutput(QChar c)
{
	slotOutput(QString(c));
}

void MainWindow::slotOutput(QString str)
{
	m_ui.consoleBox->append(str);
}

void MainWindow::speedSliderMoved(int value)
{
	if (m_interpreter != NULL)
	{
		m_fullSpeedExecution = false;
		if (!m_autoStepTimer->isActive())
			m_autoStepTimer->start(1000 / value);
		else
			m_autoStepTimer->setInterval(1000 / value);
	}
	
	m_ui.pauseButton->setChecked(false);
	m_ui.fullSpeedButton->setChecked(false);
}

void MainWindow::slotSave()
{
	QString filename = QFileDialog::getSaveFileName(this, "Save File...", m_settings.value("filedir").toString(), "BeQunge source (*.beq)");
	
	QDir dir(filename);
	m_settings.setValue("filedir", dir.absolutePath());

	m_fungeSpace->save(filename);
}

void MainWindow::slotCopyChangeToCodeFungeSpace(Coord c)
{
	m_fungeSpace->setChar(c, m_executionFungeSpace->changes()[c].second);
	m_executionFungeSpace->removeChange(c);
}

void MainWindow::pauseButtonClicked(bool checked)
{
	if (checked)
	{
		m_autoStepTimer->stop();
		m_fullSpeedExecution = false;
		m_ui.fullSpeedButton->setChecked(false);
	}
	else
		speedSliderMoved(m_ui.speedSlider->value());
}

void MainWindow::fullSpeedButtonClicked(bool checked)
{
	if (checked)
	{
		m_autoStepTimer->stop();
		m_fullSpeedExecution = true;
		m_ui.pauseButton->setChecked(false);
		doFullSpeedExecution();
	}
	else
		speedSliderMoved(m_ui.speedSlider->value());
}

void MainWindow::doFullSpeedExecution()
{
	if (m_interpreter == NULL)
		return;
	int i=0;
	while (m_fullSpeedExecution)
	{
		QList<Interpreter::Status> status = m_interpreter->stepAll();
		if (status.count(Interpreter::End) == status.size())
		{
			slotStop();
			m_fullSpeedExecution = false;
		}

		++i;
		if (i == 100)
		{
			QCoreApplication::processEvents();
			i = 0;
		}
	}
}

void MainWindow::provideInput(QChar c)
{
	if (m_interpreter == NULL)
		return;
	m_interpreter->provideInput(c);
	QTimer::singleShot(0, this, SLOT(doFullSpeedExecution()));
}

void MainWindow::provideInput(int i)
{
	if (m_interpreter == NULL)
		return;
	m_interpreter->provideInput(i);
	QTimer::singleShot(0, this, SLOT(doFullSpeedExecution()));
}

void MainWindow::slotAbout()
{
	QString aboutString = "<p><b>About BeQunge</b></p>"
		"<p>BeQunge is an n-dimensional code editor, interpreter and debugger for the Funge "
		"programming language.  BeQunge adheres as closely as possible to the Funge-98 "
		"specification.<p>"
		"<p><b>Authors</b></p>"
		"<p>David Sansome &lt;ds505@ecs.soton.ac.uk&gt;<br>"
		"John Maguire &lt;jbm305@ecs.soton.ac.uk&gt;<br>"
		"Alexandra Jones &lt;aj705@soton.ac.uk&gt;</p>";
	QMessageBox::about(this, "About BeQunge", aboutString);
}

void MainWindow::slotAboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::slotToggleBreakpoint()
{
	Coord c = m_glView->textCursorPos();
	
	m_fungeSpace->toggleBreakpoint(c);
	if (m_executionFungeSpace != NULL)
		m_executionFungeSpace->toggleBreakpoint(c);
}

void MainWindow::slotToggleWatchpoint()
{
	Coord c = m_glView->textCursorPos();
	
	m_fungeSpace->toggleWatchpoint(c);
	if (m_executionFungeSpace != NULL)
		m_executionFungeSpace->toggleWatchpoint(c);
}

void MainWindow::slotClearAllBreakpoints()
{
	m_fungeSpace->clearAllBreakpoints();
	if (m_executionFungeSpace != NULL)
		m_executionFungeSpace->clearAllBreakpoints();
}

void MainWindow::slotClearAllWatchpoints()
{
	m_fungeSpace->clearAllWatchpoints();
	if (m_executionFungeSpace != NULL)
		m_executionFungeSpace->clearAllWatchpoints();
}

void MainWindow::slotWatchpointTriggered(Coord, int)
{
	if (!m_ui.pauseButton->isChecked())
		m_ui.pauseButton->click();
	m_glView->followIp(m_interpreter->ip());
}




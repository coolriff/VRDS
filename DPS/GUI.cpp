#include "GUI.h"

GUI::GUI(Ogre::Viewport* vp, Ogre::SceneManager* mSceneMgr, Ogre::RenderWindow* mWindow)
{
	this->vp = vp;
	this->mSceneMgr = mSceneMgr;
	this->mWindow = mWindow;

	Command_Play = false;
	Command_Pause = false;
	Command_Slow_Motion = false;
	Command_Clear_Screen = false;
	Command_Cloth_Demo_1 = false;
	Command_Cloth_Demo_2 = false;
	Command_Cloth_Demo_3 = false;
	Command_Softbody_Demo_1 = false;
	Command_Softbody_Demo_2 = false;
	Command_Softbody_Demo_3 = false;
	Command_Deformable_Demo_1 = false;
	Command_Deformable_Demo_2 = false;
	Command_Deformable_Demo_3 = false;
	Command_Deformable_Demo_4 = false;
	Command_ResetCamera = false;
	Theme_Choice = 0;
	Command_SwitchTheme = false;
	Command_ScreenShot = false;
	Command_Enable_Mini_Camera = false;
	Command_Disable_Mini_Camera = false;
	Command_Disable_FPS = false;
	Command_Enable_FPS = false;
	Command_Solid = false;
	Command_Wireframe = false;
	Command_Points = false;
	Command_Quit = false;
	Simulation_Default = true;
	Simulation_Stop = false;

	mGUIPlatform = new MyGUI::OgrePlatform();
	mGUIPlatform->initialise(mWindow, mSceneMgr);
	mGuiSystem = new MyGUI::Gui();
	mGuiSystem->initialise();
}


GUI::~GUI(void)
{
}

void GUI::createGUI(size_t _index)
{
	if (_index == 0)
	{
		MyGUI::ResourceManager::getInstance().load("MyGUI_BlueWhiteTheme.xml");
	}
	else if (_index == 1)
	{
		MyGUI::ResourceManager::getInstance().load("MyGUI_BlackBlueTheme.xml");
	}
	else if (_index == 2)
	{
		MyGUI::ResourceManager::getInstance().load("MyGUI_BlackOrangeTheme.xml");
	}

	menuPtr = MyGUI::LayoutManager::getInstance().loadLayout("DPS.layout");
	menuListener();
	createMiniCamera(mSceneMgr->getCamera("miniCam"));
	createSimulationSpeedWindow();
/*	createFPSWindow();*/
// 	MyGUI::ButtonPtr button= mGuiSystem->createWidget<MyGUI::Button>("Button",10,10,300,26, MyGUI::Align::Default,"Main");
// 	button->setCaption("Hao");
}

void GUI::destroyGUI(void)
{
	mGuiSystem->shutdown();
	delete mGuiSystem;

	mGUIPlatform->shutdown();
	delete mGUIPlatform;
}

void GUI::menuListener(void)
{
	MyGUI::Widget* widget;

	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Quit"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Clear_Screen"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_Mini_Camera"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_Mini_Camera"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_FPS"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_FPS"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Points"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_ScreenShot"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	if(widget = mGuiSystem->findWidget<MyGUI::Widget>("Command_Slow_Motion"))
	{
		widget-> eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);
	}
	
}

void GUI::selectedMenuItem(MyGUI::Widget* sender)
{
	std::string name = sender->getName();
	if (name == "Command_Quit")
	{
		Command_Quit = true;
	} 
	if(name == "Command_Clear_Screen")
	{
		Command_Clear_Screen = true;
	}
	if(name == "Command_Enable_Mini_Camera")
	{
		Command_Enable_Mini_Camera = true;
		Command_Disable_Mini_Camera = false;
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_Mini_Camera")->setEnabled(false);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_Mini_Camera")->setEnabled(true);
		miniCameraWindow->setVisible(true);
	}
	if(name == "Command_Disable_Mini_Camera")
	{
		Command_Disable_Mini_Camera = true;
		Command_Enable_Mini_Camera = false;
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_Mini_Camera")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_Mini_Camera")->setEnabled(false);
		miniCameraWindow->setVisible(false);
	}
	if(name == "Command_Enable_FPS")
	{
		Command_Enable_FPS = true;
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_FPS")->setEnabled(false);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_FPS")->setEnabled(true);
/*		FPSWindow->setVisible(true);*/
	}
	if(name == "Command_Disable_FPS")
	{
		Command_Disable_FPS = true;
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_FPS")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_FPS")->setEnabled(false);
/*		FPSWindow->setVisible(false);*/
	}
	if(name == "Command_Solid")
	{
		vp->getCamera()->setPolygonMode(Ogre::PM_SOLID);

		mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid")->setEnabled(false);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Points")->setEnabled(true);
	}
	if(name == "Command_Wireframe")
	{
		vp->getCamera()->setPolygonMode(Ogre::PM_WIREFRAME);

		mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe")->setEnabled(false);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Points")->setEnabled(true);
	}
	if(name == "Command_Points")
	{
		vp->getCamera()->setPolygonMode(Ogre::PM_POINTS);

		mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe")->setEnabled(true);
		mGuiSystem->findWidget<MyGUI::Widget>("Command_Points")->setEnabled(false);
	}
	if(name == "Command_ScreenShot")
	{
		Command_ScreenShot = true;
	}
	if(name == "Command_Slow_Motion")
	{
		Command_Slow_Motion = true;
	}
	if(name == "Simulation_Default")
	{
		if(Simulation_Default)
		{
			Simulation_Default = false;
			Simulation_Stop = true;
		}
		else
		{
			Simulation_Default = true;
			Simulation_Stop = false;
		}
	}
	if(name == "Simulation_Stop")
	{
		if(Simulation_Stop)
		{
			Simulation_Stop = false;
			Simulation_Default = true;
		}
		else
		{
			Simulation_Stop = true;
			Simulation_Default = false;
		}
	}

}

void GUI::createMiniCamera(Ogre::Camera* miniCam)
{
	const MyGUI::IntSize& size = MyGUI::RenderManager::getInstance().getViewSize();
	miniCameraWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCS", MyGUI::IntCoord(size.width - 360, size.height - 270, 360, 270), MyGUI::Align::Right|MyGUI::Align::Bottom, "Overlapped");
	miniCameraWindow->setCaption("Camera View");
	miniCameraWindow->setMinSize(MyGUI::IntSize(100, 100));
	miniCameraWindow->setVisible(false);
	MyGUI::Canvas* canvas = miniCameraWindow->createWidget<MyGUI::Canvas>("Canvas", MyGUI::IntCoord(0, 0, miniCameraWindow->getClientCoord().width, miniCameraWindow->getClientCoord().height), MyGUI::Align::Stretch);
	canvas->setPointer("hand");

	wraps::RenderBox* gRenderBox = new wraps::RenderBox();
	gRenderBox->setCanvas(canvas);
	gRenderBox->setViewport(miniCam);
	gRenderBox->setBackgroundColour(MyGUI::Colour::Black);
}

void GUI::createSimulationSpeedWindow(void)
{
	const MyGUI::IntSize& size = MyGUI::RenderManager::getInstance().getViewSize();

	simulationPtr = MyGUI::LayoutManager::getInstance().loadLayout("Simulation.layout");

	simulationWindow = mGuiSystem->findWidget<MyGUI::Window>("Simulation_Window");
	simulationWindow->eventWindowButtonPressed += MyGUI::newDelegate(this, &GUI::selectedWindowItem);
	simulationWindow->setVisible(true);
	simulationWindow->setPosition(size.width - 360, size.height - (size.height - 26));

	MyGUI::Button* button = mGuiSystem->findWidget<MyGUI::Button>("Simulation_Default");
	button->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);

	button = mGuiSystem->findWidget<MyGUI::Button>("Simulation_Stop");
	button->eventMouseButtonClick += MyGUI::newDelegate(this, &GUI::selectedMenuItem);

	mGuiSystem->findWidget<MyGUI::ScrollBar>("Status_SpeedBar")->eventScrollChangePosition += newDelegate(this, &GUI::modifySimulationSpeed);
}


void GUI::selectedWindowItem(MyGUI::Window* widget, const std::string& name)
{
	MyGUI::Window* window = widget->castType<MyGUI::Window>(); 
	if (name == "close")
	{
		window->destroySmooth();
	}
	else if (name == "minimized") 
	{ 
		// hide window and show button in your taskbar 
	} 
	else if (name == "maximized") 
	{ 
		// maximized window 
	} 
}

void GUI::modifySimulationSpeed(MyGUI::ScrollBar* sender, size_t pos)
{
	mGuiSystem->findWidget<MyGUI::TextBox>("Status_Speed")->setCaption("Simulation Speed: " + MyGUI::utility::toString(pos)+"%");
}

// void GUI::createFPSWindow(void)
// {
// 	const MyGUI::IntSize& size = MyGUI::RenderManager::getInstance().getViewSize();
// 
// 	FPSWindow = mGuiSystem->findWidget<MyGUI::Window>("FPSWindow");
// 	FPSWindow->setPosition(10, size.height-FPSWindow->getHeight()-10);
// 	FPSWindow->setVisible(true);
// }



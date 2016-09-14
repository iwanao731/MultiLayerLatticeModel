#include "ofApp.h"

// 
namespace{
	bool screen;
	bool _condition; //start, stop;
	bool _bone;
	bool _mesh;
	bool _wire;
	bool _index;
	float volume;	//sound volume
	bool _alpha;
	int _count;
	bool _reset;
	bool _floor;
	bool _background;
	bool _rotate_y;
	int _frame;
	bool boolSaveimg;
	float _fat;
	float weight_alpha; // skinning weight
	bool guiVis;
	float rot;
	string colorName;
	string layerName;
	float motionRate;
	bool renderExternalForce;
	bool _renderParticle;
	bool c_distance;
	bool c_shear;
	bool c_collision;
	bool debugDraw(false);
	float dist_alpha;
	float shear_alpha;
	int vPreservingSwitch;
	ofVec3f ballPos;
	float radius;
	float sumVolume;
	ofVec3f nearestVertex;
	int nearestIndex;
	bool _pick;
	bool _onPicking;
	float fatRatio[30];
	int selectBone;
	//bool _reset;
}

int boneNum;
float **weight;
ObjModel src, surface, surface2;
ofxBvh origin;

// LSM初期化
float f_stiff;
float f_damp;
float s_stiff;
float s_damp;
float m_stiff;
float m_damp;
int _width;
float bone_dist;

//--------------------------------------------------------------
void ofApp::setup(){

	//ofSetVerticalSync(true);
	//ofSetFrameRate(30);
	renderingTime = 0.0;
	bvhTimeValue = 0.0;
	allTime = 0.0;
	updateTime = 0.0;

    //m_cam.setupPerspective( false, 60.0f, 0.1f, 100.0f );
    //m_cam.setDistance(40.0f);
    //m_cam.setGlobalPosition( 30.0f, 15.0f, 20.0f );
    m_cam.lookAt( ofVec3f( 0.0f, 0.0f, 0.0f ) );
    
    cam.setupPerspective( false, 60.0f, 0.1f, 1800.0f );
    //cam.setDistance(40.0f);
    cam.setGlobalPosition( -205.0f, 330.0f, 0.0f );
	cam.setTarget(ofVec3f(0.0, 320.0, 0.0f));

	m_shader.load( "shaders/mainScene.vert", "shaders/mainScene.frag" );
  
    setupLights();

	//ofSetFrameRate(0.01);	 // 0.07

	//cam.setPosition(0,1000,-1000);
	//cam.setTarget(ofVec3f(0,0,0));

	// --------------------
	// Frame Rate Sample
	// 30 fps-> 1.0
	// 20 fps-> 0.666 
	// 12 fps-> 0.4
	// 10 fps-> 0.333
	//  6 fps-> 0.2
	//  5 fps-> 0.166
	// --------------------
	motionRate = 1.0f; // 0.01f/30.0f; //30.0f; // 0.04;// 1666; //0.23 //0.666
	radius = 30.0;
	ballPos.set(150,0,0);
	sumVolume = 0.0f;

	// OBJ
	// Rabbit
	obj.Load("data/model/Rabbit/point.obj");
	surface.Load("data/model/Rabbit/skin.obj");
	surface2 = surface;

	src = obj;

	// BVH
	// Rabbit Dancing
	bvhFileName = "model/Rabbit/Naturaldance_Take_024.bvh";
	bvh.load(bvhFileName);
	origin.load(bvhFileName);

	bvh.play();
	bvh.setLoop(true);
	bvh.setRate(motionRate);
		
	// GUI
	guiSetup1();
	guiSetup2();
	guiSetup3();
	guiSetup4();
	guiSetup5();
	gui1->setDrawBack(false);
	gui2->setDrawBack(false);
	gui3->setDrawBack(false);
	gui4->setDrawBack(false);
	gui5->setDrawBack(false);

    //gui1->loadSettings("GUI/guiSettings.xml"); 
    //gui2->loadSettings("GUI/guiSettings2.xml"); 
    //gui3->loadSettings("GUI/guiSettings3.xml");     
    //gui4->loadSettings("GUI/guiSettings4.xml"); 
    //gui5->loadSettings("GUI/guiSettings5.xml");

	// LSM Initialize
	LSM.Initialize(f_stiff, f_damp, s_stiff, s_damp, m_stiff, m_damp, bone_dist, _fat, _width, &bvh, &src, &surface, weight_alpha);

	reset();

}

//--------------------------------------------------------------
void ofApp::setWeight()
{
	//---------------------
	// weight
	//---------------------

	int c = -16;
	float scale = 10;
	boolSaveimg = false;

	// initialize
	weight = new float *[bvh.getNumJoints()];
	for(int i=0; i<bvh.getNumJoints(); i++){
		weight[i] = new float [obj.GetVertexNum()];
		for(int j=0; j<obj.GetVertexNum(); j++){
			weight[i][j] = 0.0;
		}
	}

	// Set value
	for(int i=0; i<bvh.getNumJoints(); i++)
	{
		ofVec3f P0 = bvh.getJoint(i)->getPosition();
		int count = bvh.getJoint(i)->getChildren().size();
		ofVec3f *P1 = new ofVec3f [count];
		for( int k=0; k<count; k++)
		{
			P1[k] = bvh.getJoint(i)->getChildren().at(k)->getPosition();

			for(int j=0; j<obj.GetVertexNum(); j++)
			{
				ofVec3f P2(obj.GetVertex(j).x*scale, obj.GetVertex(j).y*scale, obj.GetVertex(j).z*scale);
				weight[i][j] = SkinWeight(P0, P1[k], P2, c);
			}
		}
	}

	//---------------------
	// Normalize
	//---------------------
	struct weight2{
		std::vector<int> boneIndex;
		std::vector<float> w;
	};

	weight2 *_weight = new weight2 [obj.GetVertexNum()];
	for(int i=0; i<bvh.getNumJoints(); i++){
		for(int j=0; j<obj.GetVertexNum(); j++){
			_weight[j].w.push_back(weight[i][j]);
			_weight[j].boneIndex.push_back(i); 
		}
	}

	// Normalize
	for(int j=0; j<obj.GetVertexNum(); j++){
		float count = 0.0;
		for(int k=0; k<_weight[j].w.size(); k++){
			count += _weight[j].w.at(k);
		}
		for(int l=0; l<_weight[j].w.size(); l++){
			_weight[j].w.at(l) /= count;
		}
	}

	// Set value
	for(int i=0; i<obj.GetVertexNum(); i++){
		for(int j=0; j<_weight[i].boneIndex.size(); j++){
			int k = _weight[i].boneIndex.at(j);
			weight[k][i] = _weight[i].w.at(j);
		}
	}
}

//--------------------------------------------------------------
void ofApp::setManhattanWeight()
{

}

//--------------------------------------------------------------
void ofApp::guiSetup1()
{
	screen = false;
	_condition = false;
	volume = 0.0;
	_alpha = false;
	_bone = false;
	_mesh = true;
	_wire = false;
	_count = 0;
	_index = false;
	boneNum = 0;
	_reset = true;
	_floor = true;
	_background = false;
	_rotate_y = false;
	_frame = 0;
	guiVis = true;
	colorName = "PINK";
	renderExternalForce = false;
	_renderParticle = false;
	_pick = false;
	_onPicking = false;
	ofEnableSmoothing();
	ofSetCircleResolution(60);

	float dim = 16;
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 200-xInit;

	gui1 = new ofxUICanvas( 0, 0, length+xInit, ofGetHeight());

	// Disp FPS
    gui1->addWidgetDown(new ofxUILabel("SKLTN BSD PHYS", OFX_UI_FONT_LARGE));         
    gui1->addSpacer(length-xInit, 1); 
    gui1->addFPSSlider("FPS SLIDER", length-xInit, dim*.25, 1000);
	gui1->addSpacer(length-xInit, 1); 

	// Read
	gui1->addLabelButton("READ MODEL", _condition);
	gui1->addSpacer(length-xInit, 1); 
	gui1->addLabelButton("READ SKELETON", false);
	gui1->addSpacer(length-xInit, 1); 

	// Button
	gui1->addWidgetDown(new ofxUILabel("BUTTONS", OFX_UI_FONT_MEDIUM)); 
    gui1->addToggle("BONE", _bone);
	gui1->addToggle("WIRE", _wire);
	gui1->addToggle("SKIN", _mesh);
	gui1->addToggle("INDEX", _index);
	gui1->addToggle("FLOOR", _floor);
	gui1->addToggle("BACK", _background);
	gui1->addToggle("ROTATE_Y", _rotate_y);
	gui1->addToggle("ARROW", renderExternalForce);
	gui1->addToggle("PARTICLE", _renderParticle);
	gui1->addToggle("PICK", _pick);
	gui1->addSpacer(length-xInit, 1);

    vector<string> names; 
	names.push_back("PINK");
	names.push_back("BLUE");
	names.push_back("YELLO");
	gui1->addWidgetDown(new ofxUILabel("COLOR", OFX_UI_FONT_MEDIUM)); 	
	gui1->addRadio("SKIN COLOR", names, OFX_UI_ORIENTATION_HORIZONTAL, dim, dim);
    gui1->addSpacer(length-xInit, 1);

	// Motion Speed
	gui1->addWidgetDown(new ofxUILabel("MOTION", OFX_UI_FONT_MEDIUM)); 	
    gui1->addSlider("MOTIONRATE", 0, 1, motionRate, length-xInit,dim);
	bvh.setRate(motionRate);
	gui1->addSpacer(length-xInit, 1);

	// Sound Volume
	gui1->addWidgetDown(new ofxUILabel("MUSIC", OFX_UI_FONT_MEDIUM)); 	
    gui1->addSlider("SOUND", 0, 1, volume, length-xInit,dim);
	sound.setVolume(volume);
	gui1->addSpacer(length-xInit, 1);

	// START/STOP
	gui1->addLabelToggle("START/STOP", _condition);
	gui1->addSpacer(length-xInit, 1);

	// RESET
	gui1->addLabelButton("RESET", false);
	gui1->addSpacer(length-xInit, 1);

	// FULL SCREEN button
	gui1->addLabelToggle("FULL SCREEN", screen);
	gui1->addSpacer(length-xInit, 2);

	// GUI Theme
	gui1->setTheme(OFX_UI_THEME_GRAYRED);
    gui1->autoSizeToFitWidgets();

	ofAddListener(gui1->newGUIEvent,this,&ofApp::guiEvent);
}

//--------------------------------------------------------------
void ofApp::guiSetup2()
{
	m_stiff = 1.8;
	m_damp = 0.60;
	f_stiff = 0.5; //0.21; //1.50; //0.03; //0.31; //0.03; //1.50; //0.03; //26; //2.93;//4.0;//0.31; //0.96; //3.18; //0.44; //75;
	f_damp = 0.5; // 0.62; //0.23; //0.62;//0.22; //0.10; //45; //0.15; //0.2;
	s_stiff = 1.7; //0.35; //1.0 ;//0.31; //1.50; //0.03; //0.31;
	s_damp = 0.40;
	_width = 3;
	bone_dist = 15;

	float dim = 16; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 200-xInit; 

    gui2 = new ofxUICanvas(length+xInit, 0, length+xInit, ofGetHeight());     
	gui2->addWidgetDown(new ofxUILabel("SIMULATION", OFX_UI_FONT_LARGE)); 
	gui2->addSpacer(length-xInit, 1); 

	// WIDTH
	gui2->addWidgetDown(new ofxUILabel("REGION", OFX_UI_FONT_MEDIUM)); 	
    gui2->addSlider("WIDTH", 0, 10, _width, length-xInit,dim);
	gui2->addSpacer(length-xInit, 3); 

	// FAT LAYER
	// Stiffness
	gui2->addWidgetDown(new ofxUILabel("FAT LAYER", OFX_UI_FONT_MEDIUM));
    gui2->addSlider("F_STIFFNESS", 0, 4.0, f_stiff, length-xInit,dim);
	gui2->addSpacer(length-xInit, 1); 

	// Dampiing
	//gui2->addWidgetDown(new ofxUILabel("DAMP SLIDER", OFX_UI_FONT_MEDIUM)); 	
    gui2->addSlider("F_DAMP", 0, 1, f_damp, length-xInit,dim);
	gui2->addSpacer(length-xInit, 3); 

	// SKIN LAYER
	// Stiffness
	gui2->addWidgetDown(new ofxUILabel("SKIN LAYER", OFX_UI_FONT_MEDIUM));
    gui2->addSlider("S_STIFFNESS", 0, 4.0, s_stiff, length-xInit,dim);
	gui2->addSpacer(length-xInit, 1); 

	// Damping
	//gui2->addWidgetDown(new ofxUILabel("DAMP SLIDER", OFX_UI_FONT_MEDIUM)); 	
    gui2->addSlider("S_DAMP", 0, 1.0, s_damp, length-xInit,dim);
	gui2->addSpacer(length-xInit, 3); 

	// MUSCLE LAYER
	// Stiffness
	gui2->addWidgetDown(new ofxUILabel("MUSCLE LAYER", OFX_UI_FONT_MEDIUM));
    gui2->addSlider("M_STIFFNESS", 0, 4.0, m_stiff, length-xInit,dim);
	gui2->addSpacer(length-xInit, 1); 

	// Damping
	//gui2->addWidgetDown(new ofxUILabel("DAMP SLIDER", OFX_UI_FONT_MEDIUM)); 	
    gui2->addSlider("M_DAMP", 0, 1, m_damp, length-xInit,dim);
	gui2->addSpacer(length-xInit, 3); 
	gui2->setTheme(OFX_UI_THEME_GRAYRED);
    gui2->autoSizeToFitWidgets(); 

	ofAddListener(gui2->newGUIEvent,this,&ofApp::guiEvent);
}


//--------------------------------------------------------------
void ofApp::guiSetup3()
{
	weight_alpha = 0.5;

	float dim = 16; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 200-xInit; 

	gui3 = new ofxUICanvas(length*2+xInit*2+4, 0, length+xInit, ofGetHeight());     
    gui3->addWidgetDown(new ofxUILabel("SKINNING", OFX_UI_FONT_LARGE));
	gui3->addSpacer(length-xInit, 1); 

	gui3->addWidgetDown(new ofxUILabel("WEIGHT", OFX_UI_FONT_MEDIUM)); 	
    gui3->addSlider("WEIGHT_ALPHA", 0, 1, weight_alpha, length-xInit,dim);
	gui3->addSpacer(length-xInit, 1);

	// TRANSPARENT
	gui3->addLabelToggle("TRANSPARENT", _alpha);
	gui3->addSpacer(length-xInit, 1);

	// Slect Joint
	gui3->addWidgetDown(new ofxUILabel("SELECT JOINT", OFX_UI_FONT_MEDIUM)); 
	vector<string> names;
	names.push_back( "Reset" );
	names.push_back( "Site" );
	for(int i=0; i<bvh.getNumJoints(); i++)	
		if( bvh.getJoint(i)->getName() != "Site")
			names.push_back( bvh.getJoint(i)->getName() );

	gui3->setWidgetFontSize(OFX_UI_FONT_SMALL);
	gui3->addDropDownList("DROP DOWN", names, length-xInit);
	gui3->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

	gui3->setTheme(OFX_UI_THEME_GRAYRED);
    gui3->autoSizeToFitWidgets(); 

	ofAddListener(gui3->newGUIEvent,this,&ofApp::guiEvent);
}

//--------------------------------------------------------------
void ofApp::guiSetup4()
{
	_fat = 0.3;
	for(int i=0; i<30; i++){
		fatRatio[i] = _fat; 
	}
	//fatRatio[0] = 0.2;
	//fatRatio[1] = 0.5;
	//fatRatio[2] = 0.5;
	//fatRatio[3] = 0.5;
	//fatRatio[4] = 0.3;
	//fatRatio[5] = 0.4;
	//fatRatio[6] = 0.2;
	//fatRatio[7] = 0.3;

	float dim = 16; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 200-xInit; 

	gui4 = new ofxUICanvas(length*3+xInit*3+4, 0, length+xInit, ofGetHeight());     
    gui4->addWidgetDown(new ofxUILabel("MULTI LAYER", OFX_UI_FONT_LARGE));
	gui4->addSpacer(length-xInit, 1); 

	gui4->addLabelToggle("LAYER SET", false);
	gui4->addSpacer(length-xInit, 1);

	// Visualize
	vector<string> names; 
	names.push_back("R");
	names.push_back("B");
	names.push_back("M");
	names.push_back("F");
	names.push_back("S");
	gui4->addWidgetDown(new ofxUILabel("LAYER", OFX_UI_FONT_MEDIUM)); 	
	gui4->addRadio("LAYER VISUALIZE", names, OFX_UI_ORIENTATION_HORIZONTAL, dim, dim); 
    gui4->addSpacer(length-xInit, 1);

	// Fat layer
	gui4->addSpacer(length-xInit, 2); 
    gui4->addWidgetDown(new ofxUILabel("MUSCLE SLIDERS", OFX_UI_FONT_MEDIUM)); 
	gui4->addSlider("0", 0.0, 1.0, fatRatio[0], dim+2, 60);
	gui4->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
	gui4->addSlider("1", 0.0, 1.0, fatRatio[1], dim+2, 60);
	gui4->addSlider("2", 0.0, 1.0, fatRatio[2], dim+2, 60);
	gui4->addSlider("3", 0.0, 1.0, fatRatio[3], dim+2, 60);
	gui4->addSlider("4", 0.0, 1.0, fatRatio[4], dim+2, 60);
	gui4->addSlider("5", 0.0, 1.0, fatRatio[5], dim+2, 60);
	gui4->addSlider("6", 0.0, 1.0, fatRatio[6], dim+2, 60);
	gui4->addSlider("7", 0.0, 1.0, fatRatio[7], dim+2, 60);
	//gui4->addSlider("8", 0.0, 1.0, fatRatio[8], dim+2, 60);
	//gui4->addSlider("9", 0.0, 1.0, fatRatio[9], dim+2, 60);
	gui4->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
	gui4->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);

	gui4->addWidgetDown(new ofxUILabel("BONE", OFX_UI_FONT_MEDIUM)); 	
    gui4->addSlider("BONE_DIST", 0, 40, bone_dist, length-xInit,dim);
	gui4->addSpacer(length-xInit, 1);
	
	//gui4->addWidgetDown(new ofxUILabel("MUSCLE SLIDER", OFX_UI_FONT_MEDIUM)); 	
 //   gui4->addSlider("MUSCLE", 0, 1, _fat, length-xInit,dim);
	//gui4->addSpacer(length-xInit, 1); 

	gui4->setTheme(OFX_UI_THEME_GRAYRED);
    gui4->autoSizeToFitWidgets(); 

	ofAddListener(gui4->newGUIEvent,this,&ofApp::guiEvent);
}

void ofApp::guiSetup5()
{
	c_distance = true;
	c_shear = true;
	c_collision = false;
	dist_alpha = 1.0; //1.0;
	shear_alpha = 1.0; //1.0;
	vPreservingSwitch = 0;

	float dim = 16; 
	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING; 
    float length = 200-xInit; 

	gui5 = new ofxUICanvas(length*4+xInit*4+4, 0, length+xInit, ofGetHeight());     
    gui5->addWidgetDown(new ofxUILabel("CONSTRINTS", OFX_UI_FONT_LARGE));
	gui5->addSpacer(length-xInit, 1); 

	gui5->addWidgetDown(new ofxUILabel("BUTTONS", OFX_UI_FONT_MEDIUM)); 
	gui5->addToggle("DISTANCE", c_distance);
	gui5->addToggle("SHEAR", c_shear);
	gui5->addToggle("COLLISION", c_collision);
	gui5->addSpacer(length-xInit, 1);

	// 
	gui5->addWidgetDown(new ofxUILabel("DIST_ALPHA", OFX_UI_FONT_MEDIUM)); 	
    gui5->addSlider("DIST_ALPHA", 0.0, 1.0, dist_alpha, length-xInit,dim);
	gui5->addSpacer(length-xInit, 1); 

	gui5->addWidgetDown(new ofxUILabel("SHEAR_ALPHA", OFX_UI_FONT_MEDIUM)); 	
    gui5->addSlider("SHEAR_ALPHA", 0.0, 1.0, shear_alpha, length-xInit,dim);
	gui5->addSpacer(length-xInit, 1); 

    vector<string> names; 
	names.push_back("OFF");
	names.push_back("WHOLE");
	names.push_back("JOINT");
	gui5->addWidgetDown(new ofxUILabel("VOLUME PRESERVING", OFX_UI_FONT_MEDIUM)); 	
	gui5->addRadio("SWITCH", names, OFX_UI_ORIENTATION_HORIZONTAL, dim, dim); 
    gui5->addSpacer(length-xInit, 1);

	// Moving graph
	gui5->addWidgetDown(new ofxUILabel("VOLUME_PRESERVE", OFX_UI_FONT_MEDIUM)); 	
	vector<float> buffer; 
    for(int i = 0; i < 256; i++) buffer.push_back(i);
    //mg = (ofxUIMovingGraph *) gui5->addWidgetDown(new ofxUIMovingGraph(length-xInit, 120, buffer, 256, 0, 1500, "MOVING GRAPH"));
	mg = (ofxUIMovingGraph *) gui5->addWidgetDown(new ofxUIMovingGraph(length-xInit, 120, buffer, 256, 1.2e+007, 2.0e+007, "MOVING GRAPH"));
	gui5->setTheme(OFX_UI_THEME_GRAYRED);
    gui5->autoSizeToFitWidgets(); 

	ofAddListener(gui5->newGUIEvent,this,&ofApp::guiEvent);
}

//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e)
{
	string name = e.widget->getName(); 
	int kind = e.widget->getKind();

	if(name == "SOUND")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		volume = slider->getScaledValue(); 	
		sound.setVolume(volume);
	}
	if(name == "MOTIONRATE")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		motionRate = slider->getScaledValue(); 	
		bvh.setRate(motionRate);	
	}	
	if(name == "TRANSPARENT")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		_alpha = button->getValue();
	}
	if(name == "LAYER SET")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		if(button->getValue()){
			_renderParticle = true;
			_mesh = false;
			_wire = false;
			layerName = "M";
			//_index = true;
			_background = true;
			_floor = false;
			_count = 2;
		}else{
			_renderParticle = false;
			_mesh = true;
			//_wire = true;
			_index = false;
			_background = false;
			_floor = true;
		}
	}
	else if(name == "WIDTH")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		_width = (int)slider->getScaledValue(); 	
	}
	else if(name == "F_STIFFNESS")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		f_stiff = slider->getScaledValue(); 	
	}
	else if(name == "F_DAMP")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		f_damp = slider->getScaledValue(); 	
	}
	else if(name == "S_STIFFNESS")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		s_stiff = slider->getScaledValue(); 	
	}
	else if(name == "S_DAMP")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		s_damp = slider->getScaledValue(); 	
	}
	else if(name == "M_STIFFNESS")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		m_stiff = slider->getScaledValue(); 	
	}
	else if(name == "M_DAMP")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		m_damp = slider->getScaledValue(); 	
	}
	else if(name == "BONE_DIST")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		bone_dist = slider->getScaledValue(); 	
		LSM.Render4(_fat);
	}
	else if(name == "MUSCLE")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		_fat = slider->getScaledValue(); 	
		LSM.Render4(_fat);
	}
	else if(name == "0" )
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		int value = ofToInt(name);
		fatRatio[value] = slider->getScaledValue();
		selectBone = value;
		LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
	}
	else if(name == "1")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		int value = 6;
		fatRatio[value] = slider->getScaledValue();
		selectBone = value;
		LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
	}
	else if(name == "2")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		int value = 7;
		fatRatio[value] = slider->getScaledValue();
		selectBone = value;
		LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
	}
	else if(name == "3")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		for(int i=8; i<=10; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
	}
	else if(name == "4")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		for(int i=1; i<=2; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
		for(int i=21; i<=22; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
	}
	else if(name == "5")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		for(int i=3; i<=5; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
		for(int i=23; i<=25; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
	}
	else if(name == "6")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		for(int i=16; i<=18; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
		for(int i=11; i<=13; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
	}
	else if(name == "7")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		for(int i=19; i<=20; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
		for(int i=14; i<=15; i++){
			int value = i;
			fatRatio[value] = slider->getScaledValue();
			selectBone = value;
			LSM.SetLayer(fatRatio[value], selectBone, m_stiff, m_damp, f_stiff, f_damp);
		}
	}
	else if(name == "8")
	{
	//	ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
	//	LSM.SetLayer(1.0, 7);
	//	LSM.SetLayer(1.0, 8);
	//	LSM.SetLayer(1.0, 10);
	//	LSM.SetLayer(1.0, 11);
	//	LSM.SetLayer(1.0, 15);
	//	LSM.SetLayer(1.0, 16);
	//	LSM.SetLayer(1.0, 17);
	//	LSM.SetLayer(1.0, 18);
	//	LSM.SetLayer(1.0, 20);

	//	//LSM.SetLayer(1.0, 26);
	//	//LSM.SetLayer(1.0, 27);
	//	LSM.SetLayer(1.0, 28);
	//	LSM.SetLayer(1.0, 29);
	//	LSM.SetLayer(1.0, 30);
	//	LSM.SetLayer(1.0, 31);
	//	//LSM.SetLayer(1.0, 32);
	//	//LSM.SetLayer(1.0, 33);
	//	LSM.SetLayer(1.0, 34);
	//	LSM.SetLayer(1.0, 35);
	//	LSM.SetLayer(1.0, 36);
	}
	else if(name == "WEIGHT_ALPHA")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		weight_alpha = slider->getScaledValue(); 	
	}	
	else if(name == "BONE")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_bone = button->getValue();
	}
	else if(name == "FLOOR")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_floor = button->getValue();
	}
	else if(name == "ARROW")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		renderExternalForce = button->getValue();
	}	
	else if(name == "PARTICLE")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_renderParticle = button->getValue();
	}	
	else if(name == "BACK")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_background = button->getValue();
	}
	else if(name == "ROTATE_Y")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_rotate_y = button->getValue();
	}	
	else if(name == "WIRE")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_wire = button->getValue();
	}
	else if(name == "SKIN")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_mesh = button->getValue();
	}
	else if(name == "INDEX")
	{
        ofxUIButton *button = (ofxUIButton *) e.widget; 
		_index = button->getValue();
		_count++;
		if(_count == 3) _count = 0;
	}
	else if(name == "PICK")
	{
		ofxUIButton *button = (ofxUIButton *) e.widget; 
		_pick = button->getValue();
	}
	else if(name == "R")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		layerName = e.widget->getName();
	}
	else if(name == "B")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		layerName = e.widget->getName();
	}
	else if(name == "M")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		layerName = e.widget->getName();
	}
	else if(name == "F")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		layerName = e.widget->getName();
	}
	else if(name == "S")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		layerName = e.widget->getName();
	}
	else if(name == "PINK")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		colorName = e.widget->getName();
	}
	else if(name == "BLUE")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		colorName = e.widget->getName();
	}
	else if(name == "YELLO")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		colorName = e.widget->getName();
	}
	else if(name == "DISTANCE")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		c_distance = button->getValue();
		if(c_distance){
			std::cout << "Distance Constraint: ON" << std::endl;
		}else{
			std::cout << "Distance Constraint: OFF" << std::endl;
		}
	}
	else if(name == "SHEAR")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		c_shear = button->getValue();
		if(c_shear){
			std::cout << "Shear Constraint: ON" << std::endl;
		}else{
			std::cout << "Shear Constraint: OFF" << std::endl;
		}
	}
	else if(name == "COLLISION")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		c_collision = button->getValue();
		if(c_shear){
			std::cout << "Collision Constraint: ON" << std::endl;
		}else{
			std::cout << "Collision Constraint: OFF" << std::endl;
		}
	}
	else if(name == "OFF")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		vPreservingSwitch = 0;
		std::cout << "Volume Preserve: OFF" << std::endl;
	}
	else if(name == "WHOLE")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		vPreservingSwitch = 1;
		std::cout << "Volume Preserve: Whole Body" << std::endl;
	}
	else if(name == "JOINT")
	{
		ofxUIRadio *button = (ofxUIRadio *) e.widget;
		vPreservingSwitch = 2;
		std::cout << "Volume Preserve: Each Joint" << std::endl;
	}
	else if(name == "DIST_ALPHA")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		dist_alpha = slider->getScaledValue(); 	
	}	
	else if(name == "SHEAR_ALPHA")
	{
		ofxUIBiLabelSlider *slider = (ofxUIBiLabelSlider *) e.widget; 
		shear_alpha = slider->getScaledValue(); 	
	}		
	else if(name == "READ MODEL")
	{
		cout << "Read lattice model:" << endl;
		ofFileDialogResult openFileResult1 = ofSystemLoadDialog("Select a point.obj"); 
		if (openFileResult1.bSuccess){
			ofLogVerbose("User selected a file");
			string filename = openFileResult1.filePath;
			string name = openFileResult1.fileName;
			cout << "Reading..." << name << endl;
			ObjModel _obj(filename.c_str());
			src = _obj;
		}else {
			ofLogVerbose("User hit cancel");
		}
		cout << "Read Skin model:" << endl;
		ofFileDialogResult openFileResult2 = ofSystemLoadDialog("Select a skin.obj");
		if (openFileResult2.bSuccess){
			ofLogVerbose("User selected a file");
			string filename = openFileResult2.filePath;
			string name = openFileResult2.fileName;
			cout << "Reading..." << name << endl;
			ObjModel _obj(filename.c_str());
			surface = _obj;
			surface2 = surface;
		}else {
			ofLogVerbose("User hit cancel");
		}
		cout << "Read BVH model:" << endl;
		ofFileDialogResult openFileResult3 = ofSystemLoadDialog("Select a .bvh"); 
		if (openFileResult3.bSuccess){
			ofLogVerbose("User selected a file");
			string filename = openFileResult3.getPath();
			string name = openFileResult3.fileName;
			cout << "Reading..." << name << endl;
			bvh.stop();
			origin.stop();
			bvh.unload();
			origin.unload();
			bvh.load(filename);
			origin.load(filename);
		}else {
			ofLogVerbose("User hit cancel");
		}
		// Reset
		reset();
	}
	else if(name == "READ SKELETON")
	{
		ofFileDialogResult openFileResult = ofSystemLoadDialog("Select a .bvh"); 
		if (openFileResult.bSuccess){
			ofLogVerbose("User selected a file");
			string filename = openFileResult.filePath;
			string name = openFileResult.fileName;
			cout << "Reading:" << name << endl;
			bvh.unload();
			origin.unload();
			bvh.load("bone/" + filename);
			origin.load("bone/" + filename);
		}else {
			ofLogVerbose("User hit cancel");
		}
	}
	else if(name == "START/STOP")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
        cout << "START/STOP: " << button->getValue() << endl;
		sound.setPaused(_condition);
		_condition = !_condition;
	}
	else if(name == "RESET")
	{
		ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget; 
		reset();
	}
	else if(name == "FULL SCREEN")
    {
        ofxUIImageToggle *button = (ofxUIImageToggle *) e.widget;
		screen = button->getValue();
		if(screen)	ofToggleFullscreen();
		else 	ofToggleFullscreen();
    }
	else if(kind == 17 && name != "FULL SCREEN" )
	{
		 string jointName = e.widget->getName();
		 std::cout << "Joint::" << jointName << std::endl;
		 for(int i=0; i<bvh.getNumJoints(); i++){
			 if(jointName == bvh.getJoint(i)->getName()){
				 _reset = false;
				 if(jointName == "Site"){
					 boneNum = 0;
				 }else{
					 boneNum = i;
				 }
				 std::cout << boneNum << std::endl;
			 }
			 else if(jointName == "Reset"){
				 _reset = true;
				 boneNum = -1;
			 }
		 }
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	if(_condition){

		if( !QueryPerformanceFrequency(&freqAll) ){}
		if( QueryPerformanceCounter(&startAll) ){}

		if( !QueryPerformanceFrequency(&freq) ){}
		if( QueryPerformanceCounter(&start) ){}

		bvh.update();

		if( !QueryPerformanceCounter(&end) ){}
		bvhTimeValue += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;

		//if(bvh.getFrame()>bvh.getNumFrames()-10)
		//	_frame = 0;

		//SkinUpdate();

		if( !QueryPerformanceFrequency(&freqUpdate) ){}
		if( QueryPerformanceCounter(&startUpdate) ){}

		LSM.Update(&bvh, &origin, &src, _fat, _frame, c_distance, c_shear, c_collision, vPreservingSwitch, dist_alpha, shear_alpha, ballPos, radius);

		if( !QueryPerformanceCounter(&endUpdate) ){}
		updateTime += (double)(endUpdate.QuadPart - startUpdate.QuadPart) / freqUpdate.QuadPart * 1000;

		_frame++;
	}

	// Wave Visualization
	//mg->addPoint(LSM.SendInitialVolume());
	//mg->addPoint(LSM.SendCurrentVolume());
	//cout << LSM.SendCurrentVolume() - LSM.SendInitialVolume() << endl;

	//float v0 = LSM.CalcVolume(&surface);
	//float v1 = LSM.CalcVolume(&surface2);
	//if(v0>0 && v1>0){
	//	mg->addPoint(v0);
	//	mg->addPoint(v1/100000);
	//	//std::cout << v0 << "," << v1/100000 << std::endl;
	//}

	//mg->addPoint(ofGetFrameRate());

	// Get the sound volume
	//float * val = ofSoundGetSpectrum(1);

	// Set the spectrogram
	//float vol = val[0]/volume;
	//mg->addPoint(vol);
}

//--------------------------------------------------------------
void ofApp::draw()
{
	if(_condition){
		if( !QueryPerformanceFrequency(&freq) ){}
		if( QueryPerformanceCounter(&start) ){}
	}

	// Background
	if(_background)
		//ofBackgroundGradient(ofColor(64), ofColor(0));
		ofBackground(100);
	else
		ofBackground(255);

    
    if (!m_bPaused) {
        m_angle += 0.25f;
    }

    ofDisableAlphaBlending();

    m_shadowLight.enable();
   
    // render linear depth buffer from light view
    m_shadowLight.beginShadowMap();
		drawMain();
    m_shadowLight.endShadowMap();
    
    // render final scene
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
    m_shader.begin();

    m_shadowLight.bindShadowMapTexture(0); // bind shadow map texture to unit 0
    m_shader.setUniform1i("u_ShadowMap", 0); // set uniform to unit 0
    m_shader.setUniform1f("u_LinearDepthConstant", m_shadowLight.getLinearDepthScalar()); // set near/far linear scalar
    m_shader.setUniformMatrix4f("u_ShadowTransMatrix", m_shadowLight.getShadowMatrix(cam)); // specify our shadow matrix

	cam.begin();

    m_shadowLight.enable();
		drawMain();
	m_shadowLight.disable();
   
	m_shadowLight.unbindShadowMapTexture();
    m_shader.end();


	if(debugDraw){
        glDisable(GL_CULL_FACE);
        m_shadowLight.draw();
        m_shadowLight.debugShadowMap();
    }

	cam.end();


	glDisable(GL_TEXTURE_2D);
	ofDisableLighting();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
	glColor3f(1.0,1.0,1.0);

	// Pick vertices
	if(_pick){
		if(_onPicking){
			nearestVertex = LSM.PointPick(nearestIndex, mouseX, mouseY, cam);
			ofSetColor(0, 255, 0);
			ofSphere(nearestVertex, 10.0);
			float dist = ofDist(nearestVertex.x, nearestVertex.y, mouseX, mouseY)/10;
			ofSetLineWidth(dist);
			ofDrawArrow(nearestVertex, ofVec3f(mouseX, mouseY), dist);
		}
	}
	glEnable(GL_DEPTH_TEST);	

	if(_condition){
		if( !QueryPerformanceCounter(&end) ){}
		renderingTime += (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000;

		if( !QueryPerformanceCounter(&endAll) ){}
		allTime += (double)(endAll.QuadPart - startAll.QuadPart) / freqAll.QuadPart * 1000;
	}
}

void ofApp::drawMain()
{
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
    
    // floor like plane
	if(_floor){
		ofPushMatrix();
		ofScale(2000.0f, 200.0f, 2000.0f);
		ofTranslate(0.0, 0.1, 0.0); // rabbit
		ofBox(0,0,0,2.0f);
		ofPopMatrix();
	}

	ofRotateY(rot); // slowly rotate the model
	if(_rotate_y)
		rot+=0.5;

	// Display joint index
	if(_index == true)
		RenderIndex(_count);

	if(_wire){
		LSM.Render2(boneNum, _alpha);
	}else{
		LSM.RenderSkin(&surface2, colorName);
	}    

	LSM.CalcVolume(&surface2);

	if(_renderParticle){
		//LSM.Render5(layerName);
		LSM.Render3(layerName);
	}

	if(_bone)
		bvh.draw();
}
//--------------------------------------------------------------
void ofApp::exit()
{
	gui1->saveSettings("GUI/guiSettings.xml"); 
    gui2->saveSettings("GUI/guiSettings2.xml"); 
    gui3->saveSettings("GUI/guiSettings3.xml");     
    gui4->saveSettings("GUI/guiSettings4.xml");     
    gui5->saveSettings("GUI/guiSettings5.xml");    

	LSM.Exit();
	cout << "bvh update Time:	" << bvhTimeValue/_frame << " [ms]" << endl;
	cout << "rendering:	" << renderingTime/_frame << " [ms]" << endl;
	cout << "update Time: " << updateTime/_frame << " [ms]" << endl;
	cout << "all time:	" << allTime/_frame << " [ms]" << endl;

}

//--------------------------------------------------------------
void ofApp::reset()
{
	obj = src;
	surface2 = surface;
	rot = 0;
	_frame = 0;

	bvh.stop();
	bvh.setFrame(0);
	bvh.update();
	bvh.play();
	bvh.setLoop(true);
	bvh.setRate(motionRate);

	Testbed::Reset();
	Testbed::Initialize(f_stiff, f_damp, s_stiff, s_damp, m_stiff, m_damp, bone_dist, _fat, _width, &bvh, &src, &surface, weight_alpha);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key){
		case 'g':
			gui1->toggleVisible();
			gui2->toggleVisible();
			gui3->toggleVisible();
			gui4->toggleVisible();
			gui5->toggleVisible();
			guiVis = !guiVis;
			//_pick = !_pick;
			break;
		case 's':
			sound.setPaused(_condition);
			_condition = !_condition;
			break;
		case 'r':
			reset();
			break;
		case 'f':
			ofToggleFullscreen();
			break;
		case 'p':
			_pick = !_pick;
			if(_pick){
				cam.disableMouseInput();
			}
			break;
		case 'w':
			_wire = !_wire;
			break;
		case ' ':
			debugDraw = !debugDraw;
			break;
		case OF_KEY_UP:
			ballPos.x -= 10;
			break;
		case OF_KEY_DOWN:
			ballPos.x += 10;
			break;
		default:
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	float xInit = OFX_UI_GLOBAL_WIDGET_SPACING;
	float length = 200*5-xInit;
	if(guiVis){
		if( x > 0 && x < length)
			cam.disableMouseInput();
		else
			cam.enableMouseInput();
	}else{
		if(!_pick)
			cam.enableMouseInput();
		else
			cam.disableMouseInput();
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	// 頂点の選択
	if(_pick){
		_onPicking = true;
		nearestIndex = LSM.getIndexPickingPoint(x,y,cam);
		cout << nearestIndex << endl;
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	_onPicking = false;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::setupLights() {
    // ofxShadowMapLight extends ofLight - you can use it just like a regular light
    // it's set up as a spotlight, all the shadow work + lighting must be handled in a shader
    // there's an example shader in
    
    // shadow map resolution (must be power of 2), field of view, near, far
    // the larger the shadow map resolution, the better the detail, but slower
    m_shadowLight.setup( 2048, 100.0f, 0.1f, 1200.0f );
    m_shadowLight.setBlurLevel(4.0f); // amount we're blurring to soften the shadows
    
    m_shadowLight.setAmbientColor( ofFloatColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
    //m_shadowLight.setDiffuseColor( ofFloatColor( 0.9f, 0.9f, 0.9f, 1.0f ) );
	m_shadowLight.setDiffuseColor( ofFloatColor( 1.0f, 0.763f, 0.917f, 1.0f ) ); // Pink
    m_shadowLight.setSpecularColor( ofFloatColor( 1.0f, 0.7f, 0.9f, 1.0f ) );
    
	m_shadowLight.setPosition( -300.0, 450.0, 0.0 );	//rabbit
	m_shadowLight.lookAt( ofVec3f(0.0, 350.0, 0.0) );

	ofSetGlobalAmbientColor( ofFloatColor( 0.05f, 0.05f, 0.05f ) );
}

//--------------------------------------------------------------
void ofApp::RenderIndex(int i){

	if( i == 1 ){
		string index[28];
		for(int i=0; i<bvh.getNumJoints(); i++){
			ofPoint offset(5, -5, 0);
			ofPoint mouse( bvh.getJoint(i)->getPosition().x, bvh.getJoint(i)->getPosition().y, bvh.getJoint(i)->getPosition().z );
			index[i] = bvh.getJoint(i)->getName();
			glColor3d(1.0, 1.0, 1.0);
	
			// Disp Index Name
			ofDrawBitmapString( (index[i]), mouse + offset);
		}
	}else{
		string index[28];
		for(int i=0; i<bvh.getNumJoints(); i++){
			ofPoint offset(5, -5, 0);
			ofPoint mouse( bvh.getJoint(i)->getPosition().x, bvh.getJoint(i)->getPosition().y, bvh.getJoint(i)->getPosition().z );
			index[i] = bvh.getJoint(i)->getName();

			glColor3d(1.0, 1.0, 1.0);
	
			// Disp Number
			ofDrawBitmapString( ofToString(i), mouse + offset);
		}
	}
}

//--------------------------------------------------------------
VERTEX3D ofApp::ErrorToRGB(float err, float errMax, float errMin)
{
	int r, g, b;
	float norm_err = (err - errMin) / (errMax - errMin);
	float H, Hi, f, p, q, t, S = 1.0f, V = 1.0f;

	// HSV誤差をHSV表色系からRGB値に変換する. 
	// 輝度と彩度はともに1.0とする. 

	//H = 360.0f - (360.0f * norm_err + 120.0f);
	H = 360.0f - (240.0f * norm_err + 120.0f);

	if(H < 0.0f) 
	{ 
		H = 0.0f; 
	}

	Hi = (float)floor(H / 60.0f);

	f = H / 60.0f - Hi;

	p = V * (1.0f - S);
	q = V * (1.0f - f * S);
	t = V * (1.0f - (1.0f - f) * S);

	r = g = b = 0;

	if(Hi == 0) 
	{
		r = (int)(255.0f * V); 
		g = (int)(255.0f * t);
		b = (int)(255.0f * p);
	}
	if(Hi == 1)
	{
		r = (int)(255.0f * q); 
		g = (int)(255.0f * V);
		b = (int)(255.0f * p);
	}
	if(Hi == 2)
	{
		r = (int)(255.0f * p); 
		g = (int)(255.0f * V);
		b = (int)(255.0f * t);
	}
	if(Hi == 3)
	{
		r = (int)(255.0f * p); 
		g = (int)(255.0f * q);
		b = (int)(255.0f * V);
	}
	if(Hi == 4)
	{
		r = (int)(255.0f * t); 
		g = (int)(255.0f * p);
		b = (int)(255.0f * V);
	}
	if(Hi == 5)
	{
		r = (int)(255.0f * V); 
		g = (int)(255.0f * p);
		b = (int)(255.0f * q);
	}

	VERTEX3D error(r,g,b);
	return error;
}

//--------------------------------------------------------------
float ofApp::SkinWeight(ofVec3f P0, ofVec3f P1, ofVec3f P2, int c)
{
	float t, d;
	ofVec3f P;
	t = (P1-P0).dot(P0-P2)/(P1-P0).dot(P1-P0);
	P = (P1-P0)*t+P0;

	if(t>=0 && t<=1.0){
		d = P2.distance(P);
	}else if(t<0){
		d = P2.distance(P0);
	}else{
		d = P2.distance(P1);
	}

	float w = pow(d+1, c);
	return w;
}

//--------------------------------------------------------------
void ofApp::SkinUpdate()
{
	for(int i=0; i<obj.GetVertexNum(); i++)
	{
		ofVec4f v(src.GetVertex(i).x, src.GetVertex(i).y, src.GetVertex(i).z, 1.0);
		ofVec4f vv;
		vv.zero();

		for(int j=0; j<bvh.getNumJoints(); j++){
			ofxBvhJoint *joint = const_cast<ofxBvhJoint*>(bvh.getJoint(j));
			ofxBvhJoint *joint0 = const_cast<ofxBvhJoint*>(origin.getJoint(j));
			ofMatrix4x4 temp = joint0->getGlobalMatrix();
			vv += v * temp.getInverse() * joint->getGlobalMatrix() * weight[j][i];
		}
		obj.SetVertex(i, VERTEX3D(vv.x, vv.y, vv.z));
	}
}